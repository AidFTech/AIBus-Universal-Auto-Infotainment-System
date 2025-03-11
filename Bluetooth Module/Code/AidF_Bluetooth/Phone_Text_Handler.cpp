#include "Phone_Text_Handler.h"

PhoneTextHandler::PhoneTextHandler(AIBusHandler* ai_handler, ParameterList* parameters) {
	this->ai_handler = ai_handler;
	this->parameter_list = parameters;
}

//Write some text to the nav screen.
void PhoneTextHandler::writeNaviText(String text, const uint8_t group, const uint8_t area) {
	uint8_t text_data[text.length() + 3];

	text_data[0] = 0x21;
	text_data[1] = 0xA5;
	text_data[2] = ((group&0xF)<<4) | (area&0xF);

	for(int i=0;i<text.length();i+=1)
		text_data[i+3] = uint8_t(text.charAt(i));

	AIData text_msg(sizeof(text_data), ID_PHONE, ID_NAV_COMPUTER);
	text_msg.refreshAIData(text_data);

	ai_handler->writeAIData(&text_msg, parameter_list->navi_connected);
}

//Create side buttons for in-call.
void PhoneTextHandler::createCallButtons() {
	for(int i=0;i<5;i+=1)
		this->writeNaviText("", 0xB, i);

	this->writeNaviText("End Call", 0xB, 0);
}

//Create side buttons for a connected phone.
void PhoneTextHandler::createConnectedButtons() {
	for(int i=0;i<5;i+=1)
		this->writeNaviText("", 0xB, i);

	this->writeNaviText("Dial Pad", 0xB, 0);
	this->writeNaviText("Contacts", 0xB, 1);
	this->writeNaviText("Recent Calls", 0xB, 2);

	this->writeNaviText("Disconnect", 0xB, 4);
}

//Create side buttons for when no phone is connected.
void PhoneTextHandler::createDisconnectedButtons(const bool pairing_on) {
	for(int i=0;i<5;i+=1)
		this->writeNaviText("", 0xB, i);

	if(pairing_on)
		this->writeNaviText("Disable Pairing", 0xB, 0);
	else
		this->writeNaviText("Connect", 0xB, 0);
	this->writeNaviText("Device List", 0xB, 1);
}