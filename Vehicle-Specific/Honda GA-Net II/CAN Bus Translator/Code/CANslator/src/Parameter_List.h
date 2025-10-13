#include <stdint.h>

#ifndef parameter_list_h
#define parameter_list_h

enum headlight_temp_t : uint8_t {
	HEADLIGHT_TEMP_OFF,
	HEADLIGHT_TEMP_5,
	HEADLIGHT_TEMP_10,
	HEADLIGHT_TEMP_15
};

struct ParameterList {
	//Common parameters.
	bool power_on = false; //True if power is on.
	bool washer_fluid_low = false; //Washer fluid low active.

	uint8_t locale = 0; //Locale/language.

	//Settings:
	bool display_celsius = true; //Display Celsius.
	bool wiper_door_off = true; //Turn the wipers off if the doors are open.
	bool auto_wiper = true; //Enable the rain sensor.
	bool parking_lights = true; //True if parking lights should be enabled in P.
	headlight_temp_t headlight_temp_setting = HEADLIGHT_TEMP_10; //Headlight-temperature integration setting.
};

#endif