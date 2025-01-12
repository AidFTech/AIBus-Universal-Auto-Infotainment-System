#include <SDL2/SDL.h>

#ifndef circle_h
#define circle_h

int SDL_DrawCircle(SDL_Renderer* renderer, const int center_x, const int center_y, const int radius);
int SDL_FillCircle(SDL_Renderer* renderer, const int center_x, const int center_y, const int radius);

#endif