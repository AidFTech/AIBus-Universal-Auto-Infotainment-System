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

	MENU_INDEX_LEN
};

static const canslator_menu_index MENU_START_INDEX[] = {
	MENU_INDEX_SETTINGS_MAIN,
	MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE,
	MENU_INDEX_SETTINGS_HEADLIGHT
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

	//Comfort/Convenience menu.
	"Comfort/Convenience",
	"Headlight/Temperature Integration",
	"Use Parking Lights",
	"Disable Wipers with Door Open",
	"Rain-Sensing Wipers",

	"Headlight/Temperature Integration",
	"Off",
	"Headlights On Below 5°C/40°F",
	"Headlights On Below 10°C/50°F",
	"Headlights On Below 15°C/60°F"
};

MenuList getMenu(const canslator_menu_index index, const uint8_t locale);

#endif
