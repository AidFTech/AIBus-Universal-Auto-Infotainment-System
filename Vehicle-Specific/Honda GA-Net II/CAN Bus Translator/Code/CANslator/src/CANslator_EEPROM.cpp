#include "CANslator_EEPROM.h"

//Save CANslator settings to EEPROM.
void setCanslatorSettings(ParameterList* parameter_list) {
	canslator_eeprom_setting settings;
	settings.data.trim = parameter_list->special_trim;
	settings.data.transmission = parameter_list->transmission;

	settings.data.pushbutton = parameter_list->pushbutton;
	settings.data.drl_setting = parameter_list->drl_setting;

	settings.data.headlight_temp = parameter_list->headlight_temp_setting;

	settings.data.parking_lights = parameter_list->parking_lights;
	settings.data.auto_wiper = parameter_list->auto_wiper;
	settings.data.celsius = parameter_list->display_celsius;
	settings.data.wiper_door_off = parameter_list->wiper_door_off;

	settings.data.ambient_lighting = parameter_list->has_ambient_lighting;

	settings.data.ambient_light_brightness = parameter_list->ambient_light_brightness;
	settings.data.ambient_light_enable_door = parameter_list->ambient_light_enable_door;
	settings.data.ambient_light_enable_int = parameter_list->ambient_light_enable_int;

	settings.data.lanewatch_camera = parameter_list->has_lanewatch;
	settings.data.rearview_camera = parameter_list->has_rearview_camera;

	for(int i=0;i<sizeof(settings.raw);i+=1)
		EEPROM.write(i, settings.raw[i]);
}

//Load CANslator settings from EEPROM.
void getCanslatorSettings(ParameterList* parameter_list) {
	canslator_eeprom_setting settings;
	for(int i=0;i<sizeof(settings.raw);i+=1)
		settings.raw[i] = EEPROM.read(i);

	parameter_list->special_trim = settings.data.trim;
	parameter_list->transmission = settings.data.transmission;

	parameter_list->pushbutton = settings.data.pushbutton;
	parameter_list->drl_setting = settings.data.drl_setting;

	parameter_list->headlight_temp_setting = settings.data.headlight_temp;
	parameter_list->parking_lights = settings.data.parking_lights;
	parameter_list->auto_wiper = settings.data.auto_wiper;
	parameter_list->display_celsius = settings.data.celsius;
	parameter_list->wiper_door_off = settings.data.wiper_door_off;

	parameter_list->has_ambient_lighting = settings.data.ambient_lighting;

	parameter_list->ambient_light_brightness = settings.data.ambient_light_brightness;
	parameter_list->ambient_light_enable_int = settings.data.ambient_light_enable_int;
	parameter_list->ambient_light_enable_door = settings.data.ambient_light_enable_door;
}