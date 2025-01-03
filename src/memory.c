/****************************************************************************************************
 *
 * @file:    memory.c
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Contains functions for handling memory operations.
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

#include "common.h"
#include "memory.h"
#include "cpu.h"
#include "apu.h"
#include "dma.h"

// Scalers and shift values for pixel scaling
static DWord scalers[4] = {0, 6, 8, 10};
static const Byte pscaleShift[4] = {0, 6, 8, 10};

// Macro to check if THUMB mode is activated
#define THUMB_ACTIVATED (cpu->cpsr >> 5 & 1)

// EEPROM operation modes
#define EEPROM_WRITE 2
#define EEPROM_READ 3

// Current flash memory bank
Word flashBank = 0;

// Flash memory operation modes
typedef enum
{
    IDLE,       // Idle mode
    ERASE,      // Erase mode
    WRITE,      // Write mode
    BANK_SWITCH // Bank switch mode
} flashMode;

// Current flash memory mode
flashMode modeFlash = IDLE;

// Flag to indicate if flash ID mode is active
bool modeIdFlash = false;

// Flag to indicate if flash memory is used
bool usedFlash = false;

// Flags to indicate if EEPROM is used and if it is in read mode
bool usedEEPROM = false;
bool readEEPROM = false;

// EEPROM address and read address
Word addrEEPROM = 0;
Word readAddrEEPROM = 0;

// Buffer for EEPROM data
Byte buffEEPROM[0x100];

// Read from EEPROM
static Byte eepromRead(Word address, Byte offset)
{
    // Check if EEPROM is used and the address is within the EEPROM range
    if (usedEEPROM &&
        ((mem->rom > 0x1000000 && (address >> 8) == 0x0dffff) ||
         (mem->rom <= 0x1000000 && (address >> 24) == 0x00000d)))
    {
        if (!offset)
        {
            // Determine the EEPROM mode
            Byte mode = buffEEPROM[0] >> 6;

            switch (mode)
            {
            case 2:
                // EEPROM write mode
                return 1;
            case 3:
            {
                // EEPROM read mode
                Byte value = 0;

                if (eepromIdx >= 4)
                {
                    // Calculate the index and bit position
                    Byte idx = ((eepromIdx - 4) >> 3) & 7;
                    Byte bit = ((eepromIdx - 4) >> 0) & 7;

                    // Read the value from EEPROM
                    value = *(Word *)(eeprom[readAddrEEPROM | idx] >> (bit ^ 7)) & 1;
                }

                eepromIdx++;

                return value;
            }
            }
        }
    }
    else
    {
        // Read from ROM if not EEPROM
        return *(Word *)(mem->rom + (address & 0x1FFFFFF));
    }

    return 0;
}

// Write to EEPROM
static void eepromWrite(Word address, Byte offset, Byte value)
{
    // Check if the address is within the EEPROM range and offset is zero
    if (!offset &&
        ((mem->rom > 0x1000000 && (address >> 8) == 0x0dffff) ||
         (mem->rom <= 0x1000000 && (address >> 24) == 0x00000d)))
    {
        if (eepromIdx == 0)
        {
            // First write, reset readEEPROM flag and clear buffer
            readEEPROM = false;

            HalfWord i;
            for (i = 0; i < 0x100; i++)
                buffEEPROM[i] = 0;
        }

        // Calculate the index and bit position
        Byte idx = (eepromIdx >> 3) & 0xff;
        Byte bit = (eepromIdx >> 0) & 0x7;

        // Write the value to the buffer
        buffEEPROM[idx] |= (value & 1) << (bit ^ 7);

        // Check if the write is complete
        if (++eepromIdx == mem->dma[3].count.full)
        {
            // Determine the EEPROM mode
            Byte mode = buffEEPROM[0] >> 6;

            if (mode & 3)
            {
                // Calculate the EEPROM address
                bool eep512 = (eepromIdx == 2 + 6 + (mode == 2 ? 64 : 0) + 1);

                if (eep512)
                    addrEEPROM = buffEEPROM[0] & 0x3f;
                else
                    addrEEPROM = ((buffEEPROM[0] & 0x3f) << 8) | buffEEPROM[1];

                addrEEPROM <<= 3;

                if (mode == 2)
                {
                    // Write data to EEPROM
                    Byte buffAddr = eep512 ? 1 : 2;
                    DWord value = *(DWord *)(buffEEPROM + buffAddr);
                    *(DWord *)(eeprom + addrEEPROM) = value;
                }
                else
                {
                    // Set read address for EEPROM
                    readAddrEEPROM = addrEEPROM;
                }

                eepromIdx = 0;
            }
        }

        usedEEPROM = true;
    }
}

// Read from Flash memory
static Byte flashRead(Word address)
{
    if (modeIdFlash)
    {
        // Return Flash ID based on address
        switch (address)
        {
        case 0x0e000000:
            return 0x62;
        case 0x0e000001:
            return 0x13;
        }
    }
    else if (usedFlash)
    {
        // Read from Flash memory
        return flash[flashBank | (address & 0xffff)];
    }
    else
    {
        // Read from SRAM if Flash is not used
        return sram[address & 0xffff];
    }

    return 0;
}

// Write to Flash memory
static void flashWrite(Word address, Byte value)
{
    if (modeFlash == WRITE)
    {
        // Write value to Flash memory
        flash[flashBank | (address & 0xffff)] = value;
        modeFlash = IDLE;
    }
    else if (modeFlash == BANK_SWITCH && address == 0x0e000000)
    {
        // Switch Flash memory bank
        flashBank = (value & 1) << 16;
        modeFlash = IDLE;
    }
    else if (sram[0x5555] == 0xaa && sram[0x2aaa] == 0x55)
    {
        if (address == 0x0e005555)
        {
            // Handle Flash memory commands
            switch (value)
            {
            case 0x10:
                if (modeFlash == ERASE)
                {
                    // Erase Flash memory
                    Word idx;
                    for (idx = 0; idx < 0x20000; idx++)
                    {
                        flash[idx] = 0xff;
                    }
                    modeFlash = IDLE;
                }
                break;
            case 0x80:
                modeFlash = ERASE;
                break;
            case 0x90:
                modeIdFlash = true;
                break;
            case 0xa0:
                modeFlash = WRITE;
                break;
            case 0xb0:
                modeFlash = BANK_SWITCH;
                break;
            case 0xf0:
                modeIdFlash = false;
                break;
            }

            if (modeFlash || modeIdFlash)
            {
                usedFlash = true;
            }
        }
        else if (modeFlash == ERASE && value == 0x30)
        {
            // Erase a sector of Flash memory
            Word bankS = address & 0xf000;
            Word bankE = bankS + 0x1000;
            Word idx;
            for (idx = bankS; idx < bankE; idx++)
            {
                flash[flashBank | idx] = 0xff;
            }
            modeFlash = IDLE;
        }
    }

    // Write value to SRAM
    sram[address & 0xffff] = value;
}

/******************************************************************************
 * Implements WaitState Operation
 *****************************************************************************/
// Access times for 16-bit memory operations
static Word accessTime16[2][16] = {
    [0] = {1, 1, 3, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1}, // Non-sequential access times
    [1] = {1, 1, 3, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1}  // Sequential access times
};

// Access times for 32-bit memory operations
static Word accessTime32[2][16] = {
    [0] = {1, 1, 6, 1, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1}, // Non-sequential access times
    [1] = {1, 1, 6, 1, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1}  // Sequential access times
};

// Non-sequential access times for game memory
static const Byte gameNonSeq[4] = {4, 3, 2, 8};

