/****************************************************************************************************
 *
 * @file:    cpu.c
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Implementation file for GBA CPU core.
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
#include "cpu.h"
#include "memory.h"
#include "thumbInstructions.h"
#include "armInstructions.h"
#include "ppu.h"
#include "sdlUtil.h"

#define CC_UNMOD 2 // Condition code for unmodified instructions

// Macro to check if THUMB mode is activated
#define THUMB_ACTIVATED (cpu->cpsr >> 5 & 1)

// Macro to get the current processor mode
#define PROCESSOR_MODE (cpu->cpsr & 0x1F)

// Macro to extract the condition field from an instruction
#define INSTR_COND_FIELD(instr) ((instr >> 28) & 0xF)

// Macro to update the program counter and reset the instruction pipeline
#define PC_UPDATE(new_pc)   \
    cpu->regs[15] = new_pc; \
    cpu->pipeline = 0;

// Number of cycles per frame
#define CYCLES_PER_FRAME 280896

Word getPSR(void)
{
    switch (PROCESSOR_MODE)
    {
    case USER:
    case SYSTEM:
        return cpu->cpsr; // Return Current Program Status Register (CPSR)
    case FIQ:
        return cpu->spsr_fiq; // Return Saved Program Status Register for FIQ mode
    case IRQ:
        return cpu->spsr_irq; // Return Saved Program Status Register for IRQ mode
    case SVC:
        return cpu->spsr_svc; // Return Saved Program Status Register for Supervisor mode
    case ABT:
        return cpu->spsr_abt; // Return Saved Program Status Register for Abort mode
    case UNDEF:
        return cpu->spsr_und; // Return Saved Program Status Register for Undefined mode
    }
    return 0; // Default case, return 0 if mode is not handled
}

void setPSR(Word val)
{
    switch (PROCESSOR_MODE)
    {
    case USER:
    case SYSTEM:
        cpu->cpsr = val; // Set Current Program Status Register (CPSR)
        break;
    case FIQ:
        cpu->spsr_fiq = val; // Set Saved Program Status Register for FIQ mode
        break;
    case IRQ:
        cpu->spsr_irq = val; // Set Saved Program Status Register for IRQ mode
        break;
    case SVC:
        cpu->spsr_svc = val; // Set Saved Program Status Register for Supervisor mode
        break;
    case ABT:
        cpu->spsr_abt = val; // Set Saved Program Status Register for Abort mode
        break;
    case UNDEF:
        cpu->spsr_und = val; // Set Saved Program Status Register for Undefined mode
        break;
    }
}

// Update the Program Counter (PC) and reset the instruction pipeline
void updatePC(Word newPC)
{
    cpu->regs[15] = newPC; // Set the new value of the Program Counter
    cpu->pipeline = 0;     // Reset the instruction pipeline
}

// Update the CPU mode
void updateCPUMode(int mode)
{
    cpu->cpsr &= ~0x1F;  // Clear the current mode bits
    cpu->cpsr |= (mode); // Set the new mode bits
}

Word getReg(Byte regId)
{
    switch (regId)
    {
    case 0x0:
        return cpu->regs[0];
    case 0x1:
        return cpu->regs[1];
    case 0x2:
        return cpu->regs[2];
    case 0x3:
        return cpu->regs[3];
    case 0x4:
        return cpu->regs[4];
    case 0x5:
        return cpu->regs[5];
    case 0x6:
        return cpu->regs[6];
    case 0x7:
        return cpu->regs[7];
    case 0x8:
        if (PROCESSOR_MODE == FIQ)
            return cpu->regsFIQ[0];
        return cpu->regs[8];
    case 0x9:
        if (PROCESSOR_MODE == FIQ)
            return cpu->regsFIQ[1];
        return cpu->regs[9];
    case 0xA:
        if (PROCESSOR_MODE == FIQ)
            return cpu->regsFIQ[2];
        return cpu->regs[10];
    case 0xB:
        if (PROCESSOR_MODE == FIQ)
            return cpu->regsFIQ[3];
        return cpu->regs[11];
    case 0xC:
        if (PROCESSOR_MODE == FIQ)
            return cpu->regsFIQ[4];
        return cpu->regs[12];
    case 0xD:
        switch (PROCESSOR_MODE)
        {
        case USER:
        case SYSTEM:
            return cpu->regs[13];
        case FIQ:
            return cpu->regsFIQ[5];
        case IRQ:
            return cpu->regsIRQ[0];
        case SVC:
            return cpu->regsSVC[0];
        case ABT:
            return cpu->regsABT[0];
        case UNDEF:
            return cpu->regsUND[0];
        default:
            fprintf(stderr, "CPU Error: invalid\n");
            exit(1);
        }
    case 0xE:
        switch (PROCESSOR_MODE)
        {
        case USER:
        case SYSTEM:
            return cpu->regs[14];
        case FIQ:
            return cpu->regsFIQ[6];
        case IRQ:
            return cpu->regsIRQ[1];
        case SVC:
            return cpu->regsSVC[1];
        case ABT:
            return cpu->regsABT[1];
        case UNDEF:
            return cpu->regsUND[1];
        default:
            fprintf(stderr, "CPU Error: invalid\n");
            exit(1);
        }
    case 0xF:
        return cpu->regs[15];
    }
}

void setReg(Byte regId, Word val)
{
    switch (regId)
    {
    case 0x0:
        cpu->regs[0] = val;
        break;
    case 0x1:
        cpu->regs[1] = val;
        break;
    case 0x2:
        cpu->regs[2] = val;
        break;
    case 0x3:
        cpu->regs[3] = val;
        break;
    case 0x4:
        cpu->regs[4] = val;
        break;
    case 0x5:
        cpu->regs[5] = val;
        break;
    case 0x6:
        cpu->regs[6] = val;
        break;
    case 0x7:
        cpu->regs[7] = val;
        break;
    case 0x8:
        if (PROCESSOR_MODE == FIQ)
        {
            cpu->regsFIQ[0] = val;
        }
        else
        {
            cpu->regs[8] = val;
        }
        break;
    case 0x9:
        if (PROCESSOR_MODE == FIQ)
        {
            cpu->regsFIQ[1] = val;
        }
        else
        {
            cpu->regs[9] = val;
        }
        break;
    case 0xA:
        if (PROCESSOR_MODE == FIQ)
        {
            cpu->regsFIQ[2] = val;
        }
        else
        {
            cpu->regs[10] = val;
        }
        break;
    case 0xB:
        if (PROCESSOR_MODE == FIQ)
        {
            cpu->regsFIQ[3] = val;
        }
        else
        {
            cpu->regs[11] = val;
        }
        break;
    case 0xC:
        if (PROCESSOR_MODE == FIQ)
        {
            cpu->regsFIQ[4] = val;
        }
        else
        {
            cpu->regs[12] = val;
        }
        break;
    case 0xD:
        switch (PROCESSOR_MODE)
        {
        case USER:
        case SYSTEM:
            cpu->regs[13] = val;
            break;
        case FIQ:
            cpu->regsFIQ[5] = val;
            break;
        case IRQ:
            cpu->regsIRQ[0] = val;
            break;
        case SVC:
            cpu->regsSVC[0] = val;
            break;
        case ABT:
            cpu->regsABT[0] = val;
            break;
        case UNDEF:
            cpu->regsUND[0] = val;
            break;
        default:
            fprintf(stderr, "CPU Error: invalid\n");
            exit(1);
        }
        break;
    case 0xE:
        switch (PROCESSOR_MODE)
        {
        case USER:
        case SYSTEM:
            cpu->regs[14] = val;
            break;
        case FIQ:
            cpu->regsFIQ[6] = val;
            break;
        case IRQ:
            cpu->regsIRQ[1] = val;
            break;
        case SVC:
            cpu->regsSVC[1] = val;
            break;
        case ABT:
            cpu->regsABT[1] = val;
            break;
        case UNDEF:
            cpu->regsUND[1] = val;
            break;
        default:
            fprintf(stderr, "CPU Error: invalid\n");
            exit(1);
        }
        break;
    case 0xF:
        updatePC(THUMB_ACTIVATED ? val & ~0x1 : val & ~0x3);
        break;
    }
}

Bit getCC(Flag cc)
{
    switch (cc)
    {
    case N:
        return (getPSR() >> 31) & 1; // Negative flag
    case Z:
        return (getPSR() >> 30) & 1; // Zero flag
    case C:
        return (getPSR() >> 29) & 1; // Carry flag
    case V:
        return (getPSR() >> 28) & 1; // Overflow flag
    }
    return 0; // Default case, return 0 if flag is not handled
}

void setCC(Byte n, int z, int c, int v)
{
    Word curr_psr = getPSR(); // Get the current Program Status Register (PSR)

    if (n != CC_UNMOD)
        curr_psr = n ? (UINT32_C(1) << 31) | curr_psr : ~(UINT32_C(1) << 31) & curr_psr; // Set or clear the Negative flag
    if (z != CC_UNMOD)
        curr_psr = z ? (1 << 30) | curr_psr : ~(1 << 30) & curr_psr; // Set or clear the Zero flag
    if (c != CC_UNMOD)
        curr_psr = c ? (1 << 29) | curr_psr : ~(1 << 29) & curr_psr; // Set or clear the Carry flag
    if (v != CC_UNMOD)
        curr_psr = v ? (1 << 28) | curr_psr : ~(1 << 28) & curr_psr; // Set or clear the Overflow flag

    setPSR(curr_psr); // Update the Program Status Register (PSR)
}

bool evalCond(Byte opcode)
{
    switch (opcode)
    {
    case 0x0:
        return getCC(Z); // Equal (Z set)
    case 0x1:
        return !getCC(Z); // Not equal (Z clear)
    case 0x2:
        return getCC(C); // Carry set (C set)
    case 0x3:
        return !getCC(C); // Carry clear (C clear)
    case 0x4:
        return getCC(N); // Negative (N set)
    case 0x5:
        return !getCC(N); // Positive or zero (N clear)
    case 0x6:
        return getCC(V); // Overflow (V set)
    case 0x7:
        return !getCC(V); // No overflow (V clear)
    case 0x8:
        return getCC(C) & !getCC(Z); // Unsigned higher (C set and Z clear)
    case 0x9:
        return !getCC(C) | getCC(Z); // Unsigned lower or same (C clear or Z set)
    case 0xA:
        return getCC(N) == getCC(V); // Signed greater or equal (N equals V)
    case 0xB:
        return getCC(N) ^ getCC(V); // Signed less than (N not equal to V)
    case 0xC:
        return !getCC(Z) && (getCC(N) == getCC(V)); // Signed greater than (Z clear and N equals V)
    case 0xD:
        return getCC(Z) || (getCC(N) ^ getCC(V)); // Signed less or equal (Z set or N not equal to V)
    case 0xE:
        return true; // Always
    }
    return false; // Default case, return false if opcode is not handled
}

// Set or clear a specific flag in the CPSR based on a condition
static void cpuFlagSet(Word flag, bool cond)
{
    if (cond)
        cpu->cpsr |= flag; // Set the flag if the condition is true
    else
        cpu->cpsr &= ~flag; // Clear the flag if the condition is false
}

// Load banked registers into the general-purpose registers based on the CPU mode
static void bankToReg(int8_t mode)
{
    switch (mode)
    {

    case FIQ:
        cpu->regs[8] = cpu->regsFIQ[0];
        cpu->regs[9] = cpu->regsFIQ[1];
        cpu->regs[10] = cpu->regsFIQ[2];
        cpu->regs[11] = cpu->regsFIQ[3];
        cpu->regs[12] = cpu->regsFIQ[4];
        cpu->regs[13] = cpu->regsFIQ[5];
        cpu->regs[14] = cpu->regsFIQ[6];
        break;

    case IRQ:
        cpu->regs[13] = cpu->regsIRQ[0];
        cpu->regs[14] = cpu->regsIRQ[1];
        break;

    case SVC:
        cpu->regs[13] = cpu->regsSVC[0];
        cpu->regs[14] = cpu->regsSVC[1];
        break;

    case ABT:
        cpu->regs[13] = cpu->regsABT[0];
        cpu->regs[14] = cpu->regsABT[1];
        break;

    case UNDEF:
        cpu->regs[13] = cpu->regsUND[0];
        cpu->regs[14] = cpu->regsUND[1];
        break;
    }
}

// Save general-purpose registers into the banked registers based on the CPU mode
static void regToBank(int8_t mode)
{

    switch (mode)
    {

    case FIQ:
        cpu->regsFIQ[0] = cpu->regs[8];
        cpu->regsFIQ[1] = cpu->regs[9];
        cpu->regsFIQ[2] = cpu->regs[10];
        cpu->regsFIQ[3] = cpu->regs[11];
        cpu->regsFIQ[4] = cpu->regs[12];
        cpu->regsFIQ[5] = cpu->regs[13];
        cpu->regsFIQ[6] = cpu->regs[14];
        break;

    case IRQ:
        cpu->regsIRQ[0] = cpu->regs[13];
        cpu->regsIRQ[1] = cpu->regs[14];
        break;

    case SVC:
        cpu->regsSVC[0] = cpu->regs[13];
        cpu->regsSVC[1] = cpu->regs[14];
        break;

    case ABT:
        cpu->regsABT[0] = cpu->regs[13];
        cpu->regsABT[1] = cpu->regs[14];
        break;

    case UNDEF:
        cpu->regsUND[1] = cpu->regs[13];
        cpu->regsUND[2] = cpu->regs[14];
        break;
    }
}

// Test if a specific flag is set in the CPSR
static bool flagTST(Word flag)
{
    return (cpu->cpsr & flag) != 0;
}

bool cpuInThumb()
{
    return (cpu->cpsr & (1 << 5)) != 0;
}

// Set the CPU mode and update the banked registers
static void cpuModeSet(int8_t mode)
{
    int8_t curr = cpu->cpsr & 0x1f; // Get the current CPU mode

    cpu->cpsr = 0;     // Clear the CPSR
    cpu->cpsr |= mode; // Set the new CPU mode

    regToBank(curr); // Save the current general-purpose registers to the banked registers
    bankToReg(mode); // Load the banked registers for the new mode into the general-purpose registers
}

// Set the Saved Program Status Register (SPSR) based on the current CPU mode
static void setSPSR(Word spsr)
{
    switch (cpu->cpsr & 0x1f)
    {
    case FIQ:
        cpu->spsr_fiq = spsr;
        break;
    case IRQ:
        cpu->spsr_irq = spsr;
        break;
    case SVC:
        cpu->spsr_svc = spsr;
        break;
    case ABT:
        cpu->spsr_abt = spsr;
        break;
    case UNDEF:
        cpu->spsr_und = spsr;
        break;
    }
}

// Handle a CPU interrupt
void cpuInterrupt(Word address, int8_t mode)
{
    Word cpsr = cpu->cpsr;
    cpuModeSet(mode); // Set the CPU mode
    setSPSR(cpsr);    // Save the current CPSR to the SPSR

    // Set FIQ Disable flag based on exception
    if (address == ARM_VEC_FIQ || address == ARM_VEC_RESET)
        cpuFlagSet((1 << 6), true);

    /*
     * Adjust PC based on exception
     *
     * Exception      ARM Mode LR Adjustment    THUMB Mode LR Adjustment
     * ---------------------------------------------------------------
     * UND (Undefined Instruction)    PC - 4                 PC - 2
     * SVC (Software Interrupt)       PC - 4                 PC - 2
     * Other Exceptions (e.g., IRQ, FIQ)   PC - 4                 PC - 4
     */

    if (address == ARM_VEC_UND || address == ARM_VEC_SVC)
    {
        if (cpuInThumb())
            cpu->regs[14] = cpu->regs[15] - 2; // THUMB mode adjustment
        else
            cpu->regs[14] = cpu->regs[15] - 4; // ARM mode adjustment
    }
    else if (address != ARM_VEC_RESET)
    {
        // For other exceptions, adjust LR by -4 in both ARM and THUMB modes
        cpu->regs[14] = cpu->regs[15] - 4;
    }

    cpuFlagSet((1 << 5), false); // Clear the THUMB flag
    cpuFlagSet((1 << 7), true);  // Set the IRQ Disable flag

    cpu->regs[15] = address;            // Set the PC to the interrupt vector address
    cpu->pipeline = fetchInstruction(); // Fetch the next instruction
}

