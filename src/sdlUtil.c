/****************************************************************************************************
 *
 * @file:    sdlUtil.c
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Implementation file for SDL utility functions.
 *
 * @references:
 *      gdkGBA - https://github.com/gdkchan/gdkGBA
 *
 * @license:
 * GNU General Public License version 2.
 * Copyright (C) 2024 - Nolan Olhausen
 ****************************************************************************************************/

#include "sdlUtil.h"
#include "apu.h"
#include "ppu.h"

#define FRAME_WIDTH 240
#define FRAME_HEIGHT 160

#define RGB_VALUE(n) (((n) << 3) | ((n) >> 2))

void sdlInit()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    initFrameBuffer();
    window = SDL_CreateWindow("GBA Emulator",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              480, 320,
                              SDL_WINDOW_SHOWN);

    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_BGRA8888,
        SDL_TEXTUREACCESS_STREAMING,
        240,
        160);

    texPitch = 240 * 4;
    SDL_AudioSpec spec = {
        .freq = 32768,          // 32KHz
        .format = AUDIO_S16SYS, // Signed 16 bits System endiannes
        .channels = 2,          // Stereo
        .samples = 512,         // 16ms
        .callback = soundMix,
        .userdata = NULL};

    SDL_OpenAudio(&spec, NULL);
    SDL_PauseAudio(0);
}

void sdlUninit()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_CloseAudio();
    SDL_Quit();
}

void sdlRenderFrame(SDL_Renderer *renderer, uint32_t *frame)
{
    SDL_UpdateTexture(texture, NULL, frame, FRAME_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer); // Clear the previous frame
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer); // Render the new frame
}
