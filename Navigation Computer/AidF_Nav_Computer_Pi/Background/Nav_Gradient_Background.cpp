#include "Nav_Gradient_Background.h"

BR_Gradient::BR_Gradient(const uint16_t lx, const uint16_t ly, const uint32_t color_1, const uint32_t color_2, const bool vertical, const uint16_t sq_size) : Background(lx, ly) {
	this->color1 = color_1;
	this->color2 = color_2;

	this->vertical = vertical;
	this->sq_size = sq_size;
}

uint32_t BR_Gradient::getColorAt(int16_t x, int16_t y) {
	uint16_t w = this->LX, h = this->LY;

	if(this->vertical) {
		const int16_t tx = x;
		x = y;
		y = tx;
		w = this->LY;
		h = this->LX;
	}

	const int16_t sq_start = h/2 - this->sq_size/2, sq_end = h/2 + this->sq_size/2;

	const uint8_t r1 = getRedComponent(color1), g1 = getGreenComponent(color1), b1 = getBlueComponent(color1), a1 = getAlphaComponent(color1);
	const uint8_t r2 = getRedComponent(color2), g2 = getGreenComponent(color2), b2 = getBlueComponent(color2), a2 = getAlphaComponent(color2);
	const int16_t rd = int16_t(r2) - int16_t(r1), gd = int16_t(g2) - int16_t(g1), bd = int16_t(b2) - int16_t(b1), ad = int16_t(a2) - int16_t(a1);

	if(y>=sq_start && y < sq_end) {
		return (r2<<24)|(g2<<16)|(b2<<8)|a2;
	} else if(y < sq_start) {
		const int16_t res_r = rd*y/sq_start + r1;
		const int16_t res_g = gd*y/sq_start + g1;
		const int16_t res_b = bd*y/sq_start + b1;
		const int16_t res_a = ad*y/sq_start + a1;

		return (uint8_t(res_r)<<24)|(uint8_t(res_g)<<16)|(uint8_t(res_b)<<8)|(uint8_t(res_a));
	} else {
		const int16_t gy = y - sq_end;

		const int16_t res_r = r2 - rd*gy/sq_start;
		const int16_t res_g = g2 - gd*gy/sq_start;
		const int16_t res_b = b2 - bd*gy/sq_start;
		const int16_t res_a = a2 - ad*gy/sq_start;

		return (uint8_t(res_r)<<24)|(uint8_t(res_g)<<16)|(uint8_t(res_b)<<8)|(uint8_t(res_a));
	}
}

void BR_Gradient::drawBackground(SDL_Renderer* renderer, const int16_t x, const int16_t y, const uint16_t w, const uint16_t h) {
	if(!vertical) {
		const int16_t sq_start = this->LY/2 - this->sq_size/2, sq_end = this->LY/2 + this->sq_size/2;
		for(uint16_t i=0;i<sq_start;i+=1) {
			const uint32_t color = getColorAt(0, i);
			SDL_SetRenderDrawColor(renderer, getRedComponent(color), getGreenComponent(color), getBlueComponent(color), getAlphaComponent(color));
			SDL_RenderDrawLine(renderer, 0, i, this->LX, i);
		}

		SDL_SetRenderDrawColor(renderer, getRedComponent(color2), getGreenComponent(color2), getBlueComponent(color2), getAlphaComponent(color2));
		SDL_Rect rect = {0,sq_start,this->LX, sq_end-sq_start};
		SDL_RenderFillRect(renderer, &rect);

		for(uint16_t i=sq_end;i<this->LY;i+=1) {
			const uint32_t color = getColorAt(0, i);
			SDL_SetRenderDrawColor(renderer, getRedComponent(color), getGreenComponent(color), getBlueComponent(color), getAlphaComponent(color));
			SDL_RenderDrawLine(renderer, 0, i, this->LX, i);
		}

	} else {
		const int16_t sq_start = this->LX/2 - this->sq_size/2, sq_end = this->LX/2 + this->sq_size/2;
		for(uint16_t i=0;i<sq_start;i+=1) {
			const uint32_t color = getColorAt(i, 0);
			SDL_SetRenderDrawColor(renderer, getRedComponent(color), getGreenComponent(color), getBlueComponent(color), getAlphaComponent(color));
			SDL_RenderDrawLine(renderer, i, 0, i, this->LY);
		}

		SDL_SetRenderDrawColor(renderer, getRedComponent(color2), getGreenComponent(color2), getBlueComponent(color2), getAlphaComponent(color2));
		SDL_Rect rect = {sq_start, 0, sq_end - sq_start, this->LY};
		SDL_RenderFillRect(renderer, &rect);

		for(uint16_t i=sq_end;i<this->LX;i+=1) {
			const uint32_t color = getColorAt(i, 0);
			SDL_SetRenderDrawColor(renderer, getRedComponent(color), getGreenComponent(color), getBlueComponent(color), getAlphaComponent(color));
			SDL_RenderDrawLine(renderer, i, 0, i, this->LY);
		}
	}
}