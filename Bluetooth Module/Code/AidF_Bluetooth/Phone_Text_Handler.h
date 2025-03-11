#include <Arduino.h>
#include <stdint.h>

#include "AIBus_Handler.h"
#include "Parameter_List.h"

#ifndef phone_text_handler_h
#define phone_text_handler_h

class PhoneTextHandler {
public: 
	PhoneTextHandler(AIBusHandler* ai_handler, ParameterList* parameters);

	void writeNaviText(String text, const uint8_t group, const uint8_t area);

	void createCallButtons();
	void createConnectedButtons();
	void createDisconnectedButtons(const bool pairing_on);
private:
	AIBusHandler* ai_handler;
	ParameterList* parameter_list;
};

#endif