#include "Nav_Background.h"

#ifndef nav_solid_background_h
#define nav_solid_background_h

class BR_Solid : public Background {
public:
	BR_Solid(const uint16_t lx, const uint16_t ly, const uint32_t color_l);
	uint32_t getColorAt(int16_t x, int16_t y);
	void drawBackground(SDL_Renderer* renderer, const int16_t x, const int16_t y, const uint16_t w, const uint16_t h);
private:
	uint32_t color;
};

#endif