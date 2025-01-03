/****************************************************************************************************
 *
 * @file:    cpu.h
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Header file for GBA CPU core.
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
#include "armInstructions.h"
#include "thumbInstructions.h"
#include "memory.h"
#include "ppu.h"

// CPSR (Current Program Status Register) operation types
#define CPSR_SUB 0
#define CPSR_ADD 1
#define CPSR_LOG 2
#define CPSR_MOVS 3

// CPU states
enum CPU_STATES
{
    RUN = 0,
    HALT = 1,
    STOP = 2,
};

// Instruction modes
enum INSTR_MODE
{
    ARM_MODE = 0,
    THUMB_MODE
};

// Condition codes for instructions
enum COND
{
    EQ = 0, // Equal
    NE,     // Not Equal
    CS,     // Carry Set
    CC,     // Carry Clear
    MI,     // Minus
    PL,     // Plus
    VS,     // Overflow Set
    VC,     // Overflow Clear
    HI,     // Higher
    LS,     // Lower or Same
    GE,     // Greater or Equal
    LT,     // Less Than
    GT,     // Greater Than
    LE,     // Less or Equal
    AL      // Always
};

// Instruction types
enum INSTR_TYPE
{
    BX = 0,
    BDT,
    BL,
    SWI,
    UND,
    SDT,
    SDS,
    MUL,
    HDTRI,
    PSRT,
    DPROC,
    TSWI,
    UB,
    CB,
    MLS,
    LBL,
    AOSP,
    PPR,
    LSH,
    SPRLS,
    LA,
    LSIO,
    LSRO,
    LSSEBH,
    PCRL,
    HROBX,
    ALU,
    MCASI,
    AS,
    MSR
};

// Flags for the CPSR
typedef enum
{
    N, // Negative
    Z, // Zero
    C, // Carry
    V  // Overflow
} Flag;

// CPU modes
enum CPU_MODE
{
    USER = 0x10,
    FIQ = 0x11,
    IRQ = 0x12,
    SVC = 0x13,
    ABT = 0x17,
    UNDEF = 0x1B,
    SYSTEM = 0x1F
};

// IRQ vectors
typedef enum IRQ_VECTOR irqVect;

enum IRQ_VECTOR
{
    LCD_VBLANK = 0x0,
    LCD_HBLANK = 0x1,
    LCD_VCOUNT = 0x2,
    TIMER0OF = 0x3,
    TIMER1OF = 0x4,
    TIMER2OF = 0x5,
    TIMER3OF = 0x6,
    SERIAL = 0x7,
    DMA0 = 0x8,
    DMA1 = 0x9,
    DMA2 = 0xA,
    DMA3 = 0xB,
    KEYPAD = 0xC,
    GAMEPAK = 0xD,
};

// Structure to hold the state of the CPU
typedef struct
{
    // General registers
    Word regs[16]; // Not banked, used by more than one mode

    // FIQ mode registers
    Word regsFIQ[7];

    // Supervisor mode registers
    Word regsSVC[2];

    // Abort mode registers
    Word regsABT[2];

    // IRQ mode registers
    Word regsIRQ[2];

    // Undefined mode registers
    Word regsUND[2];

    // Status registers
    Word cpsr; // Current Program Status Register

    // Saved Program Status Registers for different modes
    Word spsr_fiq; // FIQ mode
    Word spsr_svc; // Supervisor mode
    Word spsr_abt; // Abort mode
    Word spsr_irq; // IRQ mode
    Word spsr_und; // Undefined mode

    enum CPU_MODE cpuMode;     // Current CPU mode
    enum INSTR_MODE instrMode; // Current instruction mode
    enum CPU_STATES cpuState;  // Current CPU state
    enum COND cond;            // Current condition code

    Byte carry; // Carry flag

    int typeFromDecode; // Instruction type from decode

    Word pipeline; // Instruction pipeline

    DWord cycle; // Cycle count
} cpuCore;

// ARM vector addresses
#define ARM_VEC_RESET 0x00  // Reset
#define ARM_VEC_UND 0x04    // Undefined
#define ARM_VEC_SVC 0x08    // Supervisor Call
#define ARM_VEC_PABT 0x0c   // Prefetch Abort
#define ARM_VEC_DABT 0x10   // Data Abort
#define ARM_VEC_ADDR26 0x14 // Address exceeds 26 bits (legacy)
#define ARM_VEC_IRQ 0x18    // IRQ
#define ARM_VEC_FIQ 0x1c    // Fast IRQ

extern cpuCore *cpu; // External reference to CPU core

/**
 * @brief Starts the GBA emulator with the given ROM and BIOS.
 *
 * @param rom Path to the ROM file.
 * @param bios Path to the BIOS file.
 */
void startGBA(char *rom, char *bios);

/**
 * @brief Executes the input for a given number of cycles.
 *
 * @param cycles Number of cycles to execute.
 */
void executeInput(Word cycles);

/**
 * @brief Fetches the next instruction to be executed.
 *
 * @return The fetched instruction.
 */
Word fetchInstruction();

/**
 * @brief Decodes the given instruction.
 *
 * @param instr The instruction to decode.
 * @return Status of the decoding process.
 */
static int decodeInstruction(Word instr);

/**
 * @brief Executes the current instruction.
 *
 * @return Cycles passed.
 */
static int execute(void);

/**
 * @brief Gets the value of the specified register.
 *
 * @param regId The ID of the register.
 * @return The value of the register.
 */
Word getReg(Byte regId);

/**
 * @brief Sets the value of the specified register.
 *
 * @param regId The ID of the register.
 * @param val The value to set.
 */
void setReg(Byte regId, Word val);

/**
 * @brief Gets the condition code flag.
 *
 * @param cc The condition code flag.
 * @return The value of the condition code flag.
 */
Bit getCC(Flag cc);

/**
 * @brief Sets the condition code flags.
 *
 * @param n Negative flag.
 * @param z Zero flag.
 * @param c Carry flag.
 * @param v Overflow flag.
 */
void setCC(Byte n, int z, int c, int v);

/**
 * @brief Sets the Program Status Register (PSR) value.
 *
 * @param val The value to set.
 */
void setPSR(Word val);

/**
 * @brief Gets the value of the Program Status Register (PSR).
 *
 * @return The value of the PSR.
 */
Word getPSR(void);

/**
 * @brief Evaluates the condition based on the opcode.
 *
 * @param opcode The opcode to evaluate.
 * @return True if the condition is met, false otherwise.
 */
bool evalCond(Byte opcode);

/**
 * @brief Updates the Current Program Status Register (CPSR) flags.
 *
 * @param n Negative flag.
 * @param z Zero flag.
 * @param c Carry flag.
 * @param v Overflow flag.
 */
void updateCPSR(Byte n, int z, int c, int v);

/**
 * @brief Checks if the ARM processor is in Thumb mode.
 *
 * @return True if in Thumb mode, false otherwise.
 */
bool cpuInThumb();