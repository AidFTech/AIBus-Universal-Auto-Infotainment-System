#include <stdint.h>
#include <string>

#include "Settings_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../Saved_Settings.h"

#include "../Locale/Locale.h"
#include "../Vehicle_Information/Vehicle_Info_Parameters.h"

#ifndef settings_info_window_h
#define settings_info_window_h

enum settings_info_menu_t : uint8_t {
	SETTINGS_INFO_MENU_MAIN,
	SETTINGS_INFO_MENU_PARAM,
};

class Settings_Info_Window : public Settings_Window {
public:
	Settings_Info_Window(AttributeList *attribute_list, InfoParameters *info_parameters);

private:
	void initInfoMenuMain();
	void initParamSettingsMenu(const uint8_t active_param);

	void handleEnterButton();
	void handleBackButton();

	void saveParamSettings();

	settings_info_menu_t settings_info_menu = SETTINGS_INFO_MENU_MAIN;
	uint8_t active_param = 0;

	InfoParameters* info_parameters;
};

#endif