/****************************************************************************************************
 *
 * @file:    armInstructions.h
 * @author:  Nolan Olhausen
 * @date:    2024-09-6
 *
 * @brief:
 *      Header file for ARM instructions decoding.
 *
 * @references:
 *      Decoding the ARM7TDMI - https://www.gregorygaines.com/blog/decoding-the-arm7tdmi-instruction-set-game-boy-advance/
 *
 * @license:
 * GNU General Public License version 2.
 * Copyright (C) 2024 - Nolan Olhausen
 ****************************************************************************************************/

#pragma once

#include "common.h"

/**
 * @enum SHIFT_TYPE
 * @brief Enum representing the types of shift operations.
 */
enum SHIFT_TYPE
{
    LSL, /**< Logical Shift Left */
    LSR, /**< Logical Shift Right */
    ASR, /**< Arithmetic Shift Right */
    ROR  /**< Rotate Right */
};

/**
 * @enum DPROC_OPCODE
 * @brief Enum representing the data processing opcodes.
 */
enum DPROC_OPCODE
{
    AND, /**< Logical AND */
    EOR, /**< Exclusive OR */
    SUB, /**< Subtract */
    RSB, /**< Reverse Subtract */
    ADD, /**< Add */
    ADC, /**< Add with Carry */
    SBC, /**< Subtract with Carry */
    RSC, /**< Reverse Subtract with Carry */
    TST, /**< Test */
    TEQ, /**< Test Equivalence */
    CMP, /**< Compare */
    CMN, /**< Compare Negative */
    ORR, /**< Logical OR */
    MOV, /**< Move */
    BIC, /**< Bit Clear */
    MVN  /**< Move Not */
};

/**
 * @brief Checks if the given code corresponds to a BX instruction.
 * @param code The instruction code.
 * @return True if the code is a BX instruction, false otherwise.
 */
bool armIsBX(Word code);

/**
 * @brief Checks if the given code corresponds to a Block Data Transfer instruction.
 * @param code The instruction code.
 * @return True if the code is a Block Data Transfer instruction, false otherwise.
 */
bool armIsBDT(Word code);

/**
 * @brief Checks if the given code corresponds to a Branch with Link instruction.
 * @param code The instruction code.
 * @return True if the code is a Branch with Link instruction, false otherwise.
 */
bool armIsBL(Word code);

/**
 * @brief Checks if the given code corresponds to a Software Interrupt instruction.
 * @param code The instruction code.
 * @return True if the code is a Software Interrupt instruction, false otherwise.
 */
bool armIsSWI(Word code);

/**
 * @brief Checks if the given code corresponds to an Undefined instruction.
 * @param code The instruction code.
 * @return True if the code is an Undefined instruction, false otherwise.
 */
bool armIsUND(Word code);

/**
 * @brief Checks if the given code corresponds to a Single Data Transfer instruction.
 * @param code The instruction code.
 * @return True if the code is a Single Data Transfer instruction, false otherwise.
 */
bool armIsSDT(Word code);

/**
 * @brief Checks if the given code corresponds to a Single Data Swap instruction.
 * @param code The instruction code.
 * @return True if the code is a Single Data Swap instruction, false otherwise.
 */
bool armIsSDS(Word code);

/**
 * @brief Checks if the given code corresponds to a Multiply instruction.
 * @param code The instruction code.
 * @return True if the code is a Multiply instruction, false otherwise.
 */
bool armIsMUL(Word code);

/**
 * @brief Checks if the given code corresponds to a Multiply Long instruction.
 * @param code The instruction code.
 * @return True if the code is a Multiply Long instruction, false otherwise.
 */
bool armIsMULL(Word code);

/**
 * @brief Checks if the given code corresponds to a Halfword and Signed Data Transfer instruction.
 * @param code The instruction code.
 * @return True if the code is a Halfword and Signed Data Transfer instruction, false otherwise.
 */
bool armIsHDTRI(Word code);

/**
 * @brief Checks if the given code corresponds to a PSR Transfer instruction.
 * @param code The instruction code.
 * @return True if the code is a PSR Transfer instruction, false otherwise.
 */
bool armIsPSRT(Word code);

/**
 * @brief Checks if the given code corresponds to a Data Processing instruction.
 * @param code The instruction code.
 * @return True if the code is a Data Processing instruction, false otherwise.
 */
bool armIsDPROC(Word code);

/**
 * @brief Performs a barrel shift operation.
 * @param shift The shift type.
 * @param op The operand to be shifted.
 * @param carry Pointer to a boolean to store the carry out.
 * @return The result of the barrel shift operation.
 */
Word barrelShifter(ShiftType shiftType, Word operand2, size_t shift, bool regShiftByImm);

/**
 * @brief Processes a BX instruction.
 * @param instr The instruction code.
 */
void procBX(Word instr);

/**
 * @brief Processes a Block Data Transfer instruction.
 * @param instr The instruction code.
 */
void procBDT(Word instr);

/**
 * @brief Processes a Branch with Link instruction.
 * @param instr The instruction code.
 */
void procBL(Word instr);

/**
 * @brief Processes a Software Interrupt instruction.
 * @param instr The instruction code.
 */
void procSWI(Word instr);

/**
 * @brief Processes an Undefined instruction.
 * @param instr The instruction code.
 */
void procUND(Word instr);

/**
 * @brief Processes a Single Data Transfer instruction.
 * @param instr The instruction code.
 */
void procSDT(Word instr);

/**
 * @brief Processes a Single Data Swap instruction.
 * @param instr The instruction code.
 */
void procSDS(Word instr);

/**
 * @brief Processes a Multiply instruction.
 * @param instr The instruction code.
 */
void procMUL(Word instr);

/**
 * @brief Processes a Halfword and Signed Data Transfer instruction.
 * @param instr The instruction code.
 */
void procHDTRI(Word instr);

/**
 * @brief Processes a PSR Transfer instruction.
 * @param instr The instruction code.
 */
void procPSRT(Word instr);

/**
 * @brief Processes a Data Processing instruction.
 * @param instr The instruction code.
 */
void procDPROC(Word instr);