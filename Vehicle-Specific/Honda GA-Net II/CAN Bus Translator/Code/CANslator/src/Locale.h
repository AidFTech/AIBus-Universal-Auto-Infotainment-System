#include <stdint.h>

#include "Locale_Common.h"

#ifndef locale_h
#define locale_h

enum canslator_menu_index : menu_index_t {
	//Vehicle settings menu.
	MENU_INDEX_SETTINGS_MAIN,
	MENU_INDEX_SETTINGS_MAIN_DRIVING_SUPPORT_SYSTEM,
	MENU_INDEX_SETTINGS_MAIN_TRIP_COMPUTER,
	MENU_INDEX_SETTINGS_MAIN_KEYLESS_ACCESS,
	MENU_INDEX_SETTINGS_MAIN_LIGHTING,
	MENU_INDEX_SETTINGS_MAIN_DOORS,
	MENU_INDEX_SETTINGS_MAIN_COMFORT_CONVENIENCE,
	MENU_INDEX_SETTINGS_MAIN_AMBIENT_LIGHTING,

	//Lighting menu.
	MENU_INDEX_SETTINGS_LIGHTING,
	MENU_INDEX_SETTINGS_LIGHTING_INTERIOR_TIME,
	MENU_INDEX_SETTINGS_LIGHTING_AUTO_OFF_TIMER,
	MENU_INDEX_SETTINGS_LIGHTING_PARKING_LIGHTS,
	MENU_INDEX_SETTINGS_LIGHTING_AUTO_SENSITIVITY,

	//Auto light sensitivity menu.
	MENU_INDEX_SETTINGS_AUTO_SENSITIVITY,
	MENU_INDEX_SETTINGS_AUTO_SENSITIVITY_MIN,
	MENU_INDEX_SETTINGS_AUTO_SENSITIVITY_LOW,
	MENU_INDEX_SETTINGS_AUTO_SENSITIVITY_MED,
	MENU_INDEX_SETTINGS_AUTO_SENSITIVITY_HIGH,
	MENU_INDEX_SETTINGS_AUTO_SENSITIVITY_MAX,

	//Comfort/Convenience menu.
	MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE,
	MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE_HEADLIGHT_TEMP,
	MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE_PARKING_LIGHTS,
	MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE_WIPER_DOOR_OFF,
	MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE_RAINSENSOR,

	//Headlight/temperature integration menu.
	MENU_INDEX_SETTINGS_HEADLIGHT,
	MENU_INDEX_SETTINGS_HEADLIGHT_OFF,
	MENU_INDEX_SETTINGS_HEADLIGHT_5C,
	MENU_INDEX_SETTINGS_HEADLIGHT_10C,
	MENU_INDEX_SETTINGS_HEADLIGHT_15C,

	//Ambient Light menu.
	MENU_INDEX_SETTINGS_AMBIENT_LIGHTING,
	MENU_INDEX_SETTINGS_AMBIENT_LIGHTING_BRIGHTNESS,
	MENU_INDEX_SETTINGS_AMBIENT_LIGHTING_DOOR,
	MENU_INDEX_SETTINGS_AMBIENT_LIGHTING_INTERIOR,

	//Secret dev menu.
	MENU_INDEX_SECRET_MAIN,
	MENU_INDEX_SECRET_MAIN_TRIM_OPTIONS,
	MENU_INDEX_SECRET_MAIN_TRANSMISSION,
	MENU_INDEX_SECRET_MAIN_OUTSIDE_TEMP_DISPLAY,
	MENU_INDEX_SECRET_MAIN_ADDITIONAL_OPTIONS,

	MENU_INDEX_LEN
};

static const canslator_menu_index MENU_START_INDEX[] = {
	MENU_INDEX_SETTINGS_MAIN,
	MENU_INDEX_SETTINGS_LIGHTING,
	MENU_INDEX_SETTINGS_AUTO_SENSITIVITY,
	MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE,
	MENU_INDEX_SETTINGS_HEADLIGHT,
	MENU_INDEX_SETTINGS_AMBIENT_LIGHTING,
	MENU_INDEX_SECRET_MAIN,

	MENU_INDEX_LEN
};

static const char* MENUS_ENG[] = {
	//Vehicle settings menu.
	"Vehicle Settings",
	"Driving Support System",
	"Trip Computer",
	"Keyless Access",
	"Lighting",
	"Doors",
	"Comfort/Convenience",
	"Ambient Lighting",

	//Lighting menu.
	"Lighting",
	"Interior Light Dimming Time",
	"Headlight Auto Off Timer",
	"Parking Lights",
	"Auto Light Sensitivity",

	//Auto light sensitivity menu.
	"Auto Light Sensitivity",
	"Minimum",
	"Low",
	"Medium",
	"High",
	"Maximum",

	//Comfort/Convenience menu.
	"Comfort/Convenience",
	"Headlight/Temperature Integration",
	"Use Parking Lights",
	"Disable Wipers with Door Open",
	"Rain-Sensing Wipers",

	//Headlight/Temperature Integration menu.
	"Headlight/Temperature Integration",
	"Off",
	"Headlights On Below 5°C/40°F",
	"Headlights On Below 10°C/50°F",
	"Headlights On Below 15°C/60°F",

	//Ambient Lighting menu.
	"Ambient Lighting",
	"Brightness",
	"Ambient Lighting with Door",
	"Ambient Lighting with Interior Lights",

	//Secret menu.
	"BCM Configuration",
	"Trim",
	"Transmission",
	"Outside Temp Display",
	"Additional Options",

};

MenuList getMenu(const canslator_menu_index index, const uint8_t locale);

#endif
