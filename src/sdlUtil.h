/****************************************************************************************************
 *
 * @file:    sdlUtil.h
 * @author:  Nolan Olhausen
 * @date: 2024-09-6
 *
 * @brief:
 *      Header file for SDL utility functions.
 *
 * @references:
 *      gdkGBA - https://github.com/gdkchan/gdkGBA
 *
 * @license:
 * GNU General Public License version 2.
 * Copyright (C) 2024 - Nolan Olhausen
 ****************************************************************************************************/

#include <SDL.h>
#include <SDL_audio.h>

/**
 * @brief SDL window used for rendering.
 */
SDL_Window *window;

/**
 * @brief SDL renderer used for rendering.
 */
SDL_Renderer *renderer;

/**
 * @brief SDL texture used for rendering.
 */
SDL_Texture *texture;

/**
 * @brief Pitch of the texture in bytes.
 */
int32_t texPitch;

/**
 * @brief Initializes SDL components.
 *
 * This function initializes the SDL window, renderer, and texture.
 */
void sdlInit();

/**
 * @brief Uninitializes SDL components.
 *
 * This function cleans up and shuts down the SDL window, renderer, and texture.
 */
void sdlUninit();

/**
 * @brief Renders a frame using SDL.
 *
 * @param renderer The SDL renderer to use for rendering.
 * @param frame The frame data to render.
 *
 * This function renders a frame to the SDL window using the provided renderer and frame data.
 */
void sdlRenderFrame(SDL_Renderer *renderer, uint32_t *frame);