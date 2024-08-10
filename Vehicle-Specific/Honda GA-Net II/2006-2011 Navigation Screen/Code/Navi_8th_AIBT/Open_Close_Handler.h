#include <Arduino.h>
#include <MCP23S08.h>
#include <elapsedMillis.h>

#include "AIBus_Handler.h"
#include "AIBT_Parameters.h"

#ifndef open_close_handler_h
#define open_close_handler_h

class OpenCloseHandler {
public:
	OpenCloseHandler(MCP23S08* open_mcp, const int8_t open_pin,
					MCP23S08* close_mcp, const int8_t close_pin,
					MCP23S08* open_motor_mcp, const int8_t open_motor_pin,
					MCP23S08* close_motor_mcp, const int8_t close_motor_pin,
					MCP23S08* stop_motor_mcp, const int8_t stop_motor_pin,
					MCP23S08* stop_motor_ind_mcp, const int8_t stop_motor_ind_pin,
					MCP23S08* close_ind_mcp, const int8_t close_ind_pin,
					const int8_t motor_pos, AIBusHandler* ai_handler, ParameterList* parameters);
	void loop();

	void forceOpen();
	void forceClose();
private:
	MCP23S08* open_mcp = NULL, *close_mcp = NULL, *open_motor_mcp = NULL, *close_motor_mcp = NULL, *stop_motor_mcp = NULL, *stop_motor_ind_mcp = NULL, *close_ind_mcp = NULL;
	int8_t open_pin = -1, close_pin = -1, open_motor_pin = -1, close_motor_pin = -1, stop_motor_pin = -1, stop_motor_ind_pin = -1, close_ind_pin = -1;

	int8_t motor_pos_pin = -1;

	AIBusHandler* ai_handler;
	ParameterList* parameters;

	bool open_pressed = false, close_pressed = false;
	bool close_led = false;

	int motor_position = 0;

	void checkButtonPress();
	void getMotorPosition();
};

#endif