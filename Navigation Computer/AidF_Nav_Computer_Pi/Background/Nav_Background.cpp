#include "Nav_Background.h"

Background::Background(uint16_t lx, uint16_t ly) {
	this->LX = lx;
	this->LY = ly;
}

uint32_t Background::getColorAt(int16_t x, int16_t y) {
	return 0x0; //Dummy function for generic background types.
}

void Background::drawBackground(SDL_Renderer* renderer, const int16_t x, const int16_t y, const uint16_t w, const uint16_t h) {
	//Dummy function.
}