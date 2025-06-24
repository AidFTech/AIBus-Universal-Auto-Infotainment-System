#include <stdint.h>
#include <string>

#include "Settings_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../AIBus_Handler.h"

#ifndef settings_clock_window_h
#define settings_clock_window_h

#define SETTINGS_CLOCK_MENU_MAIN 0
#define SETTINGS_CLOCK_MENU_FORMAT 1
#define SETTINGS_CLOCK_MENU_AUTO 2

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

	int8_t settings_clock_menu;
};

#endif