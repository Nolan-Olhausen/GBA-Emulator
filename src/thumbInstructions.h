/****************************************************************************************************
 *
 * @file:    thumbInstructions.h
 * @author:  Nolan Olhausen
 * @date:    2024-09-6
 *
 * @brief:
 *      Header file for Thumb instructions decoding.
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
 * @brief Check if the given Thumb instruction is a Software Interrupt (SWI).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a SWI, false otherwise.
 */
bool thumbIsSWI(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is an Unconditional Branch (UB).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a UB, false otherwise.
 */
bool thumbIsUB(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Conditional Branch (CB).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a CB, false otherwise.
 */
bool thumbIsCB(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Multiply and Subtract (MLS).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a MLS, false otherwise.
 */
bool thumbIsMLS(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Load Byte (LBL).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a LBL, false otherwise.
 */
bool thumbIsLBL(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is an Add Offset to Stack Pointer (AOSP).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is an AOSP, false otherwise.
 */
bool thumbIsAOSP(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Push/Pop Registers (PPR).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a PPR, false otherwise.
 */
bool thumbIsPPR(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Logical Shift (LSH).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a LSH, false otherwise.
 */
bool thumbIsLSH(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Store/Load Multiple (SPRLS).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a SPRLS, false otherwise.
 */
bool thumbIsSPRLS(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Load Address (LA).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a LA, false otherwise.
 */
bool thumbIsLA(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Load/Store Immediate Offset (LSIO).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a LSIO, false otherwise.
 */
bool thumbIsLSIO(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Load/Store Register Offset (LSRO).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a LSRO, false otherwise.
 */
bool thumbIsLSRO(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Load/Store Sign-Extended Byte/Halfword (LSSEBH).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a LSSEBH, false otherwise.
 */
bool thumbIsLSSEBH(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Program Counter Relative Load (PCRL).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a PCRL, false otherwise.
 */
bool thumbIsPCRL(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Hi Register Operations/Branch Exchange (HROBX).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a HROBX, false otherwise.
 */
bool thumbIsHROBX(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is an Arithmetic Logic Unit (ALU) operation.
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is an ALU operation, false otherwise.
 */
bool thumbIsALU(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Move/Compare/Add/Subtract Immediate (MCASI).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a MCASI, false otherwise.
 */
bool thumbIsMCASI(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is an Add/Subtract (AS).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is an AS, false otherwise.
 */
bool thumbIsAS(HalfWord code);

/**
 * @brief Check if the given Thumb instruction is a Move Shifted Register (MSR).
 *
 * @param code The 16-bit Thumb instruction code.
 * @return true if the instruction is a MSR, false otherwise.
 */
bool thumbIsMSR(HalfWord code);

/**
 * @brief Process the given Thumb Software Interrupt (SWI) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTSWI(HalfWord instr);

/**
 * @brief Process the given Thumb Unconditional Branch (UB) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTUB(HalfWord instr);

/**
 * @brief Process the given Thumb Conditional Branch (CB) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTCB(HalfWord instr);

/**
 * @brief Process the given Thumb Multiply and Subtract (MLS) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTMLS(HalfWord instr);

/**
 * @brief Process the given Thumb Load Byte (LBL) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTLBL(HalfWord instr);

/**
 * @brief Process the given Thumb Add Offset to Stack Pointer (AOSP) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTAOSP(HalfWord instr);

/**
 * @brief Process the given Thumb Push/Pop Registers (PPR) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTPPR(HalfWord instr);

/**
 * @brief Process the given Thumb Logical Shift (LSH) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTLSH(HalfWord instr);

/**
 * @brief Process the given Thumb Store/Load Multiple (SPRLS) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTSPRLS(HalfWord instr);

/**
 * @brief Process the given Thumb Load Address (LA) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTLA(HalfWord instr);

/**
 * @brief Process the given Thumb Load/Store Immediate Offset (LSIO) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTLSIO(HalfWord instr);

/**
 * @brief Process the given Thumb Load/Store Register Offset (LSRO) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTLSRO(HalfWord instr);

/**
 * @brief Process the given Thumb Load/Store Sign-Extended Byte/Halfword (LSSEBH) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTLSSEBH(HalfWord instr);

/**
 * @brief Process the given Thumb Program Counter Relative Load (PCRL) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTPCRL(HalfWord instr);

/**
 * @brief Process the given Thumb Hi Register Operations/Branch Exchange (HROBX) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTHROBX(HalfWord instr);

/**
 * @brief Process the given Thumb Arithmetic Logic Unit (ALU) operation instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTALU(HalfWord instr);

/**
 * @brief Process the given Thumb Move/Compare/Add/Subtract Immediate (MCASI) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTMCASI(HalfWord instr);

/**
 * @brief Process the given Thumb Add/Subtract (AS) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTAS(HalfWord instr);

/**
 * @brief Process the given Thumb Move Shifted Register (MSR) instruction.
 *
 * @param instr The 16-bit Thumb instruction.
 */
void procTMSR(HalfWord instr);