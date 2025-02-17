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

PowerFlowCorner::PowerFlowCorner(SDL_Renderer* renderer, const int16_t x, const int16_t y, const uint16_t r) {
	this->renderer = renderer;
	this->x = x;
	this->y = y;
	this->r = r;
}

PowerFlowCorner::~PowerFlowCorner() {

}

//Draw the outline with no flow.
void PowerFlowCorner::drawOutline(const uint32_t fill_color, const uint32_t outline_color, const uint8_t angle) {
	uint8_t last_r, last_g, last_b, last_a;
	SDL_GetRenderDrawColor(renderer, &last_r, &last_g, &last_b, &last_a);

	for(int i=0;i<256;i+=1)
		this->drawAngleLine(angle, i, fill_color);

	SDL_SetRenderDrawColor(renderer, getRedComponent(outline_color), getGreenComponent(outline_color), getBlueComponent(outline_color), getAlphaComponent(outline_color));
	this->drawArc(angle);

	int rect_x = x, rect_y = y;

	if(angle == CORNER_ANGLE_UR || angle == CORNER_ANGLE_DR)
		rect_x = x + r - 1;
	if(angle == CORNER_ANGLE_DL || angle == CORNER_ANGLE_DR)
		rect_y = y + r - 1;

	SDL_Rect dot_rect = {rect_x, rect_y, 2, 2};
	SDL_RenderFillRect(renderer, &dot_rect);

	SDL_SetRenderDrawColor(renderer, last_r, last_g, last_b, last_a);
}

//Draw the filled outline.
void PowerFlowCorner::drawFilled(const uint32_t fill_color1, const uint32_t fill_color2, const uint32_t outline_color, const int frame, const uint8_t angle, const bool cw) {
	uint8_t last_r, last_g, last_b, last_a;
	SDL_GetRenderDrawColor(renderer, &last_r, &last_g, &last_b, &last_a);

	const int line_count = 2*this->r;

	uint32_t lines[line_count];
	for(int i=0;i<line_count;i+=1) {
		int position = i + frame*(cw ? -1 : 1);
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

	for(int i=0;i<256;i+=1) {
		int line_pos = i*line_count/256;
		while(line_pos < 0)
			line_pos += line_count;
			
		this->drawAngleLine(angle, i, lines[line_pos]);
	}

	SDL_SetRenderDrawColor(renderer, getRedComponent(outline_color), getGreenComponent(outline_color), getBlueComponent(outline_color), getAlphaComponent(outline_color));
	this->drawArc(angle);

	int rect_x = x, rect_y = y;

	if(angle == CORNER_ANGLE_UR || angle == CORNER_ANGLE_DR)
		rect_x = x + r - 1;
	if(angle == CORNER_ANGLE_DL || angle == CORNER_ANGLE_DR)
		rect_y = y + r - 1;

	SDL_Rect dot_rect = {rect_x, rect_y, 2, 2};
	SDL_RenderFillRect(renderer, &dot_rect);

	SDL_SetRenderDrawColor(renderer, last_r, last_g, last_b, last_a);
}

//Draw an arc.
void PowerFlowCorner::drawArc(const uint8_t angle) {
	const int radius = this->r + 1;

	int center_x = this->x, center_y = this->y;
	if(angle == CORNER_ANGLE_DR || angle == CORNER_ANGLE_UR)
		center_x = this->x + this->r;
	if(angle == CORNER_ANGLE_DR || angle == CORNER_ANGLE_DL)
		center_y = this->y + this->r;

	const int diameter = radius*2;
	int x = radius - 1, y = 0, tx = 1, ty = 1, error = tx - diameter;

	int result = 0;
	
	while(x >= y) {
		if(angle == CORNER_ANGLE_UL) {
			result = SDL_RenderDrawPoint(renderer, center_x + x, center_y + y);
			result = SDL_RenderDrawPoint(renderer, center_x + y, center_y + x);
		}

		if(angle == CORNER_ANGLE_DL) {
			result = SDL_RenderDrawPoint(renderer, center_x + x, center_y - y);
			result = SDL_RenderDrawPoint(renderer, center_x + y, center_y - x);
		}

		if(angle == CORNER_ANGLE_DR) {
			result = SDL_RenderDrawPoint(renderer, center_x - x, center_y - y);
			result = SDL_RenderDrawPoint(renderer, center_x - y, center_y - x);
		}

		if(angle == CORNER_ANGLE_UR) {
			result = SDL_RenderDrawPoint(renderer, center_x - x, center_y + y);
			result = SDL_RenderDrawPoint(renderer, center_x - y, center_y + x);
		}

		if(error <= 0) {
			y += 1;
			error += ty;
			ty += 2;
		} else if(error > 0) {
			x -= 1;
			tx += 2;
			error += tx - diameter;
		}
	}
}

//Draw a "fill" line.
void PowerFlowCorner::drawAngleLine(const uint8_t angle, const uint8_t pos, const uint32_t color) {
	uint8_t last_r, last_g, last_b, last_a;
	SDL_GetRenderDrawColor(renderer, &last_r, &last_g, &last_b, &last_a);

	const int radius = this->r + 1;

	int center_x = this->x, center_y = this->y;
	if(angle == CORNER_ANGLE_DR || angle == CORNER_ANGLE_UR)
		center_x = this->x + this->r;
	if(angle == CORNER_ANGLE_DR || angle == CORNER_ANGLE_DL)
		center_y = this->y + this->r;

	const int diameter = radius*2;
	int x = radius - 1, y = 0, tx = 1, ty = 1, error = tx - diameter;

	const int iter = pos*diameter/255;

	uint8_t max_iter = iter;
	if(max_iter > radius)
		max_iter = diameter - iter;

	for(int i=0;i<max_iter;i+=1) {
		if(error <= 0) {
			y += 1;
			error += ty;
			ty += 2;
		} else if(error > 0) {
			x -= 1;
			tx += 2;
			error += tx - diameter;
		}
	}

	SDL_SetRenderDrawColor(renderer, getRedComponent(color), getGreenComponent(color), getBlueComponent(color), getAlphaComponent(color));

	int ep_x = x, ep_y = y;
	if(pos > 127) {
		ep_x = y;
		ep_y = x;
	}

	if(angle == CORNER_ANGLE_UL)
		SDL_RenderDrawLine(renderer, center_x, center_y, center_x + ep_x, center_y + ep_y);
	else if(angle == CORNER_ANGLE_UR)
		SDL_RenderDrawLine(renderer, center_x, center_y, center_x - ep_x, center_y + ep_y);
	else if(angle == CORNER_ANGLE_DR)
		SDL_RenderDrawLine(renderer, center_x, center_y, center_x - ep_x, center_y - ep_y);
	else if(angle == CORNER_ANGLE_DL)
		SDL_RenderDrawLine(renderer, center_x, center_y, center_x + ep_x, center_y - ep_y);
	
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