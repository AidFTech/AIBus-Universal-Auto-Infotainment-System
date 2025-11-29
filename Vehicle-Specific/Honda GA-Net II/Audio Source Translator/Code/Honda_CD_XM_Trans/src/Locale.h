#include <stdint.h>

#include "Locale_Common.h"

#ifndef locale_h
#define locale_h

enum translator_menu_index : menu_index_t {
	//CDC settings menu.
	MENU_INDEX_CDC_SETTINGS,
	MENU_INDEX_CDC_SETTINGS_TRACK_LIST,
	MENU_INDEX_CDC_SETTINGS_CHANGE_DISC,
	MENU_INDEX_CDC_SETTINGS_AUTOSTART,
	MENU_INDEX_CDC_SETTINGS_TEXT_IMID,
	MENU_INDEX_CDC_SETTINGS_SCROLL,
	MENU_INDEX_CDC_SETTINGS_AUDIO,

	//CDC disc menu.
	MENU_INDEX_CDC_DISC,
	MENU_INDEX_CDC_DISC_NUMBER,

	//Tape settings menu.
	MENU_INDEX_TAPE_SETTINGS,
	MENU_INDEX_TAPE_SETTINGS_FWD_START,
	MENU_INDEX_TAPE_SETTINGS_AUTO_START,
	MENU_INDEX_TAPE_SETTINGS_AUDIO,

	//XM settings menu.
	MENU_INDEX_XM_SETTINGS,
	MENU_INDEX_XM_SETTINGS_PRESETS,
	MENU_INDEX_XM_SETTINGS_DIRECT_TUNE,
	MENU_INDEX_XM_SETTINGS_CHANNEL_LIST,
	MENU_INDEX_XM_SETTINGS_MANUAL_ENTRY,
	MENU_INDEX_XM_SETTINGS_SCROLL_INFO,
	MENU_INDEX_XM_SETTINGS_AUDIO,

	//XM preset menu.
	MENU_INDEX_XM_PRESET,
	MENU_INDEX_XM_PRESET_CHANNEL,

	//XM direct tune.
	MENU_INDEX_XM_DIRECT,
	MENU_INDEX_XM_DIRECT_ENTER,

	MENU_INDEX_LEN
};

static const translator_menu_index MENU_START_INDEX[] = {
	MENU_INDEX_CDC_SETTINGS,
	MENU_INDEX_CDC_DISC,
	MENU_INDEX_TAPE_SETTINGS,
	MENU_INDEX_XM_SETTINGS,
	MENU_INDEX_XM_PRESET,
	MENU_INDEX_XM_DIRECT,
};

static const char* MENUS_ENG[] = {
	//CDC settings menu.
	"CDC Settings",
	"Track List",
	"Change Disc",
	"Auto Start",
	"CD Text on IMID",
	"Scroll Info Text",
	"Audio Settings",

	//CDC disc menu.
	"Select Disc",
	"Disc ",

	//Tape settings menu.
	"Tape Settings",
	"Start in Forward Mode",
	"Auto Start",
	"Audio Settings",

	//XM settings menu.
	"XM Tuner Settings",
	"Presets",
	"Direct Tune",
	"Channel List",
	"Manual Entry",
	"Scroll Info Text",
	"Audio Settings",

	//XM preset menu.
	"Select Preset",
	". CH",

	//XM direct tune menu.
	"Enter Channel Number",
	"Enter",
};

MenuList getMenu(const translator_menu_index index, const uint8_t locale);

#endif