// Check for pending IRQs and handle them
void cpuCheckIRQ()
{
    if (!flagTST((1 << 7)) &&                       // Check if IRQs are enabled
        (mem->iwpdc.ie.full & 1) &&                 // Check if the master IRQ enable flag is set
        (mem->iwpdc.ie.full & mem->iwpdc.i_f.full)) // Check if any IRQs are pending
        cpuInterrupt(ARM_VEC_IRQ, IRQ);             // Handle the IRQ
}

// Reset the CPU
void cpuReset()
{
    cpuInterrupt(ARM_VEC_RESET, SVC); // Trigger a reset interrupt and switch to Supervisor mode
}

void startGBA(char *rom, char *bios)
{
    // Load BIOS and ROM into memory
    loadBios(bios);
    loadRom(rom);

    // Allocate and initialize memory for EEPROM, SRAM, and Flash
    eeprom = malloc(0x2000);
    memset(eeprom, 0, 0x2000);
    sram = malloc(0x10000);
    memset(sram, 0, 0x10000);
    flash = malloc(0x20000);
    memset(flash, 0, 0x20000);

    // Set the initial CPU mode to SYSTEM
    cpu->cpsr = 0;
    cpu->cpsr |= SYSTEM;

    // Initialize the instruction pipeline
    cpu->pipeline = 0xF0000000;

    // Set all keypad buttons to "released"
    mem->keypad.keyinput.full = 0x3FF;
    mem->iwpdc.waitcnt.full = 0;

    bool skipBios = true;

    if (skipBios)
    {
        // Initialize stack pointers for different modes
        cpu->regsSVC[0] = 0x03007FE0;
        cpu->regsIRQ[0] = 0x03007FA0;
        cpu->regs[13] = 0x03007F00;

        // Initialize PC and default mode
        cpu->regs[15] = 0x08000000;
        mem->iwpdc.postflag.bits.flag = 1;
        mem->comm.rcnt.full = 0x8000;
        mem->biosBus = 0xe129f000;
        cpu->pipeline = 0;
    }
    else
    {
        cpuReset(); // Reset the CPU if not skipping BIOS
    }

    updateWait(); // Update memory wait states
}