void updateWait()
{
    // Update non-sequential access times for 16-bit memory operations
    accessTime16[0][(CART_0_START >> 24)] = 1 + gameNonSeq[mem->iwpdc.waitcnt.bits.ws01];
    accessTime16[0][(CART_0_END >> 24)] = 1 + gameNonSeq[mem->iwpdc.waitcnt.bits.ws01];
    accessTime16[0][(CART_1_START >> 24)] = 1 + gameNonSeq[mem->iwpdc.waitcnt.bits.ws11];
    accessTime16[0][(CART_1_END >> 24)] = 1 + gameNonSeq[mem->iwpdc.waitcnt.bits.ws11];
    accessTime16[0][(CART_2_START >> 24)] = 1 + gameNonSeq[mem->iwpdc.waitcnt.bits.ws21];
    accessTime16[0][(CART_2_END >> 24)] = 1 + gameNonSeq[mem->iwpdc.waitcnt.bits.ws21];
    accessTime16[0][(SRAM_START >> 24)] = 1 + gameNonSeq[mem->iwpdc.waitcnt.bits.sram];

    // Update sequential access times for 16-bit memory operations
    accessTime16[1][(CART_0_START >> 24)] = 1 + (mem->iwpdc.waitcnt.bits.ws02 ? 1 : 2);
    accessTime16[1][(CART_0_END >> 24)] = 1 + (mem->iwpdc.waitcnt.bits.ws02 ? 1 : 2);
    accessTime16[1][(CART_1_START >> 24)] = 1 + (mem->iwpdc.waitcnt.bits.ws12 ? 1 : 4);
    accessTime16[1][(CART_1_END >> 24)] = 1 + (mem->iwpdc.waitcnt.bits.ws12 ? 1 : 4);
    accessTime16[1][(CART_2_START >> 24)] = 1 + (mem->iwpdc.waitcnt.bits.ws22 ? 1 : 8);
    accessTime16[1][(CART_2_END >> 24)] = 1 + (mem->iwpdc.waitcnt.bits.ws22 ? 1 : 8);
    accessTime16[1][(SRAM_START >> 24)] = 1 + gameNonSeq[mem->iwpdc.waitcnt.bits.sram];

    // Update access times for 32-bit memory operations
    for (Word x = (CART_0_START >> 24); x <= (SRAM_START >> 24); ++x)
    {
        accessTime32[0][x] = accessTime16[0][x] + accessTime16[1][x];
        accessTime32[1][x] = 2 * accessTime16[1][x];
    }
}

/******************************************************************************
 * Implements IRQ Operations
 *****************************************************************************/

void triggerIRQ(HalfWord flag)
{
    // Set the interrupt flag
    mem->iwpdc.i_f.full |= flag;

    // Exit power-down mode
    mem->iwpdc.haltcnt.bits.powerDown = false;
}

/******************************************************************************
 * Implements Timer Memory Operations
 *****************************************************************************/

void updateTimer(Word cycles)
{
    Byte timerId;
    Bit overflow = false;

    for (timerId = 0; timerId < 4; timerId++)
    {
        // Check if the timer is enabled
        if (!(mem->timers[timerId].control.full & (1 << 7)))
        {
            overflow = false;
            continue;
        }

        // Check if the timer is cascaded
        if (mem->timers[timerId].control.full & (1 << 2))
        {
            if (overflow)
                mem->timers[timerId].counter.full++;
        }
        else
        {
            // Update the timer based on the cycle count and prescaler
            Byte shift = pscaleShift[mem->timers[timerId].control.full & 3];
            Word inc = (timerTemps[timerId] += cycles) >> shift;

            mem->timers[timerId].counter.full += inc;
            timerTemps[timerId] -= inc << shift;
        }

        // Check for timer overflow
        if ((overflow = (mem->timers[timerId].counter.full > 0xffff)))
        {
            // Reload the timer and handle overflow
            mem->timers[timerId].counter.full = mem->timers[timerId].reload.full + (mem->timers[timerId].counter.full - 0x10000);

            // Handle sound FIFO for timer 0 and 1
            if (((mem->sound.soundcnt_h.full >> 10) & 1) == timerId)
            {
                fifoLoad(0);

                if (mem->sound.fifo[0].size <= 0x10)
                    dmaTransferFIFO(1);
            }

            if (((mem->sound.soundcnt_h.full >> 14) & 1) == timerId)
            {
                fifoLoad(1);

                if (mem->sound.fifo[1].size <= 0x10)
                    dmaTransferFIFO(2);
            }
        }

        // Trigger an interrupt request if enabled and overflow occurred
        if ((mem->timers[timerId].control.full & (1 << 6)) && overflow)
            triggerIRQ((1 << 3) << timerId);
    }
}

void memWriteTimer(Word timerId, Byte byte)
{
    Byte old = mem->timers[timerId].control.bytes[0];

    mem->timers[timerId].control.bytes[0] = byte;

    // Enable or disable the timer based on the control byte
    if (byte & (1 << 7))
        timerENB |= (1 << timerId);
    else
        timerENB &= ~(1 << timerId);

    // If the timer was just enabled, reset the counter and timer temp
    if ((old ^ byte) & byte & (1 << 7))
    {
        mem->timers[timerId].counter.full = mem->timers[timerId].reload.full;
        timerTemps[timerId] = 0;
    }
}

/******************************************************************************
 * Implements File Loading Operations
 *****************************************************************************/

void loadBios(char *biosFile)
{
    // Open the BIOS file in binary read mode
    FILE *fp = fopen(biosFile, "rb");
    if (fp == NULL)
    {
        // Print an error message and exit if the file cannot be opened
        fprintf(stderr, "ERROR: file (%s) failed to open\n", biosFile);
        exit(1);
    }

    // Move the file pointer to the end of the file to determine its size
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET); // Move the file pointer back to the beginning

    // Read the BIOS file into the BIOS memory
    fread(mem->bios, sizeof(Byte), size, fp);

    // Close the file
    fclose(fp);
}

void loadRom(char *romFile)
{
    // Open the ROM file in binary read mode
    FILE *fp = fopen(romFile, "rb");
    if (fp == NULL)
    {
        // Print an error message and exit if the file cannot be opened
        fprintf(stderr, "ERROR: file (%s) failed to open\n", romFile);
        exit(1);
    }

    // Move the file pointer to the end of the file to determine its size
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET); // Move the file pointer back to the beginning

    // Read the ROM file into the ROM memory
    fread(mem->rom, sizeof(Byte), size, fp);

    // Close the file
    fclose(fp);
}

/******************************************************************************
 * Implements Memory Read Operations
 *****************************************************************************/

