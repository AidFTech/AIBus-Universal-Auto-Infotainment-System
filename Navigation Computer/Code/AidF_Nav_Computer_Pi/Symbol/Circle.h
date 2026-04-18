#include <SDL2/SDL.h>

#ifndef circle_h
#define circle_h

#ifdef __cplusplus
extern "C" {
#endif
int SDL_DrawCircle(SDL_Renderer* renderer, const int center_x, const int center_y, const int radius);
int SDL_FillCircle(SDL_Renderer* renderer, const int center_x, const int center_y, const int radius);
#ifdef __cplusplus
}
#endif

#endif