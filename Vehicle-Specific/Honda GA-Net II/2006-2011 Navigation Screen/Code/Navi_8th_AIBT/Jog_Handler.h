#include <Arduino.h>
#include <MCP23S08.h>
#include <elapsedMillis.h>

#include "AIBus_Handler.h"
#include "AIBT_Parameters.h"
#include "Button_Codes.h"

#ifndef jog_handler_h
#define jog_handler_h

class JogHandler {
public:
	JogHandler(MCP23S08* up_mcp, const int8_t up_pin,
				MCP23S08* down_mcp, const int8_t down_pin,
				MCP23S08* left_mcp, const int8_t left_pin,
				MCP23S08* right_mcp, const int8_t right_pin,
				MCP23S08* enter_mcp, const int8_t enter_pin,
				AIBusHandler* ai_handler, ParameterList* parameters);

	void loop();

private:
	MCP23S08 *up_mcp = NULL, *down_mcp = NULL, *left_mcp = NULL, *right_mcp = NULL, *enter_mcp = NULL;
	int8_t up_pin = -1, down_pin = -1, left_pin = -1, right_pin = -1, enter_pin = -1;

	AIBusHandler* ai_handler;
	ParameterList* parameters;

	void checkJogPress();
	void checkJogHold();

	void sendButtonMessage(const uint8_t button, const uint8_t state, const uint8_t recipient);
};

#endif