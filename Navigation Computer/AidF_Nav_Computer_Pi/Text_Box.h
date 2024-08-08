#include <stdint.h>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "AidF_Color_Profile.h"
#include "Symbol/Symbol_Handler.h"

#ifndef text_box_h
#define text_box_h

#define ALIGN_H_L 0
#define ALIGN_H_C 1
#define ALIGN_H_R 2

#define ALIGN_V_T 0
#define ALIGN_V_M 1
#define ALIGN_V_B 2

class TextBox {
public:
	TextBox(SDL_Renderer* renderer,
			const int16_t x,
			const int16_t y,
			const uint16_t w,
			const uint16_t h,
			const uint8_t h_indent,
			const uint8_t v_indent,
			const uint8_t size,
			uint32_t* text_color);
	~TextBox();

	void setRenderer(SDL_Renderer* renderer);

	void setWidth(const uint16_t w);
	void setHeight(const uint16_t h);

	void setText(std::string text);
	void setText(const char* text);
	void renderText();
	
	std::string getText();
	void drawText();
private:
	SDL_Renderer* renderer;
	SDL_Texture* texture = NULL;

	int16_t x, y;
	uint16_t w, h, text_w, text_h;

	std::string text = "";

	uint8_t h_indent, v_indent, size;
	uint32_t* color;

	SymbolHandler* symbol_handler = new SymbolHandler();
};

#endif