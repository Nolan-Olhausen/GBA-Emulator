/****************************************************************************************************
 *
 * @file:    memory.h
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Header file for GBA Memory aspects.
 *          > Defines memory map regions
 *          > Defines I/O registers
 *          > Defines the individual I/O regions as structs
 *          > Defines memory core comprised of I/O structs and additional memory related fields
 *          > Defines memory related operations (readWord, writeWord, etc.)
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

#pragma once

#include "ppu.h"
#include "apu.h"
#include "dma.h"

HalfWord eepromIdx;
Byte *eeprom;
Byte *sram;
Byte *flash;

/******************************************************************************
 * Defines memory map regions
 *****************************************************************************/
#define BIOS_START (0x00000000) // BIOS - System ROM         (16 KBytes)
#define BIOS_END (0x00003FFF)
// 00004000-01FFFFFF   Not used
#define EWRAM_START (0x02000000) // WRAM - On-board Work RAM  (256 KBytes) 2 Wait
#define EWRAM_END (0x0203FFFF)
// 02040000-02FFFFFF   Not used
#define IWRAM_START (0x03000000) // WRAM - On-chip Work RAM   (32 KBytes)
#define IWRAM_END (0x03007FFF)
// 03008000-03FFFFFF   Not used
#define IO_START (0x04000000) // I/O Registers
#define IO_END (0x040003FF)
// 04000400-04FFFFFF   Not used
#define PALRAM_START (0x05000000) // BG/OBJ Palette RAM        (1 Kbyte)
#define PALRAM_END (0x050003FF)
// 05000400-05FFFFFF   Not used
#define VRAM_START (0x06000000) // VRAM - Video RAM          (96 KBytes)
#define VRAM_END (0x06017FFF)
// 06018000-06FFFFFF   Not used
#define OAM_START (0x07000000) // OAM - OBJ Attributes      (1 Kbyte)
#define OAM_END (0x070003FF)
// 07000400-07FFFFFF   Not used
#define CART_0_START (0x08000000) // Game Pak ROM/FlashROM (max 32MB) - Wait State 0
#define CART_0_END (0x09FFFFFF)
#define CART_1_START (0x0A000000) // Game Pak ROM/FlashROM (max 32MB) - Wait State 1
#define CART_1_END (0x0BFFFFFF)
#define CART_2_START (0x0C000000) // Game Pak ROM/FlashROM (max 32MB) - Wait State 2
#define CART_2_END (0x0DFFFFFF)
#define SRAM_START (0x0E000000) // Game Pak SRAM    (max 64 KBytes) - 8bit Bus width
#define SRAM_END (0x0E00FFFF)

/******************************************************************************
 * Defines I/O registers
 *****************************************************************************/
enum IO_REGS
{
    /* LCD I/O Registers */
    REG_DISPCNT = 0x04000000,  // LCD Control
    REG_GREENSWP = 0x04000002, // Undocumented - Green Swap
    REG_DISPSTAT = 0x04000004, // General LCD Status (STAT,LYC)
    REG_VCOUNT = 0x04000006,   // Vertical Counter (LY)
    REG_BG0CNT = 0x04000008,   // BG0 Control
    REG_BG1CNT = 0x0400000A,   // BG1 Control
    REG_BG2CNT = 0x0400000C,   // BG2 Control
    REG_BG3CNT = 0x0400000E,   // BG3 Control
    REG_BG0HOFS = 0x04000010,  // BG0 X-Offset
    REG_BG0VOFS = 0x04000012,  // BG0 Y-Offset
    REG_BG1HOFS = 0x04000014,  // BG1 X-Offset
    REG_BG1VOFS = 0x04000016,  // BG1 Y-Offset
    REG_BG2HOFS = 0x04000018,  // BG2 X-Offset
    REG_BG2VOFS = 0x0400001A,  // BG2 Y-Offset
    REG_BG3HOFS = 0x0400001C,  // BG3 X-Offset
    REG_BG3VOFS = 0x0400001E,  // BG3 Y-Offset
    REG_BG2PA = 0x04000020,    // BG2 Rotation/Scaling Parameter A (dx)
    REG_BG2PB = 0x04000022,    // BG2 Rotation/Scaling Parameter B (dmx)
    REG_BG2PC = 0x04000024,    // BG2 Rotation/Scaling Parameter C (dy)
    REG_BG2PD = 0x04000026,    // BG2 Rotation/Scaling Parameter D (dmy)
    REG_BG2X = 0x04000028,     // BG2 Reference Point X-Coordinate
    REG_BG2Y = 0x0400002C,     // BG2 Reference Point Y-Coordinate
    REG_BG3PA = 0x04000030,    // BG3 Rotation/Scaling Parameter A (dx)
    REG_BG3PB = 0x04000032,    // BG3 Rotation/Scaling Parameter B (dmx)
    REG_BG3PC = 0x04000034,    // BG3 Rotation/Scaling Parameter C (dy)
    REG_BG3PD = 0x04000036,    // BG3 Rotation/Scaling Parameter D (dmy)
    REG_BG3X = 0x04000038,     // BG3 Reference Point X-Coordinate
    REG_BG3Y = 0x0400003C,     // BG3 Reference Point Y-Coordinate
    REG_WIN0H = 0x04000040,    // Window 0 Horizontal Dimensions
    REG_WIN1H = 0x04000042,    // Window 1 Horizontal Dimensions
    REG_WIN0V = 0x04000044,    // Window 0 Vertical Dimensions
    REG_WIN1V = 0x04000046,    // Window 1 Vertical Dimensions
    REG_WININ = 0x04000048,    // Inside of Window 0 and 1
    REG_WINOUT = 0x0400004A,   // Inside of OBJ Window & Outside of Windows
    REG_MOSAIC = 0x0400004C,   // Mosaic Size
    // 400004Eh       -    -         Not used
    REG_BLDCNT = 0x04000050,   // Color Special Effects Selection
    REG_BLDALPHA = 0x04000052, // Alpha Blending Coefficients
    REG_BLDY = 0x04000054,     // Brightness (Fade-In/Out) Coefficient
    // 4000056h       -    -         Not used

