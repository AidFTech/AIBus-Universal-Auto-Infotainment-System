#include "CANslator_EEPROM.h"

//Save CANslator settings to EEPROM.
void setCanslatorSettings(ParameterList* parameter_list) {
	EEPROM.write(TRIM_SETTING, parameter_list->trim);
	
	uint8_t main_settings = parameter_list->headlight_temp_setting;
	
	if(parameter_list->parking_lights)
		main_settings |= CANSLATOR_SETTINGS_PARKING_LIGHTS;
	if(parameter_list->auto_wiper)
		main_settings |= CANSLATOR_SETTINGS_AUTO_WIPER;
	if(parameter_list->display_celsius)
		main_settings |= CANSLATOR_SETTINGS_CELSIUS;
	if(parameter_list->wiper_door_off)
		main_settings |= CANSLATOR_SETTINGS_WIPER_DOOR_OFF;

	EEPROM.write(COMFORT_SETTINGS_1, main_settings);
}

//Load CANslator settings from EEPROM.
void getCanslatorSettings(ParameterList* parameter_list) {
	const uint8_t trim = EEPROM.read(TRIM_SETTING);
	parameter_list->trim = (trim_t)trim;

	const uint8_t comfort_settings_1 = EEPROM.read(COMFORT_SETTINGS_1);
	parameter_list->headlight_temp_setting = (headlight_temp_t)(comfort_settings_1&CANSLATOR_SETTINGS_HEADLIGHT_TEMP);
	parameter_list->parking_lights = (comfort_settings_1&CANSLATOR_SETTINGS_PARKING_LIGHTS) != 0;
	parameter_list->auto_wiper = (comfort_settings_1&CANSLATOR_SETTINGS_AUTO_WIPER) != 0;
	parameter_list->display_celsius = (comfort_settings_1&CANSLATOR_SETTINGS_CELSIUS) != 0;
	parameter_list->wiper_door_off = (comfort_settings_1&CANSLATOR_SETTINGS_WIPER_DOOR_OFF) != 0;
}