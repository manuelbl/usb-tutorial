/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Color TFT display
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

/* Initialize TFT display */
void display_init();

/* Draw pixelmap (RGB565 format) */
void display_draw(int x, int y, int row_len, int num_rows, const uint8_t* pixels);

#endif