    /* Sound Registers */
    REG_SOUND1CNT_L = 0x04000060, // Channel 1 Sweep register       (NR10)
    REG_SOUND1CNT_H = 0x04000062, // Channel 1 Duty/Length/Envelope (NR11, NR12)
    REG_SOUND1CNT_X = 0x04000064, // Channel 1 Frequency/Control    (NR13, NR14)
    // 4000066h     -    -           Not used
    REG_SOUND2CNT_L = 0x04000068, // Channel 2 Duty/Length/Envelope (NR21, NR22)
    // 400006Ah     -    -           Not used
    REG_SOUND2CNT_H = 0x0400006C, // Channel 2 Frequency/Control    (NR23, NR24)
    // 400006Eh     -    -           Not used
    REG_SOUND3CNT_L = 0x04000070, // Channel 3 Stop/Wave RAM select (NR30)
    REG_SOUND3CNT_H = 0x04000072, // Channel 3 Length/Volume        (NR31, NR32)
    REG_SOUND3CNT_X = 0x04000074, // Channel 3 Frequency/Control    (NR33, NR34)
    // 4000076h     -    -           Not used
    REG_SOUND4CNT_L = 0x04000078, // Channel 4 Length/Envelope      (NR41, NR42)
    // 400007Ah     -    -           Not used
    REG_SOUND4CNT_H = 0x0400007C, // Channel 4 Frequency/Control    (NR43, NR44)
    // 400007Eh     -    -           Not used
    REG_SOUNDCNT_L = 0x04000080, // Control Stereo/Volume/Enable   (NR50, NR51)
    REG_SOUNDCNT_H = 0x04000082, // Control Mixing/DMA Control
    REG_SOUNDCNT_X = 0x04000084, // Control Sound on/off           (NR52)
    // 4000086h     -    -           Not used
    REG_SOUNDBIAS = 0x04000088, // Sound PWM Control
    // 400008Ah  ..   -    -         Not used
    REG_WAVE_RAM0 = 0x04000090, // Channel 3 Wave Pattern RAM
    REG_WAVE_RAM1 = 0x04000094, // Channel 3 Wave Pattern RAM
    REG_WAVE_RAM2 = 0x04000098, // Channel 3 Wave Pattern RAM
    REG_WAVE_RAM3 = 0x0400009C, // Channel 3 Wave Pattern RAM
    REG_FIFO_A_L = 0x040000A0,  // Channel A FIFO
    REG_FIFO_A_H = 0x040000A2,  // Channel A FIFO
    REG_FIFO_B_L = 0x040000A4,  // Channel B FIFO
    REG_FIFO_B_H = 0x040000A6,  // Channel B FIFO
    // 40000A8h       -    -         Not used

    /* DMA Transfer Channels */
    REG_DMA0SAD = 0x040000B0,   // DMA 0 Source Address
    REG_DMA0DAD = 0x040000B4,   // DMA 0 Destination Address
    REG_DMA0CNT_L = 0x040000B8, // DMA 0 Word Count
    REG_DMA0CNT_H = 0x040000BA, // DMA 0 Control
    REG_DMA1SAD = 0x040000BC,   // DMA 1 Source Address
    REG_DMA1DAD = 0x040000C0,   // DMA 1 Destination Address
    REG_DMA1CNT_L = 0x040000C4, // DMA 1 Word Count
    REG_DMA1CNT_H = 0x040000C6, // DMA 1 Control
    REG_DMA2SAD = 0x040000C8,   // DMA 2 Source Address
    REG_DMA2DAD = 0x040000CC,   // DMA 2 Destination Address
    REG_DMA2CNT_L = 0x040000D0, // DMA 2 Word Count
    REG_DMA2CNT_H = 0x040000D2, // DMA 2 Control
    REG_DMA3SAD = 0x040000D4,   // DMA 3 Source Address
    REG_DMA3DAD = 0x040000D8,   // DMA 3 Destination Address
    REG_DMA3CNT_L = 0x040000DC, // DMA 3 Word Count
    REG_DMA3CNT_H = 0x040000DE, // DMA 3 Control
    // 40000E0h       -    -         Not used

    /* Timer Registers */
    REG_TM0CNT_L = 0x04000100, // Timer 0 Counter/Reload
    REG_TM0CNT_H = 0x04000102, // Timer 0 Control
    REG_TM1CNT_L = 0x04000104, // Timer 1 Counter/Reload
    REG_TM1CNT_H = 0x04000106, // Timer 1 Control
    REG_TM2CNT_L = 0x04000108, // Timer 2 Counter/Reload
    REG_TM2CNT_H = 0x0400010A, // Timer 2 Control
    REG_TM3CNT_L = 0x0400010C, // Timer 3 Counter/Reload
    REG_TM3CNT_H = 0x0400010E, // Timer 3 Control
    // 4000110h       -    -         Not used

    /* Serial Communication (1) */
    REG_SIODATA32 = 0x04000120,   // SIO Data (Normal-32bit Mode; shared with below)
    REG_SIOMULTI0 = 0x04000120,   // SIO Data 0 (Parent)    (Multi-Player Mode)
    REG_SIOMULTI1 = 0x04000122,   // SIO Data 1 (1st Child) (Multi-Player Mode)
    REG_SIOMULTI2 = 0x04000124,   // SIO Data 2 (2nd Child) (Multi-Player Mode)
    REG_SIOMULTI3 = 0x04000126,   // SIO Data 3 (3rd Child) (Multi-Player Mode)
    REG_SIOCNT = 0x04000128,      // SIO Control Register
    REG_SIOMLT_SEND = 0x0400012A, // SIO Data (Local of MultiPlayer; shared below)
    REG_SIODATA8 = 0x0400012A,    // SIO Data (Normal-8bit and UART Mode)
    // 400012Ch       -    -         Not used

    /* Keypad Input */
    REG_KEYINPUT = 0x04000130, // Key Status
    REG_KEYCNT = 0x04000132,   // Key Interrupt Control

    /* Serial Communication (2) */
    REG_RCNT = 0x04000134, // SIO Mode Select/General Purpose Data
    REG_IR = 0x04000136,   // Ancient - Infrared Register (Prototypes only)
    // 4000138h       -    -         Not used
    REG_JOYCNT = 0x04000140, // SIO JOY Bus Control
    // 4000142h       -    -         Not used
    REG_JOY_RECV = 0x04000150,  // SIO JOY Bus Receive Data
    REG_JOY_TRANS = 0x04000154, // SIO JOY Bus Transmit Data
    REG_JOYSTAT = 0x04000158,   // SIO JOY Bus Receive Status
    // 400015Ah       -    -         Not used

