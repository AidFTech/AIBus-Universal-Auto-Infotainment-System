#include <stdint.h>
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../AidF_Color_Profile.h"
#include "Triangle.h"

#ifndef symbol_handler_h
#define symbol_handler_h

#define SYM_PER "##  "
#define SYM_UP "#UP "
#define SYM_DOWN "#DN "
#define SYM_FWD "#FWD"
#define SYM_REV "#REV"
#define SYM_FF "#FF "
#define SYM_REW "#REW"

#define SYM_RIGHTOFF "#ROF"
#define SYM_RIGHTON "#RON"

class SymbolHandler {
public:
	SymbolHandler();
	~SymbolHandler();
	std::string removeSymbolText(std::string text);
	void drawSymbols(SDL_Renderer* renderer, SDL_Texture* texture, TTF_Font* font, std::string text, const int16_t start_x, const int16_t start_y, const uint16_t size, uint32_t* text_color);
private:
	uint16_t* symbol_pos;
	std::string* symbol;
	uint16_t symbol_count = 0;
};

#endif