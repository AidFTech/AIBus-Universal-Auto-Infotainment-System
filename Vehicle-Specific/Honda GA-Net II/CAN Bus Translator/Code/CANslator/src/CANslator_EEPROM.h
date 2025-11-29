#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

#include "Parameter_List.h"

#ifndef canslator_eeprom_h
#define canslator_eeprom_h

enum eeprom_settings_index_t: uint8_t {
	TRIM_SETTING, //The trim of the vehicle.
	KEY_SETTING, //Pushbutton or traditional ignition.
	COMFORT_SETTINGS_1,
};

#define CANSLATOR_SETTINGS_HEADLIGHT_TEMP 0x3
#define CANSLATOR_SETTINGS_PARKING_LIGHTS _BV(2)
#define CANSLATOR_SETTINGS_CELSIUS _BV(3)
#define CANSLATOR_SETTINGS_WIPER_DOOR_OFF _BV(4)
#define CANSLATOR_SETTINGS_AUTO_WIPER _BV(5)

void setCanslatorSettings(ParameterList* parameter_list);
void getCanslatorSettings(ParameterList* parameter_list);

#endif