    /* Interrupt, Waitstate, and Power-Down Control */
    REG_IE = 0x04000200,      // Interrupt Enable Register
    REG_IF = 0x04000202,      // Interrupt Request Flags / IRQ Acknowledge
    REG_WAITCNT = 0x04000204, // Game Pak Waitstate Control
    // 4000206h       -    -         Not used
    REG_IME = 0x04000208, // Interrupt Master Enable Register
    // 400020Ah       -    -         Not used
    REG_POSTFLG = 0x04000300, // Undocumented - Post Boot Flag
    REG_HALTCNT = 0x04000301, // Undocumented - Power Down Control
    // 4000302h       -    -         Not used
    // 4000410h  ?    ?    ?         Undocumented - Purpose Unknown / Bug ??? 0FFh
    // 4000411h       -    -         Not used
    // 4000800h  4    R/W  ?         Undocumented - Internal Memory Control (R/W)
    // 4000804h       -    -         Not used
    // 4xx0800h  4    R/W  ?         Mirrors of 4000800h (repeated each 64K)
    // 4700000h  4    W    (3DS)     Disable ARM7 bootrom overlay (3DS only)
};

/******************************************************************************
 * Defines the individual I/O regions as structs
 *****************************************************************************/
/*
 * Struct for values of necessary LCD registers
 */
struct LCD
{
    /*
     * Unions make each part (full, bytes, bits) occupy same space in memory,
     * so setting full or bytes when R/W will fill out bits section for us,
     * making accessing specific bits easy
     */
    union
    {
        struct
        {
            Byte bgMode : 3;        // BG Mode                    (0-5=Video Mode 0-5, 6-7=Prohibited)
            Bit cbgMode : 1;        // Reserved / CGB Mode        (0=GBA, 1=CGB; can be set only by BIOS opcodes)
            Bit frameSelect : 1;    // Display Frame Select       (0-1=Frame 0-1) (for BG Modes 4,5 only)
            Bit hblackInterval : 1; // H-Blank Interval Free      (1=Allow access to OAM during H-Blank)
            Bit objCharMap : 1;     // OBJ Character VRAM Mapping (0=Two dimensional, 1=One dimensional)
            Bit forcedBlank : 1;    // Forced Blank               (1=Allow FAST access to VRAM,Palette,OAM)
            Bit bg0 : 1;            // Screen Display BG0         (0=Off, 1=On)
            Bit bg1 : 1;            // Screen Display BG1         (0=Off, 1=On)
            Bit bg2 : 1;            // Screen Display BG2         (0=Off, 1=On)
            Bit bg3 : 1;            // Screen Display BG3         (0=Off, 1=On)
            Bit obj : 1;            // Screen Display OBJ         (0=Off, 1=On)
            Bit win0 : 1;           // Window 0 Display Flag      (0=Off, 1=On)
            Bit win1 : 1;           // Window 1 Display Flag      (0=Off, 1=On)
            Bit objWin : 1;         // OBJ Window Display Flag    (0=Off, 1=On)
        } bits;
        Byte bytes[2]; // Break full halfword into R/W-able bytes
        HalfWord full;
    } dispcnt;

    union
    {
        Byte bytes[2];
        HalfWord full;
    } greenswp;

    union
    {
        struct
        {
            Bit vblank : 1;         // V-Blank flag   (Read only) (1=VBlank) (set in line 160..226; not 227)
            Bit hblank : 1;         // H-Blank flag   (Read only) (1=HBlank) (toggled in all lines, 0..227)
            Bit vcounter : 1;       // V-Counter flag (Read only) (1=Match)  (set in selected line)     (R)
            Bit vblankIRQ : 1;      // V-Blank IRQ Enable         (1=Enable)                          (R/W)
            Bit hblankIRQ : 1;      // H-Blank IRQ Enable         (1=Enable)                          (R/W)
            Bit vcounterIRQ : 1;    // V-Counter IRQ Enable       (1=Enable)                          (R/W)
            Bit : 1;                // Not used (0) / DSi: LCD Initialization Ready (0=Busy, 1=Ready)   (R)
            Bit : 1;                // Not used (0) / NDS: MSB of V-Vcount Setting (LYC.Bit8) (0..262)(R/W)
            Byte vcountSetting : 8; // V-Count Setting (LYC)      (0..227)                            (R/W)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } dispstat;

    union
    {
        struct
        {
            Byte scanline : 1; // Current Scanline (LY)      (0..227)                              (R)
            Bit : 1;           // Not used (0) / NDS: MSB of Current Scanline (LY.Bit8) (0..262)   (R)
            Byte : 1;          // Not Used (0)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } vcount;

    union
    {
        struct
        {
            Byte bgPriority : 2; // BG Priority           (0-3, 0=Highest)
            Byte charBase : 2;   // Character Base Block  (0-3, in units of 16 KBytes) (=BG Tile Data)
            Byte : 2;            // Not used (must be zero) (except in NDS mode: MSBs of char base)
            Bit mosaic : 1;      // Mosaic                (0=Disable, 1=Enable)
            Bit palette : 1;     // Colors/Palettes       (0=16/16, 1=256/1)
            Byte screenBase : 5; // Screen Base Block     (0-31, in units of 2 KBytes) (=BG Map Data)
            Bit wrap : 1;        // BG0/BG1: Not used (except in NDS mode: Ext Palette Slot for BG0/BG1)
                                 // BG2/BG3: Display Area Overflow (0=Transparent, 1=Wraparound)
            Byte screenSize : 2; // Screen Size (0-3)
                                 // Value  Text Mode      Rotation/Scaling Mode
                                 // 0      256x256 (2K)   128x128   (256 bytes)
                                 // 1      512x256 (4K)   256x256   (1K)
                                 // 2      256x512 (4K)   512x512   (4K)
                                 // 3      512x512 (8K)   1024x1024 (16K)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } bgcnt[4]; // BG0-BG3 are structured the same, so we can make this an array to save code
                // other structs below will also follow this pattern

    union
    {
        struct
        {
            HalfWord offset : 9; // Offset (0-511)
            Byte : 7;            // Not used
        } bits;
        Byte bytes[2];
        HalfWord full;
    } bghofs[4];

    union
    {
        struct
        {
            HalfWord offset : 9; // Offset (0-511)
            Byte : 7;            // Not used
        } bits;
        Byte bytes[2];
        HalfWord full;
    } bgvofs[4];

    union
    {
        struct
        {
            Byte fractional : 8; // Fractional portion (8 bits)
            Byte integer : 7;    // Integer portion    (7 bits)
            Bit sign : 1;        // Sign               (1 bit)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } bgpa[2]; // BG2-BG3, same as the next few below