Byte memReadIO(Word addr)
{
    switch (addr)
    {
    /* LCD I/O Registers */
    case REG_DISPCNT:
        return (mem->lcd.dispcnt.bytes[0]);
    case REG_DISPCNT + 1:
        return (mem->lcd.dispcnt.bytes[1]);
    case REG_GREENSWP:
        return (mem->lcd.greenswp.bytes[0]);
    case REG_GREENSWP + 1:
        return (mem->lcd.greenswp.bytes[1]);
    case REG_DISPSTAT:
        return (mem->lcd.dispstat.bytes[0]);
    case REG_DISPSTAT + 1:
        return (mem->lcd.dispstat.bytes[1]);
    case REG_VCOUNT:
        return (mem->lcd.vcount.bytes[0]);
    case REG_VCOUNT + 1:
        return (mem->lcd.vcount.bytes[1]);
    case REG_BG0CNT:
        return (mem->lcd.bgcnt[0].bytes[0]);
    case REG_BG0CNT + 1:
        return (mem->lcd.bgcnt[0].bytes[1]);
    case REG_BG1CNT:
        return (mem->lcd.bgcnt[1].bytes[0]);
    case REG_BG1CNT + 1:
        return (mem->lcd.bgcnt[1].bytes[1]);
    case REG_BG2CNT:
        return (mem->lcd.bgcnt[2].bytes[0]);
    case REG_BG2CNT + 1:
        return (mem->lcd.bgcnt[2].bytes[1]);
    case REG_BG3CNT:
        return (mem->lcd.bgcnt[3].bytes[0]);
    case REG_BG3CNT + 1:
        return (mem->lcd.bgcnt[3].bytes[1]);
    case REG_WININ:
        return (mem->lcd.winin.bytes[0]);
    case REG_WININ + 1:
        return (mem->lcd.winin.bytes[1]);
    case REG_WINOUT:
        return (mem->lcd.winout.bytes[0]);
    case REG_WINOUT + 1:
        return (mem->lcd.winout.bytes[1]);
    case REG_BLDCNT:
        return (mem->lcd.bldcnt.bytes[0]);
    case REG_BLDCNT + 1:
        return (mem->lcd.bldcnt.bytes[1]);
    case REG_BLDALPHA:
        return (mem->lcd.bldalpha.bytes[0]);
    case REG_BLDALPHA + 1:
        return (mem->lcd.bldalpha.bytes[1]);

    /* Sound Registers */
    case REG_SOUND1CNT_L:
        return (mem->sound.sound1cnt_l.bytes[0]);
    case REG_SOUND1CNT_L + 1:
        return (mem->sound.sound1cnt_l.bytes[1]);
    case REG_SOUND1CNT_H:
        return (mem->sound.sound1cnt_h.bytes[0] & 0xC0);
    case REG_SOUND1CNT_H + 1:
        return (mem->sound.sound1cnt_h.bytes[1]);
    case REG_SOUND1CNT_X + 1:
        return (mem->sound.sound1cnt_x.bytes[1] & 0x40);
    case REG_SOUND2CNT_L:
        return (mem->sound.sound2cnt_l.bytes[0] & 0xC0);
    case REG_SOUND2CNT_L + 1:
        return (mem->sound.sound2cnt_l.bytes[1]);
    case REG_SOUND2CNT_H + 1:
        return (mem->sound.sound2cnt_h.bytes[1] & 0x40);
    case REG_SOUND3CNT_L:
        return (mem->sound.sound3cnt_l.bytes[0] & 0xE0);
    case REG_SOUND3CNT_H + 1:
        return (mem->sound.sound3cnt_h.bytes[1] & 0xE0);
    case REG_SOUND3CNT_X + 1:
        return (mem->sound.sound3cnt_x.bytes[1] & 0x40);
    case REG_SOUND4CNT_L + 1:
        return (mem->sound.sound4cnt_l.bytes[1]);
    case REG_SOUND4CNT_H:
        return (mem->sound.sound4cnt_h.bytes[0]);
    case REG_SOUND4CNT_H + 1:
        return (mem->sound.sound4cnt_h.bytes[1] & 0x40);
    case REG_SOUNDCNT_L:
        return (mem->sound.soundcnt_l.bytes[0]);
    case REG_SOUNDCNT_L + 1:
        return (mem->sound.soundcnt_l.bytes[1]);
    case REG_SOUNDCNT_H:
        return (mem->sound.soundcnt_h.bytes[0]);
    case REG_SOUNDCNT_H + 1:
        return (mem->sound.soundcnt_h.bytes[1]);
    case REG_SOUNDCNT_X:
        return (mem->sound.soundcnt_x.bytes[0] & 0x8F);
    case REG_SOUNDBIAS:
        return (mem->sound.soundbias.bytes[0]);
    case REG_SOUNDBIAS + 1:
        return (mem->sound.soundbias.bytes[1]);
    case REG_WAVE_RAM0 + 0:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[0].bytes[0]);
    case REG_WAVE_RAM0 + 1:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[0].bytes[1]);
    case REG_WAVE_RAM0 + 2:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[1].bytes[0]);
    case REG_WAVE_RAM0 + 3:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[1].bytes[1]);
    case REG_WAVE_RAM1 + 0:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[2].bytes[0]);
    case REG_WAVE_RAM1 + 1:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[2].bytes[1]);
    case REG_WAVE_RAM1 + 2:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[3].bytes[0]);
    case REG_WAVE_RAM1 + 3:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[3].bytes[1]);
    case REG_WAVE_RAM2 + 0:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[4].bytes[0]);
    case REG_WAVE_RAM2 + 1:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[4].bytes[1]);
    case REG_WAVE_RAM2 + 2:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[5].bytes[0]);
    case REG_WAVE_RAM2 + 3:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[5].bytes[1]);
    case REG_WAVE_RAM3 + 0:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[6].bytes[0]);
    case REG_WAVE_RAM3 + 1:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[6].bytes[1]);
    case REG_WAVE_RAM3 + 2:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[7].bytes[0]);
    case REG_WAVE_RAM3 + 3:
        return (mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[7].bytes[1]);

    /* DMA Transfer Channels */
    case REG_DMA0CNT_H:
        return (mem->dma[0].control.bytes[0]);
    case REG_DMA0CNT_H + 1:
        return (mem->dma[0].control.bytes[1]);
    case REG_DMA1CNT_H:
        return (mem->dma[1].control.bytes[0]);
    case REG_DMA1CNT_H + 1:
        return (mem->dma[1].control.bytes[1]);
    case REG_DMA2CNT_H:
        return (mem->dma[2].control.bytes[0]);
    case REG_DMA2CNT_H + 1:
        return (mem->dma[2].control.bytes[1]);
    case REG_DMA3CNT_H:
        return (mem->dma[3].control.bytes[0]);
    case REG_DMA3CNT_H + 1:
        return (mem->dma[3].control.bytes[1]);

    /* Timer Registers */
    case REG_TM0CNT_L:
        return (mem->timers[0].counter.bytes[0]);
    case REG_TM0CNT_L + 1:
        return (mem->timers[0].counter.bytes[1]);
    case REG_TM0CNT_H:
        return (mem->timers[0].control.bytes[0]);
    case REG_TM1CNT_L:
        return (mem->timers[1].counter.bytes[0]);
    case REG_TM1CNT_L + 1:
        return (mem->timers[1].counter.bytes[1]);
    case REG_TM1CNT_H:
        return (mem->timers[1].control.bytes[0]);
    case REG_TM2CNT_L:
        return (mem->timers[2].counter.bytes[0]);
    case REG_TM2CNT_L + 1:
        return (mem->timers[2].counter.bytes[1]);
    case REG_TM2CNT_H:
        return (mem->timers[2].control.bytes[0]);
    case REG_TM3CNT_L:
        return (mem->timers[3].counter.bytes[0]);
    case REG_TM3CNT_L + 1:
        return (mem->timers[3].counter.bytes[1]);
    case REG_TM3CNT_H:
        return (mem->timers[3].control.bytes[0]);

    /* Keypad Input */
    case REG_KEYINPUT:
        return (mem->keypad.keyinput.bytes[0]);
    case REG_KEYINPUT + 1:
        return (mem->keypad.keyinput.bytes[1]);
    case REG_KEYCNT:
        return (mem->keypad.keycnt.bytes[0]);
    case REG_KEYCNT + 1:
        return (mem->keypad.keycnt.bytes[1]);

    /* Serial Communication */
    case REG_SIOCNT:
        return (mem->comm.siocnt.bytes[0]);
    case REG_SIOCNT + 1:
        return (mem->comm.siocnt.bytes[1]);
    case REG_RCNT:
        return (mem->comm.rcnt.bytes[0]);
    case REG_RCNT + 1:
        return (mem->comm.rcnt.bytes[1]);

    /* Interrupt, Waitstate, and Power-Down Control */
    case REG_IE:
        return (mem->iwpdc.ie.bytes[0]);
    case REG_IE + 1:
        return (mem->iwpdc.ie.bytes[1]);
    case REG_IF:
        return (mem->iwpdc.i_f.bytes[0]);
    case REG_IF + 1:
        return (mem->iwpdc.i_f.bytes[1]);
    case REG_WAITCNT:
        return (mem->iwpdc.waitcnt.bytes[0]);
    case REG_WAITCNT + 1:
        return (mem->iwpdc.waitcnt.bytes[1]);
    case REG_IME:
        return (mem->iwpdc.ime.bytes[0]);
    case REG_POSTFLG:
        return (mem->iwpdc.postflag.full);

    /*
     * Default to return 0 for registers that dont have read access.
     * If invalid read though, could cause issues, but should be uncommon
     */
    default:
        printf("Potential Invalid I/O read at %08X, returning 0", addr);
        return (0);
    }
}

