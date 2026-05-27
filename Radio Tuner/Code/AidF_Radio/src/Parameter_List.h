#include <stdint.h>
#include <Arduino.h>
#include <Vector.h>
#include <elapsedMillis.h>

#include "AIBus.h"

#ifndef parameter_list_h
#define parameter_list_h

#define PRESET_COUNT 6
#define MINUTE_TIMER 60000

enum svc_setting_t : uint8_t {
	SVC_OFF,
	SVC_LOW,
	SVC_MED,
	SVC_HIGH,
};

enum header_rds_setting_t : uint8_t {
	HEADER_RDS_OFF,
	HEADER_RDS_INFO_MODE,
	HEADER_RDS_ALWAYS,
};

enum fm_band_setting_t : uint8_t {
	FM_BAND_ITU,
	FM_BAND_JAPAN,
	FM_BAND_BRAZIL,
	FM_BAND_OIRT,
};

enum fm_inc_setting_t {
	FM_INC_50KHZ,
	FM_INC_100KHZ,
	FM_INC_200KHZ_ODD,
	FM_INC_200KHZ_EVEN,
};

enum am_band_setting_t : uint8_t {
	AM_BAND_WEST,
	AM_BAND_EAST,
	AM_BAND_AUSTRALIA,
};

enum rds_callsign_t : uint8_t {
	RDS_CALLSIGN_PS,
	RDS_CALLSIGN_PI_US_CANADA,
};

enum source_button_t : uint8_t {
	SOURCE_LIST,
	SOURCE_CYCLE_MOVING,
	SOURCE_CYCLE
};

struct ParameterList {
	bool power_on = false, audio_on = false;
	uint8_t key_position = 0, door_position = 0;

	bool monitor_on = true;
	
	uint16_t vehicle_speed = 0;
	bool computer_connected = false, screen_connected = false, amp_connected = false, mirror_connected = false;
	
	bool manual_tune_mode = false, bass_adjust = false, treble_adjust = false, balance_adjust = false, fader_adjust = false, aux_level_adjust = false, nav_cut_adjust = false;

	uint8_t imid_char = 0, imid_lines = 0;
	bool imid_connected = false, imid_radio = false;

	bool timer_active = false;

	int8_t hour = -1, min = -1, offset = 0;
	int8_t day = -1, month = -1;
	uint16_t year = 0;
	elapsedMillis minute_timer;
	bool send_time = true, send_12h = false, auto_clock = true;
	bool received_time_change_message = false; //True if a clock set message has been received.
	
	uint16_t fm1_tune, fm2_tune, am_tune;
	bool fm_stereo = false, has_rds = false, info_mode = false;

	uint16_t fm_lower_limit = 8400, fm_upper_limit = 10800, am_lower_limit = 530, am_upper_limit = 1710;
	uint16_t fm_start = 8750, am_start = 600;
	uint8_t fm_inc = 10, am_inc = 10;

	fm_band_setting_t fm_band = FM_BAND_ITU;
	fm_inc_setting_t fm_inc_setting = FM_INC_100KHZ;
	am_band_setting_t am_band = AM_BAND_WEST;

	rds_callsign_t rds_callsign_source = RDS_CALLSIGN_PS;
	
	uint16_t fm1_presets[PRESET_COUNT];
	uint16_t fm2_presets[PRESET_COUNT];
	uint16_t am_presets[PRESET_COUNT];

	int16_t clock_freq = -1; //The time-setting frequency.

	uint8_t current_preset = 0, preferred_preset = 0;
	bool tune_changed = false;

	int8_t rds_index = -1;
	String rds_station_name, rds_program_name;

	svc_setting_t svc = SVC_OFF;

	header_rds_setting_t header_rds_setting = HEADER_RDS_INFO_MODE;

	uint8_t last_sub = 0;

	uint8_t last_control = ID_NAV_COMPUTER;

	bool handshake_timer_active = false;
	elapsedMillis handshake_timer;
	uint8_t handshake_source_list[16];
	Vector<uint8_t> handshake_sources;

	bool digital_mode = false; //True if the source is digital.
	bool phone_active = false, digital_amp = false;

	bool scan_on = false;
	elapsedMillis scan_timer;

	uint16_t screen_w = 800, screen_h = 480;
	uint16_t audio_option_height = 0x23, setting_option_height = 40;

	uint8_t aux_level = 5, prompt_cut = 2;

	source_button_t source_button_mode = SOURCE_LIST; //Source button function.
	bool dac_filter_mode = false, steering_control_preset = true;

	uint8_t locale = 0; //Language
};

#endif
