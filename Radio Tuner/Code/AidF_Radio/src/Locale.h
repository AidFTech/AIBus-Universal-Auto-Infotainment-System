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

	//SVC menu:
	MENU_INDEX_SVC,
	MENU_INDEX_SVC_OFF,
	MENU_INDEX_SVC_LOW,
	MENU_INDEX_SVC_MED,
	MENU_INDEX_SVC_HIGH,

	//Audio settings menu:
	MENU_INDEX_AUDIO_SETTINGS,
	MENU_INDEX_AUDIO_SETTINGS_TONE,
	MENU_INDEX_AUDIO_SETTINGS_SOURCE_MODE,
	MENU_INDEX_AUDIO_SETTINGS_NAV_CUT,
	MENU_INDEX_AUDIO_SETTINGS_AUX_LEVEL,
	MENU_INDEX_AUDIO_SETTINGS_DAC_LATENCY,

	//Source button function menu:
	MENU_INDEX_SOURCE_BUTTON,
	MENU_INDEX_SOURCE_BUTTON_LIST,
	MENU_INDEX_SOURCE_BUTTON_CYCLE_MOVING,
	MENU_INDEX_SOURCE_BUTTON_CYCLE,

	//DAC filter latency menu:
	MENU_INDEX_DAC_LATENCY,
	MENU_INDEX_DAC_LATENCY_LOW,
	MENU_INDEX_DAC_LATENCY_NORMAL,

	//Radio settings menu:
	MENU_INDEX_RADIO_SETTINGS,
	MENU_INDEX_RADIO_SETTINGS_RDS_FLASH,
	MENU_INDEX_RADIO_SETTINGS_STEERING_WHEEL_MODE,
	MENU_INDEX_RADIO_SETTINGS_FM_BAND,
	MENU_INDEX_RADIO_SETTINGS_FM_INCREMENT,
	MENU_INDEX_RADIO_SETTINGS_AM_BAND,
	MENU_INDEX_RADIO_SETTINGS_RDS_CALLSIGN,
	MENU_INDEX_RADIO_SETTINGS_AUDIO,

	//RDS flash menu:
	MENU_INDEX_RDS_FLASH_SETTINGS,
	MENU_INDEX_RDS_FLASH_SETTINGS_OFF,
	MENU_INDEX_RDS_FLASH_SETTINGS_INFO_MODE,
	MENU_INDEX_RDS_FLASH_SETTINGS_ALWAYS,

	//FM band menu:
	MENU_INDEX_FM_BAND,
	MENU_INDEX_FM_BAND_ITU,
	MENU_INDEX_FM_BAND_JAPAN,
	MENU_INDEX_FM_BAND_BRAZIL,
	MENU_INDEX_FM_BAND_OIRT,

	//FM increment menu:
	MENU_INDEX_FM_INC,
	MENU_INDEX_FM_INT_50KHZ,
	MENU_INDEX_FM_INT_100KHZ,
	MENU_INDEX_FM_INT_200KHZ_ODD,
	MENU_INDEX_FM_INT_200KHZ_EVEN,

	//AM band menu:
	MENU_INDEX_AM_BAND,
	MENU_INDEX_AM_BAND_WEST,
	MENU_INDEX_AM_BAND_EAST,
	MENU_INDEX_AM_BAND_AUSTRALIA,

	//RDS callsign menu:
	MENU_INDEX_RDS_CALLSIGN,
	MENU_INDEX_RDS_CALLSIGN_PS,
	MENU_INDEX_RDS_CALLSIGN_PI_US_CANADA,

	//Steering wheel control menu:
	MENU_INDEX_STEERING_CONTROL,
	MENU_INDEX_STEERING_CONTROL_SEEK,
	MENU_INDEX_STEERING_CONTROL_PRESET,

	MENU_INDEX_LEN
};

static const radio_menu_index MENU_START_INDEX[] = {
	MENU_INDEX_RADIO_MAIN_MENU,
	MENU_INDEX_SOURCES,
	MENU_INDEX_TONE,
	MENU_INDEX_SVC,
	MENU_INDEX_AUDIO_SETTINGS,
	MENU_INDEX_SOURCE_BUTTON,
	MENU_INDEX_DAC_LATENCY,
	MENU_INDEX_RADIO_SETTINGS,
	MENU_INDEX_RDS_FLASH_SETTINGS,
	MENU_INDEX_FM_BAND,
	MENU_INDEX_FM_INC,
	MENU_INDEX_AM_BAND,
	MENU_INDEX_RDS_CALLSIGN,
	MENU_INDEX_STEERING_CONTROL,

	MENU_INDEX_LEN
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

	//SVC menu:
	"SVC",
	"Off",
	"Low",
	"Medium",
	"High",

	//Audio menu:
	"Audio Settings",
	"Tone",
	"Source Button Function",
	"Nav Prompt Cut",
	"Aux Level",
	"DAC Filter Latency",

	//Source button function menu:
	"Source Button Function",
	"List",
	"Cycle when Moving",
	"Cycle",

	//DAC latency menu:
	"DAC Filter Latency",
	"Low Latency",
	"Normal Latency",

	//Radio settings menu:
	"Radio Settings",
	"Flash RDS on Change",
	"Steering Wheel Control",
	"FM Band",
	"FM Increment",
	"AM Band/Increment",
	"RDS Callsign",
	"Audio Settings",

	//RDS flash menu:
	"Flash RDS on Change",
	"Off",
	"With Info Display Active",
	"Always",

	//FM band menu:
	"FM Band",
	"ITU R1,2,3 - 87.0-108.0MHz",
	"Japan - 76.0-95.0MHz",
	"Brazil - 76.0-108.0MHz",
	"OIRT (Eastern Europe) - 65.0-74.0MHz",

	//FM increment menu:
	"FM Increment",
	"50kHz",
	"100kHz (Europe, Africa)",
	"200kHz Odd (US/Canada, Australia)",
	"200kHz Even",

	//AM band menu:
	"AM Band/Increment",
	"Americas - 530-1700kHz",
	"Eurasia/Africa - 531-1602kHz",
	"Australia - 531-1701kHz",

	//RDS callsign menu:
	"RDS Callsign",
	"PS",
	"PI - US/Canada",

	//Steering wheel control menu:
	"Steering Wheel Control",
	"Seek",
	"Change Preset",
};

const char* getString(const radio_text_index index, const uint8_t locale);
MenuList getMenu(const radio_menu_index index, const uint8_t locale);

#endif