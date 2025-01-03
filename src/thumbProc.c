/****************************************************************************************************
 *
 * @file:    thumbProc.c
 * @author:  Nolan Olhausen
 * @date: 2024-09-8
 *
 * @brief:
 *      Implementation of THUMB mode instructions.
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
#include "thumbInstructions.h"

// Macro to update the Program Counter (PC) and reset the instruction pipeline
#define PC_UPDATE(new_pc)   \
    cpu->regs[15] = new_pc; \
    cpu->pipeline = 0;

// Condition code for unmodified instructions
#define CC_UNMOD 2

// Macro to perform a rotate right (ROR) operation
#define ROR(operand, shiftAmount) (((operand) >> ((shiftAmount) & 31)) | ((operand) << ((-(shiftAmount)) & 31)))

void procTSWI(HalfWord instr)
{
    cpu->regsSVC[1] = cpu->regs[15] - 2;         // Save the return address in the Supervisor mode link register
    cpu->spsr_svc = cpu->cpsr;                   // Save the current Program Status Register (CPSR) to the Supervisor mode SPSR
    cpu->cpsr = (cpu->cpsr & 0xffffff00) + 0x93; // Set the CPSR to Supervisor mode with interrupts disabled
    cpu->regs[15] = PC_UPDATE(0x00000008); // Set the Program Counter to the SWI vector address and reset the pipeline
    cpu->cycle += 3;                       // Increment the cycle count by 3 (2S+1N)
}

void procTUB(HalfWord instr)
{
    int32_t offset = instr & 0x7ff; // Extract the offset from the instruction
    offset = offset << 1;           // Left shift the offset by 1
    if ((offset & 0x800) != 0)
    {
        offset = (int32_t)(offset | 0xfffff000); // Sign-extend the offset if necessary
    }
    cpu->regs[15] = PC_UPDATE(cpu->regs[15] + offset); // Update the Program Counter with the new address
    cpu->cycle += 3;                                   // Increment the cycle count by 3
}

void procTCB(HalfWord instr)
{
    Byte cond = (instr >> 8) & 0xf; // Extract the condition code from the instruction
    if (evalCond(cond))
    {                                                                             // Evaluate the condition code
        int32_t offset = (int32_t)((Word)((int32_t)(int8_t)(instr & 0xff)) << 1); // Extract and sign-extend the offset
        cpu->regs[15] = PC_UPDATE(cpu->regs[15] + offset);                        // Update the Program Counter with the new address
        cpu->cycle += 3;                                                          // Increment the cycle count by 3
    }
    else
    {
        cpu->cycle += 1; // Increment the cycle count by 1 if the condition is not met
    }
}

void procTMLS(HalfWord instr)
{
    Bit L = (instr >> 11) & 0x1;   // Load/Store bit
    Byte Rb = (instr >> 8) & 0x7;  // Base register
    Byte regList = (instr) & 0xff; // Register list
    Byte n = 0;
    Word addr = cpu->regs[Rb]; // Base address
    Word count = 0;

    if (L == 0x1)
    { // Load
        if (!regList)
        {
            setReg(15, memReadWord(cpu->regs[Rb])); // Load PC
            setReg(Rb, cpu->regs[Rb] + 0x40);       // Increment base register
            return;
        }

        // Calculate the total number of bytes to load
        for (int i = 0; i < 8; ++i)
        {
            if ((regList >> i) & 0x1)
            {
                count += 4;
            }
        }

        cpu->regs[Rb] += count; // Increment base register by the total byte count

        // Load registers from memory
        for (int i = 0; i < 8; i++)
        {
            if ((regList >> i) & 0x1)
            {
                setReg(i, memReadWord(addr));
                addr += 4;
                n += 1;
            }
        }
        cpu->cycle += (n + 2); // Increment cycle count
    }
    else
    { // Store
        if (!regList)
        {
            memWriteWord(cpu->regs[Rb], cpu->regs[15] + 2); // Store PC
            Word temp = cpu->regs[Rb];
            cpu->regs[Rb] += 0x40; // Increment base register
            return;
        }

        // Calculate the total number of bytes to store
        for (int i = 0; i < 8; ++i)
        {
            if ((regList >> i) & 0x1)
            {
                count += 4;
            }
        }

        bool first = true;

        // Store registers to memory
        for (int i = 0; i < 8; i++)
        {
            if ((regList >> i) & 0x1)
            {
                memWriteWord(addr, cpu->regs[i]);
                addr += 4;
                if (first)
                {
                    cpu->regs[Rb] += count; // Increment base register by the total byte count
                    first = false;
                }
                n += 1;
            }
        }
        cpu->cycle += (n + 1); // Increment cycle count
    }
}

void procTLBL(HalfWord instr)
{
    Bit H = (instr >> 11) & 0x1;     // High bit
    Word offset = ((instr) & 0x7ff); // Extract the offset from the instruction

    if (!H)
    {
        // First half of the long branch
        if ((offset & 0x400) != 0)
        {
            offset = ((int32_t)(offset | 0xFFFFF800)); // Sign-extend the offset if necessary
        }
        cpu->regs[14] = cpu->regs[15] + (int32_t)((Word)offset << 12); // Save the offset in the link register
        cpu->cycle += 1;                                               // Increment the cycle count by 1
    }
    else
    {
        // Second half of the long branch
        Word lr;

        lr = cpu->regs[14] + (offset << 1);      // Calculate the new address
        cpu->regs[14] = (cpu->regs[15] - 2) | 1; // Save the return address in the link register
        cpu->regs[15] = PC_UPDATE(lr);           // Update the Program Counter with the new address
        cpu->cycle += 3;                         // Increment the cycle count by 3
    }
}

void procTAOSP(HalfWord instr)
{
    Bit S = (instr >> 7) & 0x1; // Subtract bit
    Byte word = (instr) & 0x7f; // Offset value
    if (S == 0x1)
    {
        cpu->regs[13] = cpu->regs[13] - (word << 2); // Subtract offset from stack pointer
    }
    else
    {
        cpu->regs[13] = cpu->regs[13] + (word << 2); // Add offset to stack pointer
    }
    cpu->cycle += 1; // Increment the cycle count by 1
}

void procTPPR(HalfWord instr)
{
    Bit L = (instr >> 11) & 0x1;   // Load/Store bit
    Bit R = (instr >> 8) & 0x1;    // Register bit
    Byte regList = (instr) & 0xff; // Register list
    Byte n = 0;

    if (L == 0x1)
    { // Load
        for (int i = 0; i < 8; i++)
        {
            if ((regList >> i) & 0x1)
            {
                setReg(i, memReadWord(cpu->regs[13]));
                cpu->regs[13] = cpu->regs[13] + 4;
                n += 1;
            }
        }
        if (R == 0x1)
        {
            cpu->regs[15] = PC_UPDATE(memReadWord(cpu->regs[13]) & 0xfffffffe);
            cpu->regs[13] = cpu->regs[13] + 4;
            cpu->cycle += 3;
        }
        cpu->cycle += (n + 2);
    }
    else
    { // Store
        if (R == 0x1)
        {
            cpu->regs[13] = cpu->regs[13] - 4;
            memWriteWord(cpu->regs[13], cpu->regs[14]);
            n += 1;
        }
        for (int i = 7; i >= 0; i--)
        {
            if ((regList >> i) & 0x1)
            {
                cpu->regs[13] = cpu->regs[13] - 4;
                memWriteWord(cpu->regs[13], cpu->regs[i]);
                n += 1;
            }
        }
        cpu->cycle += (n + 1);
    }
}

void procTLSH(HalfWord instr)
{
    Bit L = (instr >> 11) & 0x1;                  // Load/Store bit
    Byte offset = (instr >> 6) & 0x1f;            // Offset value
    Byte Rb = (instr >> 3) & 0x7;                 // Base register
    Byte Rd = (instr) & 0x7;                      // Destination register
    Word address = cpu->regs[Rb] + (offset << 1); // Calculate the address

    if (L == 0x1)
    { // Load
        Word addr = (cpu->regs[Rb] + (offset << 1));
        Word rotate = (addr & 0b1) * 8;     // Determine if rotation is needed
        Word value = memReadHalfWord(addr); // Read the halfword from memory
        setReg(Rd, ROR(value, rotate));     // Rotate the value if needed and set the destination register
        cpu->cycle += 3;                    // Increment the cycle count by 3
        if (Rd == 15)
        {                    // If the destination register is the PC
            cpu->cycle += 2; // Increment the cycle count by an additional 2
        }
    }
    else
    {                                                                                        // Store
        memWriteHalfWord(cpu->regs[Rb] + (offset << 1), (HalfWord)(cpu->regs[Rd] & 0xffff)); // Write the halfword to memory
        cpu->cycle += 2;                                                                     // Increment the cycle count by 2
    }
}

void procTSPRLS(HalfWord instr)
{
    Bit L = (instr >> 11) & 0x1;    // Load/Store bit
    Byte Rd = (instr >> 8) & 0x7;   // Destination register
    HalfWord word = (instr) & 0xff; // Offset value
    if (L == 0x1)
    {                                              // Load
        Word addr = (cpu->regs[13] + (word << 2)); // Calculate the address
        Word rotate = (addr % 4) << 3;             // Determine if rotation is needed
        Word value = memReadWord(addr);            // Read the word from memory
        setReg(Rd, ROR(value, rotate));            // Rotate the value if needed and set the destination register
        cpu->cycle += 3;                           // Increment the cycle count by 3
        if (Rd == 15)
        {                    // If the destination register is the PC
            cpu->cycle += 2; // Increment the cycle count by an additional 2
        }
    }
    else
    {                                                             // Store
        memWriteWord(cpu->regs[13] + (word << 2), cpu->regs[Rd]); // Write the word to memory
        cpu->cycle += 2;                                          // Increment the cycle count by 2
    }
}

void procTLA(HalfWord instr)
{
    Bit SP = (instr >> 11) & 0x1;   // Stack Pointer bit
    Byte Rd = (instr >> 8) & 0x7;   // Destination register
    HalfWord word = (instr) & 0xff; // Offset value
    if (SP == 0x1)
    {
        setReg(Rd, cpu->regs[13] + (word << 2)); // Load address relative to the stack pointer
    }
    else
    {
        setReg(Rd, (cpu->regs[15] & 0xfffffffc) + (word << 2)); // Load address relative to the program counter
    }
    cpu->cycle += 1; // Increment the cycle count by 1
    if (Rd == 15)
    {
        cpu->cycle += 2; // Increment the cycle count by an additional 2 if the destination register is the PC
    }
}

void procTLSIO(HalfWord instr)
{
    Bit B = (instr >> 12) & 0x1;       // Byte/Word bit
    Bit L = (instr >> 11) & 0x1;       // Load/Store bit
    Byte offset = (instr >> 6) & 0x1f; // Offset value
    Byte Rb = (instr >> 3) & 0x7;      // Base register
    Byte Rd = (instr) & 0x7;           // Destination register
    if (L == 0x1)
    { // Load
        if (B == 0x1)
        {
            setReg(Rd, memReadByte(cpu->regs[Rb] + offset)); // Load byte from memory
        }
        else
        {
            Word addr = cpu->regs[Rb] + (offset << 2); // Calculate the address
            Word rotate = (addr % 4) << 3;             // Determine if rotation is needed
            Word value = memReadWord(addr);            // Read the word from memory
            setReg(Rd, ROR(value, rotate));            // Rotate the value if needed and set the destination register
        }
        cpu->cycle += 3; // Increment the cycle count by 3
        if (Rd == 15)
        {                    // If the destination register is the PC
            cpu->cycle += 2; // Increment the cycle count by an additional 2
        }
    }
    else
    { // Store
        if (B == 0x1)
        {
            memWriteByte(cpu->regs[Rb] + offset, cpu->regs[Rd]); // Store byte to memory
        }
        else
        {
            memWriteWord(cpu->regs[Rb] + (offset << 2), cpu->regs[Rd]); // Store word to memory
        }
        cpu->cycle += 2; // Increment the cycle count by 2
    }
}

void procTLSRO(HalfWord instr)
{
    Bit L = (instr >> 11) & 0x1;  // Load/Store bit
    Bit B = (instr >> 10) & 0x1;  // Byte/Word bit
    Byte Ro = (instr >> 6) & 0x7; // Offset register
    Byte Rb = (instr >> 3) & 0x7; // Base register
    Byte Rd = (instr) & 0x7;      // Destination register
    if (L == 0x1)
    { // Load
        if (B == 0x1)
        {
            setReg(Rd, memReadByte(cpu->regs[Ro] + cpu->regs[Rb])); // Load byte from memory
        }
        else
        {
            Word addr = (cpu->regs[Ro] + cpu->regs[Rb]); // Calculate the address
            Word rotate = (addr % 4) << 3;               // Determine if rotation is needed
            Word value = memReadWord(addr);              // Read the word from memory
            setReg(Rd, ROR(value, rotate));              // Rotate the value if needed and set the destination register
        }
        cpu->cycle += 3; // Increment the cycle count by 3
        if (Rd == 15)
        {                    // If the destination register is the PC
            cpu->cycle += 2; // Increment the cycle count by an additional 2
        }
    }
    else
    { // Store
        if (B == 0x1)
        {
            memWriteByte(cpu->regs[Ro] + cpu->regs[Rb], cpu->regs[Rd]); // Store byte to memory
        }
        else
        {
            memWriteWord(cpu->regs[Ro] + cpu->regs[Rb], cpu->regs[Rd]); // Store word to memory
        }
        cpu->cycle += 2; // Increment the cycle count by 2
    }
}

void procTLSSEBH(HalfWord instr)
{
    Bit H = (instr >> 11) & 0x1;               // Halfword bit
    Bit S = (instr >> 10) & 0x1;               // Sign-extend bit
    Byte Ro = (instr >> 6) & 0x7;              // Offset register
    Byte Rb = (instr >> 3) & 0x7;              // Base register
    Byte Rd = (instr) & 0x7;                   // Destination register
    Word addr = cpu->regs[Ro] + cpu->regs[Rb]; // Calculate the address

    switch ((S << 1) | H)
    {
    case 0:
        // Store halfword
        memWriteHalfWord(addr, cpu->regs[Rd]);
        cpu->cycle += 2; // Increment the cycle count by 2
        break;
    case 1:
        // Load halfword
        Word rotate = (addr & 0b1) * 8;     // Determine if rotation is needed
        Word value = memReadHalfWord(addr); // Read the halfword from memory
        cpu->regs[Rd] = ROR(value, rotate); // Rotate the value if needed and set the destination register
        cpu->cycle += 2;                    // Increment the cycle count by 2
        if (Rd == 15)
        {                    // If the destination register is the PC
            cpu->cycle += 2; // Increment the cycle count by an additional 2
        }
        break;
    case 2:
        // Load sign-extended byte
        cpu->regs[Rd] = (int32_t)(int8_t)memReadByte(addr); // Read the byte from memory and sign-extend it
        cpu->cycle += 2;                                    // Increment the cycle count by 2
        if (Rd == 15)
        {                    // If the destination register is the PC
            cpu->cycle += 2; // Increment the cycle count by an additional 2
        }
        break;
    case 3:
        // Load sign-extended halfword
        if (addr & 0x1)
        {
            cpu->regs[Rd] = (int32_t)(int8_t)memReadByte(addr); // Read the byte from memory and sign-extend it
        }
        else
        {
            Word rotate = (addr & 0b1) * 8;                                 // Determine if rotation is needed
            Word value = memReadHalfWord(addr);                             // Read the halfword from memory
            cpu->regs[Rd] = (int32_t)(int16_t)(uint16_t)ROR(value, rotate); // Rotate the value if needed and sign-extend it
        }
        cpu->cycle += 2; // Increment the cycle count by 2
        if (Rd == 15)
        {                    // If the destination register is the PC
            cpu->cycle += 2; // Increment the cycle count by an additional 2
        }
        break;
    }
}

void procTPCRL(HalfWord instr)
{
    Byte Rd = (instr >> 8) & 0x7;                                        // Destination register
    Byte word = instr & 0xff;                                            // Offset value
    setReg(Rd, memReadWord((cpu->regs[15] & 0xfffffffd) + (word << 2))); // Load the value from memory and set the destination register
    cpu->cycle += 3;                                                     // Increment the cycle count by 3
    if (Rd == 15)
    {                    // If the destination register is the PC
        cpu->cycle += 2; // Increment the cycle count by an additional 2
    }
}

void procTHROBX(HalfWord instr)
{
    Byte op = (instr >> 8) & 0x3;            // Operation code
    Bit H1 = (instr >> 7) & 0x1;             // High register bit 1
    Bit H2 = (instr >> 6) & 0x1;             // High register bit 2
    Byte Rs = ((instr >> 3) & 0x7) + H2 * 8; // Source register
    Byte Rd = ((instr) & 0x7) + H1 * 8;      // Destination register

    switch (op)
    {
    case 0:
        // Add
        setReg(Rd, getReg(Rs) + getReg(Rd));
        cpu->cycle += 1;
        break;
    case 1:
        // Compare
        setCC((cpu->regs[Rd] - cpu->regs[Rs]) >> 31, !(cpu->regs[Rd] - cpu->regs[Rs]), cpu->regs[Rd] >= cpu->regs[Rs], ((cpu->regs[Rd] >> 31) != (cpu->regs[Rs] >> 31)) && ((cpu->regs[Rd] >> 31) != ((cpu->regs[Rd] - cpu->regs[Rs]) >> 31)));
        cpu->cycle += 1;
        break;
    case 2:
        // Move
        if (Rd == 15)
        {
            setReg(Rd, cpu->regs[Rs]);
            cpu->cycle += 2;
        }
        else
        {
            setReg(Rd, cpu->regs[Rs]);
        }
        cpu->cycle += 1;
        break;
    case 3:
        // Branch exchange
        cpu->regs[15] = PC_UPDATE(cpu->regs[Rs] & 0xFFFFFFFE);
        if (cpu->regs[Rs] & 0x1)
        {
            cpu->cpsr |= 0x20; // Set THUMB mode
        }
        else
        {
            cpu->cpsr &= ~0x20; // Clear THUMB mode
        }
        cpu->cycle += 3;
        break;
    }
    if (Rd == 15)
    {
        cpu->cycle += 2;
    }
}

void procTALU(HalfWord instr)
{
    Byte op = (instr >> 6) & 0xf; // Operation code
    Byte Rs = (instr >> 3) & 0x7; // Source register
    Byte Rd = instr & 0x7;        // Destination register
    Byte m = 0;
    Word op1 = cpu->regs[Rd];
    Word op2 = cpu->regs[Rs];
    bool carry;

    switch (op)
    {
    case 0:
        // AND
        setReg(Rd, (op1 & op2));
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], CC_UNMOD, CC_UNMOD);
        cpu->cycle += 1;
        break;
    case 1:
        // EOR
        setReg(Rd, (op1 ^ op2));
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], CC_UNMOD, CC_UNMOD);
        cpu->cycle += 1;
        break;
    case 2:
        // LSL (Logical Shift Left)
        op2 &= 0xFF;
        if (op2 == 0)
        {
            carry = CC_UNMOD;
        }
        else if (op2 >= 1 && op2 <= 32)
        {
            op1 <<= op2 - 1;
            carry = op1 >> 31;
            op1 <<= 1;
        }
        else
        {
            op1 = 0;
            carry = 0;
        }
        setReg(Rd, op1);
        setCC(op1 >> 31, !op1, carry, CC_UNMOD);
        cpu->cycle += 2;
        break;
    case 3:
        // LSR (Logical Shift Right)
        op2 &= 0xFF;
        if (op2 == 0)
        {
            carry = CC_UNMOD;
        }
        else if (op2 >= 1 && op2 <= 32)
        {
            op1 >>= op2 - 1;
            carry = op1 & 0b1;
            op1 >>= 1;
        }
        else
        {
            op1 = 0;
            carry = 0;
        }
        setReg(Rd, op1);
        setCC(op1 >> 31, !op1, carry, CC_UNMOD);
        cpu->cycle += 2;
        break;
    case 4:
        // ASR (Arithmetic Shift Right)
        op2 &= 0xFF;
        if (op2 == 0)
        {
            carry = CC_UNMOD;
        }
        else if (op2 >= 1 && op2 <= 32)
        {
            op1 = (int32_t)op1 >> (op2 - 1);
            carry = op1 & 0b1;
            op1 = (int32_t)op1 >> 1;
        }
        else
        {
            carry = op1 >> 31;
            op1 = carry ? 0xFFFFFFFF : 0;
        }
        setReg(Rd, op1);
        setCC(op1 >> 31, !op1, carry, CC_UNMOD);
        cpu->cycle += 2;
        break;
    case 5:
        // ADC (Add with Carry)
        carry = getCC(C);
        setReg(Rd, op1 + op2 + carry);
        setCC(((cpu->regs[Rd] & 0x80000000) != 0), (cpu->regs[Rd] == 0), ((op1 + op2 + carry) > UINT32_MAX), (((op1 + op2 + carry) < INT32_MIN) | ((op1 + op2 + carry) > INT32_MAX)));
        cpu->cycle += 1;
        break;
    case 6:
        // SBC (Subtract with Carry)
        carry = getCC(C);
        setReg(Rd, op1 - op2 + carry - 1);
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], ((op1 - op2 - !carry) <= UINT32_MAX), (((op1 - op2 - !carry) < INT32_MIN) | (((cpu->regs[Rd]) - (cpu->regs[Rs]) - !carry) > INT32_MAX)));
        cpu->cycle += 1;
        break;
    case 7:
        // ROR (Rotate Right)
        if (op2 > 32)
        {
            op2 = ((op2 - 1) % 32) + 1;
        }

        if (op2 == 0)
        {
            carry = CC_UNMOD;
        }
        else
        {
            carry = (op1 >> (op2 - 1)) & 0b1;
            op1 = ROR(op1, op2);
        }
        setReg(Rd, op1);
        setCC(op1 >> 31, !op1, carry, CC_UNMOD);
        cpu->cycle += 2;
        break;
    case 8:
        // TST (Test)
        setCC((op1 & op2) >> 31, !(op1 & op2), CC_UNMOD, CC_UNMOD);
        cpu->cycle += 1;
        break;
    case 9:
        // NEG (Negate)
        setReg(Rd, (0 - op2));
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], ((0 - op2) <= UINT32_MAX), (((0 - op2) < INT32_MIN) | ((0 - op2) > INT32_MAX)));
        cpu->cycle += 1;
        break;
    case 10:
        // CMP (Compare)
        setCC((op1 - op2) >> 31, !(op1 - op2), op1 >= op2, ((op1 >> 31) != (op2 >> 31)) && ((op1 >> 31) != ((op1 - op2) >> 31)));
        cpu->cycle += 1;
        break;
    case 11:
        // CMN (Compare Negative)
        setCC((op1 + op2) >> 31, !(op1 + op2), ((op1 >> 31) + (op2 >> 31) > ((op1 + op2) >> 31)), ((op1 >> 31) == (op2 >> 31)) && ((op1 >> 31) != ((op1 + op2) >> 31)));
        cpu->cycle += 1;
        break;
    case 12:
        // ORR (Logical OR)
        setReg(Rd, (op1 | op2));
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], CC_UNMOD, CC_UNMOD);
        cpu->cycle += 1;
        break;
    case 13:
        // MUL (Multiply)
        if (((op1 >> 8) & 0xffffff) == 0x0 || ((op1 >> 8) & 0xffffff) == 0xffffff)
        {
            m = 1;
        }
        else if (((op1 >> 16) & 0xffff) == 0x0 || ((op1 >> 16) & 0xffff) == 0xffff)
        {
            m = 2;
        }
        else if (((op1 >> 24) & 0xff) == 0x0 || ((op1 >> 24) & 0xff) == 0xff)
        {
            m = 3;
        }
        else
        {
            m = 4;
        }
        setReg(Rd, op1 * op2);
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], 0, CC_UNMOD);
        cpu->cycle += m + 1;
        break;
    case 14:
        // BIC (Bit Clear)
        setReg(Rd, op1 & ~op2);
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], CC_UNMOD, CC_UNMOD);
        cpu->cycle += 1;
        break;
    case 15:
        // MVN (Move Not)
        setReg(Rd, ~op2);
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], CC_UNMOD, CC_UNMOD);
        cpu->cycle += 1;
        break;
    }
    if (Rd == 15)
    {
        cpu->cycle += 2;
    }
}

void procTMCASI(HalfWord instr)
{
    Byte op = (instr >> 11) & 0x3; // Operation code
    Byte Rd = (instr >> 8) & 0x7;  // Destination register
    Byte offset = (instr) & 0xff;  // Offset value
    Word temp = 0;

    switch (op)
    {
    case 0:
        // MOV
        temp = cpu->regs[Rd];
        setReg(Rd, offset);
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], CC_UNMOD, CC_UNMOD);
        cpu->cycle += 1;
        break;
    case 1:
        // CMP
        setCC((cpu->regs[Rd] - offset) >> 31, !(cpu->regs[Rd] - offset), temp >= offset, ((temp >> 31) != (offset >> 31)) && ((temp >> 31) != (cpu->regs[Rd] >> 31)));
        cpu->cycle += 1;
        break;
    case 2:
        // ADD
        temp = cpu->regs[Rd];
        setReg(Rd, cpu->regs[Rd] + offset);
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], ((temp >> 31) + (offset >> 31) > (cpu->regs[Rd] >> 31)), ((temp >> 31) == (offset >> 31)) && ((temp >> 31) != (cpu->regs[Rd] >> 31)));
        cpu->cycle += 1;
        break;
    case 3:
        // SUB
        temp = cpu->regs[Rd];
        setReg(Rd, cpu->regs[Rd] - offset);
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], temp >= offset, ((temp >> 31) != (offset >> 31)) && ((temp >> 31) != (cpu->regs[Rd] >> 31)));
        cpu->cycle += 1;
        break;
    }
    if (Rd == 15)
    {
        cpu->cycle += 2;
    }
}

void procTAS(HalfWord instr)
{
    Byte opcode = (instr >> 9) & 0x3; // Operation code
    Byte Rn = (instr >> 6) & 0x7;     // First operand register
    Byte Rs = (instr >> 3) & 0x7;     // Second operand register
    Byte Rd = instr & 0x7;            // Destination register
    Word op1 = cpu->regs[Rs];
    Word op2 = cpu->regs[Rn];
    switch (opcode)
    {
    case 0:
        // ADD (register)
        setReg(Rd, (op1 + op2));
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], ((op1 >> 31) + (op2 >> 31) > (cpu->regs[Rd] >> 31)), ((op1 >> 31) == (op2 >> 31)) && ((op1 >> 31) != (cpu->regs[Rd] >> 31)));
        cpu->cycle += 1;
        break;
    case 1:
        // SUB (register)
        setReg(Rd, op1 - op2);
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], op1 >= op2, ((op1 >> 31) != (op2 >> 31)) && ((op1 >> 31) != (cpu->regs[Rd] >> 31)));
        cpu->cycle += 1;
        break;
    case 2:
        // ADD (immediate)
        setReg(Rd, op1 + Rn);
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], ((op1 >> 31) + (Rn >> 31) > (cpu->regs[Rd] >> 31)), ((op1 >> 31) == (Rn >> 31)) && ((op1 >> 31) != (cpu->regs[Rd] >> 31)));
        cpu->cycle += 1;
        break;
    case 3:
        // SUB (immediate)
        setReg(Rd, op1 - Rn);
        setCC(cpu->regs[Rd] >> 31, !cpu->regs[Rd], op1 >= Rn, ((op1 >> 31) != (Rn >> 31)) && ((op1 >> 31) != (cpu->regs[Rd] >> 31)));
        cpu->cycle += 1;
        break;
    }
    if (Rd == 15)
    {
        cpu->cycle += 2;
    }
}

void procTMSR(HalfWord instr)
{
    Byte op = (instr >> 11) & 0x3;     // Operation code
    Byte offset = (instr >> 6) & 0x1f; // Offset value
    Byte Rs = (instr >> 3) & 0x7;      // Source register
    Byte Rd = instr & 0x7;             // Destination register
    Word value = cpu->regs[Rs];
    bool carry = getCC(C);

    switch (op)
    {
    case 0:
        // Logical Shift Left (LSL)
        if (offset != 0)
        {
            value <<= offset - 1;
            carry = value >> 31;
            value <<= 1;
        }
        setReg(Rd, value);
        setCC(value >> 31, !value, carry, CC_UNMOD);
        cpu->cycle += 1;
        break;
    case 1:
        // Logical Shift Right (LSR)
        if (offset == 0)
        {
            offset = 32;
        }

        value >>= offset - 1;
        carry = value & 0b1;
        value >>= 1;

        setReg(Rd, value);
        setCC(value >> 31, !value, carry, CC_UNMOD);
        cpu->cycle += 1;
        break;
    case 2:
        // Arithmetic Shift Right (ASR)
        if (offset == 0)
        {
            offset = 32;
        }

        value = (int32_t)value >> (offset - 1);
        carry = value & 0b1;
        value = (int32_t)value >> 1;

        setReg(Rd, value);
        setCC(value >> 31, !value, carry, CC_UNMOD);
        cpu->cycle += 1;
        break;
    }
    if (Rd == 15)
    {
        cpu->cycle += 2;
    }
}