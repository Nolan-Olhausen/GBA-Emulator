/****************************************************************************************************
 *
 * @file:    apu.c
 * @author:  Nolan Olhausen
 * @date: 2024-09-8
 *
 * @brief:
 *      Audio Processing Unit (APU) implementation.
 *          > Implements FIFO operations
 *          > Implements Channel 1 (Tone & Sweep) operations
 *          > Implements Channel 2 (Tone) operations
 *          > Implements Channel 3 (Wave) operations
 *          > Implements Channel 4 (Noise) operations
 *          > Implements Sound operations
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

int8_t fifoSamp[2];
static double dutyLut[4] = {0.125, 0.250, 0.500, 0.750};                             // Duty Lookup Table
static double dutyLut2[4] = {0.875, 0.750, 0.500, 0.250};                            // Duty Lookup Table 2
static int32_t volLut[8] = {0x000, 0x024, 0x049, 0x06d, 0x092, 0x0b6, 0x0db, 0x100}; // Volume Lookup Table
static int32_t clockLut[4] = {0xa, 0x9, 0x8, 0x7};                                   // Clock Lookup Table
int16_t buffer[16384];                                                               // Audio Buffer
Word current = 0;                                                                    // Current Audio Buffer
Word write = 0x200;                                                                  // Write Audio Buffer
Word soundCycles = 0;                                                                // Sound Cycles

/******************************************************************************
 * Implements FIFO Operations
 *****************************************************************************/

void fifoCopy(Bit id)
{
    if (mem->sound.fifo[id].size + 4 > 32)
        return; // FIFO full

    mem->sound.fifo[id].capacity[mem->sound.fifo[id].size++] = mem->sound.fifo[id].reg.bytes[0];
    mem->sound.fifo[id].capacity[mem->sound.fifo[id].size++] = mem->sound.fifo[id].reg.bytes[1];
    mem->sound.fifo[id].capacity[mem->sound.fifo[id].size++] = mem->sound.fifo[id].reg.bytes[3];
    mem->sound.fifo[id].capacity[mem->sound.fifo[id].size++] = mem->sound.fifo[id].reg.bytes[4];
}

void fifoLoad(Bit id)
{
    if (mem->sound.fifo[id].size)
    {
        fifoSamp[id] = mem->sound.fifo[id].capacity[0];
        mem->sound.fifo[id].size--;

        for (Byte i = 0; i < mem->sound.fifo[id].size; i++)
        {
            mem->sound.fifo[id].capacity[i] = mem->sound.fifo[id].capacity[i + 1];
        }
    }
}

void fifoReset(Bit id)
{
    memset(&mem->sound.fifo[id], 0, sizeof(mem->sound.fifo[0]));
}

/******************************************************************************
 * Implements Channel 1 (Tone & Sweep) Operations
 *****************************************************************************/

void channel1Reset()
{
    mem->sound.sound1cnt_x.bits.initial = 0;
    mem->sound.soundcnt_x.bits.sound1 = 0;

    channelStates[0].envelopeTime = 0;
    channelStates[0].lengthTime = 0;
    channelStates[0].phase = 0;
    channelStates[0].samples = 0;
    channelStates[0].sweepTime = 0;
}

