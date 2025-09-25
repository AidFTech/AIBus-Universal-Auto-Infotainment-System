#include "Text_Box.h"

TextBox::TextBox(SDL_Renderer* renderer,
				const int16_t x,
				const int16_t y,
				const uint16_t w,
				const uint16_t h,
				const uint8_t h_indent,
				const uint8_t v_indent,
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
	delete this->symbol_handler;

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

std::string TextBox::getText() {
	return this->text;
}

void TextBox::setText(const char* text) {
	this->setText(std::string(text));
}

void TextBox::setText(std::string text) {
	this->text = text;

	if(text.size() <= 0)
		this->text = " ";

	if(this->renderer == NULL)
		return;

	this->text = symbol_handler->removeSymbolText(this->text);

	this->renderText();
}

void TextBox::renderText() {
	if(this->text.size() <= 0)
		return;
	
	std::string text = this->text;

	TTF_Font* AidF_Font = TTF_OpenFont("AidF Font.ttf", this->size);
	SDL_Surface* text_surface = TTF_RenderText_Solid(AidF_Font, text.c_str(), getSDLColor(*color));

	this->texture = SDL_CreateTextureFromSurface(renderer, text_surface);
	this->text_w = text_surface->w;
	this->text_h = text_surface->h;

	while(this->text_w > this->w) {
		text.pop_back();
		text_surface = TTF_RenderText_Solid(AidF_Font, (text).c_str(), getSDLColor(*color));
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
	}

	switch(this->v_indent) {
		case ALIGN_V_M:
			text_y = this->y + this->h/2 - text_h/2;
			break;
		case ALIGN_V_B:
			text_y = this->y + this->h - text_h;
			break;
	}

	SDL_Rect text_rect = {text_x, text_y, text_w, text_h};
	SDL_RenderCopy(renderer, texture, NULL, &text_rect);

	TTF_Font* AidF_Font = TTF_OpenFont("AidF Font.ttf", this->size);
	symbol_handler->drawSymbols(this->renderer, this->texture, AidF_Font, this->text, text_x, text_y, this->size, this->color);
	TTF_CloseFont(AidF_Font);
}

AngledTextBox::AngledTextBox(SDL_Renderer* renderer,
			const int16_t x,
			const int16_t y,
			const uint16_t w,
			const uint16_t h,
			const uint8_t h_indent,
			const uint8_t v_indent,
			const uint8_t size,
			const double angle,
			uint32_t* text_color) : TextBox(renderer, x, y, w, h, h_indent, v_indent, size, text_color) {
	this->angle = angle;
}

AngledTextBox::AngledTextBox(const AngledTextBox &copy) : TextBox(copy) {
	this->angle = copy.angle;
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
	}

	switch(this->v_indent) {
		case ALIGN_V_M:
			text_y = this->y + this->h/2 - text_h/2;
			break;
		case ALIGN_V_B:
			text_y = this->y + this->h - text_h;
			break;
	}

	SDL_Rect text_rect = {text_x, text_y, text_w, text_h};
	SDL_Point text_center = {0, 0};
	SDL_RenderCopyEx(renderer, texture, NULL, &text_rect, this->angle*180/M_PI, &text_center, SDL_FLIP_NONE);

	TTF_Font* AidF_Font = TTF_OpenFont("AidF Font.ttf", this->size);
	symbol_handler->drawSymbols(this->renderer, this->texture, AidF_Font, this->text, text_x, text_y, this->size, this->color);
	TTF_CloseFont(AidF_Font);
}