Word memReadWord(Word addr)
{
    addr &= ~3; // Align the address to a 4-byte boundary
    Word ret;

    switch ((addr >> 24) & 0xFF)
    {
    case 0x00:
        // Read from BIOS
        if ((addr | cpu->regs[15]) < 0x4000)
        {
            return *(Word *)(mem->bios + (addr & 0x3FFF));
        }
        else
        {
            return mem->biosBus;
        }
    case 0x02:
        // Read from external work RAM (eWRAM)
        return *(Word *)(mem->eWRAM + (addr & 0x3FFFF));
    case 0x03:
        // Read from internal work RAM (iWRAM)
        return *(Word *)(mem->iWRAM + (addr & 0x7FFF));
    case 0x04:
        // Read from I/O registers
        ret = (memReadIO(addr + 0) << 0) | (memReadIO(addr + 1) << 8) | (memReadIO(addr + 2) << 16) | (memReadIO(addr + 3) << 24);
        return ret;
    case 0x05:
        // Read from palette RAM
        return *(Word *)(mem->palRAM + (addr & 0x3FF));
    case 0x06:
        // Read from video RAM (VRAM)
        return *(Word *)(mem->vram + (addr & (addr & 0x10000 ? 0x17fff : 0x1ffff)));
    case 0x07:
        // Read from object attribute memory (OAM)
        return *(Word *)(mem->oam + (addr & 0x3FF));
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
        // Read from ROM
        return *(Word *)(mem->rom + (addr & 0x1FFFFFF));
    case 0x0C:
    case 0x0D:
        // Read from EEPROM
        ret = eepromRead(addr | 0, 0) << 0 | eepromRead(addr | 1, 1) << 8 | eepromRead(addr | 2, 2) << 16 | eepromRead(addr | 3, 3) << 24;
        return ret;
    case 0x0E:
    case 0x0F:
        // Read from flash memory
        ret = flashRead(addr | 0) << 0 | flashRead(addr | 1) << 8 | flashRead(addr | 2) << 16 | flashRead(addr | 3) << 24;
        return ret;
    }
    return 0; // Default case, return 0 if address is not handled
}

HalfWord memReadHalfWord(Word addr)
{
    HalfWord ret;
    addr &= ~1; // Align the address to a 2-byte boundary

    switch ((addr >> 24) & 0xFF)
    {
    case 0x00:
        // Read from BIOS
        if ((addr | cpu->regs[15]) < 0x4000)
        {
            return *(HalfWord *)(mem->bios + (addr & 0x3FFF));
        }
        else
        {
            return mem->biosBus;
        }
    case 0x02:
        // Read from external work RAM (eWRAM)
        return *(HalfWord *)(mem->eWRAM + (addr & 0x3FFFF));
    case 0x03:
        // Read from internal work RAM (iWRAM)
        return *(HalfWord *)(mem->iWRAM + (addr & 0x7FFF));
    case 0x04:
        // Read from I/O registers
        ret = (memReadIO(addr + 0) << 0) | (memReadIO(addr + 1) << 8);
        return ret;
    case 0x05:
        // Read from palette RAM
        return *(HalfWord *)(mem->palRAM + (addr & 0x3FF));
    case 0x06:
        // Read from video RAM (VRAM)
        return *(HalfWord *)(mem->vram + (addr & (addr & 0x10000 ? 0x17fff : 0x1ffff)));
    case 0x07:
        // Read from object attribute memory (OAM)
        return *(HalfWord *)(mem->oam + (addr & 0x3FF));
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
        // Read from ROM
        return *(HalfWord *)(mem->rom + (addr & 0x1FFFFFF));
    case 0x0C:
    case 0x0D:
        // Read from EEPROM
        ret = eepromRead(addr | 0, 0) << 0 | eepromRead(addr | 1, 1) << 8;
        return ret;
    case 0x0E:
    case 0x0F:
        // Read from flash memory
        ret = flashRead(addr | 0) << 0 | flashRead(addr | 1) << 8;
        return ret;
    }
    return 0; // Default case, return 0 if address is not handled
}

Byte memReadByte(Word addr)
{
    Byte ret;
    switch ((addr >> 24) & 0xFF)
    {
    case 0x00:
        // Read from BIOS
        if ((addr | cpu->regs[15]) < 0x4000)
        {
            return *(Byte *)(mem->bios + (addr & 0x3FFF));
        }
        else
        {
            return mem->biosBus;
        }
    case 0x02:
        // Read from external work RAM (eWRAM)
        return *(Byte *)(mem->eWRAM + (addr & 0x3FFFF));
    case 0x03:
        // Read from internal work RAM (iWRAM)
        return *(Byte *)(mem->iWRAM + (addr & 0x7FFF));
    case 0x04:
        // Read from I/O registers
        ret = (memReadIO(addr + 0) << 0);
        return ret;
    case 0x05:
        // Read from palette RAM
        return *(Byte *)(mem->palRAM + (addr & 0x3FF));
    case 0x06:
        // Read from video RAM (VRAM)
        return *(Byte *)(mem->vram + (addr & (addr & 0x10000 ? 0x17fff : 0x1ffff)));
    case 0x07:
        // Read from object attribute memory (OAM)
        return *(Byte *)(mem->oam + (addr & 0x3FF));
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
        // Read from ROM
        return *(Byte *)(mem->rom + (addr & 0x1FFFFFF));
    case 0x0C:
    case 0x0D:
        // Read from EEPROM
        ret = eepromRead(addr | 0, 0) << 0;
        return ret;
    case 0x0E:
    case 0x0F:
        // Read from flash memory
        ret = flashRead(addr | 0) << 0;
        return ret;
    }
    return 0; // Default case, return 0 if address is not handled
}

/******************************************************************************
 * Implements Memory Write Operations
 *****************************************************************************/

