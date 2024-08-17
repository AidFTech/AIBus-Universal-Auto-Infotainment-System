#include <Arduino.h>
#include <stdint.h>
#include <elapsedMillis.h>

#include "AIBus.h"

#ifndef parameter_list_h
#define parameter_list_h

#define HOLD_TIME 1000
#define OPEN_CLOSE_TIME 800

#define BUTTON_STATE_RELEASED 0
#define BUTTON_STATE_PRESSED 1
#define BUTTON_STATE_HELD 2

#define BUTTON_INDEX_OPEN 0
#define BUTTON_INDEX_AMFM 1
#define BUTTON_INDEX_CDXM 2
#define BUTTON_INDEX_AUDIO 3
#define BUTTON_INDEX_SCAN 4
#define BUTTON_INDEX_SKIPUP 5
#define BUTTON_INDEX_SKIPDOWN 6
#define BUTTON_INDEX_PRESET1 7
#define BUTTON_INDEX_PRESET2 8
#define BUTTON_INDEX_PRESET3 9
#define BUTTON_INDEX_PRESET4 10
#define BUTTON_INDEX_PRESET5 11
#define BUTTON_INDEX_PRESET6 12
#define BUTTON_INDEX_ZOOMOUT 13
#define BUTTON_INDEX_ZOOMIN 14
#define BUTTON_INDEX_CANCEL 15
#define BUTTON_INDEX_SETUP 16
#define BUTTON_INDEX_MENU 17
#define BUTTON_INDEX_MAP 18
#define BUTTON_INDEX_INFO 19
#define BUTTON_INDEX_JOGUP 20
#define BUTTON_INDEX_JOGDOWN 21
#define BUTTON_INDEX_JOGLEFT 22
#define BUTTON_INDEX_JOGRIGHT 23
#define BUTTON_INDEX_JOGENTER 24
#define BUTTON_INDEX_CLOSE 25

#define BUTTON_INDEX_SIZE 26

struct ParameterList {
	bool radio_connected = false, computer_connected = false;

	uint8_t all_dest = ID_NAV_COMPUTER, audio_dest = ID_NAV_COMPUTER, source_dest = ID_NAV_COMPUTER;

	uint8_t button_states[BUTTON_INDEX_SIZE];
	elapsedMillis button_timers[BUTTON_INDEX_SIZE];

	elapsedMillis pulse_timer;
	bool open_pulse_enabled = false, close_pulse_enabled = false;
};

#endif