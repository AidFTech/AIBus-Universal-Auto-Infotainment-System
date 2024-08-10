#include <Arduino.h>
#include <MCP23S08.h>
#include <elapsedMillis.h>

#include "AIBus_Handler.h"
#include "AIBT_Parameters.h"
#include "Button_Codes.h"

#ifndef button_handler_h
#define button_handler_h

class ButtonHandler {
	public:
		ButtonHandler(MCP23S08* input_mcp, MCP23S08* output_mcp, AIBusHandler* ai_handler, ParameterList* parameters, MCP23S08* vol_mcp, const int8_t vol_pin);
		
		void loop();
	private:
		uint8_t* button_states;
		elapsedMillis *button_timers;
		
		AIBusHandler* ai_handler;
		MCP23S08 *input_mcp, *output_mcp, *vol_mcp = NULL;

		ParameterList* parameters;

		int8_t vol_pin = -1;
		uint8_t vol_state = BUTTON_STATE_RELEASED;
		elapsedMillis vol_timer;
		
		bool amfm_pressed = false, cdxm_pressed = false;

		void checkButtonPress();
		void checkButtonHold();

		void sendButtonMessage(const uint8_t button, const uint8_t state, const uint8_t recipient);
};

int getButtonIndex(const uint8_t low_com, const uint8_t high_com);

#endif