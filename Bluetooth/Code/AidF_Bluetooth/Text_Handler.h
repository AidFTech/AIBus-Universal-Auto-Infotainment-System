#include <string>

#include <stdint.h>

#include "AIBus/AIBus.h"
#include "AIBus/Client_AIBus_Handler.h"

#include "Parameter_List.h"

using namespace std;

#ifndef text_handler_h
#define text_handler_h

class TextHandler {
public:
	TextHandler(ClientAIBusHandler* aibus_handler, ParameterList* parameters);

	void clearPhoneWindow();
	void writePhoneWindowText(string text, const uint8_t group, const uint8_t area);
	void writeSideMenuText(string text, const uint8_t entry);
private:
	ClientAIBusHandler* aibus_handler;
	ParameterList* parameters;
};

#endif