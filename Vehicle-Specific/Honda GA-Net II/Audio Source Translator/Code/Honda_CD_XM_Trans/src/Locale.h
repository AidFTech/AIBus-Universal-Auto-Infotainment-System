#include <stdint.h>

#include "Locale_Common.h"

#ifndef locale_h
#define locale_h

enum translator_text_index : unsigned int {
	//Generic strings:
	LOCALE_STRING_NO_DATA,

	//CD strings:
	LOCALE_STRING_CD_TRACK,
	LOCALE_STRING_CD_ARTIST,
	LOCALE_STRING_CD_ALBUM,
	LOCALE_STRING_CD_FOLDER,
	LOCALE_STRING_CD_FILE,

	//XM strings:
	LOCALE_STRING_XM_SONG,
	LOCALE_STRING_XM_ARTIST,
	LOCALE_STRING_XM_CHANNEL,
	LOCALE_STRING_XM_GENRE,
};

static const char* TEXT_ENG[] {
	//Generic strings:
	"No Data",
	
	//CD strings:
	"Track",
	"Artist",
	"Album",
	"Folder",
	"File",

	//XM strings:
	"Song",
	"Artist",
	"Channel",
	"Genre",
};

enum translator_menu_index : menu_index_t {
	//CDC settings menu.
	MENU_INDEX_CDC_SETTINGS,
	MENU_INDEX_CDC_SETTINGS_TRACK_LIST,
	MENU_INDEX_CDC_SETTINGS_CHANGE_DISC,
	MENU_INDEX_CDC_SETTINGS_AUTOSTART,
	MENU_INDEX_CDC_SETTINGS_AUTOINCREMENT,
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

	//IMID settings.
	MENU_INDEX_IMID_SETTINGS,
	MENU_INDEX_IMID_SETTINGS_RDS,
	MENU_INDEX_IMID_SETTINGS_VOLUME,
	MENU_INDEX_IMID_SETTINGS_CD_TEXT,
	MENU_INDEX_IMID_SETTINGS_LENGTH,

	MENU_INDEX_IMID_CHARACTER,
	MENU_INDEX_IMID_CHARACTER_8,
	MENU_INDEX_IMID_CHARACTER_10,
	MENU_INDEX_IMID_CHARACTER_12,

	MENU_INDEX_LEN
};

static const translator_menu_index MENU_START_INDEX[] = {
	MENU_INDEX_CDC_SETTINGS,
	MENU_INDEX_CDC_DISC,
	MENU_INDEX_TAPE_SETTINGS,
	MENU_INDEX_XM_SETTINGS,
	MENU_INDEX_XM_PRESET,
	MENU_INDEX_XM_DIRECT,
	MENU_INDEX_IMID_SETTINGS,
	MENU_INDEX_IMID_CHARACTER,
};

static const char* MENUS_ENG[] = {
	//CDC settings menu.
	"CDC Settings",
	"Track List",
	"Change Disc",
	"Auto Start",
	"Auto Timer Increment",
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

	//IMID Settings
	"IMID Settings (Audio)",
	"Display RDS",
	"Display Volume Bar",
	"Display CD Text",
	"Allowed Character Count",

	"Character Count",
	"8",
	"10",
	"12",
};

MenuList getMenu(const translator_menu_index index, const uint8_t locale);
const char* getString(const translator_text_index index, const uint8_t locale);

#endif
