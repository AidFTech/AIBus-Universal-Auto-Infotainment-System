#include <stdint.h>
#include <string>

#include "Settings_Window.h"

#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../Serial_AIBus_Handler.h"

#include "../Locale/Locale.h"

#ifndef settings_main_window_h
#define settings_main_window_h

class Settings_Main_Window : public Settings_Window {
public:
	Settings_Main_Window(AttributeList *attribute_list);
private:
	void handleEnterButton();

	void sendSettingsMenuRequest(const uint8_t receiver);
};

#endif