Word fetchInstruction()
{
    Word instr;
    if (cpuInThumb())
    {
        // Fetch a 16-bit instruction in THUMB mode
        instr = memReadHalfWord(cpu->regs[15]);
        cpu->regs[15] += 2;
    }
    else
    {
        // Fetch a 32-bit instruction in ARM mode
        instr = memReadWord(cpu->regs[15]);
        cpu->regs[15] += 4;
    }
    return instr;
}

static int decodeInstruction(Word instr)
{
    cpu->pipeline = fetchInstruction(); // Fetch the next instruction

    if (cpuInThumb())
    {
        // Decode THUMB instructions
        if (thumbIsSWI(instr))
        {
            return TSWI;
        }
        else if (thumbIsUB(instr))
        {
            return UB;
        }
        else if (thumbIsCB(instr))
        {
            return CB;
        }
        else if (thumbIsMLS(instr))
        {
            return MLS;
        }
        else if (thumbIsLBL(instr))
        {
            return LBL;
        }
        else if (thumbIsAOSP(instr))
        {
            return AOSP;
        }
        else if (thumbIsPPR(instr))
        {
            return PPR;
        }
        else if (thumbIsLSH(instr))
        {
            return LSH;
        }
        else if (thumbIsSPRLS(instr))
        {
            return SPRLS;
        }
        else if (thumbIsLA(instr))
        {
            return LA;
        }
        else if (thumbIsLSIO(instr))
        {
            return LSIO;
        }
        else if (thumbIsLSRO(instr))
        {
            return LSRO;
        }
        else if (thumbIsLSSEBH(instr))
        {
            return LSSEBH;
        }
        else if (thumbIsPCRL(instr))
        {
            return PCRL;
        }
        else if (thumbIsHROBX(instr))
        {
            return HROBX;
        }
        else if (thumbIsALU(instr))
        {
            return ALU;
        }
        else if (thumbIsMCASI(instr))
        {
            return MCASI;
        }
        else if (thumbIsAS(instr))
        {
            return AS;
        }
        else if (thumbIsMSR(instr))
        {
            return MSR;
        }
        else
        {
            fprintf(stderr, "DECODE ERROR\n\tMode: THUMB\n\tInstr: %08X\n\tType: UNIMPLEMENTED\n", instr);
            exit(-9);
        }
    }
    else
    {
        // Decode ARM instructions
        if (armIsBX(instr))
        {
            return BX;
        }
        else if (armIsBDT(instr))
        {
            return BDT;
        }
        else if (armIsBL(instr))
        {
            return BL;
        }
        else if (armIsSWI(instr))
        {
            return SWI;
        }
        else if (armIsUND(instr))
        {
            return UND;
        }
        else if (armIsSDT(instr))
        {
            return SDT;
        }
        else if (armIsSDS(instr))
        {
            return SDS;
        }
        else if (armIsMUL(instr) || armIsMULL(instr))
        {
            return MUL;
        }
        else if (armIsHDTRI(instr))
        {
            return HDTRI;
        }
        else if (armIsPSRT(instr))
        {
            return PSRT;
        }
        else if (armIsDPROC(instr))
        {
            return DPROC;
        }
        else
        {
            fprintf(stderr, "DECODE ERROR\n\tMode: ARM\n\tInstr: %08X\n\tType: UNIMPLEMENTED\n", instr);
            exit(-9);
        }
    }
}

