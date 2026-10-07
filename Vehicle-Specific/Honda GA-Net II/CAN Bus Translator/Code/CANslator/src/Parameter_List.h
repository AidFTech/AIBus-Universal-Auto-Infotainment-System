#include <stdint.h>

#ifndef parameter_list_h
#define parameter_list_h

#define WIPER_TIMER_L 15000
#define WIPER_TIMER_H 6000

enum transmission_t : uint8_t {
	TRANS_5AT,
	TRANS_COMMON_CVT,
	TRANS_HYBRID_CVT,
	TRANS_5MT,
	TRANS_6MT,
};

enum special_trim_t : uint8_t {
	TRIM_NORMAL,
	TRIM_HYBRID,
	TRIM_SI,
	TRIM_CNG,
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

	bool ambient_slide_active = false;

	//Settings:
	bool display_celsius = true; //Display Celsius.
	bool wiper_door_off = true; //Turn the wipers off if the doors are open.
	bool auto_wiper = true; //Enable the rain sensor.
	bool parking_lights = true; //True if parking lights should be enabled in P.
	headlight_temp_t headlight_temp_setting = HEADLIGHT_TEMP_10; //Headlight-temperature integration setting.

	uint8_t ambient_light_brightness = 0x70;
	bool ambient_light_enable_door = true, ambient_light_enable_int = true;

	//Hidden settings:
	special_trim_t special_trim: 3;
	transmission_t transmission: 2;
	drl_setting_t drl_setting: 2;
	bool parking_light_drl : 1; //True if DRLs should be on as parking lights.
	bool pushbutton: 1; //True if the car has pushbutton start.

	bool has_rearview_camera: 1, has_lanewatch: 1;
	bool has_ambient_lighting: 1;

	ParameterList() {
		special_trim = TRIM_NORMAL;
		transmission = TRANS_5AT;
		drl_setting = DRL_SETTING_OFF;
		parking_light_drl = false;
		pushbutton = false;

		has_rearview_camera = true;
		has_lanewatch = false;
		has_ambient_lighting = false;
	}
};

#endif