void memWriteIO(Word addr, Byte byte)
{
    switch (addr)
    {
    /* LCD I/O Registers */
    case REG_DISPCNT:
        if (cpu->regs[15] >= 0x4000)
        {
            // The CGB mode enable bit 3 can only be set by the bios
            byte &= 0xf7;
        }

        mem->lcd.dispcnt.bytes[0] = byte;
        break;
    case REG_DISPCNT + 1:
        mem->lcd.dispcnt.bytes[1] = byte;
        break;
    case REG_GREENSWP:
        mem->lcd.greenswp.bytes[0] = byte;
        break;
    case REG_GREENSWP + 1:
        mem->lcd.greenswp.bytes[1] = byte;
        break;
    case REG_DISPSTAT:
        mem->lcd.dispstat.bytes[0] &= 0x47;
        mem->lcd.dispstat.bytes[0] |= byte & ~0x47;
        break;
    case REG_DISPSTAT + 1:
        mem->lcd.dispstat.bytes[1] = byte;
        break;
    case REG_BG0CNT:
        mem->lcd.bgcnt[0].bytes[0] = byte;
        break;
    case REG_BG0CNT + 1:
        mem->lcd.bgcnt[0].bytes[1] = byte & 0xDF;
        break;
    case REG_BG1CNT:
        mem->lcd.bgcnt[1].bytes[0] = byte;
        break;
    case REG_BG1CNT + 1:
        mem->lcd.bgcnt[1].bytes[1] = byte & 0xDF;
        break;
    case REG_BG2CNT:
        mem->lcd.bgcnt[2].bytes[0] = byte;
        break;
    case REG_BG2CNT + 1:
        mem->lcd.bgcnt[2].bytes[1] = byte;
        break;
    case REG_BG3CNT:
        mem->lcd.bgcnt[3].bytes[0] = byte;
        break;
    case REG_BG3CNT + 1:
        mem->lcd.bgcnt[3].bytes[1] = byte;
        break;
    case REG_BG0HOFS:
        mem->lcd.bghofs[0].bytes[0] = byte;
        break;
    case REG_BG0HOFS + 1:
        mem->lcd.bghofs[0].bytes[1] = byte & 0x1;
        break;
    case REG_BG0VOFS:
        mem->lcd.bgvofs[0].bytes[0] = byte;
        break;
    case REG_BG0VOFS + 1:
        mem->lcd.bgvofs[0].bytes[1] = byte & 0x1;
        break;
    case REG_BG1HOFS:
        mem->lcd.bghofs[1].bytes[0] = byte;
        break;
    case REG_BG1HOFS + 1:
        mem->lcd.bghofs[1].bytes[1] = byte & 0x1;
        break;
    case REG_BG1VOFS:
        mem->lcd.bgvofs[1].bytes[0] = byte;
        break;
    case REG_BG1VOFS + 1:
        mem->lcd.bgvofs[1].bytes[1] = byte & 0x1;
        break;
    case REG_BG2HOFS:
        mem->lcd.bghofs[2].bytes[0] = byte;
        break;
    case REG_BG2HOFS + 1:
        mem->lcd.bghofs[2].bytes[1] = byte & 0x1;
        break;
    case REG_BG2VOFS:
        mem->lcd.bgvofs[2].bytes[0] = byte;
        break;
    case REG_BG2VOFS + 1:
        mem->lcd.bgvofs[2].bytes[1] = byte & 0x1;
        break;
    case REG_BG3HOFS:
        mem->lcd.bghofs[3].bytes[0] = byte;
        break;
    case REG_BG3HOFS + 1:
        mem->lcd.bghofs[3].bytes[1] = byte & 0x1;
        break;
    case REG_BG3VOFS:
        mem->lcd.bgvofs[3].bytes[0] = byte;
        break;
    case REG_BG3VOFS + 1:
        mem->lcd.bgvofs[3].bytes[1] = byte & 0x1;
        break;
    case REG_BG2PA:
        mem->lcd.bgpa[0].bytes[0] = byte;
        break;
    case REG_BG2PA + 1:
        mem->lcd.bgpa[0].bytes[1] = byte;
        break;
    case REG_BG2PB:
        mem->lcd.bgpb[0].bytes[0] = byte;
        break;
    case REG_BG2PB + 1:
        mem->lcd.bgpb[0].bytes[1] = byte;
        break;
    case REG_BG2PC:
        mem->lcd.bgpc[0].bytes[0] = byte;
        break;
    case REG_BG2PC + 1:
        mem->lcd.bgpc[0].bytes[1] = byte;
        break;
    case REG_BG2PD:
        mem->lcd.bgpd[0].bytes[0] = byte;
        break;
    case REG_BG2PD + 1:
        mem->lcd.bgpd[0].bytes[1] = byte;
        break;
    case REG_BG2X:
        mem->internalPX[0].bytes[0] = mem->lcd.bgx[0].bytes[0] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG2X + 1:
        mem->internalPX[0].bytes[1] = mem->lcd.bgx[0].bytes[1] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG2X + 2:
        mem->internalPX[0].bytes[2] = mem->lcd.bgx[0].bytes[2] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG2X + 3:
        mem->internalPX[0].bytes[3] = mem->lcd.bgx[0].bytes[3] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG2Y:
        mem->internalPY[0].bytes[0] = mem->lcd.bgy[0].bytes[0] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG2Y + 1:
        mem->internalPY[0].bytes[1] = mem->lcd.bgy[0].bytes[1] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG2Y + 2:
        mem->internalPY[0].bytes[2] = mem->lcd.bgy[0].bytes[2] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG2Y + 3:
        mem->internalPY[0].bytes[3] = mem->lcd.bgy[0].bytes[3] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG3PA:
        mem->lcd.bgpa[1].bytes[0] = byte;
        break;
    case REG_BG3PA + 1:
        mem->lcd.bgpa[1].bytes[1] = byte;
        break;
    case REG_BG3PB:
        mem->lcd.bgpb[1].bytes[0] = byte;
        break;
    case REG_BG3PB + 1:
        mem->lcd.bgpb[1].bytes[1] = byte;
        break;
    case REG_BG3PC:
        mem->lcd.bgpc[1].bytes[0] = byte;
        break;
    case REG_BG3PC + 1:
        mem->lcd.bgpc[1].bytes[1] = byte;
        break;
    case REG_BG3PD:
        mem->lcd.bgpd[1].bytes[0] = byte;
        break;
    case REG_BG3PD + 1:
        mem->lcd.bgpd[1].bytes[1] = byte;
        break;
    case REG_BG3X:
        mem->internalPX[1].bytes[0] = mem->lcd.bgx[1].bytes[0] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG3X + 1:
        mem->internalPX[1].bytes[1] = mem->lcd.bgx[1].bytes[1] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG3X + 2:
        mem->internalPX[1].bytes[2] = mem->lcd.bgx[1].bytes[2] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG3X + 3:
        mem->internalPX[1].bytes[3] = mem->lcd.bgx[1].bytes[3] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG3Y:
        mem->internalPY[1].bytes[0] = mem->lcd.bgy[1].bytes[0] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG3Y + 1:
        mem->internalPY[1].bytes[1] = mem->lcd.bgy[1].bytes[1] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG3Y + 2:
        mem->internalPY[1].bytes[2] = mem->lcd.bgy[1].bytes[2] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_BG3Y + 3:
        mem->internalPY[1].bytes[3] = mem->lcd.bgy[1].bytes[3] = byte;
        break; // gba->ppu.reload_internal_affine_regs = true; break;
    case REG_WIN0H:
        mem->lcd.winh[0].bytes[0] = byte;
        break;
    case REG_WIN0H + 1:
        mem->lcd.winh[0].bytes[1] = byte;
        break;
    case REG_WIN1H:
        mem->lcd.winh[1].bytes[0] = byte;
        break;
    case REG_WIN1H + 1:
        mem->lcd.winh[1].bytes[1] = byte;
        break;
    case REG_WIN0V:
        mem->lcd.winv[0].bytes[0] = byte;
        break;
    case REG_WIN0V + 1:
        mem->lcd.winv[0].bytes[1] = byte;
        break;
    case REG_WIN1V:
        mem->lcd.winv[1].bytes[0] = byte;
        break;
    case REG_WIN1V + 1:
        mem->lcd.winv[1].bytes[1] = byte;
        break;
    case REG_WININ:
        mem->lcd.winin.bytes[0] = byte & 0x3F;
        break;
    case REG_WININ + 1:
        mem->lcd.winin.bytes[1] = byte & 0x3F;
        break;
    case REG_WINOUT:
        mem->lcd.winout.bytes[0] = byte & 0x3F;
        break;
    case REG_WINOUT + 1:
        mem->lcd.winout.bytes[1] = byte & 0x3F;
        break;
    case REG_MOSAIC:
        mem->lcd.mosaic.bytes[0] = byte;
        break;
    case REG_MOSAIC + 1:
        mem->lcd.mosaic.bytes[1] = byte;
        break;
    case REG_BLDCNT:
        mem->lcd.bldcnt.bytes[0] = byte;
        break;
    case REG_BLDCNT + 1:
        mem->lcd.bldcnt.bytes[1] = byte & 0x3F;
        break;
    case REG_BLDALPHA:
        mem->lcd.bldalpha.bytes[0] = byte & 0x1F;
        break;
    case REG_BLDALPHA + 1:
        mem->lcd.bldalpha.bytes[1] = byte & 0x1F;
        break;
    case REG_BLDY:
        mem->lcd.bldy.bytes[0] = byte;
        break;
    case REG_BLDY + 1:
        mem->lcd.bldy.bytes[1] = byte;
        break;

    /* Sound Registers */
    case REG_SOUND1CNT_L:
        if (mem->sound.soundcnt_x.bits.master)
        { // most sound registers become non writable if master is disabled (only few exceptions and wont have this)
            mem->sound.sound1cnt_l.bytes[0] = byte;
        }
        break;
    case REG_SOUND1CNT_H:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound1cnt_h.bytes[0] = byte;
        }
        break;
    case REG_SOUND1CNT_H + 1:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound1cnt_h.bytes[1] = byte;
        }
        break;
    case REG_SOUND1CNT_X:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound1cnt_x.bytes[0] = byte;
        }
        break;
    case REG_SOUND1CNT_X + 1:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound1cnt_x.bytes[1] = byte;

            if (mem->sound.sound1cnt_x.bits.initial)
            {
                channel1Reset();
            }
            mem->sound.sound1cnt_x.bits.initial = 0;
        }
        break;
    case REG_SOUND2CNT_L:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound2cnt_l.bytes[0] = byte;
        }
        break;
    case REG_SOUND2CNT_L + 1:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound2cnt_l.bytes[1] = byte;
        }
        break;
    case REG_SOUND2CNT_H:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound2cnt_h.bytes[0] = byte;
        }
        break;
    case REG_SOUND2CNT_H + 1:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound2cnt_h.bytes[1] = byte;

            if (mem->sound.sound2cnt_h.bits.initial)
            {
                channel2Reset();
            }
            mem->sound.sound2cnt_h.bits.initial = 0;
        }
        break;
    case REG_SOUND3CNT_L:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound3cnt_l.bytes[0] = byte;
        }
        break;
    case REG_SOUND3CNT_L + 1:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound3cnt_l.bytes[1] = byte;
        }
        break;
    case REG_SOUND3CNT_H:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound3cnt_h.bytes[0] = byte;
        }
        break;
    case REG_SOUND3CNT_H + 1:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound3cnt_h.bytes[1] = byte;
        }
        break;
    case REG_SOUND3CNT_X:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound3cnt_x.bytes[0] = byte;
        }
        break;
    case REG_SOUND3CNT_X + 1:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound3cnt_x.bytes[1] = byte;

            if (mem->sound.sound3cnt_x.bits.initial)
            {
                channel3Reset();
            }
            mem->sound.sound3cnt_x.bits.initial = 0;
        }
        break;
    case REG_SOUND4CNT_L:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound4cnt_l.bytes[0] = byte;
        }
        break;
    case REG_SOUND4CNT_L + 1:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound4cnt_l.bytes[1] = byte;
        }
        break;
    case REG_SOUND4CNT_H:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound4cnt_h.bytes[0] = byte;
        }
        break;
    case REG_SOUND4CNT_H + 1:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.sound4cnt_h.bytes[1] = byte;

            if (mem->sound.sound4cnt_h.bits.initial)
            {
                channel4Reset();
            }
            mem->sound.sound4cnt_h.bits.initial = 0;
        }
        break;
    case REG_SOUNDCNT_L:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.soundcnt_l.bytes[0] = byte & 0x77;
        }
        break;
    case REG_SOUNDCNT_L + 1:
        if (mem->sound.soundcnt_x.bits.master)
        {
            mem->sound.soundcnt_l.bytes[1] = byte;
        }
        break;
    case REG_SOUNDCNT_H:
        mem->sound.soundcnt_h.bytes[0] = byte & 0x0F;
        break; // one of the exceptions to master
    case REG_SOUNDCNT_H + 1:
        mem->sound.soundcnt_h.bytes[1] = byte;

        if (mem->sound.soundcnt_h.bits.dmaAReset)
        {
            fifoReset(0);
            mem->sound.soundcnt_h.bits.dmaAReset = 0;
        }
        if (mem->sound.soundcnt_h.bits.dmaBReset)
        {
            fifoReset(1);
            mem->sound.soundcnt_h.bits.dmaBReset = 0;
        }
        break;

    /*
     * While Bit 7 is cleared, both PSG and FIFO sounds are disabled,
     * and all PSG registers at 4000060h..4000081h are reset to zero
     * (and must be re-initialized after re-enabling sound). However,
     * registers 4000082h and 4000088h are kept read/write-able (of which,
     * 4000082h has no function when sound is off, whilst 4000088h does
     * work even when sound is off).
     */
    case REG_SOUNDCNT_X:
        HalfWord old = mem->sound.soundcnt_x.bytes[0] & 0x80;
        mem->sound.soundcnt_x.bytes[0] = byte & 0x80;

        if (old && !mem->sound.soundcnt_x.bits.master)
        {
            fifoReset(0);
            fifoReset(1);
            channel3Reset();
            mem->sound.sound3cnt_l.full = 0;
            mem->sound.sound3cnt_h.full = 0;
            mem->sound.sound3cnt_x.full = 0;
        }
        break;
    case REG_SOUNDBIAS:
        mem->sound.soundbias.bytes[0] = byte;
        break; // soundbias is another exception to master
    case REG_SOUNDBIAS + 1:
        mem->sound.soundbias.bytes[1] = byte;
        break;
    case REG_SOUNDBIAS + 2:
        mem->sound.soundbias.bytes[2] = byte;
        break;
    case REG_SOUNDBIAS + 3:
        mem->sound.soundbias.bytes[3] = byte;
        break;
    case REG_WAVE_RAM0 + 0:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[0].bytes[0] = byte;
        break; // sound master bit no longer applies
    case REG_WAVE_RAM0 + 1:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[0].bytes[1] = byte;
        break;
    case REG_WAVE_RAM0 + 2:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[1].bytes[0] = byte;
        break;
    case REG_WAVE_RAM0 + 3:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[1].bytes[1] = byte;
        break;
    case REG_WAVE_RAM1 + 0:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[2].bytes[0] = byte;
        break;
    case REG_WAVE_RAM1 + 1:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[2].bytes[1] = byte;
        break;
    case REG_WAVE_RAM1 + 2:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[3].bytes[0] = byte;
        break;
    case REG_WAVE_RAM1 + 3:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[3].bytes[1] = byte;
        break;
    case REG_WAVE_RAM2 + 0:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[4].bytes[0] = byte;
        break;
    case REG_WAVE_RAM2 + 1:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[4].bytes[1] = byte;
        break;
    case REG_WAVE_RAM2 + 2:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[5].bytes[0] = byte;
        break;
    case REG_WAVE_RAM2 + 3:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[5].bytes[1] = byte;
        break;
    case REG_WAVE_RAM3 + 0:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[6].bytes[0] = byte;
        break;
    case REG_WAVE_RAM3 + 1:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[6].bytes[1] = byte;
        break;
    case REG_WAVE_RAM3 + 2:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[7].bytes[0] = byte;
        break;
    case REG_WAVE_RAM3 + 3:
        mem->sound.wave_ram[!mem->sound.sound3cnt_l.bits.number].reg[7].bytes[1] = byte;
        break;
    case REG_FIFO_A_L + 0:
        mem->sound.fifo[0].reg.bytes[0] = byte;
        break;
    case REG_FIFO_A_L + 1:
        mem->sound.fifo[0].reg.bytes[1] = byte;
        break;
    case REG_FIFO_A_H + 0:
        mem->sound.fifo[0].reg.bytes[2] = byte;
        break;
    case REG_FIFO_A_H + 1:
        mem->sound.fifo[0].reg.bytes[3] = byte;
        break;
    case REG_FIFO_B_L + 0:
        mem->sound.fifo[1].reg.bytes[0] = byte;
        break;
    case REG_FIFO_B_L + 1:
        mem->sound.fifo[1].reg.bytes[1] = byte;
        break;
    case REG_FIFO_B_H + 0:
        mem->sound.fifo[1].reg.bytes[2] = byte;
        break;
    case REG_FIFO_B_H + 1:
        mem->sound.fifo[1].reg.bytes[3] = byte;
        break;

    /* DMA Transfer Channels */
    case REG_DMA0SAD:
        mem->dma[0].source.bytes[0] = byte;
        break;
    case REG_DMA0SAD + 1:
        mem->dma[0].source.bytes[1] = byte;
        break;
    case REG_DMA0SAD + 2:
        mem->dma[0].source.bytes[2] = byte;
        break;
    case REG_DMA0SAD + 3:
        mem->dma[0].source.bytes[3] = byte;
        break;
    case REG_DMA0DAD:
        mem->dma[0].destination.bytes[0] = byte;
        break;
    case REG_DMA0DAD + 1:
        mem->dma[0].destination.bytes[1] = byte;
        break;
    case REG_DMA0DAD + 2:
        mem->dma[0].destination.bytes[2] = byte;
        break;
    case REG_DMA0DAD + 3:
        mem->dma[0].destination.bytes[3] = byte;
        break;
    case REG_DMA0CNT_L:
        mem->dma[0].count.bytes[0] = byte;
        break;
    case REG_DMA0CNT_L + 1:
        mem->dma[0].count.bytes[1] = byte;
        break;
    case REG_DMA0CNT_H:
        mem->dma[0].control.bytes[0] = byte & 0xE0;
        break;
    case REG_DMA0CNT_H + 1:
        dmaLoad(0, byte);
        break;
    case REG_DMA1SAD:
        mem->dma[1].source.bytes[0] = byte;
        break;
    case REG_DMA1SAD + 1:
        mem->dma[1].source.bytes[1] = byte;
        break;
    case REG_DMA1SAD + 2:
        mem->dma[1].source.bytes[2] = byte;
        break;
    case REG_DMA1SAD + 3:
        mem->dma[1].source.bytes[3] = byte;
        break;
    case REG_DMA1DAD:
        mem->dma[1].destination.bytes[0] = byte;
        break;
    case REG_DMA1DAD + 1:
        mem->dma[1].destination.bytes[1] = byte;
        break;
    case REG_DMA1DAD + 2:
        mem->dma[1].destination.bytes[2] = byte;
        break;
    case REG_DMA1DAD + 3:
        mem->dma[1].destination.bytes[3] = byte;
        break;
    case REG_DMA1CNT_L:
        mem->dma[1].count.bytes[0] = byte;
        break;
    case REG_DMA1CNT_L + 1:
        mem->dma[1].count.bytes[1] = byte;
        break;
    case REG_DMA1CNT_H:
        mem->dma[1].control.bytes[0] = byte & 0xE0;
        break;
    case REG_DMA1CNT_H + 1:
        dmaLoad(1, byte);
        break;
    case REG_DMA2SAD:
        mem->dma[2].source.bytes[0] = byte;
        break;
    case REG_DMA2SAD + 1:
        mem->dma[2].source.bytes[1] = byte;
        break;
    case REG_DMA2SAD + 2:
        mem->dma[2].source.bytes[2] = byte;
        break;
    case REG_DMA2SAD + 3:
        mem->dma[2].source.bytes[3] = byte;
        break;
    case REG_DMA2DAD:
        mem->dma[2].destination.bytes[0] = byte;
        break;
    case REG_DMA2DAD + 1:
        mem->dma[2].destination.bytes[1] = byte;
        break;
    case REG_DMA2DAD + 2:
        mem->dma[2].destination.bytes[2] = byte;
        break;
    case REG_DMA2DAD + 3:
        mem->dma[2].destination.bytes[3] = byte;
        break;
    case REG_DMA2CNT_L:
        mem->dma[2].count.bytes[0] = byte;
        break;
    case REG_DMA2CNT_L + 1:
        mem->dma[2].count.bytes[1] = byte;
        break;
    case REG_DMA2CNT_H:
        mem->dma[2].control.bytes[0] = byte & 0xE0;
        break;
    case REG_DMA2CNT_H + 1:
        dmaLoad(2, byte);
        break;
    case REG_DMA3SAD:
        mem->dma[3].source.bytes[0] = byte;
        break;
    case REG_DMA3SAD + 1:
        mem->dma[3].source.bytes[1] = byte;
        break;
    case REG_DMA3SAD + 2:
        mem->dma[3].source.bytes[2] = byte;
        break;
    case REG_DMA3SAD + 3:
        mem->dma[3].source.bytes[3] = byte;
        break;
    case REG_DMA3DAD:
        mem->dma[3].destination.bytes[0] = byte;
        break;
    case REG_DMA3DAD + 1:
        mem->dma[3].destination.bytes[1] = byte;
        break;
    case REG_DMA3DAD + 2:
        mem->dma[3].destination.bytes[2] = byte;
        break;
    case REG_DMA3DAD + 3:
        mem->dma[3].destination.bytes[3] = byte;
        break;
    case REG_DMA3CNT_L:
        mem->dma[3].count.bytes[0] = byte;
        break;
    case REG_DMA3CNT_L + 1:
        mem->dma[3].count.bytes[1] = byte;
        break;
    case REG_DMA3CNT_H:
        mem->dma[3].control.bytes[0] = byte & 0xE0;
        break;
    case REG_DMA3CNT_H + 1:
        dmaLoad(3, byte);
        break;

    /* Timer Registers */
    case REG_TM0CNT_L:
        mem->timers[0].reload.bytes[0] = byte;
        break;
    case REG_TM0CNT_L + 1:
        mem->timers[0].reload.bytes[1] = byte;
        break;
    case REG_TM0CNT_H:
        memWriteTimer(0, byte);
        break;
    case REG_TM0CNT_H + 1:
        mem->timers[0].control.bytes[1] = byte;
        break;
    case REG_TM1CNT_L:
        mem->timers[1].reload.bytes[0] = byte;
        break;
    case REG_TM1CNT_L + 1:
        mem->timers[1].reload.bytes[1] = byte;
        break;
    case REG_TM1CNT_H:
        memWriteTimer(1, byte);
        break;
    case REG_TM1CNT_H + 1:
        mem->timers[1].control.bytes[1] = byte;
        break;
    case REG_TM2CNT_L:
        mem->timers[2].reload.bytes[0] = byte;
        break;
    case REG_TM2CNT_L + 1:
        mem->timers[2].reload.bytes[1] = byte;
        break;
    case REG_TM2CNT_H:
        memWriteTimer(2, byte);
        break;
    case REG_TM2CNT_H + 1:
        mem->timers[2].control.bytes[1] = byte;
        break;
    case REG_TM3CNT_L:
        mem->timers[3].reload.bytes[0] = byte;
        break;
    case REG_TM3CNT_L + 1:
        mem->timers[3].reload.bytes[1] = byte;
        break;
    case REG_TM3CNT_H:
        memWriteTimer(3, byte);
        break;
    case REG_TM3CNT_H + 1:
        mem->timers[3].control.bytes[1] = byte;
        break;

    /* Serial Communication */
    case REG_SIOCNT:
        mem->comm.siocnt.bytes[0] = byte;
        break;
    case REG_SIOCNT + 1:
        mem->comm.siocnt.bytes[1] = byte;
        break;

    case REG_RCNT:
        mem->comm.rcnt.bytes[0] = byte;
        break;
    case REG_RCNT + 1:
        mem->comm.rcnt.bytes[1] = byte;
        break;

    /* Interrupt, Waitstate, and Power-Down Control */
    case REG_IE:
        mem->iwpdc.ie.bytes[0] = byte;
        cpuCheckIRQ();
        break;
    case REG_IE + 1:
        mem->iwpdc.ie.bytes[1] = byte;
        cpuCheckIRQ();
        break;
    case REG_IF:
        mem->iwpdc.i_f.bytes[0] &= ~byte;
        break;
    case REG_IF + 1:
        mem->iwpdc.i_f.bytes[1] &= ~byte;
        break;
    case REG_WAITCNT:
        mem->iwpdc.waitcnt.bytes[0] = byte;
        updateWait();
        break;
    case REG_WAITCNT + 1:
        mem->iwpdc.waitcnt.bytes[1] = byte;
        updateWait();
        break;
    case REG_IME:
        mem->iwpdc.ime.bytes[0] = byte;
        cpuCheckIRQ();
        break;
    case REG_IME + 1:
        mem->iwpdc.ime.bytes[1] = byte;
        cpuCheckIRQ();
        break;
    case REG_POSTFLG:
        mem->iwpdc.postflag.full = byte;
        break;
    case REG_HALTCNT:
        mem->iwpdc.haltcnt.bits.powerDown = true;
        break;
    }
}

