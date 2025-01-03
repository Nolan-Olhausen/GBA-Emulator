/****************************************************************************************************
 *
 * @file:    ppu.h
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Header file for GBA Pixel Processing Unit (PPU).
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

/**
 * @brief Advances the PPU state by one tick.
 *
 * This function should be called to update the PPU state, typically once per CPU cycle.
 */
void tickPPU(void);

/**
 * @brief Initializes the frame buffer.
 *
 * This function sets up the frame buffer, which is used to store the pixel data for each frame.
 */
void initFrameBuffer(void);

/**
 * @var frame
 * @brief Pointer to the frame buffer.
 *
 * This pointer holds the address of the frame buffer, which contains the pixel data for the current frame.
 */
Word *frame;
