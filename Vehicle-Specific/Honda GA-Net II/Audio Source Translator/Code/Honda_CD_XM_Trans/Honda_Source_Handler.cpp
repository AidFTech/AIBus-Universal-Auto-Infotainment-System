#include "Honda_Source_Handler.h"

HondaSourceHandler::HondaSourceHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list) {
	this->ie_driver = ie_driver;
	this->ai_driver = ai_driver;
	this->parameter_list = parameter_list;
	this->active_menu = &this->parameter_list->active_menu;
}

bool HondaSourceHandler::sendHandshakeAckMessage() {
	//Prepare the messages...
	uint8_t handshake_data[] = {0x2, 0x7, 0x0, 0x1, 0x2, 0x7, 0x25, 0x24, 0x6};
	IE_Message handshake_message(sizeof(handshake_data), IE_ID_RADIO, device_ie_id, 0xF, true);
	handshake_message.refreshIEData(handshake_data);
	

	uint8_t handshake2_data[] = {0x3, device_ie_id&0xFF};
	IE_Message handshake_message2(sizeof(handshake2_data), IE_ID_RADIO, device_ie_id, 0xF, true);
	handshake_message2.refreshIEData(handshake2_data);

	uint8_t handshake3_data[] = {0x6, 0x3, 0x1, 0x0, 0x2, 0x1, device_ie_id&0xFF};
	IE_Message handshake_message3(sizeof(handshake3_data), IE_ID_RADIO, device_ie_id, 0xF, true);
	handshake_message3.refreshIEData(handshake3_data);

	uint8_t handshake4_data[] = {0x4};
	IE_Message handshake_message4(sizeof(handshake4_data), IE_ID_RADIO, device_ie_id, 0xF, true);
	handshake_message4.refreshIEData(handshake4_data);
	
	//First message...
	ie_driver->sendMessage(&handshake_message, true, true);
	
	getIEAckMessage(device_ie_id);
	//delay(20);
	
	//Second message...
	ie_driver->sendMessage(&handshake_message2, true, true);
	
	getIEAckMessage(device_ie_id);
	//delay(20);

	//Third message...
	ie_driver->sendMessage(&handshake_message3, true, true);

	getIEAckMessage(device_ie_id);
	//delay(20);
	
	//Last message?
	ie_driver->sendMessage(&handshake_message4, true, true);
	
	getIEAckMessage(device_ie_id);

	uint8_t function[] = {0x0, 0x1};
	
	sendFunctionMessage(ie_driver, true, device_ie_id, function, 2);
	
	/*timeout_del = 0;
	while(!getIEAckMessage(device_ie_id)) {
		if(timeout_del > TIMEOUT_DEL)
			return false;
	}*/
	
	ie_driver->addID(this->device_ai_id);
	return true;
}

void HondaSourceHandler::sendIEAckMessage(const uint16_t recipient) {
	//ie_driver->sendAcknowledgement(IE_ID_RADIO, recipient);
}

bool HondaSourceHandler::getIEAckMessage(const uint16_t sender) {
	bool ack = false;

	elapsedMillis delay_timer = 0;
	while(!ack && delay_timer < 20) {
		IE_Message ie_d;
		if(ie_driver->readMessage(&ie_d, true, IE_ID_RADIO) == 0) {
			if(ie_d.l >= 1 && ie_d.data[0] == 0x80) {
				ack = true;
				break;
			}
		}
	}

	ie_driver->cacheAIBus();

	return ack;
}

bool HondaSourceHandler::getIEAckMessageStrict(const uint16_t sender) {
	bool ack = false;

	elapsedMillis delay_timer = 0;
	while(!ack && delay_timer < 20) {
		IE_Message ie_d;
		if(ie_driver->readMessage(&ie_d, true, IE_ID_RADIO) == 0) {
			if(ie_d.l >= 1 && ie_d.data[0] == 0x80) {
				ack = true;
				break;
			}
		}
	}

	return ack;
}

void HondaSourceHandler::sendAIAckMessage(const uint8_t receiver) {
	ai_driver->sendAcknowledgement(device_ai_id, receiver);
}

//Ask the radio to switch over.
void HondaSourceHandler::requestRadioControl() {
	uint8_t request_data[] = {0x10, 0x10, device_ai_id};
	AIData request_msg(sizeof(request_data), device_ai_id, ID_RADIO);
	request_msg.refreshAIData(request_data);

	ai_driver->writeAIData(&request_msg, parameter_list->radio_connected);
}

bool HondaSourceHandler::getEstablished() {
	return this->source_established;
}

