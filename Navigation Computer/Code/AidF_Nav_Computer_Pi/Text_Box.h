#include <stdint.h>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "AidF_Color_Profile.h"
#include "Symbol/Symbol_Handler.h"

#ifndef text_box_h
#define text_box_h

enum align_h_t : uint8_t {
	ALIGN_H_L,
	ALIGN_H_C,
	ALIGN_H_R
};

enum align_v_t : uint8_t {
	ALIGN_V_T,
	ALIGN_V_M,
	ALIGN_V_B
};

class TextBox {
public:
	TextBox(SDL_Renderer* renderer,
			const int16_t x,
			const int16_t y,
			const uint16_t w,
			const uint16_t h,
			const align_h_t h_indent,
			const align_v_t v_indent,
			const uint8_t size,
			uint32_t* text_color);
	~TextBox();
	TextBox(const TextBox &copy);
	TextBox operator=(const TextBox &copy);

	void setRenderer(SDL_Renderer* renderer);

	void setWidth(const uint16_t w);
	void setHeight(const uint16_t h);

	void setText(std::string text);
	void setText(const char* text);
	void renderText();
	
	std::string getText();
	virtual void drawText();
protected:
	virtual void copy(const TextBox &copy);

	SDL_Renderer* renderer;
	SDL_Texture* texture = NULL;

	int16_t x, y;
	uint16_t w, h, text_w, text_h;

	std::string text = "";

	align_h_t h_indent;
	align_v_t v_indent;
	uint8_t size;
	uint32_t* color;

	SymbolHandler symbol_handler;
};

class AngledTextBox : public TextBox {
public:
	AngledTextBox(SDL_Renderer* renderer,
			const int16_t x,
			const int16_t y,
			const uint16_t w,
			const uint16_t h,
			const align_h_t h_indent,
			const align_v_t v_indent,
			const uint8_t size,
			const double angle,
			uint32_t* text_color);
	
	AngledTextBox(const AngledTextBox &copy);
	AngledTextBox operator=(const TextBox &copy);
	AngledTextBox operator=(const AngledTextBox &copy);

	void drawText();
private:
	double angle = 0;
};

#endif