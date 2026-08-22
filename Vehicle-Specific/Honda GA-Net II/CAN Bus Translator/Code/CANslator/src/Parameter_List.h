#include <stdint.h>

#ifndef parameter_list_h
#define parameter_list_h

#define WIPER_TIMER_L 15000
#define WIPER_TIMER_H 6000

enum trim_t : uint8_t {
	TRIM_COMMON_AUTO_5AT,
	TRIM_COMMON_AUTO_CVT,
	TRIM_COMMON_MANUAL,
	TRIM_HYBRID,
	TRIM_SI,
	TRIM_CNG
};

enum drl_setting_t : uint8_t  {
	DRL_SETTING_OFF, //For factory DRLs.
	DRL_SETTING_WINK,
	DRL_SETTING_FULL,
};

enum headlight_temp_t : uint8_t {
	HEADLIGHT_TEMP_OFF,
	HEADLIGHT_TEMP_5,
	HEADLIGHT_TEMP_10,
	HEADLIGHT_TEMP_15
};

enum ambient_state_t : uint8_t {
	AMBIENT_LIGHTS_OFF,
	AMBIENT_LIGHTS_DIMMED,
	AMBIENT_LIGHTS_FULL
};

struct ParameterList {
	//Common parameters.
	bool power_on = false; //True if power is on.
	bool washer_fluid_low = false; //Washer fluid low active.
	bool display_miles = false; //True if miles are displayed.

	bool left_drl_on = false, right_drl_on = false; //True if DRLs are/should be on.
	ambient_state_t ambient_state = AMBIENT_LIGHTS_OFF;

	uint8_t brightness;

	uint8_t key_pos = 0, doors_open = 0;
	bool switched_on = false; //True if the key has been on at all during this drive.

	bool computer_connected = false; //True if the nav computer is connected.

	uint8_t locale = 0; //Locale/language.

	uint32_t wiper_time_limit = WIPER_TIMER_L; //The wiper time limit.

	bool reverse_on = false; //True if the car is in reverse.

	//Settings:
	bool display_celsius = true; //Display Celsius.
	bool wiper_door_off = true; //Turn the wipers off if the doors are open.
	bool auto_wiper = true; //Enable the rain sensor.
	bool parking_lights = true; //True if parking lights should be enabled in P.
	headlight_temp_t headlight_temp_setting = HEADLIGHT_TEMP_10; //Headlight-temperature integration setting.

	uint8_t ambient_light_brightness = 0x70;
	bool ambient_light_enable_door = true, ambient_light_enable_int = true;

	//Hidden settings:
	trim_t trim = TRIM_HYBRID;
	drl_setting_t drl_setting = DRL_SETTING_OFF;
	bool parking_light_drl = false; //True if DRLs should be on as parking lights.
};

#endif