static int8_t channel1Sample()
{
    // Enable sound channel 1
    mem->sound.soundcnt_x.bits.sound1 = 1;

    // Retrieve sound parameters from memory
    Byte sweepTime = mem->sound.sound1cnt_l.bits.time;
    Byte duty = mem->sound.sound1cnt_h.bits.duty;
    Byte envStep = mem->sound.sound1cnt_h.bits.time;
    Byte envVolume = mem->sound.sound1cnt_h.bits.volume;
    Byte len = mem->sound.sound1cnt_h.bits.length;
    HalfWord hertz = mem->sound.sound1cnt_x.bits.frequency;

    // Calculate sound properties
    double frequency = 131072 / (2048 - hertz);
    double length = (64 - len) / 256.0;
    double sweep = 0.0078 * (sweepTime + 1);
    double envelope = envStep / 64.0;
    double samples = 32768 / frequency;

    // Check if length counter is enabled
    if (mem->sound.sound1cnt_x.bits.length)
    {
        channelStates[0].lengthTime += (1.0 / 32768);

        // If length time exceeds the calculated length, disable sound
        if (channelStates[0].lengthTime >= length)
        {
            mem->sound.soundcnt_x.bits.sound1 = 0; // disable
            return 0;
        }
    }

    // Update sweep time
    channelStates[0].sweepTime += (1.0 / 32768);

    if (channelStates[0].sweepTime >= sweep)
    {
        channelStates[0].sweepTime -= sweep;
        Byte shift = mem->sound.sound1cnt_l.bits.number;

        if (shift)
        {
            Word shifted = hertz >> shift;

            // Adjust frequency based on sweep direction
            if (mem->sound.sound1cnt_l.bits.direction)
            {
                hertz -= shifted;
            }
            else
            {
                hertz += shifted;
            }

            // Ensure frequency is within valid range
            if (hertz < 0x7ff)
            {
                mem->sound.sound1cnt_x.full &= ~0x7ff;
                mem->sound.sound1cnt_x.full |= hertz;
            }
            else
            {
                mem->sound.soundcnt_x.bits.sound1 = 0;
            }
        }
    }

    // Update envelope time
    if (envStep)
    {
        channelStates[0].envelopeTime += (1.0 / 32768);

        if (channelStates[0].envelopeTime >= envelope)
        {
            channelStates[0].envelopeTime -= envelope;

            // Adjust volume based on envelope direction
            if (mem->sound.sound1cnt_h.bits.direction)
            {
                if (envVolume < 0xF)
                    envVolume++;
            }
            else
            {
                if (envVolume > 0x0)
                    envVolume--;
            }

            mem->sound.sound1cnt_h.full &= ~0xf000;
            mem->sound.sound1cnt_h.full |= envVolume << 12;
        }
    }

    // Update sample count
    channelStates[0].samples++;

    // Determine phase based on duty cycle
    if (channelStates[0].samples)
    {
        double phase = samples * dutyLut[duty];

        if (channelStates[0].samples > phase)
        {
            channelStates[0].samples -= phase;
            channelStates[0].phase = 0;
        }
    }
    else
    {
        double phase = samples * dutyLut2[duty];

        if (channelStates[0].samples > phase)
        {
            channelStates[0].samples -= phase;
            channelStates[0].phase = 1;
        }
    }

    // Return the sample value based on the current phase and volume
    return channelStates[0].phase ? (envVolume / 15.0) * 0x7F : (envVolume / 15.0) * -0x80;
}

/******************************************************************************
 * Implements Channel 2 (Tone) Operations
 *****************************************************************************/

void channel2Reset()
{
    mem->sound.sound2cnt_h.bits.initial = 0;
    mem->sound.soundcnt_x.bits.sound2 = 0;

    channelStates[1].envelopeTime = 0;
    channelStates[1].lengthTime = 0;
    channelStates[1].phase = 0;
    channelStates[1].samples = 0;
    channelStates[1].sweepTime = 0;
}

