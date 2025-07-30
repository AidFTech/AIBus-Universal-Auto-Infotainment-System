#include <stdint.h>
#include <string>

#include "Settings_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../AIBus_Handler.h"

#ifndef settings_ext_window_h
#define settings_ext_window_h

class Settings_Ext_Window : public Settings_Window {
public:
	Settings_Ext_Window(AttributeList *attribute_list);

	void exitWindow();
private:
	void handleEnterButton();
	void handleBackButton();
};

#endif
