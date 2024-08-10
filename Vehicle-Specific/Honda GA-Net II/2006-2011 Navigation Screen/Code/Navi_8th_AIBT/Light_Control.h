#include <Arduino.h>
#include <MCP23S08.h>

#ifndef light_control_h
#define light_control_h

class LightController {
	public:
		LightController(const int8_t cathode, const int8_t anode);
		LightController(const int8_t cathode, const int8_t anode, MCP23S08* anode_mcp);

		void lightOn(const uint8_t dimmer);
		void lightOff();

	private:
		int8_t cathode_pin, anode_pin;
		MCP23S08 *anode_mcp = NULL;
};

#endif
