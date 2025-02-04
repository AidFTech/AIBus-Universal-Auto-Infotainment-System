#include <stdint.h>
#include <SDL2/SDL.h>

#include "../AidF_Color_Profile.h"
#include "../Background/Nav_Background.h"
#include "../AIBus_Handler.h"

#ifndef attribute_list_h
#define attribute_list_h

#define DAY_NIGHT_AUTO 0
#define DAY_NIGHT_DAY 1
#define DAY_NIGHT_NIGHT 2

#define NEXT_WINDOW_LAST -1
#define NEXT_WINDOW_NULL 0
#define NEXT_WINDOW_MAIN 1
#define NEXT_WINDOW_MAP 2
#define NEXT_WINDOW_AUDIO 3
#define NEXT_WINDOW_PHONE 4
#define NEXT_WINDOW_CONSUMPTION 5
#define NEXT_WINDOW_SETTINGS_MAIN 6
#define NEXT_WINDOW_SETTINGS_DISPLAY 7
#define NEXT_WINDOW_SETTINGS_INFO 8
#define NEXT_WINDOW_SETTINGS_CLOCK 9
#define NEXT_WINDOW_SETTINGS_FORMAT 10
#define NEXT_WINDOW_SETTINGS_COLOR 11
#define NEXT_WINDOW_SETTINGS_COLOR_PICKER 12
#define NEXT_WINDOW_VEHICLE_INFO 13
#define NEXT_WINDOW_MIRROR 14

#define PHONE_TYPE_NONE 0
#define PHONE_TYPE_APPLE 3
#define PHONE_TYPE_ANDROID 5

struct AttributeList {
	SDL_Renderer* renderer;

	AidFColorProfile *color_profile, *day_profile, *night_profile;
	Background *br;

	uint16_t w, h;

	int8_t next_window = NEXT_WINDOW_NULL;
	uint8_t day_night_settings = DAY_NIGHT_AUTO;
	bool canslator_connected = false, radio_connected = false, mirror_connected = false;
	
	bool background_changed = false; //True if the background was changed by the user.
	bool text_changed = false; //True if the text color was changed.

	bool phone_active = false; //True if a phone mirror is active.
	uint8_t phone_type = PHONE_TYPE_NONE;
	std::string phone_name = "";

	AIBusHandler* aibus_handler;
};

#endif
