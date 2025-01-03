/****************************************************************************************************
 *
 * @file:    ppu.c
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Implementation file for GBA Pixel Processing Unit (PPU).
 *
 * @references:
 *      GBATEK - https://problemkaputt.de/gbatek.htm
 *      Hades  - https://github.com/hades-emu/Hades
 *      gdkGBA - https://github.com/gdkchan/gdkGBA
 *
 * @license:
 * GNU General Public License version 2.
 * Copyright (C) 2024 - Nolan Olhausen
 ****************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "ppu.h"
#include "memory.h"
#include "apu.h"
#include "sdlUtil.h"
#include "cpu.h"

#define FRAME_WIDTH 240
#define FRAME_HEIGHT 160
#define TOTAL_HEIGHT 228

#define CYCLES_PER_SCANLINE 1232
#define CYCLES_PER_HDRAW 1006

#define VBLK_FLAG (1 << 0) // Vertical blank flag
#define HBLK_FLAG (1 << 1) // Horizontal blank flag
#define VCNT_FLAG (1 << 2) // Vertical counter flag
#define VBLK_IRQ (1 << 3)  // Vertical blank interrupt request
#define HBLK_IRQ (1 << 4)  // Horizontal blank interrupt request
#define VCNT_IRQ (1 << 5)  // Vertical counter interrupt request

#define FRAME_BUFFER_SIZE (FRAME_WIDTH * TOTAL_HEIGHT * sizeof(Word))

int cycles = 0; // Number of cycles elapsed

// Lookup tables for tile sizes
static const Byte xTilesLut[16] = {1, 2, 4, 8, 2, 4, 4, 8, 1, 1, 2, 4, 0, 0, 0, 0};
static const Byte yTilesLut[16] = {1, 2, 4, 8, 1, 1, 2, 4, 2, 4, 4, 8, 0, 0, 0, 0};

void initFrameBuffer(void)
{
    // Allocate aligned memory for the frame buffer
    frame = (Word *)_aligned_malloc(FRAME_BUFFER_SIZE, sizeof(Word));
    if (!frame)
    {
        // Print an error message and exit if allocation fails
        printf("ERROR: Failed to allocate aligned frame buffer\n");
        exit(1);
    }
    // Clear the frame buffer
    memset(frame, 0, FRAME_BUFFER_SIZE);
}

// Render objects (sprites) with a specific priority
static void renderOBJ(Byte prio)
{
    // Check if object rendering is enabled
    if (!(mem->lcd.dispcnt.full & (1 << 12)))
        return;

    Byte objIdx;
    Word offset = 0x3f8;

    // Calculate the starting address for the current scanline in the frame buffer
    Word surfAddr = mem->lcd.vcount.full * 240 * 4;

    // Iterate through all objects (sprites)
    for (objIdx = 0; objIdx < 128; objIdx++)
    {
        // Read object attributes from OAM (Object Attribute Memory)
        HalfWord attr0 = mem->oam[offset + 0] | (mem->oam[offset + 1] << 8);
        HalfWord attr1 = mem->oam[offset + 2] | (mem->oam[offset + 3] << 8);
        HalfWord attr2 = mem->oam[offset + 4] | (mem->oam[offset + 5] << 8);

        offset -= 8;

        // Extract object properties from attributes
        int16_t objY = (attr0 >> 0) & 0xff;
        bool affine = (attr0 >> 8) & 0x1;
        bool dblSize = (attr0 >> 9) & 0x1;
        bool hidden = (attr0 >> 9) & 0x1;
        Byte objSHP = (attr0 >> 14) & 0x3;
        Byte affineP = (attr1 >> 9) & 0x1f;
        Byte objSize = (attr1 >> 14) & 0x3;
        Byte chrPrio = (attr2 >> 10) & 0x3;

        // Skip objects that do not match the specified priority or are hidden
        if (chrPrio != prio || (!affine && hidden))
            continue;

        int16_t pa, pb, pc, pd;

        // Initialize affine transformation parameters
        pa = pd = 0x100; // 1.0
        pb = pc = 0x000; // 0.0

        // If affine transformation is enabled, read the parameters from OAM
        if (affine)
        {
            Word pBase = affineP * 32;

            pa = mem->oam[pBase + 0x06] | (mem->oam[pBase + 0x07] << 8);
            pb = mem->oam[pBase + 0x0e] | (mem->oam[pBase + 0x0f] << 8);
            pc = mem->oam[pBase + 0x16] | (mem->oam[pBase + 0x17] << 8);
            pd = mem->oam[pBase + 0x1e] | (mem->oam[pBase + 0x1f] << 8);
        }

        // Determine the size of the object using lookup tables
        Byte lutIdx = objSize | (objSHP << 2);
        Byte xTiles = xTilesLut[lutIdx];
        Byte yTiles = yTilesLut[lutIdx];

        int32_t rcx = xTiles * 4;
        int32_t rcy = yTiles * 4;

        // Double the size if double-size mode is enabled
        if (affine && dblSize)
        {
            rcx *= 2;
            rcy *= 2;
        }

        // Adjust object Y position if it exceeds the screen height
        if (objY + rcy * 2 > 0xff)
            objY -= 0x100;

        // Check if the object is within the current scanline
        if (objY <= (int32_t)mem->lcd.vcount.full && (objY + rcy * 2 > mem->lcd.vcount.full))
        {
            // Extract additional object properties
            Byte objMode = (attr0 >> 10) & 0x3;
            bool mosaic = (attr0 >> 12) & 0x1;
            bool is256 = (attr0 >> 13) & 0x1;
            int16_t objX = (attr1 >> 0) & 0x1ff;
            bool flipX = (attr1 >> 12) & 0x1;
            bool flipY = (attr1 >> 13) & 0x1;
            HalfWord chrNum = (attr2 >> 0) & 0x3ff;
            Byte chrPal = (attr2 >> 12) & 0xf;

            // Calculate the base address of the character data in VRAM
            Word chrBase = 0x10000 | chrNum * 32;

            // Adjust object X position
            objX <<= 7;
            objX >>= 7;

            int32_t x, y = mem->lcd.vcount.full - objY;

            // Flip Y coordinate if flipY is enabled
            if (!affine && flipY)
                y ^= (yTiles * 8) - 1;

            // Calculate tile block size and pixel line row size
            Byte tsz = is256 ? 64 : 32; // Tile block size (in bytes, = (8 * 8 * bpp) / 8)
            Byte lsz = is256 ? 8 : 4;   // Pixel line row size (in bytes)

            // Calculate initial affine transformation offsets
            int32_t ox = pa * -rcx + pb * (y - rcy) + (xTiles << 10);
            int32_t oy = pc * -rcx + pd * (y - rcy) + (yTiles << 10);

            // Flip X coordinate if flipX is enabled
            if (!affine && flipX)
            {
                ox = (xTiles * 8 - 1) << 8;
                pa = -0x100;
            }

            // Calculate tile row stride
            Word tys = (mem->lcd.dispcnt.full & (1 << 6)) ? xTiles * tsz : 1024; // Tile row stride

            // Calculate the starting address for the current scanline in the frame buffer
            Word address = surfAddr + objX * 4;

            // Iterate through the object pixels
            for (x = 0; x < rcx * 2; x++, ox += pa, oy += pc, address += 4)
            {
                if (objX + x < 0)
                    continue;
                if (objX + x >= 240)
                    break;

                Word vramAddr;
                Word palIdx;

                // Calculate tile coordinates
                HalfWord tileX = ox >> 11;
                HalfWord tileY = oy >> 11;

                if (ox < 0 || tileX >= xTiles)
                    continue;
                if (oy < 0 || tileY >= yTiles)
                    continue;

                // Calculate character coordinates
                HalfWord chrX = (ox >> 8) & 7;
                HalfWord chrY = (oy >> 8) & 7;

                // Calculate the address of the character data in VRAM
                Word chr_addr = chrBase + tileY * tys + chrY * lsz;

                // Read the pixel data from VRAM
                if (is256)
                {
                    vramAddr = chr_addr + tileX * 64 + chrX;
                    palIdx = mem->vram[vramAddr];
                }
                else
                {
                    vramAddr = chr_addr + tileX * 32 + (chrX >> 1);
                    palIdx = (mem->vram[vramAddr] >> (chrX & 1) * 4) & 0xf;
                }

                // Calculate the address of the palette entry
                Word palAddr = 0x100 | palIdx | (!is256 ? chrPal * 16 : 0);

                // Write the pixel data to the frame buffer if it is not transparent
                if (palIdx)
                    frame[address / 4] = mem->palette[palAddr];
            }
        }
    }
}

// Background enable flags for different display modes
static const Byte bgENB[3] = {0xf, 0x7, 0xc};

// Render backgrounds
static void renderBG()
{
    Byte mode = mem->lcd.dispcnt.full & 7;          // Get the current display mode
    Word surfAddr = mem->lcd.vcount.full * 240 * 4; // Calculate the starting address for the current scanline in the frame buffer

    switch (mode)
    {
    case 0:
    case 1:
    case 2:
    {
        int8_t prio, bgIdx;

        // Iterate through all priorities and background layers
        for (prio = 3; prio >= 0; prio--)
        {
            for (bgIdx = 3; bgIdx >= 0; bgIdx--)
            {
                // Skip disabled background layers
                if (bgIdx == 3 && !mem->lcd.dispcnt.bits.bg3)
                    continue;
                if (bgIdx == 2 && !mem->lcd.dispcnt.bits.bg2)
                    continue;
                if (bgIdx == 1 && !mem->lcd.dispcnt.bits.bg1)
                    continue;
                if (bgIdx == 0 && !mem->lcd.dispcnt.bits.bg0)
                    continue;
                // Skip background layers that do not match the current priority
                if ((mem->lcd.bgcnt[bgIdx].bits.bgPriority) != prio)
                    continue;

                // Get background properties
                Word chrBase = (mem->lcd.bgcnt[bgIdx].bits.charBase) << 14;
                bool is256 = mem->lcd.bgcnt[bgIdx].bits.palette;
                HalfWord screenBase = (mem->lcd.bgcnt[bgIdx].bits.screenBase) << 11;
                bool affineWrap = mem->lcd.bgcnt[bgIdx].bits.wrap;
                HalfWord screenSize = mem->lcd.bgcnt[bgIdx].bits.screenSize;

                bool affine = ((mode == 2) || (mode == 1 && bgIdx == 2));
                Word address = surfAddr;

                if (affine)
                {
                    // Affine background rendering
                    int16_t pa = mem->lcd.bgpa[bgIdx].full;
                    int16_t pb = mem->lcd.bgpb[bgIdx].full;
                    int16_t pc = mem->lcd.bgpc[bgIdx].full;
                    int16_t pd = mem->lcd.bgpd[bgIdx].full;

                    int32_t ox = ((int32_t)mem->internalPX[bgIdx].full << 4) >> 4;
                    int32_t oy = ((int32_t)mem->internalPY[bgIdx].full << 4) >> 4;
                    mem->internalPX[bgIdx].full += pb;
                    mem->internalPY[bgIdx].full += pd;

                    Byte tms = 16 << screenSize;
                    Byte tmsk = tms - 1;

                    Byte x;

                    // Iterate through the pixels in the scanline
                    for (x = 0; x < 240; x++, ox += pa, oy += pc, address += 4)
                    {
                        int16_t tmx = ox >> 11;
                        int16_t tmy = oy >> 11;

                        if (affineWrap)
                        {
                            tmx &= tmsk;
                            tmy &= tmsk;
                        }
                        else
                        {
                            if (tmx < 0 || tmx >= tms)
                                continue;
                            if (tmy < 0 || tmy >= tms)
                                continue;
                        }

                        HalfWord chrX = (ox >> 8) & 7;
                        HalfWord chrY = (oy >> 8) & 7;

                        Word mapAddr = screenBase + tmy * tms + tmx;

                        Word vramAddr = chrBase + mem->vram[mapAddr] * 64 + chrY * 8 + chrX;

                        HalfWord palIdx = mem->vram[vramAddr];
                        if (palIdx)
                            frame[address / 4] = mem->palette[palIdx];
                    }
                }
                else
                {
                    // Regular background rendering
                    HalfWord oy = mem->lcd.vcount.full + mem->lcd.bgvofs[bgIdx].full;
                    HalfWord tmy = oy >> 3;
                    HalfWord screenY = (tmy >> 5) & 1;

                    Byte x;

                    // Iterate through the pixels in the scanline
                    for (x = 0; x < 240; x++)
                    {
                        HalfWord ox = x + mem->lcd.bghofs[bgIdx].full;
                        HalfWord tmx = ox >> 3;
                        HalfWord screenX = (tmx >> 5) & 1;

                        HalfWord chrX = ox & 7;
                        HalfWord chrY = oy & 7;

                        HalfWord palIdx;
                        HalfWord palBase = 0;

                        Word mapAddr = screenBase + (tmy & 0x1f) * 32 * 2 + (tmx & 0x1f) * 2;

                        // Adjust map address based on screen size
                        switch (screenSize)
                        {
                        case 1:
                            mapAddr += screenX * 2048;
                            break;
                        case 2:
                            mapAddr += screenY * 2048;
                            break;
                        case 3:
                            mapAddr += screenX * 2048 + screenY * 4096;
                            break;
                        }

                        HalfWord tile = mem->vram[mapAddr + 0] | (mem->vram[mapAddr + 1] << 8);

                        HalfWord chrNum = (tile >> 0) & 0x3ff;
                        bool flipX = (tile >> 10) & 0x1;
                        bool flipY = (tile >> 11) & 0x1;
                        Byte chrPal = (tile >> 12) & 0xf;

                        if (!is256)
                            palBase = chrPal * 16;

                        if (flipX)
                            chrX ^= 7;
                        if (flipY)
                            chrY ^= 7;

                        Word vramAddr;

                        if (is256)
                        {
                            vramAddr = chrBase + chrNum * 64 + chrY * 8 + chrX;
                            palIdx = mem->vram[vramAddr];
                        }
                        else
                        {
                            vramAddr = chrBase + chrNum * 32 + chrY * 4 + (chrX >> 1);
                            palIdx = (mem->vram[vramAddr] >> (chrX & 1) * 4) & 0xf;
                        }

                        Word palAddr = palIdx | palBase;
                        if (palIdx)
                            frame[address / 4] = mem->palette[palAddr];
                        address += 4;
                    }
                }
            }

            renderOBJ(prio); // Render objects with the current priority
        }
    }
    break;

    case 3:
    {
        Byte x;
        Word frameAddr = mem->lcd.vcount.full * 480;

        for (x = 0; x < 240; x++)
        {
            HalfWord pixel = mem->vram[frameAddr + 0] | (mem->vram[frameAddr + 1] << 8);

            Byte r = ((pixel >> 0) & 0x1f) << 3;
            Byte g = ((pixel >> 5) & 0x1f) << 3;
            Byte b = ((pixel >> 10) & 0x1f) << 3;

            Word rgba = 0xff;

            rgba |= (r | (r >> 5)) << 8;
            rgba |= (g | (g >> 5)) << 16;
            rgba |= (b | (b >> 5)) << 24;
            frame[surfAddr / 4] = rgba;

            surfAddr += 4;

            frameAddr += 2;
        }
    }
    break;

    case 4:
    {
        Byte x, screen = (mem->lcd.dispcnt.full >> 4) & 1;
        Word frameAddr = 0xa000 * screen + mem->lcd.vcount.full * 240;

        for (x = 0; x < 240; x++)
        {
            Byte palIdx = mem->vram[frameAddr++];
            frame[surfAddr / 4] = mem->palette[palIdx];

            surfAddr += 4;
        }
    }
    break;
    }
}

// Render a single scanline
static void renderScanline()
{
    Word addrS = mem->lcd.vcount.full * 240 * 4; // Calculate the starting address for the current scanline in the frame buffer
    Word addrE = addrS + 240 * 4;                // Calculate the ending address for the current scanline in the frame buffer
    Word pal0 = mem->palette[0];                 // Get the background color from the palette

    // Clear the scanline with the background color
    for (Word addr = addrS; addr < addrE; addr += 0x10)
    {
        for (int i = 0; i < 4; i++)
        {
            frame[(addr | (i * 4)) / 4] = pal0;
        }
    }

    Byte mode = mem->lcd.dispcnt.bits.bgMode; // Get the current display mode

    if (mode > 2)
    {
        // Render backgrounds and objects for modes 3, 4, and 5
        renderBG(0);
        for (int p = 0; p < 4; p++)
        {
            renderOBJ(p);
        }
    }
    else
    {
        // Render backgrounds for modes 0, 1, and 2
        renderBG();
    }
}

// Start the vertical blank period
static void vblankStart()
{
    if (mem->lcd.dispstat.full & (1 << 3))
        triggerIRQ((1 << 0)); // Trigger a V-Blank interrupt if enabled

    mem->lcd.dispstat.full |= (1 << 0); // Set the V-Blank flag
}

// Start the horizontal blank period
static void hblankStart()
{
    if (mem->lcd.dispstat.full & (1 << 4))
        triggerIRQ((1 << 1)); // Trigger an H-Blank interrupt if enabled

    mem->lcd.dispstat.full |= (1 << 1); // Set the H-Blank flag
}

// Handle V-Count match
static void vcountMatch()
{
    if (mem->lcd.dispstat.full & (1 << 5))
        triggerIRQ((1 << 2)); // Trigger a V-Count interrupt if enabled

    mem->lcd.dispstat.full |= (1 << 2); // Set the V-Count flag
}

void tickPPU(void)
{
    mem->lcd.dispstat.full &= ~VBLK_FLAG; // Clear the V-Blank flag

    SDL_LockTexture(texture, NULL, &frame, &texPitch); // Lock the texture for rendering
    for (mem->lcd.vcount.full = 0; mem->lcd.vcount.full < TOTAL_HEIGHT; mem->lcd.vcount.full++)
    {
        mem->lcd.dispstat.full &= ~(HBLK_FLAG | VCNT_FLAG); // Clear the H-Blank and V-Count flags

        // V-Count match and V-Blank start
        if (mem->lcd.vcount.full == mem->lcd.dispstat.bytes[1])
            vcountMatch(); // Handle V-Count match

        if (mem->lcd.vcount.full == FRAME_HEIGHT)
        {
            // Initialize internal background coordinates
            mem->internalPX[0].full = mem->lcd.bgx[0].full;
            mem->internalPY[0].full = mem->lcd.bgy[0].full;

            mem->internalPX[1].full = mem->lcd.bgx[1].full;
            mem->internalPY[1].full = mem->lcd.bgy[1].full;

            vblankStart();       // Start the V-Blank period
            dmaTransfer(VBLANK); // Perform V-Blank DMA transfer
        }
        executeInput(1006); // Execute input for H-Draw period

        // H-Blank start
        if (mem->lcd.vcount.full < FRAME_HEIGHT)
        {
            renderScanline();    // Render the current scanline
            dmaTransfer(HBLANK); // Perform H-Blank DMA transfer
        }

        hblankStart();                   // Start the H-Blank period
        executeInput(1232 - 1006);       // Execute input for H-Blank period
        soundClock(CYCLES_PER_SCANLINE); // Update the sound clock
    }
    SDL_UnlockTexture(texture);                    // Unlock the texture
    SDL_RenderCopy(renderer, texture, NULL, NULL); // Copy the texture to the renderer
    SDL_RenderPresent(renderer);                   // Present the renderer
    soundOverflow();                               // Handle sound overflow
}