    union
    {
        struct
        {
            Byte fractional : 8; // Fractional portion (8 bits)
            Byte integer : 7;    // Integer portion    (7 bits)
            Bit sign : 1;        // Sign               (1 bit)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } bgpb[2];

    union
    {
        struct
        {
            Byte fractional : 8; // Fractional portion (8 bits)
            Byte integer : 7;    // Integer portion    (7 bits)
            Bit sign : 1;        // Sign               (1 bit)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } bgpc[2];

    union
    {
        struct
        {
            Byte fractional : 8; // Fractional portion (8 bits)
            Byte integer : 7;    // Integer portion    (7 bits)
            Bit sign : 1;        // Sign               (1 bit)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } bgpd[2];

    union
    {
        struct
        {
            Byte fractional : 8; // Fractional portion (8 bits)
            Word integer : 19;   // Integer portion    (19 bits)
            Bit sign : 1;        // Sign               (1 bit)
            Byte : 4;            // Not used
        } bits;
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } bgx[2]; // BG2-BG3

    union
    {
        struct
        {
            Byte fractional : 8; // Fractional portion (8 bits)
            Word integer : 19;   // Integer portion    (19 bits)
            Bit sign : 1;        // Sign               (1 bit)
            Byte : 4;            // Not used
        } bits;
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } bgy[2]; // BG2-BG3

    union
    {
        struct
        {
            Byte maxH : 8; // X2, Rightmost coordinate of window, plus 1
            Byte minH : 8; // X1, Leftmost coordinate of window
        } bits;
        Byte bytes[2];
        HalfWord full;
    } winh[2]; // WIN0-WIN1

    union
    {
        struct
        {
            Byte maxV : 8; // Y2, Bottom-most coordinate of window, plus 1
            Byte minV : 8; // Y1, Top-most coordinate of window
        } bits;
        Byte bytes[2];
        HalfWord full;
    } winv[2]; // WIN0-WIN1

    union
    {
        struct
        {
            Byte win0BG : 4;     // Window 0 BG0-BG3 Enable Bits     (0=No Display, 1=Display)
            Bit win0OBJ : 1;     // Window 0 OBJ Enable Bit          (0=No Display, 1=Display)
            Bit win0Effects : 1; // Window 0 Color Special Effect    (0=Disable, 1=Enable)
            Byte : 2;            // Not used
            Byte win1BG : 4;     // Window 1 BG0-BG3 Enable Bits     (0=No Display, 1=Display)
            Bit win1OBJ : 1;     // Window 1 OBJ Enable Bit          (0=No Display, 1=Display)
            Bit win1Effects : 1; // Window 1 Color Special Effect    (0=Disable, 1=Enable)
            Byte : 2;            // Not used
        } bits;
        Byte bytes[2];
        HalfWord full;
    } winin;

    union
    {
        struct
        {
            Byte outBG : 4;     // Outside BG0-BG3 Enable Bits      (0=No Display, 1=Display)
            Bit outOBJ : 1;     // Outside OBJ Enable Bit           (0=No Display, 1=Display)
            Bit outEffects : 1; // Outside Color Special Effect     (0=Disable, 1=Enable)
            Byte : 2;           // Not used
            Byte objBG : 4;     // OBJ Window BG0-BG3 Enable Bits   (0=No Display, 1=Display)
            Bit objOBJ : 1;     // OBJ Window OBJ Enable Bit        (0=No Display, 1=Display)
            Bit objEffects : 1; // OBJ Window Color Special Effect  (0=Disable, 1=Enable)
            Byte : 2;           // Not used
        } bits;
        Byte bytes[2];
        HalfWord full;
    } winout;

    union
    {
        struct
        {
            Byte bgHSize : 4;  // BG Mosaic H-Size  (minus 1)
            Byte bgVSize : 4;  // BG Mosaic V-Size  (minus 1)
            Byte objHSize : 4; // OBJ Mosaic H-Size (minus 1)
            Byte objVSize : 4; // OBJ Mosaic V-Size (minus 1)
            HalfWord : 16;     // Not used
        } bits;
        Byte bytes[4];
        Byte halfwords[2];
        Word full;
    } mosaic;

    union
    {
        struct
        {
            Bit bg01 : 1;    // BG0 1st Target Pixel (Background 0)
            Bit bg11 : 1;    // BG1 1st Target Pixel (Background 1)
            Bit bg21 : 1;    // BG2 1st Target Pixel (Background 2)
            Bit bg31 : 1;    // BG3 1st Target Pixel (Background 3)
            Bit obj1 : 1;    // OBJ 1st Target Pixel (Top-most OBJ pixel)
            Bit bd1 : 1;     // BD  1st Target Pixel (Backdrop)
            Byte effect : 2; // Color Special Effect (0-3, see below)
                             // 0 = None                (Special effects disabled)
                             // 1 = Alpha Blending      (1st+2nd Target mixed)
                             // 2 = Brightness Increase (1st Target becomes whiter)
                             // 3 = Brightness Decrease (1st Target becomes blacker)
            Bit bg02 : 1;    // BG0 2nd Target Pixel (Background 0)
            Bit bg12 : 1;    // BG1 2nd Target Pixel (Background 1)
            Bit bg22 : 1;    // BG2 2nd Target Pixel (Background 2)
            Bit bg32 : 1;    // BG3 2nd Target Pixel (Background 3)
            Bit obj2 : 1;    // OBJ 2nd Target Pixel (Top-most OBJ pixel)
            Bit bd2 : 1;     // BD  2nd Target Pixel (Backdrop)
            Byte : 2;        // Not used
        } bits;
        Byte bytes[2];
        HalfWord full;
    } bldcnt;

    union
    {
        struct
        {
            Byte eva : 5; // EVA Coefficient (1st Target) (0..16 = 0/16..16/16, 17..31=16/16)
            Byte : 3;     // Not used
            Byte evb : 5; // EVB Coefficient (2nd Target) (0..16 = 0/16..16/16, 17..31=16/16)
            Byte : 3;     // Not used
        } bits;
        Byte bytes[2];
        HalfWord full;
    } bldalpha;

    union
    {
        struct
        {
            Byte evy : 5; // EVY Coefficient (Brightness) (0..16 = 0/16..16/16, 17..31=16/16)
            Word : 26;    // Not used
        } bits;
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } bldy;

