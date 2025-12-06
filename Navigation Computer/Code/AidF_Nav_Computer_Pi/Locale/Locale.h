#include <stdint.h>

#include "Locale_Common.h"

#ifndef locale_h
#define locale_h

enum nav_text_index: unsigned int {
	//Nav Messages
	LOCALE_STRING_MAP_NOT_FOUND,

	//Setting Headers
	LOCALE_STRING_SET_CLOCK,
	LOCALE_STRING_CANT_SET_CLOCK,
	
	//Consumption Headers
	LOCALE_STRING_CONSUMPTION,

	//Vehicle Info Headers
	LOCALE_STRING_VEHICLE_INFORMATION,
	LOCALE_STRING_HYBRID_POWER_FLOW,

	//Vehicle Info Parameters
	LOCALE_STRING_COOLANT_TEMP,
	LOCALE_STRING_OUTSIDE_TEMP,
	LOCALE_STRING_RANGE,
	LOCALE_STRING_INST_ECONOMY,
	LOCALE_STRING_AVG_ECONOMY,
	LOCALE_STRING_TRIP_TIMER,
	LOCALE_STRING_TRIP_DISTANCE,
	LOCALE_STRING_CRUISE_SPEED,
	LOCALE_STRING_GEAR,

	//Phone Mirror Headers
	LOCALE_STRING_MIRROR,
	LOCALE_STRING_MIRROR_WAITING_1,
	LOCALE_STRING_MIRROR_WAITING_2,
	LOCALE_STRING_MIRROR_WAITING_GENERIC,
	LOCALE_STRING_MIRROR_NOT_CONNECTED,
};

static const char* TEXT_ENG[] {
	//Nav Messages
	"Map data not found. Please check the mounted SD card.",

	//Setting Headers
	"Set Clock",
	"Disable Auto Clock Set before adjusting the time.",

	//Consumption Headers
	"Consumption",

	//Vehicle Info Headers
	"Vehicle Information",
	"Hybrid Power Flow",

	//Vehicle Info Parameters
	"Coolant Temp",
	"Outside Temp",
	"Range",
	"Inst. Economy",
	"Avg Economy",
	"Trip Timer",
	"Trip Distance",
	"Cruise Speed",
	"Gear",

	//Mirror Messages
	"Phone Mirror",
	"Waiting for ",
	".",
	"Waiting for phone to connect.",
	"Phone not connected.",
};

enum nav_menu_index : menu_index_t {
	//Main menu.
	MENU_INDEX_MAIN,
	MENU_INDEX_MAIN_NAVIGATION,
	MENU_INDEX_MAIN_AUDIO,
	MENU_INDEX_MAIN_PHONE,
	MENU_INDEX_MAIN_SETTINGS,
	MENU_INDEX_MAIN_MONITOR_OFF,
	MENU_INDEX_MAIN_CONSUMPTION,
	MENU_INDEX_MAIN_INFORMATION,
	MENU_INDEX_MAIN_MIRROR,

	//Information menu.
	MENU_INDEX_INFORMATION_MAIN,
	MENU_INDEX_INFORMATION_MAIN_DISP_1,
	MENU_INDEX_INFORMATION_MAIN_DISP_2,
	MENU_INDEX_INFORMATION_MAIN_DISP_3,
	MENU_INDEX_INFORMATION_MAIN_DISP_4,
	MENU_INDEX_INFORMATION_MAIN_UNIT,
	MENU_INDEX_INFORMATION_MAIN_CHARGE_ASSIST,
	MENU_INDEX_INFORMATION_MAIN_CRUISE,

	//Information param menu.
	MENU_INDEX_INFORMATION_PARAM,
	MENU_INDEX_INFORMATION_PARAM_OFF,
	MENU_INDEX_INFORMATION_PARAM_BATTERY,
	MENU_INDEX_INFORMATION_PARAM_OUTSIDE_TEMP,
	MENU_INDEX_INFORMATION_PARAM_COOLANT_TEMP,
	MENU_INDEX_INFORMATION_PARAM_INST_ECONOMY,
	MENU_INDEX_INFORMATION_PARAM_AVERAGE_ECONOMY,
	MENU_INDEX_INFORMATION_PARAM_TRIP_TIMER,
	MENU_INDEX_INFORMATION_PARAM_CRUISE_SPEED,
	MENU_INDEX_INFORMATION_PARAM_GEAR,
	MENU_INDEX_INFORMATION_PARAM_RANGE,
	MENU_INDEX_INFORMATION_PARAM_TRIP_DISTANCE,
	MENU_INDEX_INFORMATION_PARAM_REMAINING_TIME,
	MENU_INDEX_INFORMATION_PARAM_REMAINING_DIST,

	//Main settings menu.
	MENU_INDEX_SETTINGS_MAIN,
	MENU_INDEX_SETTINGS_MAIN_DISPLAY,
	MENU_INDEX_SETTINGS_MAIN_INFO,
	MENU_INDEX_SETTINGS_MAIN_CLOCK,
	MENU_INDEX_SETTINGS_MAIN_UNIT_FORMAT,
	MENU_INDEX_SETTINGS_MAIN_VEHICLE,
	MENU_INDEX_SETTINGS_MAIN_AUDIO,

