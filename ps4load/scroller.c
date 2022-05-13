/*
  testing sinf() from math.h
  masterzorag@gmail.com, 2015-2018
*/

#include <math.h>          // sinf()
#include <string.h>        // strlen()
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define FONT_W  32
#define FONT_H  40
#define STEP_X  -2         // horizontal displacement
#define SWIDTH  1920
#define STRING  "PSL1GHT"

static int sx = SWIDTH,
           sl = 0;

static SDL_Texture* scrollText;

/***********************************************************************
* Compute string len once, then reuse value
***********************************************************************/
void init_sinetext(SDL_Renderer* renderer, const char* filepath)
{
    sl = strlen(STRING) * FONT_W;

    SDL_Surface* tempSurface = IMG_Load(filepath);
    SDL_SetColorKey(tempSurface, SDL_TRUE, SDL_MapRGB(tempSurface->format, 0, 0, 0));

    scrollText = SDL_CreateTextureFromSurface(renderer, tempSurface);
    SDL_SetTextureBlendMode(scrollText, SDL_BLENDMODE_BLEND);

    // We no longer need the surface, we only need it for texture creation
    SDL_FreeSurface(tempSurface);
}

/***********************************************************************
* Move string by defined step
***********************************************************************/
static void move_sinetext(void)
{
    sx += STEP_X;

    if(sx < -sl)           // horizontal bound, then loop
        sx = SWIDTH + FONT_W;
}

/***********************************************************************
* Draw a string of chars, amplifing y by sin(x)
***********************************************************************/
void draw_sinetext(SDL_Renderer* renderer, int y)
{
    int x = sx;       // every call resets the initial x
    float amp;

    for(int i = 0; i < (sl / FONT_W); i++)
    {
        amp = sinf(x      // testing sinf() from math.h
                 * 0.02)  // it turns out in num of bends
                 * 20;    // +/- vertical bounds over y

        if(x > 0 && x < SWIDTH - FONT_W)
        {
            SDL_Rect src = {
                .x = FONT_W * i,
                .y = 0,
                .w = FONT_W,
                .h = FONT_H,
            };
            SDL_Rect pos = {
                .x = x,
                .y = y + amp,
                .w = FONT_W,
                .h = FONT_H,
            };

            SDL_RenderCopy(renderer, scrollText, &src, &pos);
        }

        x += FONT_W;
    }

    move_sinetext();
}
