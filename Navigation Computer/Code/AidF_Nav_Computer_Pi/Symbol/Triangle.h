#include <SDL2/SDL.h>

#ifndef triangle_h
#define triangle_h

int SDL_RenderTriangle(SDL_Renderer* renderer, SDL_Texture* texture, int x1, int y1, int x2, int y2, int x3, int y3, SDL_Color color);

#endif