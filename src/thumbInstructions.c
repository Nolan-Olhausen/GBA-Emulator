/****************************************************************************************************
 *
 * @file:    thumbInstructions.c
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Thumb instruction set decoding.
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
#include "thumbInstructions.h"

bool thumbIsSWI(HalfWord code)
{
    Word softwareInterruptFormat = 0b1101111100000000;

    Word formatMask = 0b1111111100000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == softwareInterruptFormat;
}

bool thumbIsUB(HalfWord code)
{
    Word unconditionalBranchFormat = 0b1110000000000000;

    Word formatMask = 0b1111100000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == unconditionalBranchFormat;
}

bool thumbIsCB(HalfWord code)
{
    Word conditionalBranchFormat = 0b1101000000000000;

    Word formatMask = 0b1111000000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == conditionalBranchFormat;
}

bool thumbIsMLS(HalfWord code)
{
    Word multipleLoadStoreFormat = 0b1100000000000000;

    Word formatMask = 0b1111000000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == multipleLoadStoreFormat;
}

bool thumbIsLBL(HalfWord code)
{
    Word longBranchWithLinkFormat = 0b1111000000000000;

    Word formatMask = 0b1111000000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == longBranchWithLinkFormat;
}

bool thumbIsAOSP(HalfWord code)
{
    Word addOffsetToStackPointerFormat = 0b1011000000000000;

    Word formatMask = 0b1111111100000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == addOffsetToStackPointerFormat;
}

bool thumbIsPPR(HalfWord code)
{
    Word pushopRegistersFormat = 0b1011010000000000;

    Word formatMask = 0b1111011000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == pushopRegistersFormat;
}

bool thumbIsLSH(HalfWord code)
{
    Word loadStoreHalfwordFormat = 0b1000000000000000;

    Word formatMask = 0b1111000000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == loadStoreHalfwordFormat;
}

bool thumbIsSPRLS(HalfWord code)
{
    Word spRelativeLoadStoreFormat = 0b1001000000000000;

    Word formatMask = 0b1111000000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == spRelativeLoadStoreFormat;
}

bool thumbIsLA(HalfWord code)
{
    Word loadAddressFormat = 0b1010000000000000;

    Word formatMask = 0b1111000000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == loadAddressFormat;
}

bool thumbIsLSIO(HalfWord code)
{
    Word loadStoreImmediateOffsetFormat = 0b0110000000000000;

    Word formatMask = 0b1110000000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == loadStoreImmediateOffsetFormat;
}

bool thumbIsLSRO(HalfWord code)
{
    Word loadStoreRegisterOffsetFormat = 0b0101000000000000;

    Word formatMask = 0b1111001000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == loadStoreRegisterOffsetFormat;
}

bool thumbIsLSSEBH(HalfWord code)
{
    Word loadStoreSignExtendedByteHalfwordFormat = 0b0101001000000000;

    Word formatMask = 0b1111001000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == loadStoreSignExtendedByteHalfwordFormat;
}

bool thumbIsPCRL(HalfWord code)
{
    Word pcRelativeLoadFormat = 0b0100100000000000;

    Word formatMask = 0b1111100000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == pcRelativeLoadFormat;
}

bool thumbIsHROBX(HalfWord code)
{
    Word hiRegisterOperationsBranchExchangeFormat = 0b0100010000000000;

    Word formatMask = 0b1111110000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == hiRegisterOperationsBranchExchangeFormat;
}

bool thumbIsALU(HalfWord code)
{
    Word aluOperationsFormat = 0b0100000000000000;

    Word formatMask = 0b1111110000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == aluOperationsFormat;
}

bool thumbIsMCASI(HalfWord code)
{
    Word moveCompareAddSubtractImmediateFormat = 0b0010000000000000;

    Word formatMask = 0b1110000000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == moveCompareAddSubtractImmediateFormat;
}

bool thumbIsAS(HalfWord code)
{
    Word addSubtractFormat = 0b0001100000000000;

    Word formatMask = 0b1111100000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == addSubtractFormat;
}

bool thumbIsMSR(HalfWord code)
{
    Word moveShiftedRegistersFormat = 0b0000000000000000;

    Word formatMask = 0b1110000000000000;

    Word extractedFormat = code & formatMask;

    return extractedFormat == moveShiftedRegistersFormat;
}