void memWriteWord(Word addr, Word word)
{
    addr &= ~3; // Align the address to a 4-byte boundary
    switch ((addr >> 24) & 0xF)
    {
    case 0x02: // External Work RAM (eWRAM)
        *(Word *)(mem->eWRAM + (addr & 0x3FFFF)) = word;
        break;
    case 0x03: // Internal Work RAM (iWRAM)
        *(Word *)(mem->iWRAM + (addr & 0x7FFF)) = word;
        break;
    case 0x04: // I/O registers
        memWriteIO(addr + 0, (Byte)((word) >> 0));
        memWriteIO(addr + 1, (Byte)((word) >> 8));
        memWriteIO(addr + 2, (Byte)((word) >> 16));
        memWriteIO(addr + 3, (Byte)((word) >> 24));
        break;
    case 0x05: // Palette RAM
        *(Word *)(mem->palRAM + (addr & 0x3FF)) = word;
        addr &= 0x3FE;
        HalfWord pixel = mem->palRAM[addr] | (mem->palRAM[addr + 1] << 8);
        Byte r = ((pixel >> 0) & 0x1F) << 3;
        Byte g = ((pixel >> 5) & 0x1F) << 3;
        Byte b = ((pixel >> 10) & 0x1F) << 3;

        Word rgb = 0xFF;

        rgb |= (r | (r >> 5)) << 8;
        rgb |= (g | (g >> 5)) << 16;
        rgb |= (b | (b >> 5)) << 24;

        mem->palette[addr >> 1] = rgb;
        break;
    case 0x06: // Video RAM (VRAM)
        *(Word *)(mem->vram + (addr & (addr & 0x10000 ? 0x17fff : 0x1ffff))) = word;
        break;
    case 0x07: // Object Attribute Memory (OAM)
        *(Word *)(mem->oam + (addr & 0x3FF)) = word;
        break;
    case 0x0C: // EEPROM
    case 0x0D: // EEPROM
        eepromWrite(addr | 0, 0, (Byte)((word) >> 0));
        eepromWrite(addr | 1, 1, (Byte)((word) >> 8));
        eepromWrite(addr | 2, 2, (Byte)((word) >> 16));
        eepromWrite(addr | 3, 3, (Byte)((word) >> 24));
        break;
    case 0x0E: // Flash memory
    case 0x0F: // Flash memory
        flashWrite(addr | 0, (Byte)((word) >> 0));
        flashWrite(addr | 1, (Byte)((word) >> 8));
        flashWrite(addr | 2, (Byte)((word) >> 16));
        flashWrite(addr | 3, (Byte)((word) >> 24));
        break;
    default: // Illegal write
        break;
    }
}

