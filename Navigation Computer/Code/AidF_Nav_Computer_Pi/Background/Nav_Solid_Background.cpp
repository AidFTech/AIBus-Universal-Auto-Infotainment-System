#include "Nav_Solid_Background.h"

BR_Solid::BR_Solid(const uint16_t lx, const uint16_t ly, const uint32_t color_l) : Background(lx, ly) {
	this->color = color_l;
}

uint32_t BR_Solid::getColorAt(int16_t x, int16_t y) {
	return this->color;
}

void BR_Solid::drawBackground(SDL_Renderer* renderer, const int16_t x, const int16_t y, const uint16_t w, const uint16_t h) {
	SDL_SetRenderDrawColor(renderer, getRedComponent(this->color), getGreenComponent(this->color), getBlueComponent(this->color), getAlphaComponent(this->color));
	if(x == 0 && y == 0 && w == this->LX && h == this->LY) {
		SDL_RenderClear(renderer);
	} else {
		SDL_Rect background_rect = {x,y,w,h};
		SDL_RenderFillRect(renderer, &background_rect);
	}
}