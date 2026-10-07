#include <stdint.h>
#include <string>

#include "Settings_Window.h"
#include "Settings_Secret_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../Serial_AIBus_Handler.h"

#ifndef settings_ext_window_h
#define settings_ext_window_h

class Settings_Ext_Window : public Settings_Window {
public:
	Settings_Ext_Window(AttributeList *attribute_list);

	void exitWindow();

	void drawWindow();
private:
	void handleEnterButton();
	void handleBackButton();

	bool exited = false;
};

class Settings_Secret_Ext_Window : public Settings_Ext_Window {
public:
	Settings_Secret_Ext_Window(AttributeList* attribute_list) : Settings_Ext_Window(attribute_list) {
		this->back_index = NEXT_WINDOW_SETTINGS_SECRET;
	}
};

#endif