	//Display settings menu.
	MENU_INDEX_SETTINGS_DISPLAY,
	MENU_INDEX_SETTINGS_DISPLAY_COLORS,
	MENU_INDEX_SETTINGS_DISPLAY_DAY_NIGHT,
	MENU_INDEX_SETTINGS_DISPLAY_UPPER_HEADER,

	//Color settings menu.
	MENU_INDEX_SETTINGS_COLORS,
	MENU_INDEX_SETTINGS_COLORS_CUSTOM,
	MENU_INDEX_SETTINGS_COLORS_SAVE,

	//Day/night settings menu.
	MENU_INDEX_SETTINGS_DAY_NIGHT,
	MENU_INDEX_SETTINGS_DAY_NIGHT_AUTO,
	MENU_INDEX_SETTINGS_DAY_NIGHT_DAY,
	MENU_INDEX_SETTINGS_DAY_NIGHT_NIGHT,

	//Color picker menu.
	MENU_INDEX_COLOR_PICKER,
	MENU_INDEX_COLOR_PICKER_BACKGROUND,
	MENU_INDEX_COLOR_PICKER_TEXT,
	MENU_INDEX_COLOR_PICKER_BUTTON,
	MENU_INDEX_COLOR_PICKER_SELECTION,
	MENU_INDEX_COLOR_PICKER_HEADERBAR,
	MENU_INDEX_COLOR_PICKER_OUTLINE,
	MENU_INDEX_COLOR_PICKER_DAY,
	MENU_INDEX_COLOR_PICKER_NIGHT,

	//Clock settings menu.
	MENU_INDEX_SETTINGS_CLOCK,
	MENU_INDEX_SETTINGS_CLOCK_CLOCK_FORMAT,
	MENU_INDEX_SETTINGS_CLOCK_AUTO_SET,
	MENU_INDEX_SETTINGS_CLOCK_MANUAL_SET,

	//Clock format settings menu.
	MENU_INDEX_SETTINGS_CLOCK_FORMAT,
	MENU_INDEX_SETTINGS_CLOCK_FORMAT_12H,
	MENU_INDEX_SETTINGS_CLOCK_FORMAT_24H,

	//Auto clock set menu.
	MENU_INDEX_SETTINGS_AUTO_SET,
	MENU_INDEX_SETTINGS_AUTO_SET_VEHICLE,
	MENU_INDEX_SETTINGS_AUTO_SET_GPS,
	MENU_INDEX_SETTINGS_AUTO_SET_RADIO,
	MENU_INDEX_SETTINGS_AUTO_SET_MANUAL,

	MENU_INDEX_LEN
};

static const nav_menu_index MENU_START_INDEX[] = {
	MENU_INDEX_MAIN,
	MENU_INDEX_INFORMATION_MAIN,
	MENU_INDEX_INFORMATION_PARAM,
	MENU_INDEX_SETTINGS_MAIN,
	MENU_INDEX_SETTINGS_DISPLAY,
	MENU_INDEX_SETTINGS_COLORS,
	MENU_INDEX_SETTINGS_DAY_NIGHT,
	MENU_INDEX_COLOR_PICKER,
	MENU_INDEX_SETTINGS_CLOCK,
	MENU_INDEX_SETTINGS_CLOCK_FORMAT,
	MENU_INDEX_SETTINGS_AUTO_SET
};

static const char* MENUS_ENG[] = {
	//Main menu.
	"Main Menu",
	"Navigation",
	"Audio",
	"Phone",
	"Settings",
	"Monitor Off",
	"Consumption",
	"Information",
	"Phone Mirror",

	//Information menu.
	"Vehicle Information Settings",
	"Lower Display 1",
	"Lower Display 2",
	"Lower Display 3",
	"Lower Display 4",
	"Units",
	"Display Charge/Assist",
	"Display Cruise Speed",

	//Information parameter menu.
	"Lower Display ",
	"Off",
	"Battery Voltage",
	"Outside Temperature",
	"Coolant Temperature",
	"Instantaneous Economy",
	"Trip Average Economy",
	"Trip Timer",
	"Cruise Speed",
	"Gear",
	"Range",
	"Trip Distance",
	"Remaining Time (nav)",
	"Remaining Distance (nav)",

	//Main settings menu.
	"Settings",
	"Display Settings",
	"Info Settings",
	"Clock Settings",
	"Unit/Format Settings",
	"Vehicle Settings",
	"Audio Settings",

	//Display settings menu.
	"Display Settings",
	"Colors",
	"Day/Night Mode",
	"Upper Header",

	//Color settings menu.
	"Colors",
	"Custom",
	"Save Preset",

	//Day/night settings menu.
	"Day/Night Mode",
	"Auto",
	"Day",
	"Night",

	//Color picker menu.
	"Color Selection",
	"Background",
	"Text",
	"Button",
	"Selection",
	"Headerbar",
	"Outline",
	"Day",
	"Night",

	//Clock settings menu.
	"Clock Settings",
	"Clock Format",
	"Auto Clock Set",
	"Set Clock",

	//Clock format settings menu.
	"Clock Format",
	"12-hour",
	"24-hour",

	//Auto clock set menu.
	"Auto Clock Set",
	"From Vehicle",
	"From GPS",
	"From Radio",
	"Set Clock Manually",
};

const char* getString(const nav_text_index index, const uint8_t locale);

const char* getMenuTitle(const nav_menu_index index, const uint8_t locale);
MenuList getMenu(const nav_menu_index index, const uint8_t locale);

#endif
