/****************************************************************************************************
 *
 * @file:    apu.h
 * @author:  Nolan Olhausen
 * @date: 2024-09-8
 *
 * @brief:
 *      Header file for GBA Audio Processing Unit (APU).
 *
 * @references:
 *      GBATEK - https://problemkaputt.de/gbatek.htm
 *      gdkGBA - https://github.com/gdkchan/gdkGBA
 *
 * @license:
 * GNU General Public License version 2.
 * Copyright (C) 2024 - Nolan Olhausen
 ****************************************************************************************************/
#pragma once
#include "common.h"

/**
 * @brief Wave position and sample count for channel 3.
 */
Byte wavePosition;
Byte waveSamples;

/**
 * @struct channelState
 * @brief Structure to hold the state of each sound channel.
 *
 * @var channelState::phase
 * Phase for channels 1 and 2.
 * @var channelState::lfsr
 * Linear Feedback Shift Register for channel 4.
 * @var channelState::samples
 * Sample count for all channels.
 * @var channelState::lengthTime
 * Length counter for all channels.
 * @var channelState::sweepTime
 * Sweep time for channel 1.
 * @var channelState::envelopeTime
 * Envelope time for channels 1, 2, and 4.
 */
typedef struct
{
    Bit phase;
    HalfWord lfsr;
    double samples;
    double lengthTime;
    double sweepTime;
    double envelopeTime;
} channelState;

/**
 * @brief Array to hold the state of all 4 sound channels.
 */
channelState channelStates[4];

/**
 * @brief Copies data to the FIFO buffer.
 *
 * @param id The ID of the FIFO buffer.
 */
void fifoCopy(Bit id);

/**
 * @brief Loads data into the FIFO buffer.
 *
 * @param id The ID of the FIFO buffer.
 */
void fifoLoad(Bit id);

/**
 * @brief Resets the FIFO buffer.
 *
 * @param id The ID of the FIFO buffer.
 */
void fifoReset(Bit id);

/**
 * @brief Resets the state of channel 1.
 */
void channel1Reset();

/**
 * @brief Resets the state of channel 2.
 */
void channel2Reset();

/**
 * @brief Resets the state of channel 3.
 */
void channel3Reset();

/**
 * @brief Resets the state of channel 4.
 */
void channel4Reset();

/**
 * @brief Samples the output of channel 1.
 *
 * @return The sampled output as an 8-bit integer.
 */
int8_t channel1Sample();

/**
 * @brief Samples the output of channel 2.
 *
 * @return The sampled output as an 8-bit integer.
 */
int8_t channel2Sample();

/**
 * @brief Samples the output of channel 3.
 *
 * @return The sampled output as an 8-bit integer.
 */
int8_t channel3Sample();

/**
 * @brief Samples the output of channel 4.
 *
 * @return The sampled output as an 8-bit integer.
 */
int8_t channel4Sample();

/**
 * @brief Handles sound overflow operations.
 */
void soundOverflow();

/**
 * @brief Mixes the sound stream.
 *
 * @param stream Pointer to the sound stream buffer.
 * @param len Length of the sound stream buffer.
 */
void soundMix(Byte *stream, int32_t len);

/**
 * @brief Clips the sound data to prevent overflow.
 *
 * @param data The sound data to be clipped.
 * @return The clipped sound data as a 16-bit integer.
 */
static int16_t soundClip(int32_t data);

/**
 * @brief Advances the sound clock by a specified number of cycles.
 *
 * @param cyc The number of cycles to advance the sound clock.
 */
void soundClock(Word cyc);