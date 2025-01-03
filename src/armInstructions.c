/****************************************************************************************************
 *
 * @file:    armInstructions.c
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Arm instruction set decoding.
 *
 * @references:
 *      Decoding the ARM7TDMI - https://www.gregorygaines.com/blog/decoding-the-arm7tdmi-instruction-set-game-boy-advance/
 *
 * @license:
 * GNU General Public License version 2.
 * Copyright (C) 2024 - Nolan Olhausen
 ****************************************************************************************************/

#include "common.h"
#include "cpu.h"
#include "armInstructions.h"

bool armIsBX(Word code)
{
    Word branchAndExchangeFormat = 0b00000001001011111111111100010000;
    Word formatMask = 0b00001111111111111111111111110000;
    Word extractedFormat = code & formatMask;
    return extractedFormat == branchAndExchangeFormat;
}

bool armIsBDT(Word code)
{
    Word blockDataTransferFormat = 0b00001000000000000000000000000000;
    Word formatMask = 0b00001110000000000000000000000000;
    Word extractedFormat = code & formatMask;
    return extractedFormat == blockDataTransferFormat;
}

bool armIsBL(Word code)
{
    Word branchFormat = 0b00001010000000000000000000000000;
    Word branchWithLinkFormat = 0b00001011000000000000000000000000;
    Word formatMask = 0b00001111000000000000000000000000;
    Word extractedFormat = code & formatMask;
    return extractedFormat == branchFormat || extractedFormat == branchWithLinkFormat;
}

bool armIsSWI(Word code)
{
    Word softwareInterruptFormat = 0b00001111000000000000000000000000;
    Word formatMask = 0b00001111000000000000000000000000;
    Word extractedFormat = code & formatMask;
    return extractedFormat == softwareInterruptFormat;
}

bool armIsUND(Word code)
{
    Word undefinedFormat = 0b00000110000000000000000000010000;
    Word formatMask = 0b00001110000000000000000000010000;
    Word extractedFormat = code & formatMask;
    return extractedFormat == undefinedFormat;
}

bool armIsSDT(Word code)
{
    Word singleDataTransferFormat = 0b00000100000000000000000000000000;
    Word formatMask = 0b00001100000000000000000000000000;
    Word extractedFormat = code & formatMask;
    return extractedFormat == singleDataTransferFormat;
}

bool armIsSDS(Word code)
{
    Word singleDataSwapFormat = 0b00000001000000000000000010010000;
    Word formatMask = 0b00001111100000000000111111110000;
    Word extractedFormat = code & formatMask;
    return extractedFormat == singleDataSwapFormat;
}

bool armIsMUL(Word code)
{
    Word multiplyFormat = 0b00000000000000000000000010010000;
    Word formatMask = 0b00001111100000000000000011110000;
    Word extractedFormat = code & formatMask;
    return extractedFormat == multiplyFormat;
}

bool armIsMULL(Word code)
{
    Word multiplyLongFormat = 0b00000000100000000000000010010000;
    Word formatMask = 0b00001111100000000000000011110000;
    Word extractedFormat = code & formatMask;
    return extractedFormat == multiplyLongFormat;
}

bool armIsHDTRI(Word code)
{
    Word halfwordDataTransferRegisterFormat = 0b00000000000000000000000010010000;
    Word formatMask = 0b00001110010000000000111110010000;
    Word extractedFormat = code & formatMask;

    Word halfwordDataTransferImmediateFormat = 0b00000000010000000000000010010000;
    Word formatMask2 = 0b00001110010000000000000010010000;
    Word extractedFormat2 = code & formatMask2;

    return extractedFormat == halfwordDataTransferRegisterFormat || extractedFormat2 == halfwordDataTransferImmediateFormat;
}

bool armIsPSRT(Word code)
{
    Word mrsFormat = 0b00000001000011110000000000000000;
    Word formatMask = 0b00001111101111110000000000000000;
    Word extractedFormat = code & formatMask;

    Word msrFormat = 0b00000001001000001111000000000000;
    Word formatMask2 = 0b00001101101100001111000000000000;
    Word extractedFormat2 = code & formatMask2;

    return extractedFormat == mrsFormat || extractedFormat2 == msrFormat;
}

bool armIsDPROC(Word code)
{
    Word dataProcessingFormat = 0b00000000000000000000000000000000;
    Word formatMask = 0b00001100000000000000000000000000;
    Word extractedFormat = code & formatMask;
    return extractedFormat == dataProcessingFormat;
}