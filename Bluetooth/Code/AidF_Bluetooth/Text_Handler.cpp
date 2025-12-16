#include "Text_Handler.h"

TextHandler::TextHandler(ClientAIBusHandler* aibus_handler, ParameterList* parameters) {
	this->aibus_handler = aibus_handler;
	this->parameters = parameters;
}

//Write the given text to the group and area on the audio window of the nav screen.
void TextHandler::writeAudioWindowText(string text, const uint8_t group, const uint8_t area, const bool refresh) {
	uint8_t text_data[text.length() + 3];
	text_data[0] = 0x23;
	text_data[1] = 0x60 | (group&0xF) | (refresh ? 0x10 : 0x0);
	text_data[2] = area;
	for(int i=0;i<text.length();i+=1)
		text_data[i+3] = uint8_t(text[i]);

	AIData text_msg(sizeof(text_data), ID_PHONE, ID_NAV_COMPUTER, text_data);
	aibus_handler->writeAIData(&text_msg);
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

//Write the given text to the nav computer header.
void TextHandler::writeNavHeaderText(string text) {
	uint8_t header_data[text.length() + 2];
	header_data[0] = 0x22;
	header_data[1] = 0x61;
	for(int i=0;i<text.length();i+=1)
		header_data[i+2] = uint8_t(text[i]);

	AIData header_msg(sizeof(header_data), ID_PHONE, ID_NAV_COMPUTER, header_data);
	aibus_handler->writeAIData(&header_msg);
}

//Write the given text to the side menu in the phone window.
void TextHandler::writeSideMenuText(string text, const uint8_t entry) {
	writePhoneWindowText(text, 0xB, entry);
}

//Write metadata information to the source.
void TextHandler::writeMetadata(string data, const uint8_t recipient, const uint8_t line) {
	uint8_t meta_data[data.length() + 3];
	meta_data[0] = 0x23;
	meta_data[1] = 0x60 | (line&0xF);
	meta_data[2] = 0x1;
	for(int i=0;i<data.length();i+=1)
		meta_data[i+3] = uint8_t(data[i]);

	AIData meta_msg(sizeof(meta_data), ID_PHONE, recipient, meta_data);

	bool ack = true;
	if(recipient == ID_RADIO && !parameters->radio_connected)
		ack = false;
	else if(recipient == ID_IMID_SCR && (parameters->imid_char <= 0 || parameters->imid_lines <= 0))
		ack = false;
	else if(recipient == 0xFF)
		ack = false;

	aibus_handler->writeAIData(&meta_msg, ack);
}