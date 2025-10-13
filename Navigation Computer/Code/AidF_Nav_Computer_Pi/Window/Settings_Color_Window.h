#include <stdint.h>
#include <string>
#include <vector>

#include "Settings_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../AIBus_Handler.h"
#include "../Ini_Color_Preset.h"

#include "../Locale/Locale.h"

#ifndef settings_color_window_h
#define settings_color_window_h

enum settings_color_main_t : uint8_t {
	SETTINGS_COLOR_MENU_MAIN,
	SETTINGS_COLOR_MENU_PICKER
};

class Settings_Color_Window : public Settings_Window {
public:
	Settings_Color_Window(AttributeList *attribute_list);
private:
	void handleEnterButton();

	void initColorMainMenu();
	
	settings_color_main_t color_menu;
};

#endif
