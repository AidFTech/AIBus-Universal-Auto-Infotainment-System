#include <stdint.h>

#ifndef locale_h
#define locale_h

enum menu_index_t {
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

static const menu_index_t MENU_START_INDEX[] = {
	MENU_INDEX_SETTINGS_MAIN,
	MENU_INDEX_SETTINGS_DISPLAY,
	MENU_INDEX_SETTINGS_COLORS,
	MENU_INDEX_SETTINGS_DAY_NIGHT,
	MENU_INDEX_SETTINGS_CLOCK,
	MENU_INDEX_SETTINGS_CLOCK_FORMAT,
	MENU_INDEX_SETTINGS_AUTO_SET
};

static const char* MENUS_ENG[] = {
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

struct MenuList {
	menu_index_t start, end;
	const char** menu_str;
	const char* title;

	//The menu length.
	unsigned int size() {
		return end - start;
	}

	//Get the local menu index of the option.
	int getLocalIndex(const menu_index_t index) {
		if(index < start || index >= end)
			return -1;
		else
			return index - start;
	}

	//Get a local menu entry at index.
	const char* getLocalEntry(const menu_index_t index) {
		const int new_index = getLocalIndex(index);
		if(new_index >= 0)
			return (*this)[new_index];
		else
			return nullptr;
	}

	//Get the global menu index of int index.
	menu_index_t getGlobalIndex(const int index) {
		if(index < 0 || index >= size())
			return (menu_index_t)0;
		else
			return (menu_index_t)(index + start);
	}

	const char* operator[] (int index) {
		return menu_str[index];
	}
};

const char* getMenuTitle(const menu_index_t index, const uint8_t locale);
MenuList getMenu(const menu_index_t index, const uint8_t locale);

#endif