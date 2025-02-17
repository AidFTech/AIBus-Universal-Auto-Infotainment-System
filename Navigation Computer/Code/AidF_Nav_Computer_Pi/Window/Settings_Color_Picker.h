#include <stdint.h>
#include <string>

#include "Nav_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Menu/Nav_Slider.h"
#include "../Text_Box.h"
#include "../AIBus_Handler.h"

#ifndef settings_color_picker_h
#define settings_color_picker_h

#define COLOR_OPTION_BACKGROUND 1
#define COLOR_OPTION_TEXT 2
#define COLOR_OPTION_BUTTON 3
#define COLOR_OPTION_SELECTION 4
#define COLOR_OPTION_HEADERBAR 5
#define COLOR_OPTION_OUTLINE 6

#define SLIDER_COUNT 7

class Color_Picker_Window : public NavWindow {
public:
	Color_Picker_Window(AttributeList* attribute_list);
	~Color_Picker_Window();

	void drawWindow();
	void refreshWindow();
	bool handleAIBus(AIData* ai_d);

private:
	int16_t getSliderY(const uint8_t s);

	void handleEnterButton();
	void incrementColorElement();
	void decrementColorElement();

	void incrementSlider();
	void decrementSlider();
	void setSlider(const uint8_t slider_value, const uint8_t slider);

	void setDayNightOption();
	void setSliderValues();
	void refreshDayNightProfile();

	TextBox *title_block;
	NavMenu *color_menu;

	NavSlider* color_slider[SLIDER_COUNT];
	TextBox* slider_value[SLIDER_COUNT];
	TextBox* slider_variable[SLIDER_COUNT];

	uint8_t last_day_night_setting; //Last selected day/night setting.

	uint8_t color_option = COLOR_OPTION_BACKGROUND; //The selected color attribute to change.

	bool color_option_active = false; //True if the color sliders are selected, false if not.
	bool color_slider_active = false; //True whether the slider is active.
	uint8_t color_element = 0; //The selected color component.
	
	bool solid_br = false;

	uint32_t* selected_color;
};

#endif
