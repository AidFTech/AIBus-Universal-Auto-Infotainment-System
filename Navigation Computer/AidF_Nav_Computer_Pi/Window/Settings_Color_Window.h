#include <stdint.h>
#include <string>

#include "Settings_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../AIBus_Handler.h"

#ifndef settings_color_window_h
#define settings_color_window_h

#define SETTINGS_COLOR_MENU_MAIN 0
#define SETTINGS_COLOR_MENU_PICKER 1

class Settings_Color_Window : public Settings_Window {
public:
	Settings_Color_Window(AttributeList *attribute_list);
private:
	void handleEnterButton();

	void initColorMainMenu();
	
	int8_t color_menu;
};

#endif
