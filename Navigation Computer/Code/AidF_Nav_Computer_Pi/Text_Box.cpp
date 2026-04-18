#include "Text_Box.h"
#include <SDL2/SDL_ttf.h>

TextBox::TextBox(SDL_Renderer* renderer,
				const int16_t x,
				const int16_t y,
				const uint16_t w,
				const uint16_t h,
				const align_h_t h_indent,
				const align_v_t v_indent,
				const uint8_t size,
				uint32_t* text_color) {
	this->renderer = renderer;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;

	this->h_indent = h_indent;
	this->v_indent = v_indent;

	this->size = size;

	this->color = text_color;

	this->texture = NULL;
}

TextBox::~TextBox() {
	if(this->texture != NULL)
		SDL_DestroyTexture(this->texture);
}

TextBox::TextBox(const TextBox &copy) {
	this->renderer = copy.renderer;
	this->x = copy.x;
	this->y = copy.y;
	this->w = copy.w;
	this->h = copy.h;
	this->h_indent = copy.h_indent;
	this->v_indent = copy.v_indent;
	this->size = copy.size;
	this->color = copy.color;

	this->text = copy.text;
	this->renderText();
}

TextBox TextBox::operator=(const TextBox &copy) {
	this->copy(copy);
	return *this;
}

void TextBox::setRenderer(SDL_Renderer* renderer) {
	this->renderer = renderer;
}

void TextBox::setWidth(const uint16_t w) {
	this->w = w;
	this->renderText();
}

void TextBox::setHeight(const uint16_t h) {
	this->h = h;
	this->renderText();
}

string TextBox::getText() {
	return this->text;
}

void TextBox::setText(const char* text) {
	this->setText(string(text));
}

void TextBox::setText(string text) {
	this->text = text;

	if(text.size() <= 0)
		this->text = " ";

	if(this->renderer == NULL)
		return;

	this->text = symbol_handler.removeSymbolText(this->text);

	this->renderText();
}

void TextBox::renderText() {
	if(this->text.size() <= 0)
		return;
	
	string text = asciiToUTF8(this->text);

	TTF_Font* AidF_Font = TTF_OpenFont("AidF Font.ttf", this->size);
	if(AidF_Font == NULL)
		return;
	
	SDL_Surface* text_surface = TTF_RenderUTF8_Solid(AidF_Font, text.c_str(), getSDLColor(*color));

	this->texture = SDL_CreateTextureFromSurface(renderer, text_surface);
	this->text_w = text_surface->w;
	this->text_h = text_surface->h;

	while(this->text_w > this->w) {
		text.pop_back();
		text_surface = TTF_RenderUTF8_Solid(AidF_Font, (text).c_str(), getSDLColor(*color));
		this->texture = SDL_CreateTextureFromSurface(renderer, text_surface);
		this->text_w = text_surface->w;
		this->text_h = text_surface->h;
	}

	SDL_FreeSurface(text_surface);
	TTF_CloseFont(AidF_Font);
}

void TextBox::drawText() { 
	if (this->texture == NULL || this->renderer == NULL)
		return;
	
	int16_t text_x = this->x, text_y = this->y;

	switch(this->h_indent) {
		case ALIGN_H_C:
			text_x = this->x + this->w/2 - text_w/2;
			break;
		case ALIGN_H_R:
			text_x = this->x + this->w - text_w;
			break;
		case ALIGN_H_L:
			break;
	}

	switch(this->v_indent) {
		case ALIGN_V_M:
			text_y = this->y + this->h/2 - text_h/2;
			break;
		case ALIGN_V_B:
			text_y = this->y + this->h - text_h;
			break;
		case ALIGN_V_T:
			break;
	}

	SDL_Rect text_rect = {text_x, text_y, text_w, text_h};
	SDL_RenderCopy(renderer, texture, NULL, &text_rect);

	TTF_Font* AidF_Font = TTF_OpenFont("AidF Font.ttf", this->size);
	symbol_handler.drawSymbols(this->renderer, this->texture, AidF_Font, this->text, text_x, text_y, this->size, this->color);
	TTF_CloseFont(AidF_Font);
}

//Copy parameters from another text box.
void TextBox::copy(const TextBox &copy) {
	this->renderer = copy.renderer;
	this->x = copy.x;
	this->y = copy.y;
	this->w = copy.w;
	this->h = copy.h;
	this->h_indent = copy.h_indent;
	this->v_indent = copy.v_indent;
	this->size = copy.size;
	this->color = copy.color;

	this->text = copy.text;
	this->renderText();
}

WrapTextBox::WrapTextBox(SDL_Renderer* renderer,
			const int16_t x,
			const int16_t y,
			const uint16_t w,
			const uint16_t h,
			const uint8_t size,
			uint32_t* text_color) : TextBox(renderer, x, y, w, h, ALIGN_H_L, ALIGN_V_T, size, text_color) {
		
	}