    // The below is not in the GBATEK but will be helpful for oam rendering
    union
    {
        struct
        {
            struct
            {
                HalfWord coordY : 8;
                HalfWord affine : 1;
                HalfWord virtSize : 1;
                HalfWord mode : 2;
                HalfWord mosaic : 1;
                HalfWord color256 : 1;
                HalfWord sizeHigh : 2;
            } one;
            union
            {
                struct
                {
                    HalfWord coordX : 9;
                    HalfWord : 3;
                    HalfWord hflip : 1;
                    HalfWord vflip : 1;
                    HalfWord sizeLow : 2;
                } two;
                struct
                {
                    HalfWord : 9; // coordX
                    HalfWord affineDataIdx : 5;
                    HalfWord : 2; // sizeLow
                } affineIdx;
            };
            struct
            {
                HalfWord tileIdx : 10;
                HalfWord priority : 2;
                HalfWord paletteNum : 4;
            } three;
        } full;
        HalfWord halfwords[3];
    } oamdata;
};

/*
 * Struct for values of necessary SOUND registers
 */
struct SOUND
{
    union
    {
        struct
        {
            Byte number : 3;   // Number of sweep shift      (n=0-7)
            Bit direction : 1; // Sweep Frequency Direction  (0=Increase, 1=Decrease)
            Byte time : 3;     // Sweep Time; units of 7.8ms (0-7, min=7.8ms, max=54.7ms)
            HalfWord : 9;      // Not used
        } bits;
        Byte bytes[2];
        HalfWord full;
    } sound1cnt_l;

    union
    {
        struct
        {
            Byte length : 6;   // Sound length; units of (64-n)/256s  (0-63)
            Byte duty : 2;     // Wave Pattern Duty                   (0-3, see below)
            Byte time : 3;     // Envelope Step-Time; units of n/64s  (1-7, 0=No Envelope)
            Bit direction : 1; // Envelope Direction                  (0=Decrease, 1=Increase)
            Byte volume : 4;   // Initial Volume of envelope          (1-15, 0=No Sound)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } sound1cnt_h;

    union
    {
        struct
        {
            HalfWord frequency : 11; // Frequency; 131072/(2048-n)Hz  (0-2047)
            Byte : 3;                // Not used
            Bit length : 1;          // Length Flag  (1=Stop output when length in NR11 expires)
            Bit initial : 1;         // Initial      (1=Restart Sound)
            HalfWord : 16;           // Not used
        } bits;
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } sound1cnt_x;

    union
    {
        struct
        {
            Byte length : 6;   // Sound length; units of (64-n)/256s  (0-63)
            Byte duty : 2;     // Wave Pattern Duty                   (0-3, see below)
            Byte time : 3;     // Envelope Step-Time; units of n/64s  (1-7, 0=No Envelope)
            Bit direction : 1; // Envelope Direction                  (0=Decrease, 1=Increase)
            Byte volume : 4;   // Initial Volume of envelope          (1-15, 0=No Sound)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } sound2cnt_l;

    union
    {
        struct
        {
            HalfWord frequency : 11; // Frequency; 131072/(2048-n)Hz  (0-2047)
            Byte : 3;                // Not used
            Bit length : 1;          // Length Flag  (1=Stop output when length in NR11 expires)
            Bit initial : 1;         // Initial      (1=Restart Sound)
            HalfWord : 16;           // Not used
        } bits;
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } sound2cnt_h;

    union
    {
        struct
        {
            Byte : 5;          // Not used
            Bit dimension : 1; // Wave RAM Dimension   (0=One bank/32 digits, 1=Two banks/64 digits)
            Bit number : 1;    // Wave RAM Bank Number (0-1, see below)
            Bit enable : 1;    // Sound Channel 3 Off  (0=Stop, 1=Playback)
            Byte : 8;          // Not used
        } bits;
        Byte bytes[2];
        HalfWord full;
    } sound3cnt_l;

    union
    {
        struct
        {
            Byte length : 8;     // Sound length; units of (256-n)/256s  (0-255)
            Bit : 5;             // Not used
            Byte volume : 2;     // Sound Volume  (0=Mute/Zero, 1=100%, 2=50%, 3=25%)
            Bit forceVolume : 1; // Force Volume  (0=Use above, 1=Force 75% regardless of above)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } sound3cnt_h;

    union
    {
        struct
        {
            HalfWord sampleRate : 11; // Sample Rate; 2097152/(2048-n) Hz   (0-2047)
            Byte : 3;                 // Not used
            Bit length : 1;           // Length Flag  (1=Stop output when length in NR31 expires)
            Bit initial : 1;          // Initial      (1=Restart Sound)
            HalfWord : 16;
        } bits;
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } sound3cnt_x;

    /*
     * This area contains 16 bytes (32 x 4bits) Wave Pattern data which is output by channel 3.
     * Data is played back ordered as follows: MSBs of 1st byte, followed by LSBs of 1st byte,
     * followed by MSBs of 2nd byte, and so on - this results in a confusing ordering when
     * filling Wave RAM in units of 16bit data - ie. samples would be then located in Bits
     * 4-7, 0-3, 12-15, 8-11.
     */
    struct
    {
        union
        {
            struct
            {
                Byte sample1 : 4;
                Byte sample0 : 4;
                Byte sample3 : 4;
                Byte sample2 : 4;
            } bits;
            Byte bytes[2];
            HalfWord full;
        } reg[8];  // wave_ram 0-4 L/H
    } wave_ram[2]; // two wave patterns

    union
    {
        struct
        {
            Byte length : 5;   // Sound length; units of (64-n)/256s  (0-63)
            Byte : 2;          // Not used
            Byte time : 3;     // Envelope Step-Time; units of n/64s  (1-7, 0=No Envelope)
            Bit direction : 1; // Envelope Direction                  (0=Decrease, 1=Increase)
            Byte volume : 4;   // Initial Volume of envelope          (1-15, 0=No Sound)
            HalfWord : 16;     // Not used
        } bits;
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } sound4cnt_l;

    union
    {
        struct
        {
            Byte ratio : 3;     // Dividing Ratio of Frequencies (r)
            Bit width : 1;      // Counter Step/Width (0=15 bits, 1=7 bits)
            Byte frequency : 4; // Shift Clock Frequency (s)
            Byte : 6;           // Not used
            Bit length : 1;     // Length Flag  (1=Stop output when length in NR41 expires)
            Bit initial : 1;    // Initial      (1=Restart Sound)
            HalfWord : 16;      // Not used
        } bits;
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } sound4cnt_h;

