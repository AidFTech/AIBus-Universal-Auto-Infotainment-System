#include "Circle.h"

//Draw an unfilled circle.
int SDL_DrawCircle(SDL_Renderer* renderer, const int center_x, const int center_y, const int radius) {
	//Uses Midpoint Circle Algorithm, referenced here: https://stackoverflow.com/questions/38334081/how-to-draw-circles-arcs-and-vector-graphics-in-sdl
	const int diameter = radius*2;
	int x = radius - 1, y = 0, tx = 1, ty = 1, error = tx - diameter;

	int result = 0;
	
	while(x >= y) {
		result = SDL_RenderDrawPoint(renderer, center_x + x, center_y - y);
		result = SDL_RenderDrawPoint(renderer, center_x + x, center_y + y);
		result = SDL_RenderDrawPoint(renderer, center_x - x, center_y - y);
		result = SDL_RenderDrawPoint(renderer, center_x - x, center_y + y);
		result = SDL_RenderDrawPoint(renderer, center_x + y, center_y - x);
		result = SDL_RenderDrawPoint(renderer, center_x + y, center_y + x);
		result = SDL_RenderDrawPoint(renderer, center_x - y, center_y - x);
		result = SDL_RenderDrawPoint(renderer, center_x - y, center_y + x);

		if(error <= 0) {
			y += 1;
			error += ty;
			ty += 2;
		} else if(error > 0) {
			x -= 1;
			tx += 2;
			error += tx - diameter;
		}

		if(result != 0)
			return result;
	}

	return result;
}

//Draw a filled circle.
int SDL_FillCircle(SDL_Renderer* renderer, const int center_x, const int center_y, const int radius) {
	const int diameter = radius*2;
	int x = radius - 1, y = 0, tx = 1, ty = 1, error = tx - diameter;

	int result = 0;
	
	while(x >= y) {
		result = SDL_RenderDrawPoint(renderer, center_x + x, center_y - y);
		result = SDL_RenderDrawPoint(renderer, center_x + x, center_y + y);
		result = SDL_RenderDrawPoint(renderer, center_x - x, center_y - y);
		result = SDL_RenderDrawPoint(renderer, center_x - x, center_y + y);
		result = SDL_RenderDrawPoint(renderer, center_x + y, center_y - x);
		result = SDL_RenderDrawPoint(renderer, center_x + y, center_y + x);
		result = SDL_RenderDrawPoint(renderer, center_x - y, center_y - x);
		result = SDL_RenderDrawPoint(renderer, center_x - y, center_y + x);

		result = SDL_RenderDrawLine(renderer, center_x - x, center_y - y, center_x + x, center_y - y);
		result = SDL_RenderDrawLine(renderer, center_x - x, center_y + y, center_x + x, center_y + y);

		result = SDL_RenderDrawLine(renderer, center_x - y, center_y - x, center_x + y, center_y - x);
		result = SDL_RenderDrawLine(renderer, center_x - y, center_y + x, center_x + y, center_y + x);

		if(error <= 0) {
			y += 1;
			error += ty;
			ty += 2;
		} else if(error > 0) {
			x -= 1;
			tx += 2;
			error += tx - diameter;
		}

		if(result != 0)
			return result;
	}

	return result;
}