#include "Nav_Background.h"

#ifndef nav_gradient_background_h
#define nav_gradient_background_h

class BR_Gradient : public Background {
public:
	BR_Gradient(const uint16_t lx, const uint16_t ly, const uint32_t color_1, const uint32_t color_2, const bool vertical, const uint16_t sq_size);
	uint32_t getColorAt(int16_t x, int16_t y);
	void drawBackground(SDL_Renderer* renderer, const int16_t x, const int16_t y, const uint16_t w, const uint16_t h);
private:
	uint32_t color1, color2;
	uint16_t sq_size;
	bool vertical;
};

#endif