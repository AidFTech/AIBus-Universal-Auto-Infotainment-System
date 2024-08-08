#include <stdint.h>
#include <SDL2/SDL.h>
#include "../AidF_Color_Profile.h"

#ifndef nav_background_h
#define nav_background_h

class Background {
public:
	Background(uint16_t lx, uint16_t ly);
	virtual uint32_t getColorAt(int16_t x, int16_t y);
	virtual void drawBackground(SDL_Renderer* renderer, const int16_t x, const int16_t y, const uint16_t w, const uint16_t h);
protected:
	uint16_t LX, LY;
};

#endif