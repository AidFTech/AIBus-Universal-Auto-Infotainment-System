#include "Brightness_Handler.h"

BrightnessHandler::BrightnessHandler(const uint8_t mcp_cs, const uint8_t ill_anode) :
	ill_mcp(mcp_cs, 10000, 0, 10000, 0) {
	this->ill_anode = ill_anode;
}

//Start the brightness handler MCP.
void BrightnessHandler::init() {
	ill_mcp.begin();
	ill_mcp.DigitalPotSetWiperPosition(0,0);
	ill_mcp.DigitalPotSetWiperPosition(1,256);
}

//Set the brightness.
void BrightnessHandler::setBrightness(const uint8_t brightness, const bool light_on) {
	digitalWrite(ill_anode, light_on ? HIGH : LOW);
	ill_mcp.DigitalPotSetWiperPosition(0, brightness < 255 ? brightness : 256);
	ill_mcp.DigitalPotSetWiperPosition(1, brightness < 255 ? brightness : 256);
}