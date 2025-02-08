#include "Power_Flow_Arrow.h"

PowerFlowArrow::PowerFlowArrow(SDL_Renderer* renderer, const int16_t x, const int16_t y, const uint16_t w, const uint16_t h) {
	this->renderer = renderer;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
}

PowerFlowArrow::~PowerFlowArrow() {
	
}

//Draw the outline with no arrow, i.e. no flow.
void PowerFlowArrow::drawOutline(const uint32_t fill_color, const uint32_t outline_color) {
	uint8_t last_r, last_g, last_b, last_a;
	SDL_GetRenderDrawColor(renderer, &last_r, &last_g, &last_b, &last_a);

	SDL_Rect flow_rect = {x, y, w, h};
	SDL_SetRenderDrawColor(renderer, getRedComponent(fill_color), getGreenComponent(fill_color), getBlueComponent(fill_color), getAlphaComponent(fill_color));
	SDL_RenderFillRect(renderer, &flow_rect);
	SDL_SetRenderDrawColor(renderer, getRedComponent(outline_color), getGreenComponent(outline_color), getBlueComponent(outline_color), getAlphaComponent(outline_color));
	SDL_RenderDrawRect(renderer, &flow_rect);

	SDL_SetRenderDrawColor(renderer, last_r, last_g, last_b, last_a);
}