static int8_t channel2Sample()
{
    // Enable sound channel 2
    mem->sound.soundcnt_x.bits.sound2 = 1;

    // Retrieve sound parameters from memory
    Byte duty = mem->sound.sound2cnt_l.bits.duty;
    Byte envStep = mem->sound.sound2cnt_l.bits.time;
    Byte envVolume = mem->sound.sound2cnt_l.bits.volume;
    Byte len = mem->sound.sound2cnt_l.bits.length;
    HalfWord hertz = mem->sound.sound2cnt_h.bits.frequency;

    // Calculate sound properties
    double frequency = 131072 / (2048 - hertz);
    double length = (64 - len) / 256.0;
    double envelope = envStep / 64.0;
    double samples = 32768 / frequency;

    // Check if length counter is enabled
    if (mem->sound.sound2cnt_h.bits.length)
    {
        channelStates[1].lengthTime += (1.0 / 32768);

        // If length time exceeds the calculated length, disable sound
        if (channelStates[1].lengthTime >= length)
        {
            mem->sound.soundcnt_x.bits.sound2 = 0; // disable
            return 0;
        }
    }

    // Update envelope time
    if (envStep)
    {
        channelStates[1].envelopeTime += (1.0 / 32768);

        if (channelStates[1].envelopeTime >= envelope)
        {
            channelStates[1].envelopeTime -= envelope;

            // Adjust volume based on envelope direction
            if (mem->sound.sound2cnt_l.bits.direction)
            {
                if (envVolume < 0xF)
                    envVolume++;
            }
            else
            {
                if (envVolume > 0x0)
                    envVolume--;
            }

            mem->sound.sound2cnt_l.full &= ~0xf000;
            mem->sound.sound2cnt_l.full |= envVolume << 12;
        }
    }

    // Update sample count
    channelStates[1].samples++;

    // Determine phase based on duty cycle
    if (channelStates[1].samples)
    {
        double phase = samples * dutyLut[duty];

        if (channelStates[1].samples > phase)
        {
            channelStates[1].samples -= phase;
            channelStates[1].phase = 0;
        }
    }
    else
    {
        double phase = samples * dutyLut2[duty];

        if (channelStates[1].samples > phase)
        {
            channelStates[1].samples -= phase;
            channelStates[1].phase = 1;
        }
    }

    // Return the sample value based on the current phase and volume
    return channelStates[1].phase ? (envVolume / 15.0) * 0x7F : (envVolume / 15.0) * -0x80;
}

/******************************************************************************
 * Implements Channel 3 (Wave) Operations
 *****************************************************************************/

void channel3Reset()
{
    mem->sound.sound3cnt_x.bits.initial = 0;
    mem->sound.soundcnt_x.bits.sound3 = 0;

    channelStates[2].envelopeTime = 0;
    channelStates[2].lengthTime = 0;
    channelStates[2].phase = 0;
    channelStates[2].samples = 0;
    channelStates[2].sweepTime = 0;

    if (mem->sound.sound3cnt_l.bits.dimension)
    {
        wavePosition = 0;
        waveSamples = 64;
    }
    else
    {
        wavePosition = (mem->sound.sound3cnt_l.full >> 1) & 0x20;
        waveSamples = 32;
    }
}

static int8_t channel3Sample()
{
    // Check if sound channel 3 is enabled
    if (!(mem->sound.sound3cnt_l.bits.enable))
        return 0; // sound 3 not enabled

    // Enable sound channel 3
    mem->sound.soundcnt_x.bits.sound3 = 1;

    // Retrieve sound parameters from memory
    Byte len = mem->sound.sound3cnt_h.bits.length;
    Byte volume = mem->sound.sound3cnt_h.bits.volume;
    Bit force = mem->sound.sound3cnt_h.bits.forceVolume;
    HalfWord hertz = mem->sound.sound3cnt_x.bits.sampleRate;

    // Calculate sound properties
    double frequency = 2097152 / (2048 - hertz);
    double length = (256 - len) / 256.0;
    double samples = 32768 / frequency;

    // Check if length counter is enabled
    if (mem->sound.sound3cnt_x.bits.length)
    {
        channelStates[2].lengthTime += (1.0 / 32768);

        // If length time exceeds the calculated length, disable sound
        if (channelStates[2].lengthTime >= length)
        {
            mem->sound.soundcnt_x.bits.sound3 = 0; // disable
            return 0;
        }
    }

    // Update sample count
    channelStates[2].samples++;

    // Check if the sample count exceeds the calculated samples
    if (channelStates[2].samples >= samples)
    {
        channelStates[2].samples -= samples;

        // Update wave position or reset channel
        if (--waveSamples)
        {
            wavePosition = (wavePosition + 1) & 0x3F;
        }
        else
        {
            channel3Reset();
        }
    }

    // Retrieve the current sample from wave RAM
    int8_t sample = wavePosition & 1
                        ? ((mem->sound.wave_ram[mem->sound.sound3cnt_l.bits.number].reg[(((wavePosition >> 1) & 0x1f) / 2) % 8].bytes[((wavePosition >> 1) & 0x1f) % 2] >> 0) & 0xF) - 8
                        : ((mem->sound.wave_ram[mem->sound.sound3cnt_l.bits.number].reg[(((wavePosition >> 1) & 0x1f) / 2) % 8].bytes[((wavePosition >> 1) & 0x1f) % 2] >> 4) & 0xF) - 8;

    // Adjust sample based on force volume setting
    if (force == 1)
    {
        sample = (sample >> 2) * 3; // force 75%
    }
    else
    {
        // Adjust sample based on volume setting
        switch (volume)
        {
        case 1:
            sample >>= 0;
            break; // 100%
        case 2:
            sample >>= 1;
            break; // 50%
        case 3:
            sample >>= 2;
            break; // 25%
        default:
            sample = 0; // Mute
        }
    }

    // Return the sample value based on the current phase and volume
    return sample >= 0 ? (sample / 7.0) * 0x7F : (sample / -8.0) * -0x80;
}