bool HondaSourceHandler::getSelected() {
	return this->source_sel;
}

void HondaSourceHandler::clearExternalIMID() {
	if(!parameter_list->imid_connected && parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines > 0) {
		uint8_t clear_data[2+parameter_list->external_imid_lines];
		clear_data[0] = 0x20;
		clear_data[1] = 0x60;
		for(int i=0;i<parameter_list->external_imid_lines;i+=1)
			clear_data[i+2] = i+1;
		
		AIData clear_msg(sizeof(clear_data), device_ai_id, ID_IMID_SCR);
		clear_msg.refreshAIData(clear_data);
		
		ai_driver->writeAIData(&clear_msg);
	}
}

//Send the introductory menu message.
void HondaSourceHandler::startAudioMenu(const uint8_t count, const uint8_t rows, const bool loop, String title) {
	uint8_t start_menu_data[title.length() + 12];
	
	unsigned int div = count/rows;
	if(count%rows != 0)
		div += 1;

	const uint16_t x = 0, y = 140, width = parameter_list->screen_w/div;
	
	start_menu_data[0] = 0x2B;
	start_menu_data[1] = 0x5A;
	start_menu_data[2] = rows&0x7F;
	start_menu_data[3] = count;
	start_menu_data[4] = (x&0xFF00) >> 8;
	start_menu_data[5] = x&0xFF;
	start_menu_data[6] = (y&0xFF00) >> 8;
	start_menu_data[7] = y&0xFF;
	start_menu_data[8] = (width&0xFF00)>>8;
	start_menu_data[9] = width&0xFF;
	start_menu_data[10] = 0x0;
	start_menu_data[11] = 0x23;
	
	for(unsigned int i=0;i<title.length();i+=1)
		start_menu_data[i+12] = uint8_t(title.charAt(i));
	
	if(loop)
		start_menu_data[2] |= 0x80;
	
	AIData start_menu_msg(sizeof(start_menu_data), device_ai_id, ID_NAV_COMPUTER);
	start_menu_msg.refreshAIData(start_menu_data);
	
	ai_driver->writeAIData(&start_menu_msg, parameter_list->computer_connected);
}

//Append data to the audio menu.
void HondaSourceHandler::appendAudioMenu(const uint8_t position, String text) {
	uint8_t append_menu_data[text.length() + 3];
	
	append_menu_data[0] = 0x2B;
	append_menu_data[1] = 0x51;
	append_menu_data[2] = position;
	
	for(unsigned int i=0;i<text.length();i+=1)
		append_menu_data[i+3] = uint8_t(text.charAt(i));
	
	AIData append_menu_msg(sizeof(append_menu_data), device_ai_id, ID_NAV_COMPUTER);
	append_menu_msg.refreshAIData(append_menu_data);
	
	ai_driver->writeAIData(&append_menu_msg, parameter_list->computer_connected);
}

//Display the audio menu.
void HondaSourceHandler::displayAudioMenu(const uint8_t selected) {
	uint8_t display_menu_data[] = {0x2B, 0x52, selected};
	AIData display_menu_msg(sizeof(display_menu_data), device_ai_id, ID_NAV_COMPUTER);
	display_menu_msg.refreshAIData(display_menu_data);

	ai_driver->writeAIData(&display_menu_msg, parameter_list->computer_connected);
}

void HondaSourceHandler::setMenuTitle(String title) {
	uint8_t change_title_data[title.length()+2];
	change_title_data[0] = 0x2B;
	change_title_data[1] = 0x53;
	
	for(unsigned int i=0;i<title.length();i+=1)
		change_title_data[i+2] = uint8_t(title.charAt(i));
		
	AIData change_title_msg(sizeof(change_title_data), device_ai_id, ID_NAV_COMPUTER);
	change_title_msg.refreshAIData(change_title_data);
	
	ai_driver->writeAIData(&change_title_msg, parameter_list->computer_connected);
}

//Get a text message from a string.
AIData getTextMessage(const uint8_t sender, String text, const uint8_t group, const uint8_t area, const bool refresh) {
	//text.replace("#","##  ");
	
	uint8_t text_data[3 + text.length()];
	text_data[0] = 0x23;
	text_data[1] = 0x60 | (group&0xF);
	if(refresh)
		text_data[1] |= 0x10;
	text_data[2] = area;
	
	for(uint16_t i=0;i<text.length();i+=1)
		text_data[i+3] = uint8_t(text.charAt(i));
	
	AIData text_message(sizeof(text_data), sender, ID_NAV_COMPUTER);
	text_message.refreshAIData(text_data);
	
	return text_message;
	
}
