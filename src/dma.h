/****************************************************************************************************
 *
 * @file:    dma.h
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Header file for GBA DMA aspects.
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
#include "common.h"

// DMA control flags
#define DMA_REP (1 << 9)  // DMA repeat
#define DMA_32 (1 << 10)  // DMA 32-bit transfer
#define DMA_IRQ (1 << 14) // DMA interrupt request
#define DMA_ENB (1 << 15) // DMA enable

// Enumeration for DMA timing modes
typedef enum
{
    IMMEDIATELY = 0, // DMA transfer immediately
    VBLANK = 1,      // DMA transfer during vertical blanking
    HBLANK = 2,      // DMA transfer during horizontal blanking
    SPECIAL = 3      // DMA transfer during special timing
} dmaTiming;

// Arrays to hold DMA source, destination, and count for 4 channels
Word dmaSrc[4];   // DMA source addresses
Word dmaDest[4];  // DMA destination addresses
Word dmaCount[4]; // DMA transfer counts

/**
 * @brief Initiates a DMA transfer based on the specified timing.
 *
 * @param timing The timing mode for the DMA transfer.
 */
void dmaTransfer(dmaTiming timing);

/**
 * @brief Initiates a DMA transfer using FIFO (First In, First Out) mode for the specified channel.
 *
 * @param ch The DMA channel to use for the transfer.
 */
void dmaTransferFIFO(Byte ch);

/**
 * @brief Loads a value into the specified DMA channel.
 *
 * @param ch The DMA channel to load the value into.
 * @param value The value to load into the DMA channel.
 */
void dmaLoad(Byte ch, Byte value);