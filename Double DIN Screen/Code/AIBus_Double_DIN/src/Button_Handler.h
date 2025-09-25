#include <stdint.h>
#include <elapsedMillis.h>

#include "AIBus_Handler.h"
#include "AIBT_Parameters.h"
#include "Button_Codes.h"
#include "Open_Close_Handler.h"

#ifndef button_handler_h
#define button_handler_h

#define INDEX_VOL_PUSH 0
#define INDEX_NAV_UP 2
#define INDEX_NAV_DN 1
#define INDEX_NAV_LEFT 4
#define INDEX_NAV_RIGHT 3
#define INDEX_NAV_PUSH 5

#define BUTTON_TIMER 1000

#define TOGGLE_INDEX_SIZE 6

class ButtonHandler {
public:
	ButtonHandler(AIBusHandler* ai_handler, OpenCloseHandler* open_close_handler, ParameterList* parameters);
	void loop();

	bool toggle[TOGGLE_INDEX_SIZE];

private:
	AIBusHandler* ai_handler;
	OpenCloseHandler* open_close_handler;
	ParameterList* parameters;

	int button_index[BUTTON_INDEX_SIZE];

	uint8_t button_states[BUTTON_INDEX_SIZE];
	uint8_t last_button_rec[BUTTON_INDEX_SIZE];
	elapsedMillis button_timers[BUTTON_INDEX_SIZE];

	uint8_t toggle_states[TOGGLE_INDEX_SIZE];
	uint8_t last_toggle_rec[TOGGLE_INDEX_SIZE];
	elapsedMillis toggle_timers[TOGGLE_INDEX_SIZE];

	elapsedMillis debounce_timer;

	void checkButtonPress();
	void checkButtonHold();

	void sendButtonMessage(const uint8_t button, const uint8_t state, const uint8_t recipient);
};

#endif