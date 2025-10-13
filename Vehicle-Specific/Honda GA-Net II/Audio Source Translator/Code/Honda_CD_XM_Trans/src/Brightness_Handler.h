#include <stdint.h>
#include <MCP4251.h>

#ifndef brightness_handler_h
#define brightness_handler_h

class BrightnessHandler {
public:
	BrightnessHandler(const uint8_t mcp_cs, const uint8_t ill_anode);

	void init();
	void setBrightness(const uint8_t brightness, const bool light_on);

private:
	MCP4251 ill_mcp;
	uint8_t ill_anode;
};

#endif
