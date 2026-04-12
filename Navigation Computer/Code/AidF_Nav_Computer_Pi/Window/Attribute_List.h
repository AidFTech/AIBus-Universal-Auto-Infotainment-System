#include <stdint.h>
#include <SDL2/SDL.h>

#include <vector>

#include "../AidF_Color_Profile.h"
#include "../Background/Nav_Background.h"
#include "../Serial_AIBus_Handler.h"

#ifndef attribute_list_h
#define attribute_list_h

#define DAY_NIGHT_AUTO 0
#define DAY_NIGHT_DAY 1
#define DAY_NIGHT_NIGHT 2

#define PHONE_TYPE_NONE 0
#define PHONE_TYPE_APPLE 3
#define PHONE_TYPE_ANDROID 5

enum next_window_t : int16_t {
	NEXT_WINDOW_LAST = -1,
	NEXT_WINDOW_NULL = 0,
	NEXT_WINDOW_MAIN,
	NEXT_WINDOW_MAP,
	NEXT_WINDOW_AUDIO,
	NEXT_WINDOW_PHONE,
	NEXT_WINDOW_CONSUMPTION,
	NEXT_WINDOW_SETTINGS_MAIN,
	NEXT_WINDOW_SETTINGS_DISPLAY,
	NEXT_WINDOW_SETTINGS_INFO,
	NEXT_WINDOW_SETTINGS_CLOCK,
	NEXT_WINDOW_SETTINGS_CLOCK_SET,
	NEXT_WINDOW_SETTINGS_FORMAT,
	NEXT_WINDOW_SETTINGS_COLOR,
	NEXT_WINDOW_SETTINGS_COLOR_PICKER,
	NEXT_WINDOW_VEHICLE_INFO,
	NEXT_WINDOW_MIRROR,
	NEXT_WINDOW_SETTINGS_EXT,
};

struct AttributeList {
	SDL_Renderer* renderer;

	AidFColorProfile *color_profile, *day_profile, *night_profile;
	Background *br;

	uint16_t w, h;

	uint8_t locale = 0; //TODO: This.

	next_window_t next_window = NEXT_WINDOW_NULL;
	uint8_t day_night_settings = DAY_NIGHT_AUTO;
	bool night = false;
	bool canslator_connected = false, radio_connected = false, mirror_connected = false, gps_antenna_connected = false;
	vector<uint8_t> ping_device_list = vector<uint8_t>(0);
	
	bool background_changed = false; //True if the background was changed by the user.
	bool text_changed = false; //True if the text color was changed.

	bool phone_active = false; //True if a phone mirror is active.
	uint8_t phone_type = PHONE_TYPE_NONE;
	std::string phone_name = "";

	int frame = 0;

	bool display_12h = false; //True if time is to be displayed in 12hr format with an AM/PM.
	uint8_t timekeeper = ID_RADIO; //ID of timekeeper device.
	bool auto_clock = true; //True if the timekeeper device receives its time data automatically.
	bool timekeeper_detected = false; //True if a message from the timekeeper device has been received.

	int8_t hour = -1, minute = -1; //Hour and minute.

	uint8_t active_audio_device = 0;

	uint16_t vehicle_speed = 0;
	
	unsigned long* timer = nullptr;

	SerialAIBusHandler* aibus_handler;
};

#endif
