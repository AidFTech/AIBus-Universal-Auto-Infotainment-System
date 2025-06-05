#include <stdint.h>

#include "AIBus_Handler.h"
#include "AIBT_Parameters.h"
#include "Button_Codes.h"
#include "Open_Close_Handler.h"

#ifndef button_handler_h
#define button_handler_h

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

	uint8_t* button_states;
	elapsedMillis *button_timers;

	uint8_t toggle_states[TOGGLE_INDEX_SIZE];
	elapsedMillis toggle_timers[TOGGLE_INDEX_SIZE];

	void checkButtonPress();
	void checkButtonHold();

	void sendButtonMessage(const uint8_t button, const uint8_t state, const uint8_t recipient);
};

#endif