/******************************************************************************
 * Implements Channel 4 (Noise) Operations
 *****************************************************************************/

void channel4Reset()
{
    mem->sound.sound4cnt_h.bits.initial = 0;
    mem->sound.soundcnt_x.bits.sound4 = 0;

    channelStates[3].envelopeTime = 0;
    channelStates[3].lengthTime = 0;
    channelStates[3].phase = 0;
    channelStates[3].samples = 0;
    channelStates[3].sweepTime = 0;
}

static int8_t channel4Sample()
{
    // Enable sound channel 4
    mem->sound.soundcnt_x.bits.sound4 = 1;

    // Retrieve sound parameters from memory
    Byte envStep = mem->sound.sound4cnt_l.bits.time;
    Byte envVolume = mem->sound.sound4cnt_l.bits.volume;
    Byte len = mem->sound.sound4cnt_l.bits.length;
    Byte ratio = mem->sound.sound4cnt_h.bits.ratio;
    Byte clock = mem->sound.sound4cnt_h.bits.frequency;

    // Calculate sound properties
    double frequency = ratio ? (524288 / ratio) >> (clock + 1) : (524288 * 2) >> (clock + 1);
    double length = (64 - len) / 256.0;
    double envelope = envStep / 64.0;
    double samples = 32768 / frequency;

    // Check if length counter is enabled
    if (mem->sound.sound4cnt_h.bits.length)
    {
        channelStates[3].lengthTime += (1.0 / 32768);

        // If length time exceeds the calculated length, disable sound
        if (channelStates[3].lengthTime >= length)
        {
            mem->sound.soundcnt_x.bits.sound4 = 0; // disable
            return 0;
        }
    }

    // Update envelope time
    if (envStep)
    {
        channelStates[3].envelopeTime += (1.0 / 32768);

        if (channelStates[3].envelopeTime >= envelope)
        {
            channelStates[3].envelopeTime -= envelope;

            // Adjust volume based on envelope direction
            if (mem->sound.sound4cnt_l.bits.direction)
            {
                if (envVolume < 0xf)
                    envVolume++;
            }
            else
            {
                if (envVolume > 0x0)
                    envVolume--;
            }

            mem->sound.sound4cnt_l.full &= ~0xf000;
            mem->sound.sound4cnt_l.full |= envVolume << 12;
        }
    }

    // Get the carry bit from the LFSR
    Byte carry = channelStates[3].lfsr & 1;

    // Update sample count
    channelStates[3].samples++;

    // Check if the sample count exceeds the calculated samples
    if (channelStates[3].samples >= samples)
    {
        channelStates[3].samples -= samples;

        // Shift the LFSR
        channelStates[3].lfsr >>= 1;

        // Calculate the new high bit
        Byte high = (channelStates[3].lfsr & 1) ^ carry;

        // Update the LFSR based on the width setting
        if (mem->sound.sound4cnt_h.bits.width)
            channelStates[3].lfsr |= (high << 6);
        else
            channelStates[3].lfsr |= (high << 14);
    }

    // Return the sample value based on the current phase and volume
    return carry ? (envVolume / 15.0) * 0x7F : (envVolume / 15.0) * -0x80;
}

/******************************************************************************
 * Implements Sound Operations
 *****************************************************************************/

void soundOverflow()
{
    // Check if the current and write pointers are in the same 16KB block
    if ((current / 16384) == (write / 16384))
    {
        // Mask the pointers to stay within the 16KB block
        current &= 16383;
        write &= 16383;
    }
}