static int execute(void)
{
    Word instr = cpu->pipeline ? cpu->pipeline : fetchInstruction();
    int type = decodeInstruction(instr);
    int cyclesStart = cpu->cycle;

    if (THUMB_ACTIVATED)
    {
        switch (type)
        {
        case BX:
        case BDT:
        case BL:
        case SWI:
        case UND:
        case SDT:
        case SDS:
        case MUL:
        case HDTRI:
        case PSRT:
        case DPROC:
            fprintf(stderr, "EXECUTE ERROR\n\tMode: THUMB\n\tInstr: %08X\n\tType: %d\n", instr, type);
            exit(-10);
        case TSWI:
            procTSWI(instr);
            break;
        case UB:
            procTUB(instr);
            break;
        case CB:
            procTCB(instr);
            break;
        case MLS:
            procTMLS(instr);
            break;
        case LBL:
            procTLBL(instr);
            break;
        case AOSP:
            procTAOSP(instr);
            break;
        case PPR:
            procTPPR(instr);
            break;
        case LSH:
            procTLSH(instr);
            break;
        case SPRLS:
            procTSPRLS(instr);
            break;
        case LA:
            procTLA(instr);
            break;
        case LSIO:
            procTLSIO(instr);
            break;
        case LSRO:
            procTLSRO(instr);
            break;
        case LSSEBH:
            procTLSSEBH(instr);
            break;
        case PCRL:
            procTPCRL(instr);
            break;
        case HROBX:
            procTHROBX(instr);
            break;
        case ALU:
            procTALU(instr);
            break;
        case MCASI:
            procTMCASI(instr);
            break;
        case AS:
            procTAS(instr);
            break;
        case MSR:
            procTMSR(instr);
            break;
        default:
            fprintf(stderr, "EXECUTE ERROR\n\tMode: THUMB\n\tInstr: %08X\n\tType: NO MATCH\n", instr);
            exit(-10);
        }
    }
    else
    {
        if (evalCond(INSTR_COND_FIELD(instr)))
        {
            switch (type)
            {
            case BX:
                procBX(instr);
                break;
            case BDT:
                procBDT(instr);
                break;
            case BL:
                procBL(instr);
                break;
            case SWI:
                procSWI(instr);
                break;
            case UND:
                procUND(instr);
                break;
            case SDT:
                procSDT(instr);
                break;
            case SDS:
                procSDS(instr);
                break;
            case MUL:
                procMUL(instr);
                break;
            case HDTRI:
                procHDTRI(instr);
                break;
            case PSRT:
                procPSRT(instr);
                break;
            case DPROC:
                procDPROC(instr);
                break;
            case TSWI:
            case UB:
            case CB:
            case MLS:
            case LBL:
            case AOSP:
            case PPR:
            case LSH:
            case SPRLS:
            case LA:
            case LSIO:
            case LSRO:
            case LSSEBH:
            case PCRL:
            case HROBX:
            case ALU:
            case MCASI:
            case AS:
            case MSR:
                fprintf(stderr, "EXECUTE ERROR\n\tMode: ARM\n\tInstr: %08X\n\tType: %d\n", instr, type);
                exit(-10);
            default:
                fprintf(stderr, "EXECUTE ERROR\n\tMode: ARM\n\tInstr: %08X\n\tType: NO MATCH\n", instr);
                exit(-10);
            }
        }
        else
        {
            cpu->cycle += 1;
        }
    }
    return (cpu->cycle - cyclesStart);
}

void executeInput(Word cycles)
{
    int totalCycles = 0;
    while (totalCycles < cycles)
    {
        int cyclesPassed = execute(); // Execute an instruction and get the number of cycles it took
        if (timerENB)
        {
            updateTimer(cyclesPassed); // Update the timers if they are enabled
        }
        totalCycles += cyclesPassed; // Accumulate the total number of cycles
    }
}