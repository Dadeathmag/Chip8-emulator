#pragma once

#include <SDL2/SDL.h>
#include <cstdint>

// Draw ASCII text using a built-in 5x7 bitmap font (no SDL_ttf dependency).
// Color is ARGB8888; each character cell is 6 pixels wide (5 glyph + 1 gap).
void drawText(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color, int scale = 1);