void soundMix(Byte *stream, int32_t len)
{
    // Mix sound data into the provided stream buffer
    for (int32_t i = 0; i < len; i += 4)
    {
        // Mix left and right channels
        *(int16_t *)(stream + (i | 0)) = buffer[current++ & 16383] << 6;
        *(int16_t *)(stream + (i | 2)) = buffer[current++ & 16383] << 6;
    }
    // Adjust the current pointer based on the write pointer
    current += ((int32_t)(write - current) >> 8) & ~1;
}

static int16_t soundClip(int32_t data)
{
    // Clip the sound data to the range -0x200 to 0x1FF
    if (data > 0x1FF)
        data = 0x1FF;
    if (data < -0x200)
        data = -0x200;
    return data;
}

void soundClock(Word cyc)
{
    // Increment the sound cycle counter
    soundCycles += cyc;

    // Initialize DMA samples
    int16_t dmaLeftSample = 0;
    int16_t dmaRightSample = 0;

    // Calculate channel 4 and 5 samples
    int16_t ch4Sample = (fifoSamp[0] << 1) >> !(mem->sound.soundcnt_h.full & 4);
    int16_t ch5Sample = (fifoSamp[1] << 1) >> !(mem->sound.soundcnt_h.full & 8);

    // Mix DMA samples based on sound control settings
    if (mem->sound.soundcnt_h.bits.dmaALeft)
        dmaLeftSample = soundClip(dmaLeftSample + ch4Sample);
    if (mem->sound.soundcnt_h.bits.dmaBLeft)
        dmaLeftSample = soundClip(dmaLeftSample + ch5Sample);
    if (mem->sound.soundcnt_h.bits.dmaARight)
        dmaRightSample = soundClip(dmaRightSample + ch4Sample);
    if (mem->sound.soundcnt_h.bits.dmaBRight)
        dmaRightSample = soundClip(dmaRightSample + ch5Sample);

    // Process sound cycles
    while (soundCycles >= (16777216 / 32768))
    {
        // Get samples from each sound channel
        int16_t sample1 = channel1Sample();
        int16_t sample2 = channel2Sample();
        int16_t sample3 = channel3Sample();
        int16_t sample4 = channel4Sample();

        // Initialize left and right channel samples
        int32_t channelLeftSample = 0;
        int32_t channelRightSample = 0;

        // Mix channel samples based on sound control settings
        if (mem->sound.soundcnt_l.bits.left1)
            channelLeftSample = soundClip(channelLeftSample + sample1);
        if (mem->sound.soundcnt_l.bits.left2)
            channelLeftSample = soundClip(channelLeftSample + sample2);
        if (mem->sound.soundcnt_l.bits.left3)
            channelLeftSample = soundClip(channelLeftSample + sample3);
        if (mem->sound.soundcnt_l.bits.left4)
            channelLeftSample = soundClip(channelLeftSample + sample4);

        if (mem->sound.soundcnt_l.bits.right1)
            channelRightSample = soundClip(channelRightSample + sample1);
        if (mem->sound.soundcnt_l.bits.right2)
            channelRightSample = soundClip(channelRightSample + sample2);
        if (mem->sound.soundcnt_l.bits.right3)
            channelRightSample = soundClip(channelRightSample + sample3);
        if (mem->sound.soundcnt_l.bits.right4)
            channelRightSample = soundClip(channelRightSample + sample4);

        // Apply master volume settings
        channelLeftSample *= volLut[mem->sound.soundcnt_l.bits.leftMaster];
        channelRightSample *= volLut[mem->sound.soundcnt_l.bits.rightMaster];

        // Apply clock volume settings
        channelLeftSample >>= clockLut[mem->sound.soundcnt_h.bits.volume];
        channelRightSample >>= clockLut[mem->sound.soundcnt_h.bits.volume];

        // Store the mixed samples in the buffer
        buffer[write++ & 16383] = soundClip(channelLeftSample + dmaLeftSample);
        buffer[write++ & 16383] = soundClip(channelRightSample + dmaRightSample);

        // Decrement the sound cycle counter
        soundCycles -= (16777216 / 32768);
    }
}
