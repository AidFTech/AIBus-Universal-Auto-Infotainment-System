#include <stdint.h>
#include <string>

#include "Settings_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../AIBus_Handler.h"

#ifndef settings_display_window_h
#define settings_display_window_h

#define SETTINGS_DISPLAY_MENU_MAIN 0
#define SETTINGS_DISPLAY_MENU_DAYNIGHT 1
#define SETTINGS_DISPLAY_MENU_UPPER 2

class Settings_Display_Window : public Settings_Window {
public:
	Settings_Display_Window(AttributeList *attribute_list);
private:
	void initSettingsMain();
	void initSettingsNight();

	void handleEnterButton();
	void handleBackButton();

	int8_t settings_display_menu;
};

#endif
