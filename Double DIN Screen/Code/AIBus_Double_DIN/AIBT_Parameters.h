#include <Arduino.h>
#include <elapsedMillis.h>

#include "AIBus.h"

#ifndef parameter_list_h
#define parameter_list_h

#ifdef MEGACOREX
#define BT_EJECT PIN_PC0
#define BT_SOURCE PIN_PC1
#define BT_AUDIO PIN_PC2
#define BT_FMAM PIN_PC3
#define BT_AUX PIN_PC4
#define BT_SKIPUP PIN_PC5
#define BT_SKIPDN PIN_PC6
#define BT_INFO PIN_PC7
#define BT_TONE PIN_PD0
#define BT_HOME PIN_PD1
#define BT_SETUP PIN_PD2
#define BT_BACK PIN_PD3

#define BT_F1 PIN_PD4
#define BT_F2 PIN_PD5
#define BT_F3 PIN_PD6
#define BT_F4 PIN_PD7
#define BT_F5 PIN_PE0
#define BT_F6 PIN_PE1
#else
#define BT_EJECT 9
#define BT_SOURCE 10
#define BT_AUDIO 11
#define BT_FMAM 12
#define BT_AUX 13
#define BT_SKIPUP 14
#define BT_SKIPDN 15
#define BT_INFO 16
#define BT_TONE 17
#define BT_HOME 18
#define BT_SETUP 19
#define BT_BACK 20

#define BT_F1 21
#define BT_F2 22
#define BT_F3 23
#define BT_F4 24
#define BT_F5 25
#define BT_F6 26
#endif

#define BUTTON_INDEX_SIZE 18

#define BUTTON_STATE_RELEASED 0
#define BUTTON_STATE_PRESSED 1
#define BUTTON_STATE_HELD 2

struct ParameterList {
	bool radio_connected = false, computer_connected = false;

	uint8_t key_position = 0, door_position = 0;

	uint8_t all_dest = ID_NAV_COMPUTER, audio_dest = ID_NAV_COMPUTER, source_dest = ID_NAV_COMPUTER;

	uint8_t button_states[BUTTON_INDEX_SIZE];
	elapsedMillis button_timers[BUTTON_INDEX_SIZE];
};

#endif