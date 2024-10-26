#include <stdint.h>
#include <Arduino.h>
#include <Vector.h>

#include "AIBus.h"

#ifndef parameter_list_h
#define parameter_list_h

#define PRESET_COUNT 6
#define MINUTE_TIMER 60000

struct ParameterList {
	bool power_on = true;
	
	uint16_t vehicle_speed = 0;
	bool computer_connected = false, screen_connected = false, amp_connected = false, manual_tune_mode = false;

	uint8_t imid_char = 0, imid_lines = 0;
	bool imid_connected = false, imid_radio = false;

	bool timer_active = false;

	int8_t hour = -1, min = -1, offset = 0;
	elapsedMillis minute_timer;
	bool send_time = true;
	
	uint16_t fm1_tune, fm2_tune, am_tune;
	bool fm_stereo = false, has_rds = false, info_mode = false;

	uint16_t fm_lower_limit = 8400, fm_upper_limit = 10800, am_lower_limit = 550, am_upper_limit = 1600;
	uint16_t fm_start = 8750, am_start = 600;
	uint8_t fm_inc = 10, am_inc = 10;
	
	uint16_t fm1_presets[PRESET_COUNT];
	uint16_t fm2_presets[PRESET_COUNT];
	uint16_t am_presets[PRESET_COUNT];

	uint8_t current_preset = 0, preferred_preset = 0;
	bool tune_changed = false;

	int8_t rds_index = -1;
	String rds_station_name, rds_program_name;
	String rds_program_split[12];

	uint8_t last_sub = 0;

	uint8_t last_control = ID_NAV_COMPUTER;

	bool handshake_timer_active = false;
	elapsedMillis handshake_timer;
	uint8_t handshake_source_list[16];
	Vector<uint8_t> handshake_sources;

	bool digital_mode = false; //True if the source is digital.
	bool phone_active = false, digital_amp = false;

	uint16_t screen_w = 800, screen_h = 480;

	bool ai_pending = false;
};

#endif
