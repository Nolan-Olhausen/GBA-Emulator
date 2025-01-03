/****************************************************************************************************
 *
 * @file:    common.h
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Header file for common types and macros.
 *
 * @license:
 * GNU General Public License version 2.
 * Copyright (C) 2024 - Nolan Olhausen
 ****************************************************************************************************/

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define macros for minimum and maximum
#define min(a, b) ((a) > (b) ? (b) : (a))
#define max(a, b) ((a) > (b) ? (a) : (b))

// Define type aliases for common data types
typedef bool Bit;
typedef uint8_t Byte;
typedef uint16_t HalfWord;
typedef uint32_t Word;
typedef uint64_t DWord;

// Enumeration for shift types
typedef enum
{
    SHIFT_TYPE_LSL, // Logical Shift Left
    SHIFT_TYPE_LSR, // Logical Shift Right
    SHIFT_TYPE_ASR, // Arithmetic Shift Right
    SHIFT_TYPE_ROR  // Rotate Right
} ShiftType;
