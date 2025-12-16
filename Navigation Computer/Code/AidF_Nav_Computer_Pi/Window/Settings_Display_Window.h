#include <stdint.h>
#include <string>

#include "Settings_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../Serial_AIBus_Handler.h"

#include "../Locale/Locale.h"

#ifndef settings_display_window_h
#define settings_display_window_h

enum settings_display_menu_t : uint8_t {
	SETTINGS_DISPLAY_MENU_MAIN,
	SETTINGS_DISPLAY_MENU_DAYNIGHT,
	SETTINGS_DISPLAY_MENU_UPPER
};

class Settings_Display_Window : public Settings_Window {
public:
	Settings_Display_Window(AttributeList *attribute_list);
private:
	void initSettingsMain();
	void initSettingsNight();

	void handleEnterButton();
	void handleBackButton();

	settings_display_menu_t settings_display_menu;
};

#endif
