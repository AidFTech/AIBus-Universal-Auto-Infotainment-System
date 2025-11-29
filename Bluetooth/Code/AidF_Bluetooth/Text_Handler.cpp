#include "Text_Handler.h"

TextHandler::TextHandler(ClientAIBusHandler* aibus_handler, ParameterList* parameters) {
	this->aibus_handler = aibus_handler;
	this->parameters = parameters;
}

//Clear the phone window on the nav screen.
void TextHandler::clearPhoneWindow() {
	for(int i=0;i<6;i+=1)
		writePhoneWindowText("", 0, i);
	for(int i=0;i<3;i+=1)
		writePhoneWindowText("", 1, i);
	for(int i=0;i<5;i+=1)
		writeSideMenuText("", i);
}

//Write the given text to the following group and area on the nav screen.
void TextHandler::writePhoneWindowText(string text, const uint8_t group, const uint8_t area) {
	uint8_t text_data[text.length() + 3];
	text_data[0] = 0x21;
	text_data[1] = 0xA5;
	text_data[2] = ((group&0xF) << 4) | (area&0xF);
	for(int i=0;i<text.length();i+=1)
		text_data[i+3] = uint8_t(text[i]);

	AIData text_msg(sizeof(text_data), ID_PHONE, ID_NAV_COMPUTER, text_data);
	aibus_handler->writeAIData(&text_msg);
}

//Write the given text to the side menu in the phone window.
void TextHandler::writeSideMenuText(string text, const uint8_t entry) {
	writePhoneWindowText(text, 0xB, entry);
}