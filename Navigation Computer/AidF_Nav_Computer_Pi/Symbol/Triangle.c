#include "Triangle.h"

int SDL_RenderTriangle(SDL_Renderer* renderer, SDL_Texture* texture, int x1, int y1, int x2, int y2, int x3, int y3, SDL_Color color) {
	#if SDL_MAJOR_VERSION >= 2 && (SDL_PATCHLEVEL >= 18 || SDL_MINOR_VERSION > 0)
	SDL_Vertex vertex_1 = {{(float)x1, (float)y1}, {color.r, color.g, color.b, color.a}, {1,1}};
	SDL_Vertex vertex_2 = {{(float)x2, (float)y2}, {color.r, color.g, color.b, color.a}, {1,1}};
	SDL_Vertex vertex_3 = {{(float)x3, (float)y3}, {color.r, color.g, color.b, color.a}, {1,1}};

	SDL_Vertex vertices[] = {vertex_1, vertex_2, vertex_3};
	
	return SDL_RenderGeometry(renderer, texture, vertices, 3, NULL, 0);
	#else
	Uint8 last_r, last_g, last_b, last_a;
	SDL_GetRenderDrawColor(renderer, &last_r, &last_g, &last_b, &last_a);

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	SDL_RenderDrawLine(renderer, x2, y2, x3, y3);
	SDL_RenderDrawLine(renderer, x3, y3, x1, y1);

	SDL_SetRenderDrawColor(renderer, last_r, last_g, last_b, last_a);

	return 0;
	#endif
}