void memWriteHalfWord(Word addr, HalfWord halfword)
{
    addr &= ~1; // Align the address to a 2-byte boundary
    switch ((addr >> 24) & 0xF)
    {
    case 2: // External Work RAM (eWRAM)
        *(HalfWord *)(mem->eWRAM + (addr & 0x3FFFF)) = halfword;
        break;
    case 3: // Internal Work RAM (iWRAM)
        *(HalfWord *)(mem->iWRAM + (addr & 0x7FFF)) = halfword;
        break;
    case 4: // I/O registers
        memWriteIO(addr + 0, (Byte)((halfword) >> 0));
        memWriteIO(addr + 1, (Byte)((halfword) >> 8));
        break;
    case 5: // Palette RAM
        *(HalfWord *)(mem->palRAM + (addr & 0x3FF)) = halfword;
        addr &= 0x3FE;
        HalfWord pixel = mem->palRAM[addr] | (mem->palRAM[addr + 1] << 8);
        Byte r = ((pixel >> 0) & 0x1F) << 3;
        Byte g = ((pixel >> 5) & 0x1F) << 3;
        Byte b = ((pixel >> 10) & 0x1F) << 3;

        Word rgb = 0xFF;

        rgb |= (r | (r >> 5)) << 8;
        rgb |= (g | (g >> 5)) << 16;
        rgb |= (b | (b >> 5)) << 24;

        mem->palette[addr >> 1] = rgb;
        break;
    case 6: // Video RAM (VRAM)
        *(HalfWord *)(mem->vram + (addr & (addr & 0x10000 ? 0x17fff : 0x1ffff))) = halfword;
        break;
    case 7: // Object Attribute Memory (OAM)
        *(HalfWord *)(mem->oam + (addr & 0x3FF)) = halfword;
        break;
    case 0x0C: // EEPROM
    case 0x0D: // EEPROM
        eepromWrite(addr | 0, 0, (Byte)((halfword) >> 0));
        eepromWrite(addr | 1, 1, (Byte)((halfword) >> 8));
        break;
    case 0x0E: // Flash memory
    case 0x0F: // Flash memory
        flashWrite(addr | 0, (Byte)((halfword) >> 0));
        flashWrite(addr | 1, (Byte)((halfword) >> 8));
        break;
    default: // Illegal write
        break;
    }
}

