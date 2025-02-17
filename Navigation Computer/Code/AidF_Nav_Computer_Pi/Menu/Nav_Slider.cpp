#include "Nav_Slider.h"

NavSlider::NavSlider(AttributeList* attribute_list, const int16_t x, const int16_t y, const uint16_t w, const uint16_t h, const uint8_t max, const uint8_t value, const bool vertical) {
	this->attribute_list = attribute_list;
	this->renderer = attribute_list->renderer;
	
	this->x = x;
	this->y = y;
	this->h = h;
	this->w = w;

	this->max = max;
	this->value = value;
	this->vertical = vertical;
}

void NavSlider::setValue(const uint8_t new_value) {
	if(new_value >= 0 && new_value <= max)
		this->value = new_value;

	if(this->value_text != NULL)
		this->value_text->setText(std::to_string(int(this->value)));
}

//Get the set value.
uint8_t NavSlider::getValue() {
	return this->value;
}

//Get the maximum value.
uint8_t NavSlider::getMax() {
	return this->max;
}

void NavSlider::setSelected(const bool selected) {
	this->selected = selected;
}

bool NavSlider::getSelected() {
	return this->selected;
}

void NavSlider::incrementValue(const uint8_t inc) {
	if(int(this->value) + inc <= this->max)
		this->value += inc;
		
	if(this->value_text != NULL)
		this->value_text->setText(std::to_string(int(this->value)));
}

void NavSlider::decrementValue(const uint8_t inc) {
	if(int(this->value) - inc >= 0)
		this->value -= inc;
		
	if(this->value_text != NULL)
		this->value_text->setText(std::to_string(int(this->value)));
}

void NavSlider::drawSlider() {
	uint8_t last_r, last_g, last_b, last_a;
	SDL_GetRenderDrawColor(renderer, &last_r, &last_g, &last_b, &last_a);

	AidFColorProfile* color_profile = attribute_list->color_profile;
	const uint32_t button_color = color_profile->button, outline_color = color_profile->outline, text_color = color_profile->text, selection_color = color_profile->selection;

	SDL_Rect main_rect = {x, y, w, h};
	SDL_SetRenderDrawColor(renderer, getRedComponent(button_color), getGreenComponent(button_color), getBlueComponent(button_color), getAlphaComponent(button_color));
	SDL_RenderFillRect(renderer, &main_rect);

	uint16_t slider_thick = w/max;
	if(vertical)
		slider_thick = h/max;

	if(slider_thick < 10)
		slider_thick = 10;

	int16_t slider_x = x, slider_y = y;
	uint16_t slider_w = w, slider_h = h;

	if(vertical) {
		slider_h = slider_thick;
		slider_y = y + h/max*value;

		if(slider_y + slider_h > y + h)
			slider_y = y + h - slider_h;
	} else {
		slider_w = slider_thick;
		slider_x = x + w/max*value;

		if(slider_x + slider_w > x + w)
			slider_x = x + w - slider_w;
	}

	SDL_Rect slider_rect = {slider_x, slider_y, slider_w, slider_h};

	if(!selected)
		SDL_SetRenderDrawColor(renderer, getRedComponent(text_color), getGreenComponent(text_color), getBlueComponent(text_color), getAlphaComponent(text_color));
	else
		SDL_SetRenderDrawColor(renderer, getRedComponent(selection_color), getGreenComponent(selection_color), getBlueComponent(selection_color), getAlphaComponent(selection_color));

	SDL_RenderFillRect(renderer, &slider_rect);

	if(!selected)
		SDL_SetRenderDrawColor(renderer, getRedComponent(outline_color), getGreenComponent(outline_color), getBlueComponent(outline_color), getAlphaComponent(outline_color));
	else
		SDL_SetRenderDrawColor(renderer, getRedComponent(selection_color), getGreenComponent(selection_color), getBlueComponent(selection_color), getAlphaComponent(selection_color));
	SDL_RenderDrawRect(renderer, &slider_rect);
	SDL_RenderDrawRect(renderer, &main_rect);

	SDL_SetRenderDrawColor(renderer, last_r, last_g, last_b, last_a);
	
	if(this->value_text != NULL)
		this->value_text->drawText();
}

void NavSlider::setValueText(TextBox* value_text) {
	this->value_text = value_text;
	
	if(this->value_text != NULL)
		this->value_text->setText(std::to_string(int(this->value)));
}
