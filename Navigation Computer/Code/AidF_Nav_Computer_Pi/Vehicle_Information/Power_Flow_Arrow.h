#include <stdint.h>
#include <SDL2/SDL.h>

#include "../AidF_Color_Profile.h"

#ifndef power_flow_arrow_h
#define power_flow_arrow_h

#define ARROW_DIR_UP 1
#define ARROW_DIR_DOWN 2
#define ARROW_DIR_LEFT 3
#define ARROW_DIR_RIGHT 4

#define CORNER_ANGLE_UL 0
#define CORNER_ANGLE_UR 1
#define CORNER_ANGLE_DR 2
#define CORNER_ANGLE_DL 3

#define GRAD_W 30

class PowerFlowArrow {
public:
	PowerFlowArrow(SDL_Renderer* renderer, const int16_t x, const int16_t y, const uint16_t w, const uint16_t h);
	~PowerFlowArrow();
	
	void drawOutline(const uint32_t fill_color, const uint32_t outline_color);
	void drawFilled(const uint32_t fill_color1, const uint32_t fill_color2, const uint32_t outline_color, const int frame, const uint8_t dir, const bool arrowhead);
	
private:
	SDL_Renderer* renderer;
	
	int16_t x = 0, y = 0;
	uint16_t w = 0, h = 0;
};

class PowerFlowCorner {
public:
	PowerFlowCorner(SDL_Renderer* renderer, const int16_t x, const int16_t y, const uint16_t r);
	~PowerFlowCorner();

	void drawOutline(const uint32_t fill_color, const uint32_t outline_color, const uint8_t angle);
	void drawFilled(const uint32_t fill_color1, const uint32_t fill_color2, const uint32_t outline_color, const int frame, const uint8_t angle, const bool cw);

private:
	void drawArc(const uint8_t angle);
	void drawAngleLine(const uint8_t angle, const uint8_t pos, const uint32_t color);

	SDL_Renderer* renderer;
	
	int16_t x = 0, y = 0;
	uint16_t r = 0;
};

uint32_t calculateGradient(const uint32_t color1, const uint32_t color2, uint8_t position);

#endif