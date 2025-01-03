/****************************************************************************************************
 *
 * @file:    armProc.c
 * @author:  Nolan Olhausen
 * @date: 2024-09-8
 *
 * @brief:
 *      Implementation of ARM mode instructions.
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

#include "cpu.h"
#include "common.h"
#include "memory.h"
#include "armInstructions.h"

#define CC_UNMOD 2                                                         // Condition code for unmodified instructions
#define THUMB_ACTIVATED (cpu->cpsr >> 5 & 1)                               // Check if THUMB mode is activated
#define PC_VALUE (THUMB_ACTIVATED ? cpu->regs[15] + 2 : cpu->regs[15] + 4) // Get the current Program Counter value
#define PC_UPDATE(new_pc)   \
    cpu->regs[15] = new_pc; \
    cpu->pipeline = 0; // Update the Program Counter and reset the instruction pipeline

#define ROR(operand, shiftAmount) (((operand) >> ((shiftAmount) & 31)) | ((operand) << ((-(shiftAmount)) & 31))) // Rotate right operation
#define ROT_READ_SHIFT_AMOUNT(addr) (((addr) & 0x3) * 8)                                                         // Calculate the shift amount for rotation based on address

Word barrelShifter(ShiftType shiftType, Word operand2, size_t shift, bool regShiftByImm)
{
    if (!regShiftByImm && (shift == 0))
    {
        cpu->carry = CC_UNMOD;
        return operand2;
    }

    switch (shiftType)
    {
    case SHIFT_TYPE_LSL: // Logical Shift Left
        switch (shift)
        {
        case 0:
            cpu->carry = CC_UNMOD;
            break;
        case 32:
            cpu->carry = operand2 & 1;
            operand2 = 0;
            break;
        default:
            cpu->carry = shift > 32 ? 0 : (operand2 << (shift - 1)) >> 31;
            operand2 = shift > 32 ? 0 : operand2 << shift;
        }
        break;
    case SHIFT_TYPE_LSR: // Logical Shift Right
        switch (shift)
        {
        case 0:
            if (regShiftByImm)
            {
                cpu->carry = operand2 >> 31;
                operand2 = 0;
            }
            break;
        case 32:
            cpu->carry = operand2 >> 31;
            operand2 = 0;
            break;
        default:
            cpu->carry = shift > 32 ? 0 : (operand2 >> (shift - 1)) & 1;
            operand2 = shift > 32 ? 0 : operand2 >> shift;
        }
        break;
    case SHIFT_TYPE_ASR: // Arithmetic Shift Right
        if (regShiftByImm && (shift == 0))
        {
            Bit msb = operand2 >> 31;
            cpu->carry = msb;
            operand2 = msb ? ~0 : 0;
            break;
        }

        cpu->carry = shift > 31 ? operand2 >> 31 : ((int32_t)operand2 >> (shift - 1)) & 1;
        operand2 = shift > 31 ? (operand2 >> 31 ? ~0 : 0) : (int32_t)operand2 >> shift;
        break;
    case SHIFT_TYPE_ROR: // Rotate Right
        if (regShiftByImm && (shift == 0))
        {
            cpu->carry = operand2 & 1;
            operand2 = ((Word)((cpu->cpsr >> 29) & 1) << 31) | (operand2 >> 1);
            break;
        }

        operand2 = (operand2 >> (shift & 31)) | (operand2 << ((-shift) & 31));
        cpu->carry = operand2 >> 31;
        break;
    default:
        fprintf(stderr, "CPU Error: invalid shift opcode\n");
        exit(1);
    }

    return operand2;
}

void procBX(Word instr)
{
    Byte rn = instr & 0xF;      // Extract the register number
    Word rnVal = getReg(rn);    // Get the value of the register
    Word oldPC = cpu->regs[15]; // Save the old Program Counter value

    if (rnVal & 1)
    {
        cpu->cpsr |= 0x20;                       // Set THUMB mode
        cpu->regs[15] = PC_UPDATE(rnVal & ~0x1); // Update the Program Counter and reset the pipeline
    }
    else
    {
        cpu->cpsr &= ~0x20;                      // Clear THUMB mode
        cpu->regs[15] = PC_UPDATE(rnVal & ~0x3); // Update the Program Counter and reset the pipeline
    }

    cpu->cycle += 3; // Increment the cycle count by 3
}

// Custom population count function to count the number of set bits in an integer
unsigned int customPopcount(unsigned int x)
{
    unsigned int count = 0;
    while (x)
    {
        x &= (x - 1); // Clear the least significant bit set
        count++;
    }
    return count;
}

// Custom find first set (FFS) function to find the position of the first set bit in an integer
int customFFS(int x)
{
    if (x == 0)
        return 0; // If x is 0, return 0 (no bits are set)

    int position = 1; // Start from the least significant bit (1-based index)

    // Loop until we find the first set bit
    while ((x & 1) == 0)
    {
        x >>= 1;    // Shift right to check the next bit
        position++; // Increment the bit position
    }

    return position; // Return the 1-based index of the first set bit
}

void procBDT(Word instr)
{
    Bit p = (instr >> 24) & 1; // Pre/Post indexing bit
    Bit u = (instr >> 23) & 1; // Up/Down bit
    Bit s = (instr >> 22) & 1; // S bit (PSR & force user bit)
    Bit w = (instr >> 21) & 1; // Write-back bit
    Bit l = (instr >> 20) & 1; // Load/Store bit

    Byte rn = (instr >> 16) & 0xF;     // Base register
    HalfWord regList = instr & 0xFFFF; // Register list

    Word bankTransfer = 0;
    bool r15Transferred = (regList >> 0xF) & 1; // Check if R15 (PC) is in the register list

    if (s)
    {
        if (l && r15Transferred)
        {
            cpu->cpsr = getPSR(); // Load PSR if LDM and R15 is transferred
        }
        else
        {
            bankTransfer = cpu->cpsr;
            cpu->cpsr = (cpu->cpsr & ~0xFF) | USER; // Force user mode
        }
    }

    int totalTransfers = customPopcount(regList); // Count the number of registers to transfer
    bool emptyRegList = totalTransfers == 0;

    Word baseAddr = getReg(rn);       // Get the base address from the base register
    Word baseAddrOffset = u ? 4 : -4; // Calculate the address offset

    if (emptyRegList)
    {
        regList |= (1 << 0xF);                        // If the register list is empty, transfer R15 (PC)
        setReg(rn, baseAddr + (16 * baseAddrOffset)); // Update the base register
        r15Transferred = true;
    }
    else if (w)
    {
        setReg(rn, baseAddr + (totalTransfers * baseAddrOffset)); // Write-back the base register
    }

    Byte firstTransferredReg = customFFS(regList) - 1; // Find the first register to transfer

    Word baseAddrCopy = baseAddr; // Copy the base address

    int regStart, regEnd, step;
    if (u ^ emptyRegList)
    {
        regStart = 0;
        regEnd = 0x10;
        step = 1;
    }
    else
    {
        regStart = 0xF;
        regEnd = -1;
        step = -1;
    }

    for (int reg = regStart; reg != regEnd; reg += step)
    {
        bool shouldTransfer = (regList >> reg) & 1; // Check if the register should be transferred

        if (shouldTransfer)
        {
            Word transferAddr = p ? baseAddr + baseAddrOffset : baseAddr; // Calculate the transfer address

            if (l)
            {
                setReg(reg, memReadWord(transferAddr)); // Load from memory (LDM)
            }
            else
            {
                Word storedVal = reg == 0xF ? PC_VALUE : getReg(reg);
                memWriteWord(transferAddr, (reg == rn && rn == firstTransferredReg) ? baseAddrCopy : storedVal); // Store to memory (STM)
            }

            baseAddr += baseAddrOffset; // Update the base address
        }

        if (emptyRegList)
            baseAddr += baseAddrOffset; // Update the base address if the register list is empty
    }

    if (bankTransfer)
        cpu->cpsr = bankTransfer; // Restore the CPSR if needed

    if (l)
    {
        cpu->cycle += (totalTransfers + r15Transferred) + (1 + r15Transferred) + 1; // Increment the cycle count for LDM
    }
    else
    {
        cpu->cycle += (totalTransfers - 1) + 2; // Increment the cycle count for STM
    }
}

void procBL(Word instr)
{
    Bit L = (instr >> 24) & 1;                                             // Link bit
    int32_t offset = (Word)((int32_t)((instr & 0xFFFFFF) << 8) >> 8) << 2; // Sign-extend and shift the offset

    if (THUMB_ACTIVATED)
        offset >>= 1; // Adjust the offset for THUMB mode

    if (L)
        setReg(0xE, cpu->regs[15] - 4); // Save the return address in the link register (R14)

    cpu->regs[15] = PC_UPDATE(cpu->regs[15] + offset); // Update the Program Counter with the new address

    cpu->cycle += 3; // Increment the cycle count by 3
}

void procSWI(Word instr)
{
    cpu->regsSVC[1] = cpu->regs[15] - 4;   // Save the return address in the Supervisor mode link register
    cpu->spsr_svc = cpu->cpsr;             // Save the current Program Status Register (CPSR) to the Supervisor mode SPSR
    updateCPUMode(SVC);                    // Switch to Supervisor mode
    cpu->regs[15] = PC_UPDATE(0x00000008); // Set the Program Counter to the SWI vector address and reset the pipeline
    cpu->cycle += 3;                       // Increment the cycle count by 3
}

void procUND(Word instr)
{
    cpu->cycle += 1; // Increment the cycle count by 1
}

void procSDT(Word instr)
{
    Bit i = (instr >> 25) & 1; // Immediate offset bit
    Bit p = (instr >> 24) & 1; // Pre/Post indexing bit
    Bit u = (instr >> 23) & 1; // Up/Down bit
    Bit b = (instr >> 22) & 1; // Byte/Word bit
    Bit t = (instr >> 21) & 1; // Translate bit
    Bit l = (instr >> 20) & 1; // Load/Store bit

    Byte rn = (instr >> 16) & 0xF; // Base register
    Byte rd = (instr >> 12) & 0xF; // Destination register
    Byte rm = instr & 0xF;         // Offset register

    Byte shiftAmount = (instr >> 7) & 0x1F; // Shift amount
    Word offset = i ? barrelShifter((instr >> 5) & 0x3, getReg(instr & 0xF), shiftAmount, true)
                    : instr & 0xFFF; // Calculate the offset
    if (!u)
        offset = -offset; // Adjust the offset for down direction

    Word addr = getReg(rn) + (p ? offset : 0); // Calculate the address
    bool shouldWriteBack = !p || (p && t);     // Determine if write-back is needed

    if (p && b && l && !t && (rd == 0xF) && (((instr >> 28) & 0xF) == 0xF))
    {
        printf("PLD INSTRUCTION!\n");
        exit(1);
    }

    if (!p && t)
    {
        printf("memory manage bit is set\n");
        exit(1);
    }

    if (l)
    {
        // Load from memory
        Word readVal = b ? memReadByte(addr)
                         : ROR(memReadWord(addr), ROT_READ_SHIFT_AMOUNT(addr));
        setReg(rd, readVal);
    }
    else
    {
        // Store to memory
        Word storedVal = getReg(rd) + ((rd == 0xF) << 2);
        if (b)
        {
            memWriteByte(addr, storedVal);
        }
        else
        {
            memWriteWord(addr, storedVal);
        }
    }

    if (shouldWriteBack)
        if (!l || !(rn == rd))
            setReg(rn, getReg(rn) + ((rn == 0xF) << 2) + offset); // Write-back the base register

    bool r15Transferred = rd == 0xF; // Check if R15 (PC) is transferred

    if (l)
    {
        cpu->cycle += 3 + (2 * r15Transferred); // Increment the cycle count for load
    }
    else
    {
        cpu->cycle += 2; // Increment the cycle count for store
    }
}

void procSDS(Word instr)
{
    Bit b = (instr >> 22) & 1;     // Byte/Word bit
    Byte rn = (instr >> 16) & 0xF; // Base register
    Byte rd = (instr >> 12) & 0xF; // Destination register
    Byte rm = instr & 0xF;         // Source register
    Word addr = getReg(rn);        // Get the address from the base register

    if (b)
    {
        // Byte swap
        Word tempVal = memReadByte(addr); // Read the byte from memory
        memWriteByte(addr, getReg(rm));   // Write the byte to memory
        setReg(rd, tempVal);              // Set the destination register with the read value
    }
    else
    {
        // Word swap
        Word tempVal = ROR(memReadWord(addr), ROT_READ_SHIFT_AMOUNT(addr)); // Read the word from memory and rotate if needed
        memWriteWord(getReg(rn), getReg(rm));                               // Write the word to memory
        setReg(rd, tempVal);                                                // Set the destination register with the read value
    }
    cpu->cycle += 4; // Increment the cycle count by 4
}

// Custom Count Leading Zeros (CLZ) function to count the number of leading zeros in an integer
unsigned int customCLZ(unsigned int x)
{
    if (x == 0)
        return sizeof(x) * 8; // If x is 0, return the total number of bits in x

    unsigned int count = 0;
    unsigned int total_bits = sizeof(x) * 8;

    for (unsigned int shift = total_bits >> 1; shift > 0; shift >>= 1)
    {
        if ((x >> shift) == 0)
        {
            count += shift;
            x <<= shift;
        }
    }

    return count;
}

void procMUL(Word instr)
{
    Bit s = (instr >> 20) & 1;     // Set condition codes bit
    Byte rd = (instr >> 16) & 0xF; // Destination register
    Byte rn = (instr >> 12) & 0xF; // Accumulate register
    Byte rs = (instr >> 8) & 0xF;  // Source register
    Byte rm = instr & 0xF;         // Multiplier register

    Word rsVal = getReg(rs); // Get the value of the source register

    // Calculate the number of leading zeroes
    Word leadingZeroes = rsVal ^ ((int32_t)rsVal >> 31);
    int m = leadingZeroes == 0 ? 4 : 4 - (((customCLZ(leadingZeroes) + 0x7) & ~0x7) >> 3);
    if (m == 0)
        m = 4;

    switch ((instr >> 21) & 0xF)
    {
    case 0x0:
    {
        // MUL
        Word result = getReg(rm) * getReg(rs);
        if (s)
            setCC(result >> 31, result == 0, CC_UNMOD, CC_UNMOD);
        setReg(rd, result);
        cpu->cycle += 1 + m;
        break;
    }
    case 0x1:
    {
        // MLA (Multiply Accumulate)
        Word result = getReg(rm) * getReg(rs) + getReg(rn);
        if (s)
            setCC(result >> 31, result == 0, CC_UNMOD, CC_UNMOD);
        setReg(rd, result);
        cpu->cycle += 2 + m;
        break;
    }
    case 0x2:
        fprintf(stderr, "multiply opcode not implemented yet: %04X\n", (instr >> 21) & 0xF);
        exit(1);
        break;
    case 0x4:
    {
        // UMULL (Unsigned Multiply Long)
        DWord result = (DWord)getReg(rm) * (DWord)getReg(rs);
        if (s)
            setCC(result >> 63, result == 0, CC_UNMOD, CC_UNMOD);
        setReg(rn, result & ~0);
        setReg(rd, (result >> 32) & ~0);
        cpu->cycle += 2 + m;
        break;
    }
    case 0x5:
    {
        // UMLAL (Unsigned Multiply Accumulate Long)
        DWord result = (DWord)getReg(rm) * (DWord)getReg(rs) + (((DWord)getReg(rd) << (DWord)32) | (DWord)getReg(rn));
        if (s)
            setCC(result >> 63, result == 0, CC_UNMOD, CC_UNMOD);
        setReg(rn, result & ~0);
        setReg(rd, (result >> 32) & ~0);
        cpu->cycle += 3 + m;
        break;
    }
    case 0x6:
    {
        // SMULL (Signed Multiply Long)
        int64_t result = (int64_t)(int32_t)getReg(rm) * (int64_t)(int32_t)getReg(rs);
        if (s)
            setCC(result >> 63, result == 0, CC_UNMOD, CC_UNMOD);
        setReg(rn, result & ~0);
        setReg(rd, (result >> 32) & ~0);
        cpu->cycle += 2 + m;
        break;
    }
    case 0x7:
    {
        // SMLAL (Signed Multiply Accumulate Long)
        int64_t result = (int64_t)(int32_t)getReg(rm) * (int64_t)(int32_t)getReg(rs) + ((int64_t)((DWord)getReg(rd) << (DWord)32) | (int64_t)getReg(rn));
        if (s)
            setCC(result >> 63, result == 0, CC_UNMOD, CC_UNMOD);
        setReg(rn, result & ~0);
        setReg(rd, (result >> 32) & ~0);
        cpu->cycle += 3 + m;
        break;
    }
    default:
        fprintf(stderr, "CPU Error: invalid opcode\n");
        exit(1);
        break;
    }
}

void procHDTRI(Word instr)
{
    Bit p = (instr >> 24) & 1; // Pre/Post indexing bit
    Bit u = (instr >> 23) & 1; // Up/Down bit
    Bit i = (instr >> 22) & 1; // Immediate offset bit
    Bit w = (instr >> 21) & 1; // Write-back bit
    Bit l = (instr >> 20) & 1; // Load/Store bit

    Byte rn = (instr >> 16) & 0xF; // Base register
    Byte rd = (instr >> 12) & 0xF; // Destination register

    int32_t offset = i ? ((((instr >> 8) & 0xF) << 4) | (instr & 0xF)) : getReg(instr & 0xF); // Calculate the offset
    if (!u)
        offset = -offset; // Adjust the offset for down direction

    Word addr = getReg(rn) + (p ? offset : 0); // Calculate the address
    bool shouldWriteBack = (p && w) || !p;     // Determine if write-back is needed

    if (l)
    { // Load
        switch ((instr >> 5) & 0x3)
        {
        case 0x1:
        {
            // Load halfword
            bool isMisaligned = addr & 1;
            Word readVal = isMisaligned ? ROR((Word)memReadHalfWord(addr - 1), 8)
                                        : memReadHalfWord(addr);
            setReg(rd, readVal);
            break;
        }
        case 0x2:
            // Load signed byte
            setReg(rd, (int32_t)(int8_t)memReadByte(addr));
            break;
        case 0x3:
        {
            // Load signed halfword
            bool isMisaligned = addr & 1;
            Word readVal = isMisaligned ? (int32_t)(int8_t)memReadByte(addr)
                                        : (int32_t)(int16_t)memReadHalfWord(addr);
            setReg(rd, readVal);
            break;
        }
        }
    }
    else
    { // Store
        switch ((instr >> 5) & 0x3)
        {
        case 0x1:
            // Store halfword
            memWriteHalfWord(addr, getReg(rd));
            break;
        case 0x2:
            // Store doubleword (not implemented)
            printf("IMPL LDRD");
            exit(1);
            break;
        case 0x3:
            // Store doubleword (not implemented)
            printf("IMPL STRD");
            exit(1);
            break;
        }
    }

    if (shouldWriteBack)
        if (!l || !(rn == rd))
            setReg(rn, getReg(rn) + ((rn == 0xF) << 2) + offset); // Write-back the base register

    bool r15Transferred = rd == 0xF; // Check if R15 (PC) is transferred

    if (l)
    {
        cpu->cycle += 3 + (2 * r15Transferred); // Increment the cycle count for load
    }
    else
    {
        cpu->cycle += 2; // Increment the cycle count for store
    }
}

void procPSRT(Word instr)
{
    Bit psr = (instr >> 22) & 0x1;         // PSR bit
    Bit I = (instr >> 25) & 0x1;           // Immediate bit
    Bit msr = (instr >> 21) & 0x1;         // MSR bit
    Bit f = (instr >> 19) & 1;             // Flag bit
    Bit bitsOnly = (instr >> 16) & 0x1;    // Bits only bit
    Byte Rd = (instr >> 12) & 0xf;         // Destination register
    Byte Rm = (instr & 0xf);               // Source register
    Word shift = ((instr >> 8) & 0xF) * 2; // Shift amount
    Word value = instr & 0xFF;             // Immediate value
    if (shift)
    {
        value = ((value >> shift) | (value << (32 - shift))); // Rotate the immediate value
    }
    Word operand = I ? ROR(instr & 0xFF, ((instr >> 8) & 0xF) * 2)
                     : getReg(instr & 0xF); // Calculate the operand

    if (msr)
    {
        if (psr)
        {
            if (f)
                setPSR((getPSR() & 0x00FFFFFF) | (operand & 0xFF000000)); // Update the PSR flags
            if (bitsOnly)
                setPSR((getPSR() & 0xFFFFFF00) | (operand & 0x000000FF)); // Update the PSR bits
        }
        else
        {
            if (f)
                cpu->cpsr = (cpu->cpsr & 0x00FFFFFF) | (operand & 0xFF000000); // Update the CPSR flags
            if (bitsOnly)
                cpu->cpsr = (cpu->cpsr & 0xFFFFFF00) | (operand & 0x000000FF); // Update the CPSR bits
        }
    }
    else
    {
        if (psr)
        {
            setReg(Rd, getPSR()); // Transfer the PSR to the destination register
        }
        else
        {
            setReg(Rd, cpu->cpsr); // Transfer the CPSR to the destination register
        }
    }
    cpu->cycle += 1; // Increment the cycle count by 1
}

void procDPROC(Word instr)
{
    Bit i = (instr >> 25) & 1; // Immediate bit
    Bit s = (instr >> 20) & 1; // Set condition codes bit

    Byte rn = (instr >> 16) & 0xF; // First operand register
    Byte rd = (instr >> 12) & 0xF; // Destination register

    Word operand1 = getReg(rn); // Get the value of the first operand register
    Word operand2;

    bool regShift = false;
    bool r15Transferred = rd == 0xF; // Check if R15 (PC) is the destination register

    if (i)
    {
        // Immediate operand
        Byte shiftAmount = ((instr & 0xF00) >> 8) * 2;
        operand2 = barrelShifter(SHIFT_TYPE_ROR, instr & 0xFF, shiftAmount, false);
    }
    else
    {
        // Register operand
        Bit r = (instr >> 4) & 1;
        Byte shiftType = (instr >> 5) & 0x3;
        Byte rm = instr & 0xF;
        Word rmVal = getReg(rm);

        if (r)
        {
            if (rn == 0xF)
                operand1 = PC_VALUE;
            if (rm == 0xF)
                rmVal = PC_VALUE;
            Byte shiftAmount = getReg((instr >> 8) & 0xF) & 0xFF;
            operand2 = barrelShifter(shiftType, rmVal, shiftAmount, false);
            regShift = true;
        }
        else
        {
            Byte shiftAmount = (instr >> 7) & 0x1F;
            operand2 = barrelShifter(shiftType, rmVal, shiftAmount, true);
        }
    }

    switch ((instr >> 21) & 0xF)
    {
    case 0x0:
    {
        // AND
        Word result = operand1 & operand2;
        if (s)
            setCC(result >> 31, result == 0, cpu->carry, CC_UNMOD);
        setReg(rd, result);
        break;
    }
    case 0x1:
    {
        // EOR
        Word result = operand1 ^ operand2;
        if (s)
            setCC(result >> 31, result == 0, cpu->carry, CC_UNMOD);
        setReg(rd, result);
        break;
    }
    case 0x3:
    {
        // SUB (swap operands)
        Word temp = operand1;
        operand1 = operand2;
        operand2 = temp;
    }
    case 0x2:
    {
        // SUB
        Word result = operand1 - operand2;
        if (s)
            setCC(result >> 31, result == 0, operand1 >= operand2, ((operand1 >> 31) != (operand2 >> 31)) && ((operand1 >> 31) != (result >> 31)));
        setReg(rd, result);
        break;
    }
    case 0x4:
    {
        // ADD
        Word result = operand1 + operand2;
        if (s)
            setCC(result >> 31, result == 0, ((operand1 >> 31) + (operand2 >> 31) > (result >> 31)), ((operand1 >> 31) == (operand2 >> 31)) && ((operand1 >> 31) != (result >> 31)));
        setReg(rd, result);
        break;
    }
    case 0x5:
    {
        // ADC (Add with Carry)
        Word result = operand1 + operand2 + getCC(C);
        if (s)
            setCC(result >> 31, result == 0, ((operand1 >> 31) + (operand2 >> 31) > (result >> 31)), ((operand1 >> 31) == (operand2 >> 31)) && ((operand1 >> 31) != (result >> 31)));
        setReg(rd, result);
        break;
    }
    case 0x7:
    {
        // SBC (swap operands)
        Word temp = operand1;
        operand1 = operand2;
        operand2 = temp;
    }
    case 0x6:
    {
        // SBC (Subtract with Carry)
        Word result = operand1 - operand2 - !getCC(C);
        if (s)
            setCC(result >> 31, result == 0, (DWord)operand1 >= ((DWord)operand2 + !getCC(C)), ((operand1 >> 31) != (operand2 >> 31)) && ((operand1 >> 31) != (result >> 31)));
        setReg(rd, result);
        break;
    }
    case 0x8:
    {
        // TST (Test)
        Word result = operand1 & operand2;
        setCC(result >> 31, result == 0, cpu->carry, CC_UNMOD);
        break;
    }
    case 0x9:
    {
        // TEQ (Test Equivalence)
        Word result = operand1 ^ operand2;
        setCC(result >> 31, result == 0, cpu->carry, CC_UNMOD);
        break;
    }
    case 0xA:
    {
        // CMP (Compare)
        Word result = operand1 - operand2;
        setCC(result >> 31, result == 0, operand1 >= operand2, ((operand1 >> 31) != (operand2 >> 31)) && ((operand1 >> 31) != (result >> 31)));
        break;
    }
    case 0xB:
    {
        // CMN (Compare Negative)
        Word result = operand1 + operand2;
        setCC(result >> 31, result == 0, ((operand1 >> 31) + (operand2 >> 31) > (result >> 31)), ((operand1 >> 31) == (operand2 >> 31)) && ((operand1 >> 31) != (result >> 31)));
        break;
    }
    case 0xC:
    {
        // ORR (Logical OR)
        Word result = operand1 | operand2;
        if (s)
            setCC(result >> 31, result == 0, cpu->carry, CC_UNMOD);
        setReg(rd, result);
        break;
    }
    case 0xF:
        // MVN (Move Not)
        operand2 = ~operand2;
    case 0xD:
        // MOV (Move)
        if (s)
            setCC(operand2 >> 31, operand2 == 0, cpu->carry, CC_UNMOD);
        setReg(rd, operand2);
        break;
    case 0xE:
    {
        // BIC (Bit Clear)
        Word result = operand1 & ~operand2;
        if (s)
            setCC(result >> 31, result == 0, cpu->carry, CC_UNMOD);
        setReg(rd, result);
        break;
    }
    }

    if (s && r15Transferred)
    {
        cpu->cpsr = getPSR(); // Update the CPSR if R15 (PC) is transferred
    }

    cpu->cycle += (1 + r15Transferred) + regShift + r15Transferred; // Increment the cycle count
}