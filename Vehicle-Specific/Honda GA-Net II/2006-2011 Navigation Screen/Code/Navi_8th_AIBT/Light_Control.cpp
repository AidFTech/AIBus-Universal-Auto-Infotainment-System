#include "Light_Control.h"

LightController::LightController(const int8_t cathode, const int8_t anode) {
	this->anode_pin = anode;
	this->cathode_pin = cathode;

	pinMode(this->anode_pin, OUTPUT);
	pinMode(this->cathode_pin, OUTPUT);
}

LightController::LightController(const int8_t cathode, const int8_t anode, MCP23S08* anode_mcp) {
	this->anode_pin = anode;
	this->cathode_pin = cathode;
	this->anode_mcp = anode_mcp;

	pinMode(this->cathode_pin, OUTPUT);
	this->anode_mcp->pinModeIO(this->anode_pin, OUTPUT);
}

void LightController::lightOn(const uint8_t dimmer) {
	analogWrite(cathode_pin, dimmer);
	
	if(this->anode_mcp != NULL)
		anode_mcp->digitalWriteIO(anode_pin, true);
	else
		digitalWrite(anode_pin, HIGH);
}

void LightController::lightOff() {
	analogWrite(cathode_pin, 0);

	if(this->anode_mcp != NULL)
		anode_mcp->digitalWriteIO(anode_pin, false);
	else
		digitalWrite(anode_pin, LOW);
}