//Draw a filled arrow.
void PowerFlowArrow::drawFilled(const uint32_t fill_color1, const uint32_t fill_color2, const uint32_t outline_color, const int frame, const uint8_t dir, const bool arrowhead) {
	uint8_t last_r, last_g, last_b, last_a;
	SDL_GetRenderDrawColor(renderer, &last_r, &last_g, &last_b, &last_a);

	const bool vertical = dir == ARROW_DIR_UP || dir == ARROW_DIR_DOWN;
	
	int line_count;
	if(arrowhead)
		line_count = w + h;
	else {
		if(vertical)
			line_count = h;
		else
			line_count = w;
	}

	uint32_t lines[line_count];
	for(int i=0;i<line_count;i+=1) {
		int position = i + frame*((dir == ARROW_DIR_DOWN || dir == ARROW_DIR_RIGHT) ? -1 : 1);
		while(position < 0)
			position += 3*GRAD_W;

		const int state = (position/GRAD_W)%3;
		if(state == 0) {
			const uint8_t norm_pos = (position%GRAD_W)*256/GRAD_W;
			lines[i] = calculateGradient(fill_color1, fill_color2, norm_pos);
		} else if(state == 1) {
			const uint8_t norm_pos = (position%GRAD_W)*256/GRAD_W;
			lines[i] = calculateGradient(fill_color2, fill_color1, norm_pos);
		} else
			lines[i] = fill_color1;
	}

	const int adj_line_count = line_count - ((arrowhead) ? ((vertical) ? w : h) : 0);
	const int start = (arrowhead && (dir == ARROW_DIR_LEFT || dir == ARROW_DIR_UP)) ? (vertical ? w : h) : 0;

	for(int i=0;i<adj_line_count;i+=1) {
		SDL_SetRenderDrawColor(renderer, getRedComponent(lines[i + start]), getGreenComponent(lines[i + start]), getBlueComponent(lines[i + start]), getAlphaComponent(lines[i + start]));
		if(vertical)
			SDL_RenderDrawLine(renderer, x, y + i, x + w - 1, y + i);
		else
			SDL_RenderDrawLine(renderer, x + i, y, x + i, y + h - 1);
	}

	SDL_Rect flow_rect = {x, y, w, h};

	//Outline:
	SDL_SetRenderDrawColor(renderer, getRedComponent(outline_color), getGreenComponent(outline_color), getBlueComponent(outline_color), getAlphaComponent(outline_color));
	SDL_RenderDrawRect(renderer, &flow_rect);

	if(arrowhead) {
		if(vertical) {
			const int arrow_start_y = (dir == ARROW_DIR_DOWN) ? y + h - 1 : y - w + 1;
			for(int i=0;i<w;i+=1) {
				const uint32_t line_color = lines[i + ((dir == ARROW_DIR_DOWN) ? h : 0)];
				SDL_SetRenderDrawColor(renderer, getRedComponent(line_color), getGreenComponent(line_color), getBlueComponent(line_color), getAlphaComponent(line_color));

				const int j = (dir == ARROW_DIR_DOWN) ? w - 1 - i : i;
				SDL_RenderDrawLine(renderer, x + w/2 - j, arrow_start_y + i, x + w/2 + j, arrow_start_y + i);
			}

			SDL_SetRenderDrawColor(renderer, getRedComponent(outline_color), getGreenComponent(outline_color), getBlueComponent(outline_color), getAlphaComponent(outline_color));
			if(dir == ARROW_DIR_DOWN) {
				SDL_RenderDrawLine(renderer, x - w/2, y + h - 1, x, y + h - 1);
				SDL_RenderDrawLine(renderer, x + w, y + h - 1, x + 3*w/2, y + h - 1);
				SDL_RenderDrawLine(renderer, x - w/2, y + h - 1, x + w/2, y + h + w - 1);
				SDL_RenderDrawLine(renderer, x + 3*w/2, y + h - 1, x + w/2, y + h + w - 1);
			} else {
				SDL_RenderDrawLine(renderer, x - w/2, y, x, y);
				SDL_RenderDrawLine(renderer, x + w, y, x + 3*w/2, y);
				SDL_RenderDrawLine(renderer, x - w/2, y, x + w/2, y - w + 1);
				SDL_RenderDrawLine(renderer, x + 3*w/2, y, x + w/2, y - w + 1);
			}
		} else {
			const int arrow_start_x = (dir == ARROW_DIR_RIGHT) ? x + w - 1 : x - h + 1;
			for(int i=0;i<h;i+=1) {
				const uint32_t line_color = lines[i + ((dir == ARROW_DIR_RIGHT) ? w : 0)];
				SDL_SetRenderDrawColor(renderer, getRedComponent(line_color), getGreenComponent(line_color), getBlueComponent(line_color), getAlphaComponent(line_color));

				const int j = (dir == ARROW_DIR_RIGHT) ? h - 1 - i : i;
				SDL_RenderDrawLine(renderer, arrow_start_x + i, y + h/2 - j, arrow_start_x + i, y + h/2 + j);
			}

			SDL_SetRenderDrawColor(renderer, getRedComponent(outline_color), getGreenComponent(outline_color), getBlueComponent(outline_color), getAlphaComponent(outline_color));
			if(dir == ARROW_DIR_RIGHT) {
				SDL_RenderDrawLine(renderer, x + w - 1, y, x + w - 1, y - h/2);
				SDL_RenderDrawLine(renderer, x + w - 1, y + h, x + w - 1, y + 3*h/2);
				SDL_RenderDrawLine(renderer, x + w + h - 1, y + h/2, x + w - 1, y + 3*h/2);
				SDL_RenderDrawLine(renderer, x + w + h - 1, y + h/2, x + w - 1, y - h/2 - 1);
			} else {
				SDL_RenderDrawLine(renderer, x, y, x, y - h/2);
				SDL_RenderDrawLine(renderer, x, y + h, x, y + 3*h/2);
				SDL_RenderDrawLine(renderer, x - h + 1, y + h/2, x, y + 3*h/2);
				SDL_RenderDrawLine(renderer, x - h + 1, y + h/2, x, y - h/2 - 1);
			}
		}
	}

	SDL_SetRenderDrawColor(renderer, last_r, last_g, last_b, last_a);
}

uint32_t calculateGradient(const uint32_t color1, const uint32_t color2, uint8_t position) {
	const uint8_t r1 = getRedComponent(color1), g1 = getGreenComponent(color1), b1 = getBlueComponent(color1), a1 = getAlphaComponent(color1);
	const uint8_t r2 = getRedComponent(color2), g2 = getGreenComponent(color2), b2 = getBlueComponent(color2), a2 = getAlphaComponent(color2);
	const int16_t rd = int16_t(r2) - int16_t(r1), gd = int16_t(g2) - int16_t(g1), bd = int16_t(b2) - int16_t(b1), ad = int16_t(a2) - int16_t(a1);

	const int16_t res_r = rd*position/255 + r1;
	const int16_t res_g = gd*position/255 + g1;
	const int16_t res_b = bd*position/255 + b1;
	const int16_t res_a = ad*position/255 + a1;

	return (uint8_t(res_r)<<24)|(uint8_t(res_g)<<16)|(uint8_t(res_b)<<8)|(uint8_t(res_a));
}