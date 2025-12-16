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

	void writeAudioWindowText(string text, const uint8_t group, const uint8_t area, const bool refresh = true);

	void clearPhoneWindow();
	void writePhoneWindowText(string text, const uint8_t group, const uint8_t area);
	void writeSideMenuText(string text, const uint8_t entry);

	void writeNavHeaderText(string text);

	void writeMetadata(string data, const uint8_t recipient, const uint8_t line);
private:
	ClientAIBusHandler* aibus_handler;
	ParameterList* parameters;
};

#endif