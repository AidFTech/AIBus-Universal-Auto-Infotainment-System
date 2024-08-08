#include "Symbol_Handler.h"

SymbolHandler::SymbolHandler() {
	this->symbol_pos = new uint16_t[0];
	this->symbol = new std::string[0];
}

SymbolHandler::~SymbolHandler() {
	delete[] this->symbol_pos;
	delete[] this->symbol;
}

std::string SymbolHandler::removeSymbolText(std::string text) {
	std::string the_return = text;
	std::vector<uint16_t> sym_pos_v(0);
	std::vector<std::string> sym_v(0);

	uint16_t pos = 0;
	while(the_return.find('#', pos) != std::string::npos && pos < the_return.size()) {
		const uint8_t found_pos = the_return.find('#', pos);
		sym_pos_v.push_back(found_pos);
		
		sym_v.push_back(the_return.substr(found_pos, 4));

		pos = sym_pos_v.back()+1;
		if(the_return.substr(found_pos, 4).compare(SYM_PER) == 0)
			the_return.replace(found_pos, 4, "#");
		else if(the_return.substr(found_pos, 4).compare(SYM_FF) == 0 || the_return.substr(found_pos, 4).compare(SYM_REW) == 0)
			the_return.replace(found_pos, 4, "    ");
		//else if(the_return.substr(found_pos, 4).compare(SYM_RIGHTOFF) == 0 || the_return.substr(found_pos, 4).compare(SYM_RIGHTON) == 0)
		//	the_return.replace(found_pos, 4, " ");
		else
			the_return.replace(found_pos, 4, "  ");
	}

	delete[] this->symbol_pos;
	delete[] this->symbol;

	this->symbol_pos = new uint16_t[sym_pos_v.size()];
	this->symbol = new std::string[sym_v.size()];

	for(uint16_t i=0;i<sym_pos_v.size();i+=1)
		this->symbol_pos[i] = sym_pos_v[i];
	
	for(uint16_t i=0;i<sym_v.size();i+=1)
		this->symbol[i] = sym_v[i];
	
	this->symbol_count = sym_v.size();

	return the_return;
}

void SymbolHandler::drawSymbols(SDL_Renderer* renderer, SDL_Texture* texture, TTF_Font* font, std::string text, const int16_t start_x, const int16_t start_y, const uint16_t size, uint32_t* text_color){
	SDL_Color black = {0,0,0,0xFF};

	for(uint16_t s=0;s<symbol_count;s+=1) {
		uint16_t sym_x = start_x;

		if(this->symbol_pos[s] > 0) {
			std::string subtext = text.substr(0, this->symbol_pos[s]);

			int w = 0, h = 0;
			TTF_SizeText(font, subtext.c_str(), &w, &h);
			sym_x = start_x + w;
		}
		
		std::string sym_string = " ";

		if(symbol[s].compare(SYM_UP) == 0)
			SDL_RenderTriangle(renderer, NULL, sym_x + size/2, start_y, sym_x, start_y + size, sym_x + size, start_y + size, getSDLColor(*text_color));
		else if(symbol[s].compare(SYM_DOWN) == 0)
			SDL_RenderTriangle(renderer, NULL, sym_x + size/2, start_y + size, sym_x, start_y, sym_x + size, start_y, getSDLColor(*text_color));
		else if(symbol[s].compare(SYM_REV) == 0)
			SDL_RenderTriangle(renderer, NULL, sym_x, start_y + size/2, sym_x + size, start_y, sym_x + size, start_y + size, getSDLColor(*text_color));
		else if(symbol[s].compare(SYM_FWD) == 0)
			SDL_RenderTriangle(renderer, NULL, sym_x + size, start_y + size/2, sym_x, start_y, sym_x, start_y + size, getSDLColor(*text_color));
		else if(symbol[s].compare(SYM_REW) == 0) {
			SDL_RenderTriangle(renderer, NULL, sym_x, start_y + size/2, sym_x + size, start_y, sym_x + size, start_y + size, getSDLColor(*text_color));
			SDL_RenderTriangle(renderer, NULL, sym_x+size, start_y + size/2, sym_x + 2*size, start_y, sym_x + 2*size, start_y + size, getSDLColor(*text_color));
		} else if(symbol[s].compare(SYM_FF) == 0) {
			SDL_RenderTriangle(renderer, NULL, sym_x + size, start_y + size/2, sym_x, start_y, sym_x, start_y + size, getSDLColor(*text_color));
			SDL_RenderTriangle(renderer, NULL, sym_x + 2*size, start_y + size/2, sym_x + size, start_y, sym_x + size, start_y + size, getSDLColor(*text_color));
		} else if(symbol[s].compare(SYM_RIGHTOFF) == 0 || symbol[s].compare(SYM_RIGHTON) == 0) {
			const uint8_t offset = 7;

			if(symbol[s].compare(SYM_RIGHTON) == 0) {
				SDL_SetRenderDrawColor(renderer, getRedComponent(*text_color), getGreenComponent(*text_color), getBlueComponent(*text_color), getAlphaComponent(*text_color));
				SDL_Rect rect = {sym_x + 7 + offset, start_y + 7 + offset, size - 14 - offset, size - 14 - offset};
				SDL_RenderFillRect(renderer, &rect);
			}

			SDL_SetRenderDrawColor(renderer, getRedComponent(*text_color), getGreenComponent(*text_color), getBlueComponent(*text_color), getAlphaComponent(*text_color));
			for(uint8_t i=0;i<5;i+=1) {
				SDL_Rect rect = {sym_x+i + offset, start_y+i + offset, size-i*2 - offset, size-i*2 - offset};
				SDL_RenderDrawRect(renderer, &rect);
			}
		}
	}
}