    struct
    {
        union
        {
            struct
            {
                Byte data0 : 8; // Data 0 being located in least significant byte which is replayed first)
                Byte data1 : 8; // Data 1 played after Data 0
                Byte data2 : 8; // etc.
                Byte data3 : 8; // etc.
            } bits;
            Byte bytes[4];
            HalfWord halfwords[2];
            Word full;
        } reg;
        int8_t capacity[32];
        size_t read;
        size_t write;
        size_t size;
    } fifo[2]; // FIFO A/B

    union
    {
        struct
        {
            Byte rightMaster : 3; // Sound 1-4 Master Volume RIGHT (0-7)
            Bit : 1;              // Not used
            Byte leftMaster : 3;  // Sound 1-4 Master Volume LEFT (0-7)
            Bit : 1;              // Not used
            Bit right1 : 1;       // Sound 1 Enable Flags RIGHT (each Bit 8-11, 0=Disable, 1=Enable)
            Bit right2 : 1;       // Sound 2 Enable Flags RIGHT (each Bit 8-11, 0=Disable, 1=Enable)
            Bit right3 : 1;       // Sound 3 Enable Flags RIGHT (each Bit 8-11, 0=Disable, 1=Enable)
            Bit right4 : 1;       // Sound 4 Enable Flags RIGHT (each Bit 8-11, 0=Disable, 1=Enable)
            Bit left1 : 1;        // Sound 1 Enable Flags LEFT (each Bit 12-15, 0=Disable, 1=Enable)
            Bit left2 : 1;        // Sound 2 Enable Flags LEFT (each Bit 12-15, 0=Disable, 1=Enable)
            Bit left3 : 1;        // Sound 3 Enable Flags LEFT (each Bit 12-15, 0=Disable, 1=Enable)
            Bit left4 : 1;        // Sound 4 Enable Flags LEFT (each Bit 12-15, 0=Disable, 1=Enable)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } soundcnt_l;

    union
    {
        struct
        {
            Byte volume : 2;    // Sound # 1-4 Volume   (0=25%, 1=50%, 2=100%, 3=Prohibited)
            Bit dmaAVolume : 1; // DMA Sound A Volume   (0=50%, 1=100%)
            Bit dmaBVolume : 1; // DMA Sound B Volume   (0=50%, 1=100%)
            Byte : 4;           // Not used
            Bit dmaARight : 1;  // DMA Sound A Enable RIGHT (0=Disable, 1=Enable)
            Bit dmaALeft : 1;   // DMA Sound A Enable LEFT  (0=Disable, 1=Enable)
            Bit dmaATimer : 1;  // DMA Sound A Timer Select (0=Timer 0, 1=Timer 1)
            Bit dmaAReset : 1;  // DMA Sound A Reset FIFO   (1=Reset)
            Bit dmaBRight : 1;  // DMA Sound B Enable RIGHT (0=Disable, 1=Enable)
            Bit dmaBLeft : 1;   // DMA Sound B Enable LEFT  (0=Disable, 1=Enable)
            Bit dmaBTimer : 1;  // DMA Sound B Timer Select (0=Timer 0, 1=Timer 1)
            Bit dmaBReset : 1;  // DMA Sound B Reset FIFO   (1=Reset)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } soundcnt_h;

    union
    {
        struct
        {
            Bit sound1 : 1; // Sound 1 ON flag (Read Only)
            Bit sound2 : 1; // Sound 2 ON flag (Read Only)
            Bit sound3 : 1; // Sound 3 ON flag (Read Only)
            Bit sound4 : 1; // Sound 4 ON flag (Read Only)
            Byte : 3;       // Not used
            Bit master : 1; // PSG/FIFO Master Enable (0=Disable, 1=Enable) (Read/Write)
            Word : 24;      // Not used
        } bits;
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } soundcnt_x;

    union
    {
        struct
        {
            Bit : 1;                // Not used
            uint32_t bias : 9;      // Bias Level (Default=100h, converting signed samples into unsigned)
            uint32_t : 4;           // Not used
            uint32_t amplitude : 2; // Amplitude Resolution/Sampling Cycle (Default=0, see below)
            uint32_t : 16;          // Not used
        } bits;
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } soundbias;
};

/*
 * Struct for values of necessary TIMER registers
 */
struct TIMERS
{
    union
    {
        Byte bytes[2];
        HalfWord full;
    } counter;

    union
    {
        Byte bytes[2];
        HalfWord full;
    } reload;

    union
    {
        struct
        {
            Byte prescaler : 2; // Prescaler Selection (0=F/1, 1=F/64, 2=F/256, 3=F/1024)
            Bit timing : 1;     // Count-up Timing   (0=Normal, 1=See below)  ;Not used in TM0CNT_H
            Byte : 3;           // Not used
            Bit irq : 1;        // Timer IRQ Enable  (0=Disable, 1=IRQ on Timer overflow)
            Bit enable : 1;     // Timer Start/Stop  (0=Stop, 1=Operate)
            Byte : 8;           // Not used
        } bits;
        Byte bytes[2];
        HalfWord full;
    } control;
};

/*
 * Struct for values of necessary DMA registers
 */
struct DMA
{
    union
    {
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } source;

    union
    {
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } destination;

    union
    {
        Byte bytes[2];
        HalfWord full;
    } count;

    union
    {
        struct
        {
            Byte : 5;             // Not used
            Byte destination : 2; // Dest Addr Control  (0=Increment,1=Decrement,2=Fixed,3=Increment/Reload)
            Byte source : 2;      // Source Adr Control (0=Increment,1=Decrement,2=Fixed,3=Prohibited)
            Bit repeat : 1;       // DMA Repeat                   (0=Off, 1=On) (Must be zero if Bit 11 set)
            Bit type : 1;         // DMA Transfer Type            (0=16bit, 1=32bit)
            Bit gamepak : 1;      // Game Pak DRQ  - DMA3 only -  (0=Normal, 1=DRQ <from> Game Pak, DMA3)
            Byte timing : 2;      // DMA Start Timing  (0=Immediately, 1=VBlank, 2=HBlank, 3=Special)
            Bit irq : 1;          // IRQ upon end of Word Count   (0=Disable, 1=Enable)
            Bit enable : 1;       // DMA Enable                   (0=Off, 1=On)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } control;

