#include <Arduino.h>
#include <stdint.h>
#include <MCP23S08.h>
#include <elapsedMillis.h>

#include "AIBT_Parameters.h"

#ifndef open_close_handler_h
#define open_close_handler_h

#define OC_MCP_OPEN_IND 0
#define OC_MCP_CLOSE_IND 1
#define OC_MCP_OPEN_TOG 2
#define OC_MCP_CLOSE_TOG 3

#define OC_PULSE_TIMER 50

class OpenCloseHandler {
public:
	OpenCloseHandler(MCP23S08* oc_mcp, ParameterList* parameters);
	void loop();

	bool getOpen();
	bool getClosed();

	void setOpen();
	void setClosed();
private:
	MCP23S08* oc_mcp;
	ParameterList* parameters;

	elapsedMillis pulse_timer;
	bool pulse = false;
};

#endif