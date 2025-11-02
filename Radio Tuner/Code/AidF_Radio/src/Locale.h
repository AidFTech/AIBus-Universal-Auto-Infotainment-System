#include <stdint.h>

#include "Locale_Common.h"

#ifndef locale_h
#define locale_h

enum radio_text_index : unsigned int {
	LOCALE_STRING_AUDIO_OFF,
};

static const char* TEXT_ENG[] {
	"Audio Off"
};

enum radio_menu_index : menu_index_t {
	//Main radio menu.
	MENU_INDEX_RADIO_MAIN_MENU,
	MENU_INDEX_RADIO_MAIN_MENU_MANUAL,
	MENU_INDEX_RADIO_MAIN_MENU_PRESETS,
	MENU_INDEX_RADIO_MAIN_MENU_SCAN,
	MENU_INDEX_RADIO_MAIN_MENU_STATION_LIST,

	//Source menu:
	MENU_INDEX_SOURCES,
	MENU_INDEX_SOURCES_ITEM,

	//Tone menu:
	MENU_INDEX_TONE,
	MENU_INDEX_TONE_BASS,
	MENU_INDEX_TONE_TREBLE,
	MENU_INDEX_TONE_BALANCE,
	MENU_INDEX_TONE_FADER,
	MENU_INDEX_TONE_SVC,

	MENU_INDEX_LEN
};

static const radio_menu_index MENU_START_INDEX[] = {
	MENU_INDEX_RADIO_MAIN_MENU,
	MENU_INDEX_SOURCES,
	MENU_INDEX_TONE,
	MENU_INDEX_LEN,
};

static const char* MENUS_ENG[] = {
	//Main radio menu:
	"",
	"Manual Tune",
	"Presets",
	"Scan",
	"Station List",

	//Source menu:
	"Source",
	"",

	//Tone menu:
	"Tone",
	"Bass",
	"Treble",
	"Balance",
	"Fader",
	"SVC",
};

const char* getString(const radio_text_index index, const uint8_t locale);
MenuList getMenu(const radio_menu_index index, const uint8_t locale);

#endif