//Rehnder the text in the main window.
void WrapTextBox::renderText() {
	if(this->text.size() <= 0)
		return;

	string text = asciiToUTF8(this->text);

	TTF_Font* AidF_Font = TTF_OpenFont("AidF Font.ttf", this->size);
	SDL_Surface* text_surface = TTF_RenderUTF8_Solid(AidF_Font, text.c_str(), getSDLColor(*color));

	int text_w = text_surface->w;
	int text_h = text_surface->h;
	string newline = "";
	vector<string> text_vec(0);

	do {
		newline = "";
		while(text_w > this->w) {
			newline.insert(newline.begin(), text[text.length()-1]);
			text.pop_back();

			text_surface =  TTF_RenderUTF8_Solid(AidF_Font, (text).c_str(), getSDLColor(*color));

			text_w = text_surface->w;
			text_h = text_surface->h;
		}

		//Create the space split only if the new line was filled.
		if(newline.length() > 0) {
			int c = text.length() - 1;
			while(c >= 0 && text[c] > ' ') {
				newline.insert(newline.begin(), text[c]);
				text.pop_back();
				c -= 1;
			}
		}
		
		text_vec.push_back(text);
		text = newline;

		if(newline.length() <= 0)
			break;
	} while(text.length() > 0 && text_h < this->h);

	if(newline.length() > 0)
		text_vec.push_back(newline);

	string new_text = "";
	for(int i=0;i<text_vec.size();i+=1)
		new_text += text_vec.at(i) + '\n';

	text_surface = TTF_RenderUTF8_Blended_Wrapped(AidF_Font, new_text.c_str(), getSDLColor(*color), 0);
	this->texture = SDL_CreateTextureFromSurface(renderer, text_surface);
	this->text_w = text_surface->w;
	this->text_h = text_surface->h;

	SDL_FreeSurface(text_surface);
	TTF_CloseFont(AidF_Font);
}

AngledTextBox::AngledTextBox(SDL_Renderer* renderer,
			const int16_t x,
			const int16_t y,
			const uint16_t w,
			const uint16_t h,
			const align_h_t h_indent,
			const align_v_t v_indent,
			const uint8_t size,
			const double angle,
			uint32_t* text_color) : TextBox(renderer, x, y, w, h, h_indent, v_indent, size, text_color) {
	this->angle = angle;
}

AngledTextBox::AngledTextBox(const AngledTextBox &copy) : TextBox(copy) {
	this->angle = copy.angle;
}

AngledTextBox AngledTextBox::operator=(const TextBox &copy) {
	this->copy(copy);
	this->angle = 0;
	return *this;
}

AngledTextBox AngledTextBox::operator=(const AngledTextBox &copy) {
	this->copy(copy);
	this->angle = copy.angle;
	return *this;
}

//Draw the text.
void AngledTextBox::drawText() {
	if (this->texture == NULL || this->renderer == NULL)
		return;
	
	int16_t text_x = this->x, text_y = this->y;

	switch(this->h_indent) {
		case ALIGN_H_C:
			text_x = this->x + this->w/2 - text_w/2;
			break;
		case ALIGN_H_R:
			text_x = this->x + this->w - text_w;
			break;
		case ALIGN_H_L:
			break;
	}

	switch(this->v_indent) {
		case ALIGN_V_M:
			text_y = this->y + this->h/2 - text_h/2;
			break;
		case ALIGN_V_B:
			text_y = this->y + this->h - text_h;
			break;
		case ALIGN_V_T:
			break;
	}

	SDL_Rect text_rect = {text_x, text_y, text_w, text_h};
	SDL_Point text_center = {0, 0};
	SDL_RenderCopyEx(renderer, texture, NULL, &text_rect, this->angle*180/M_PI, &text_center, SDL_FLIP_NONE);

	TTF_Font* AidF_Font = TTF_OpenFont("AidF Font.ttf", this->size);
	symbol_handler.drawSymbols(this->renderer, this->texture, AidF_Font, this->text, text_x, text_y, this->size, this->color);
	TTF_CloseFont(AidF_Font);
}

//Convert a string to UTF8.
string asciiToUTF8(const string ascii_str) {
	string utf_str = ascii_str;
	int u = 0;

	for(int i=0;i<ascii_str.length();i+=1) {
		if((ascii_str[i]&0x80) != 0) {
			//Check whether this is already UTF-8 formatted. Following character should have the same condition.
			bool is_utf8 = false;
			if(i <= ascii_str.length() - 2 && (ascii_str[i+1]&0x80) != 0)
				is_utf8 = true;
			else if(i > 0 && (ascii_str[i-1]&0x80) != 0)
				is_utf8 = true;

			//If there weren't any flags to indicate UTF8, correct it.
			if(!is_utf8) {
				if((ascii_str[i]&0x40) == 0)
					utf_str.insert(utf_str.begin() + u, 0xC2);
				else {
					utf_str[u] &= (~0x40);
					utf_str.insert(utf_str.begin() + u, 0xC3);
				}
				u += 1;
			}
		}
		u += 1;
	}

	return utf_str;
}