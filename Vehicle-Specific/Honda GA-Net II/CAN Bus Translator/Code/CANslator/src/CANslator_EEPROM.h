#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

#include "Parameter_List.h"

#ifndef canslator_eeprom_h
#define canslator_eeprom_h

union canslator_eeprom_setting {
	struct data {
		special_trim_t trim: 2;
		transmission_t transmission: 3;

		bool pushbutton: 1;

		headlight_temp_t headlight_temp: 2;

		drl_setting_t drl_setting: 2;

		bool parking_lights: 1;
		bool celsius: 1;
		bool wiper_door_off: 1;
		bool auto_wiper: 1;

		uint8_t ambient_light_brightness;
		bool ambient_light_enable_door: 1;
		bool ambient_light_enable_int: 1;

		bool rearview_camera: 1;
		bool lanewatch_camera: 1;
		
		bool ambient_lighting: 1;
	} data;
	uint8_t raw[sizeof(data)];
};

void setCanslatorSettings(ParameterList* parameter_list);
void getCanslatorSettings(ParameterList* parameter_list);

#endif