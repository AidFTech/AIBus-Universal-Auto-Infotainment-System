#include <stdint.h>
#include <string>
#include <vector>

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

	bool handleAIBus(AIData* ai_msg);
private:
	vector<uint8_t> setting_id = vector<uint8_t>(0);
	vector<string> setting_name = vector<string>(0);

	void handleEnterButton();

	void addNestedRequestOption(const string option_name, const uint8_t device_id);
	void sendSettingsMenuRequest(const uint8_t receiver);
};

#endif