void memWriteByte(Word addr, Byte byte)
{
    Word newAddr;
    switch ((addr >> 24) & 0xF)
    {
    case 2: // External Work RAM (eWRAM)
        *(Byte *)(mem->eWRAM + (addr & 0x3FFFF)) = byte;
        break;
    case 3: // Internal Work RAM (iWRAM)
        *(Byte *)(mem->iWRAM + (addr & 0x7FFF)) = byte;
        break;
    case 4: // I/O registers
        memWriteIO(addr + 0, (Byte)((byte) >> 0));
        break;
    case 5: // Palette RAM
        *(Byte *)(mem->palRAM + (addr & 0x3FF)) = byte;
        addr &= 0x3FE;
        HalfWord pixel = mem->palRAM[addr] | (mem->palRAM[addr + 1] << 8);
        Byte r = ((pixel >> 0) & 0x1F) << 3;
        Byte g = ((pixel >> 5) & 0x1F) << 3;
        Byte b = ((pixel >> 10) & 0x1F) << 3;

        Word rgb = 0xFF;

        rgb |= (r | (r >> 5)) << 8;
        rgb |= (g | (g >> 5)) << 16;
        rgb |= (b | (b >> 5)) << 24;

        mem->palette[addr >> 1] = rgb;

        newAddr = addr + 1;
        *(Byte *)(mem->palRAM + (newAddr & 0x3FF)) = byte;
        newAddr &= 0x3FE;
        HalfWord pixel2 = mem->palRAM[newAddr] | (mem->palRAM[newAddr + 1] << 8);
        Byte r2 = ((pixel2 >> 0) & 0x1F) << 3;
        Byte g2 = ((pixel2 >> 5) & 0x1F) << 3;
        Byte b2 = ((pixel2 >> 10) & 0x1F) << 3;

        Word rgb2 = 0xFF;

        rgb2 |= (r2 | (r2 >> 5)) << 8;
        rgb2 |= (g2 | (g2 >> 5)) << 16;
        rgb2 |= (b2 | (b2 >> 5)) << 24;

        mem->palette[newAddr >> 1] = rgb2;
        break;
    case 6: // Video RAM (VRAM)
        *(Byte *)(mem->vram + (addr & (addr & 0x10000 ? 0x17fff : 0x1ffff))) = byte;
        newAddr = addr + 1;
        *(Byte *)(mem->vram + (newAddr & (newAddr & 0x10000 ? 0x17fff : 0x1ffff))) = byte;
        break;
    case 7: // Object Attribute Memory (OAM)
        // Byte writes to OAM are ignored
        return;
    case 0x0C: // EEPROM
    case 0x0D: // EEPROM
        eepromWrite(addr | 0, 0, ((byte) >> 0));
        break;
    case 0x0E: // Flash memory
    case 0x0F: // Flash memory
        flashWrite(addr | 0, ((byte) >> 0));
        break;
    default: // Illegal write
        break;
    }
}