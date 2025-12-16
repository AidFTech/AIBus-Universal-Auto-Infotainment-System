#include "Locale_Common.h"

#ifndef locale_h
#define locale_h

enum bta_text_index : uint8_t {
	//Header Messages
	LOCALE_STRING_PHONE_NOT_CONNECTED,
	LOCALE_STRING_PHONE_CONNECTED,

	//IMID headers:
	LOCALE_STRING_IMID_TRACK,
	LOCALE_STRING_IMID_ARTIST,
	LOCALE_STRING_IMID_ALBUM,
};

static const char* TEXT_ENG[] {
	//Header Messages
	"No Phone Connected",
	"Phone",

	//IMID headers:
	"TRACK",
	"ARTIST",
	"ALBUM",
};

enum bta_menu_index : uint8_t {
	//Main Menu, no phone
	MENU_INDEX_MAIN_NC,
	MENU_INDEX_MAIN_NC_PAIR_ON,
	MENU_INDEX_MAIN_NC_DEVICE_LIST,

	//Main Menu, phone
	MENU_INDEX_MAIN,
	MENU_INDEX_MAIN_SPEED_DIAL,
	MENU_INDEX_MAIN_DIAL_PAD,
	MENU_INDEX_MAIN_CONTACT_LIST,
	MENU_INDEX_MAIN_RECENT_CALLS,
	MENU_INDEX_MAIN_DISCONNECT,

	MENU_INDEX_LEN,
};

static const bta_menu_index MENU_START_INDEX[] = {
	MENU_INDEX_MAIN_NC,
	MENU_INDEX_MAIN,
	MENU_INDEX_LEN,
};

static const char* MENUS_ENG[] {
	//Main Menu, no phone
	"",
	"Pairing On",
	"Device List",

	//Main Menu, phone
	"",
	"Speed Dial",
	"Dial Pad",
	"Contacts",
	"Recent Calls",
	"Disconnect",
};

const char* getString(const bta_text_index index, const uint8_t locale);
MenuList getMenu(const bta_menu_index index, const uint8_t locale);

#endif