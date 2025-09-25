#include "Open_Close_Handler.h"

OpenCloseHandler::OpenCloseHandler(MCP23S08* oc_mcp, ParameterList* parameters) {
	this->oc_mcp = oc_mcp;
	this->parameters = parameters;
}

void OpenCloseHandler::loop() {
	if(pulse && pulse_timer > OC_PULSE_TIMER) {
		pulse = false;
		this->oc_mcp->digitalWriteIO(OC_MCP_OPEN_TOG, false);
		this->oc_mcp->digitalWriteIO(OC_MCP_CLOSE_TOG, false);
	}
}

//Get whether the screen is open.
bool OpenCloseHandler::getOpen() {
	return !oc_mcp->digitalReadIO(OC_MCP_OPEN_IND);
}

//Get whether the screen is closed.
bool OpenCloseHandler::getClosed() {
	return !oc_mcp->digitalReadIO(OC_MCP_CLOSE_IND);
}

//Open the screen.
void OpenCloseHandler::setOpen() {
	pulse = true;
	pulse_timer = 0;
	this->oc_mcp->digitalWriteIO(OC_MCP_OPEN_TOG, true);
}

//Close the screen.
void OpenCloseHandler::setClosed() {
	pulse = true;
	pulse_timer = 0;
	this->oc_mcp->digitalWriteIO(OC_MCP_CLOSE_TOG, true);
}