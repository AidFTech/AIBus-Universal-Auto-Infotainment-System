#include <stdint.h>
#include <SDL2/SDL.h>

#include "../Text_Box.h"
#include "../Window/Attribute_List.h"
#include "../AidF_Color_Profile.h"

#ifndef nav_slider_h
#define nav_slider_h

class NavSlider {
public:
	NavSlider(AttributeList* attribute_list, const int16_t x, const int16_t y, const uint16_t w, const uint16_t h, const uint8_t max, const uint8_t value, const bool vertical);

	void setValue(const uint8_t new_value);
	uint8_t getValue();

	void incrementValue(const uint8_t inc);
	void decrementValue(const uint8_t inc);

	void setSelected(const bool selected);
	bool getSelected();

	void drawSlider();
	
	void setValueText(TextBox* value_text);
private:
	AttributeList* attribute_list;
	SDL_Renderer* renderer;
	
	TextBox* value_text = NULL;

	int16_t x, y;
	uint16_t w, h;

	uint8_t max, value;

	bool vertical;

	bool selected = false;
};

#endif
