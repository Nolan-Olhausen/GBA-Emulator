/****************************************************************************************************
 *
 * @file:    dma.c
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Contains functions for handling DMA transfers.
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

#include "cpu.h"
#include "memory.h"
#include "dma.h"
#include "apu.h"

// Perform DMA transfer based on the specified timing
void dmaTransfer(dmaTiming timing)
{
    Byte ch;

    // Iterate over all 4 DMA channels
    for (ch = 0; ch < 4; ch++)
    {
        // Check if DMA is enabled and the timing matches
        if (!(mem->dma[ch].control.full & DMA_ENB) ||
            ((mem->dma[ch].control.full >> 12) & 3) != timing)
            continue;

        // Special handling for channel 3
        if (ch == 3)
            eepromIdx = 0;

        // Determine the unit size (2 bytes or 4 bytes)
        int8_t unitSize = (mem->dma[ch].control.full & DMA_32) ? 4 : 2;

        bool destReload = false;

        int8_t destIncrement = 0;
        int8_t srcIncrement = 0;

        // Determine destination increment mode
        switch ((mem->dma[ch].control.full >> 5) & 3)
        {
        case 0:
            destIncrement = unitSize;
            break;
        case 1:
            destIncrement = -unitSize;
            break;
        case 3:
            destIncrement = unitSize;
            destReload = true;
            break;
        }

        // Determine source increment mode
        switch ((mem->dma[ch].control.full >> 7) & 3)
        {
        case 0:
            srcIncrement = unitSize;
            break;
        case 1:
            srcIncrement = -unitSize;
            break;
        }

        // Perform the DMA transfer
        while (dmaCount[ch]--)
        {
            if (mem->dma[ch].control.full & DMA_32)
                memWriteWord(dmaDest[ch], memReadWord(dmaSrc[ch]));
            else
                memWriteHalfWord(dmaDest[ch], memReadHalfWord(dmaSrc[ch]));

            dmaDest[ch] += destIncrement;
            dmaSrc[ch] += srcIncrement;
        }

        // Trigger an interrupt request if enabled
        if (mem->dma[ch].control.full & DMA_IRQ)
            triggerIRQ((1 << 8) << ch);

        // Handle DMA repeat mode
        if (mem->dma[ch].control.full & DMA_REP)
        {
            dmaCount[ch] = mem->dma[ch].count.full;

            if (destReload)
            {
                dmaDest[ch] = mem->dma[ch].destination.full;
            }

            continue;
        }

        // Disable DMA
        mem->dma[ch].control.full &= ~DMA_ENB;
    }
}

// Perform DMA transfer for FIFO (First In, First Out) mode
void dmaTransferFIFO(Byte ch)
{
    // Check if DMA is enabled and the timing matches SPECIAL
    if (!(mem->dma[ch].control.full & DMA_ENB) ||
        ((mem->dma[ch].control.full >> 12) & 3) != SPECIAL)
        return;

    Byte i;

    // Perform the DMA transfer for 4 units
    for (i = 0; i < 4; i++)
    {
        memWriteWord(dmaDest[ch], memReadWord(dmaSrc[ch]));

        // Copy data to FIFO
        if (ch == 1)
            fifoCopy(0);
        else
            fifoCopy(1);

        // Determine source increment mode
        switch ((mem->dma[ch].control.full >> 7) & 3)
        {
        case 0:
            dmaSrc[ch] += 4;
            break;
        case 1:
            dmaSrc[ch] -= 4;
            break;
        }
    }

    // Trigger an interrupt request if enabled
    if (mem->dma[ch].control.full & DMA_IRQ)
        triggerIRQ((1 << 8) << ch);
}

// Load DMA settings and start the transfer if enabled
void dmaLoad(Byte ch, Byte value)
{
    Byte old = mem->dma[ch].control.bytes[1];

    mem->dma[ch].control.bytes[1] = value;

    // Check if DMA is enabled
    if ((old ^ value) & value & 0x80)
    {
        dmaDest[ch] = mem->dma[ch].destination.full;
        dmaSrc[ch] = mem->dma[ch].source.full;

        // Align addresses based on transfer size
        if (mem->dma[ch].control.full & DMA_32)
        {
            dmaDest[ch] &= ~3;
            dmaSrc[ch] &= ~3;
        }
        else
        {
            dmaDest[ch] &= ~1;
            dmaSrc[ch] &= ~1;
        }

        dmaCount[ch] = mem->dma[ch].count.full;

        // Perform the DMA transfer immediately
        dmaTransfer(IMMEDIATELY);
    }
}