    size_t index;
    Word internalSource;
    Word internalDest;
    Word internalCount;
    Word latch;
    Bit fifo;
    Bit video;
};

/*
 * Struct for values of necessary COMM registers
 */
struct COMM
{
    union
    {
        struct
        {
            Bit shiftClock : 1;    // Shift Clock (SC)        (0=External, 1=Internal)
            Bit internalClock : 1; // Internal Shift Clock    (0=256KHz, 1=2MHz)
            Bit si : 1;            // SI State (opponents SO) (0=Low, 1=High/None) --- (Read Only)
            Bit so : 1;            // SO during inactivity    (0=Low, 1=High) (applied ONLY when Bit7=0)
            Byte : 3;              // Not used                (Read only, always 0 ?)
            Bit start : 1;         // Start Bit               (0=Inactive/Ready, 1=Start/Active)
            Byte : 4;              // Not used                (R/W, should be 0)
            Bit length : 1;        // Transfer Length         (0=8bit, 1=32bit)
            Bit normal : 1;        // Must be "0" for Normal Mode
            Bit irq : 1;           // IRQ Enable              (0=Disable, 1=Want IRQ upon completion)
            Bit : 1;               // Not used                (Read only, always 0)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } siocnt;

    // REG_RCNT
    union
    {
        struct
        {
            Byte undoc : 4; // Undocumented (current SC,SD,SI,SO state, as for General Purpose mode)
            Byte : 5;       // Not used     (Should be 0, bits are read/write-able though)
            Byte : 5;       // Not used     (Always 0, read only)
            Bit : 1;        // Not used     (Should be 0, bit is read/write-able though)
            Bit normal : 1; // Must be zero (0) for Normal/Multiplayer/UART modes
        } bits;
        Byte bytes[2];
        HalfWord full;
    } rcnt;
};

/*
 * Struct for values of necessary KEYPAD registers
 */
struct KEYPAD
{
    union
    {
        struct
        {
            Bit a : 1;      // Button A        (0=Pressed, 1=Released)
            Bit b : 1;      // Button B        (etc.)
            Bit select : 1; // Select          (etc.)
            Bit start : 1;  // Start           (etc.)
            Bit right : 1;  // Right           (etc.)
            Bit left : 1;   // Left            (etc.)
            Bit up : 1;     // Up              (etc.)
            Bit down : 1;   // Down            (etc.)
            Bit r : 1;      // Button R        (etc.)
            Bit l : 1;      // Button L        (etc.)
            Byte : 6;       // Not used
        } bits;
        Byte bytes[2];
        HalfWord full;
    } keyinput;

    // REG_KEYCNT
    union
    {
        struct
        {
            Bit a : 1;         // Button A        (0=Ignore, 1=Select)
            Bit b : 1;         // Button B        (etc.)
            Bit select : 1;    // Select          (etc.)
            Bit start : 1;     // Start           (etc.)
            Bit right : 1;     // Right           (etc.)
            Bit left : 1;      // Left            (etc.)
            Bit up : 1;        // Up              (etc.)
            Bit down : 1;      // Down            (etc.)
            Bit r : 1;         // Button R        (etc.)
            Bit l : 1;         // Button L        (etc.)
            Byte : 4;          // Not used
            Bit irqEnable : 1; // Button IRQ Enable      (0=Disable, 1=Enable)
            Bit irqCond : 1;   // Button IRQ Condition   (0=Logical OR, 1=Logical AND)
        } bits;
        Byte bytes[2];
        HalfWord full;
    } keycnt;
};

/*
 * Struct for values of necessary IWPDC registers
 */
struct IWPDC
{
    union
    {
        struct
        {
            Bit disable : 1; // (0=Disable All, 1=See IE register)
            Word : 31;       // Not used
        } bits;
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } ime;

    union
    {
        struct
        {
            Bit vblank : 1;   // LCD V-Blank                    (0=Disable)
            Bit hblank : 1;   // LCD H-Blank                    (etc.)
            Bit vcounter : 1; // LCD V-Counter Match            (etc.)
            Bit timer0 : 1;   // Timer 0 Overflow               (etc.)
            Bit timer1 : 1;   // Timer 1 Overflow               (etc.)
            Bit timer2 : 1;   // Timer 2 Overflow               (etc.)
            Bit timer3 : 1;   // Timer 3 Overflow               (etc.)
            Bit comm : 1;     // Serial Communication           (etc.)
            Bit dma0 : 1;     // DMA 0                          (etc.)
            Bit dma1 : 1;     // DMA 1                          (etc.)
            Bit dma2 : 1;     // DMA 2                          (etc.)
            Bit dma3 : 1;     // DMA 3                          (etc.)
            Bit keypad : 1;   // Keypad                         (etc.)
            Bit gamepak : 1;  // Game Pak (external IRQ source) (etc.)
            Byte : 2;         // Not used
        } bits;
        Byte bytes[2];
        HalfWord full;
    } ie;

    union
    {
        struct
        {
            Bit vblank : 1;   // LCD V-Blank                    (1=Request Interrupt)
            Bit hblank : 1;   // LCD H-Blank                    (etc.)
            Bit vcounter : 1; // LCD V-Counter Match            (etc.)
            Bit timer0 : 1;   // Timer 0 Overflow               (etc.)
            Bit timer1 : 1;   // Timer 1 Overflow               (etc.)
            Bit timer2 : 1;   // Timer 2 Overflow               (etc.)
            Bit timer3 : 1;   // Timer 3 Overflow               (etc.)
            Bit comm : 1;     // Serial Communication           (etc.)
            Bit dma0 : 1;     // DMA 0                          (etc.)
            Bit dma1 : 1;     // DMA 1                          (etc.)
            Bit dma2 : 1;     // DMA 2                          (etc.)
            Bit dma3 : 1;     // DMA 3                          (etc.)
            Bit keypad : 1;   // Keypad                         (etc.)
            Bit gamepak : 1;  // Game Pak (external IRQ source) (etc.)
            Byte : 2;         // Not used
        } bits;
        Byte bytes[2];
        HalfWord full;
    } i_f;

