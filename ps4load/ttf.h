#pragma once

#include <stdbool.h>
#include <proto-include.h>

extern FT_Library ftLib;

typedef struct Color_rgb
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

bool InitFont(FT_Face* face, const char* fontPath, int fontSize);
void DrawString(SDL_Renderer *renderer, const char* msg, FT_Face font, Color color, int x, int y);
