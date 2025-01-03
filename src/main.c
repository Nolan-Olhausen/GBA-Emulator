/****************************************************************************************************
 *
 * @file:    main.c
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Main file for GBA emulator.
 *
 * @references:
 *      gdkGBA - https://github.com/gdkchan/gdkGBA
 *
 * @license:
 * GNU General Public License version 2.
 * Copyright (C) 2024 - Nolan Olhausen
 ****************************************************************************************************/

#include "common.h"
#include <SDL.h>
#include "cpu.h"
#include "memory.h"
#include "ppu.h"
#include "sdlUtil.h"

// Screen dimensions and pixel size
#define SCREEN_HEIGHT 160
#define SCREEN_WIDTH 240
#define PIXEL_SIZE 3

// Button definitions
#define BTN_A (1 << 0)
#define BTN_B (1 << 1)
#define BTN_SELECT (1 << 2)
#define BTN_START (1 << 3)
#define BTN_R (1 << 4)
#define BTN_L (1 << 5)
#define BTN_U (1 << 6)
#define BTN_D (1 << 7)
#define BTN_RT (1 << 8)
#define BTN_LT (1 << 9)

// Color conversion macro
#define COLOR(n) (((n) << 3) | ((n) >> 2))

cpuCore *cpu;
memoryCore *mem;

// Main function
int main(int argc, char *argv[])
{
    // Check for .gba file argument
    if (argc <= 1)
    {
        fprintf(stderr, "No .gba file provided\n");
        exit(-1);
    }

    // Allocate memory for CPU state
    cpu = (cpuCore *)malloc(sizeof(cpuCore));
    if (cpu == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for CPU state\n");
        return 1;
    }
    memset(cpu, 0, sizeof(*cpu));

    // Allocate memory for MEMORY state
    mem = (memoryCore *)malloc(sizeof(memoryCore));
    if (mem == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for MEMORY state\n");
        return 1;
    }
    memset(mem, 0, sizeof(*mem));

    // Initialize GBA with provided ROM and BIOS
    startGBA(argv[1], "src/gbaBios.bin");

    // Initialize SDL
    sdlInit();

    // Emulation running flag
    bool running = true;

    // Run loop
    while (running)
    {
        SDL_Event event;
        // Poll for SDL events
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_KEYDOWN:
                // Handle key down events
                switch (event.key.keysym.sym)
                {
                case SDLK_UP:
                    mem->keypad.keyinput.full &= ~BTN_U;
                    break;
                case SDLK_DOWN:
                    mem->keypad.keyinput.full &= ~BTN_D;
                    break;
                case SDLK_LEFT:
                    mem->keypad.keyinput.full &= ~BTN_L;
                    break;
                case SDLK_RIGHT:
                    mem->keypad.keyinput.full &= ~BTN_R;
                    break;
                case SDLK_a:
                    mem->keypad.keyinput.full &= ~BTN_A;
                    break;
                case SDLK_s:
                    mem->keypad.keyinput.full &= ~BTN_B;
                    break;
                case SDLK_q:
                    mem->keypad.keyinput.full &= ~BTN_LT;
                    break;
                case SDLK_w:
                    mem->keypad.keyinput.full &= ~BTN_RT;
                    break;
                case SDLK_TAB:
                    mem->keypad.keyinput.full &= ~BTN_SELECT;
                    break;
                case SDLK_RETURN:
                    mem->keypad.keyinput.full &= ~BTN_START;
                    break;
                default:
                    break;
                }
                break;

            case SDL_KEYUP:
                // Handle key up events
                switch (event.key.keysym.sym)
                {
                case SDLK_UP:
                    mem->keypad.keyinput.full |= BTN_U;
                    break;
                case SDLK_DOWN:
                    mem->keypad.keyinput.full |= BTN_D;
                    break;
                case SDLK_LEFT:
                    mem->keypad.keyinput.full |= BTN_L;
                    break;
                case SDLK_RIGHT:
                    mem->keypad.keyinput.full |= BTN_R;
                    break;
                case SDLK_a:
                    mem->keypad.keyinput.full |= BTN_A;
                    break;
                case SDLK_s:
                    mem->keypad.keyinput.full |= BTN_B;
                    break;
                case SDLK_q:
                    mem->keypad.keyinput.full |= BTN_LT;
                    break;
                case SDLK_w:
                    mem->keypad.keyinput.full |= BTN_RT;
                    break;
                case SDLK_TAB:
                    mem->keypad.keyinput.full |= BTN_SELECT;
                    break;
                case SDLK_RETURN:
                    mem->keypad.keyinput.full |= BTN_START;
                    break;
                default:
                    break;
                }
                break;

            case SDL_QUIT:
                // Handle quit event
                running = false;
                break;
            }
        }
        // Update the PPU (Pixel Processing Unit)
        tickPPU();
    }

    // Uninitialize SDL and free allocated memory
    sdlUninit();
    free(sram);
    free(eeprom);
    free(flash);
    free(mem);
    free(cpu);

    return 0;
}