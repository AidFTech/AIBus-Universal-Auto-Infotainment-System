#include <stdint.h>
#include <string>

#include "Settings_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../Serial_AIBus_Handler.h"

#include "../Locale/Locale.h"

#ifndef settings_clock_window_h
#define settings_clock_window_h

enum settings_clock_menu_t : uint8_t {
	SETTINGS_CLOCK_MENU_MAIN,
	SETTINGS_CLOCK_MENU_FORMAT,
	SETTINGS_CLOCK_MENU_AUTO
};

#define SETTINGS_CLOCK_MENU_LEN 4

class Settings_Clock_Window : public Settings_Window {
public:
	Settings_Clock_Window(AttributeList *attribute_list);

private:
	void initClockMain();
	void initClockFormat();
	void initClockAuto();

	void handleEnterButton();
	void handleBackButton();

	settings_clock_menu_t settings_clock_menu;
};

#endif