    union
    {
        struct
        {
            Byte sram : 2;    // SRAM Wait Control          (0..3 = 4,3,2,8 cycles)
            Byte ws01 : 2;    // Wait State 0 First Access  (0..3 = 4,3,2,8 cycles)
            Bit ws02 : 1;     // Wait State 0 Second Access (0..1 = 2,1 cycles)
            Byte ws11 : 2;    // Wait State 1 First Access  (0..3 = 4,3,2,8 cycles)
            Bit ws12 : 1;     // Wait State 1 Second Access (0..1 = 4,1 cycles; unlike above WS0)
            Byte ws21 : 2;    // Wait State 2 First Access  (0..3 = 4,3,2,8 cycles)
            Bit ws22 : 1;     // Wait State 2 Second Access (0..1 = 8,1 cycles; unlike above WS0,WS1)
            Byte phi : 2;     // PHI Terminal Output        (0..3 = Disable, 4.19MHz, 8.38MHz, 16.78MHz)
            Bit : 1;          // Not used
            Bit prefetch : 1; // Game Pak Prefetch Buffer (Pipe) (0=Disable, 1=Enable)
            Bit type : 1;     // Game Pak Type Flag  (Read Only) (0=GBA, 1=CGB) (IN35 signal)
            HalfWord : 16;    // Not used
        } bits;
        Byte bytes[4];
        HalfWord halfwords[2];
        Word full;
    } waitcnt;

    union
    {
        struct
        {
            Bit flag : 1; // Undocumented. First Boot Flag  (0=First, 1=Further)
            Byte : 7;     // Undocumented. Not used.
        } bits;
        Byte full;
    } postflag;

    union
    {
        struct
        {
            Byte : 7;          // Undocumented. Not used.
            Bit powerDown : 1; // Undocumented. Power Down Mode  (0=Halt, 1=Stop)
        } bits;
        Byte full;
    } haltcnt;
};

/******************************************************************************
 * Defines memory core comprised of I/O structs and additional memory related fields
 *****************************************************************************/
/*
 * Struct for memory core with all info
 */
typedef struct
{
    Byte bios[BIOS_END - BIOS_START + 1];       // BIOS memory
    Byte eWRAM[EWRAM_END - EWRAM_START + 1];    // External Work RAM
    Byte iWRAM[IWRAM_END - IWRAM_START + 1];    // Internal Work RAM
    Byte palRAM[PALRAM_END - PALRAM_START + 1]; // Palette RAM
    Byte vram[VRAM_END - VRAM_START + 1];       // Video RAM
    Byte oam[OAM_END - OAM_START + 1];          // Object Attribute Memory
    Byte rom[CART_0_END - CART_0_START + 1];    // ROM memory
    Word palette[0x200];                        // Palette data

    // Internal pixel data
    union
    {
        Byte bytes[4];
        Word full;
    } internalPX[2];

    union
    {
        Byte bytes[4];
        Word full;
    } internalPY[2];

    bool reloadAffine; // Flag to reload affine transformation

    Word biosBus; // BIOS bus

    struct LCD lcd;          // LCD state
    struct SOUND sound;      // Sound state
    struct TIMERS timers[4]; // Timer registers
    struct DMA dma[4];       // DMA registers
    struct COMM comm;        // Communication state
    struct KEYPAD keypad;    // Keypad state
    struct IWPDC iwpdc;      // Internal Work RAM Data Cache

    // Delayed writes for various registers
    struct
    {
        struct
        {
            union
            {
                Byte bytes[2];
                HalfWord full;
            } reload;

            union
            {
                Byte bytes[2];
                HalfWord full;
            } control;
        } timers[4];

        union
        {
            Byte bytes[2];
            HalfWord full;
        } ie; // Interrupt Enable

        union
        {
            Byte bytes[2];
            HalfWord full;
        } i_f; // Interrupt Flags

        union
        {
            Byte bytes[2];
            HalfWord full;
        } ime; // Interrupt Master Enable
    } delayedWrites;
} memoryCore;

extern memoryCore *mem; // External reference to memory core

Word timerTemps[4]; // Temporary storage for timer values
Byte timerENB;      // Timer enable flags
Byte timerIRQ;      // Timer interrupt request flags
Byte timerIE;       // Timer interrupt enable flags

/******************************************************************************
 * Defines memory related operations (readWord, writeWord, etc.)
 *****************************************************************************/

/**
 * @brief Loads the BIOS file into memory.
 *
 * @param biosFile Path to the BIOS file.
 */
void loadBios(char *biosFile);

/**
 * @brief Loads the ROM file into memory.
 *
 * @param romFile Path to the ROM file.
 */
void loadRom(char *romFile);

/**
 * @brief Triggers an interrupt request (IRQ).
 *
 * @param flag The IRQ flag to trigger.
 */
void triggerIRQ(HalfWord flag);

/**
 * @brief Checks the status of the IRQ.
 *
 * @return Bit indicating the IRQ status.
 */
Bit checkIRQ();

/**
 * @brief Updates the specified timer.
 *
 * @param timerId The ID of the timer to update.
 */
void updateTimer(Word timerId);

/**
 * @brief Writes a byte to the specified timer.
 *
 * @param timerId The ID of the timer.
 * @param byte The byte to write.
 */
void memWriteTimer(Word timerId, Byte byte);

/**
 * @brief Updates the wait state.
 */
void updateWait();

/**
 * @brief Reads a word from the specified memory address.
 *
 * @param addr The memory address to read from.
 * @return The word read from the memory address.
 */
Word memReadWord(Word addr);

/**
 * @brief Reads a half-word from the specified memory address.
 *
 * @param addr The memory address to read from.
 * @return The half-word read from the memory address.
 */
HalfWord memReadHalfWord(Word addr);

/**
 * @brief Reads a byte from the specified memory address.
 *
 * @param addr The memory address to read from.
 * @return The byte read from the memory address.
 */
Byte memReadByte(Word addr);

/**
 * @brief Reads a byte from the specified I/O address.
 *
 * @param addr The I/O address to read from.
 * @return The byte read from the I/O address.
 */
Byte memReadIO(Word addr);

/**
 * @brief Writes a word to the specified memory address.
 *
 * @param addr The memory address to write to.
 * @param word The word to write.
 */
void memWriteWord(Word addr, Word word);

/**
 * @brief Writes a half-word to the specified memory address.
 *
 * @param addr The memory address to write to.
 * @param halfword The half-word to write.
 */
void memWriteHalfWord(Word addr, HalfWord halfword);

/**
 * @brief Writes a byte to the specified memory address.
 *
 * @param addr The memory address to write to.
 * @param byte The byte to write.
 */
void memWriteByte(Word addr, Byte byte);

/**
 * @brief Writes a byte to the specified I/O address.
 *
 * @param addr The I/O address to write to.
 * @param byte The byte to write.
 */
void memWriteIO(Word addr, Byte byte);