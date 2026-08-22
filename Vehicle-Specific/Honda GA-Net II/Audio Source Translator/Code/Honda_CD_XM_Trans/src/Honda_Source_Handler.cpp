#include "Honda_Source_Handler.h"

HondaSourceHandler::HondaSourceHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list) {
	this->ie_driver = ie_driver;
	this->ai_driver = ai_driver;
	this->parameter_list = parameter_list;
	this->active_menu = &this->parameter_list->active_menu;

	ie_cache_vec.setStorage(ie_cache, 0);
}

bool HondaSourceHandler::sendHandshakeAckMessage() {
	//Prepare the messages...
	uint8_t handshake_data[] = {0x2, 0x7, 0x0, 0x1, 0x2, 0x7, 0x25, 0x24, 0x6};
	IE_Message handshake_message(sizeof(handshake_data), IE_ID_RADIO, device_ie_id, 0xF, true);
	handshake_message.refreshIEData(handshake_data);
	

	uint8_t handshake2_data[] = {0x3, uint8_t(device_ie_id&0xFF)};
	IE_Message handshake_message2(sizeof(handshake2_data), IE_ID_RADIO, device_ie_id, 0xF, true);
	handshake_message2.refreshIEData(handshake2_data);

	uint8_t handshake3_data[] = {0x6, 0x3, 0x1, 0x0, 0x2, 0x1, uint8_t(device_ie_id&0xFF)};
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
	
	ie_driver->addID(this->device_ai_id);
	return true;
}

//Send the acknowledgment message.
void HondaSourceHandler::sendIEAckMessage(const uint16_t recipient) {
	ie_driver->sendAcknowledgement(IE_ID_RADIO, recipient);
}

//Get whether the acknowledgment message was sent.
bool HondaSourceHandler::getIEAckMessage(const uint16_t sender) {
	bool ack = false;

	elapsedMillis delay_timer = 0;
	int tries = 0;
	while(!ack && delay_timer < 100 && tries < IE_TRIES) {
		IE_Message ie_d;
		const int message_result = ie_driver->readMessageStrict(&ie_d, true, IE_ID_RADIO);
		if(message_result == 0) {
			if(ie_d.l >= 1 && ie_d.data[0] == 0x80) {
				ack = true;
				break;
			}
		} else if(message_result < -1 || message_result > 0) {
			if(tries < 0) {
				delay_timer = 0;
				ie_driver->cacheAIBus();
			}
			tries += 1;
		}
		ie_driver->cacheAIBus();
	}

	return ack;
}

//Get whether the acknowledgment message was sent, resend the original if necessary.
bool HondaSourceHandler::getIEAckMessage(IE_Message* msg, const uint16_t sender) {
	int tries = 0;
	bool ack = false;

	elapsedMillis delay_timer = 0;

	while(!ack && tries < IE_TRIES && delay_timer < 500) {
		ack = getIEAckMessage(sender);
		if(!ack)
			ie_driver->sendMessage(msg, true, true);
		
		tries += 1;
	}

	return ack;
}

bool HondaSourceHandler::getIEAckMessageStrict(const uint16_t sender) {
	bool ack = false;

	elapsedMillis delay_timer = 0;
	while(!ack && delay_timer < 20) {
		IE_Message ie_d;
		const int message_result = ie_driver->readMessageStrict(&ie_d, true, IE_ID_RADIO);
		if(message_result == 0) {
			if(ie_d.l >= 1 && ie_d.data[0] == 0x80) {
				ack = true;
				break;
			}
		} else if(message_result < -1 || message_result > 0) {
			delay_timer = 0;
			ie_driver->cacheAIBus();
		}
	}

	return ack;
}

//Ask the radio to switch over.
void HondaSourceHandler::requestRadioControl() {
	uint8_t request_data[] = {0x10, 0x10, device_ai_id};
	AIData request_msg(sizeof(request_data), device_ai_id, ID_RADIO);
	request_msg.refreshAIData(request_data);

	ai_driver->writeAIData(&request_msg, parameter_list->radio_connected);
}

//Return established status.
bool HondaSourceHandler::getEstablished() {
	return this->source_established;
}

//Clear established status.
void HondaSourceHandler::clearEstablished() {
	this->source_established = false;
}

//Send an IEBus message through this source.
bool HondaSourceHandler::sourceSendIEMessage(IE_Message* msg, const bool ack) {
	ie_driver->sendMessage(msg, true, true);
	if(ack)
		return getIEAckMessage(msg, msg->receiver);
	else
		return true;
}

//Returns whether the source is selected by the radio.
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

//Send the introductory settings menu message.
void HondaSourceHandler::startSettingsMenu(const uint8_t count, const uint8_t rows, const bool loop, String title) {
	startMenu(false, count, rows, loop, title);
}

//Send the introductory audio menu message.
void HondaSourceHandler::startAudioMenu(const uint8_t count, const uint8_t rows, const bool loop, String title) {
	startMenu(true, count, rows, loop, title);
}

//Append data to the audio menu.
void HondaSourceHandler::appendMenu(const uint8_t position, String text) {
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
void HondaSourceHandler::displayMenu(const uint8_t selected) {
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

//Request the audio settings menu from the radio.
void HondaSourceHandler::requestAudioSettingsMenu() {
	open_audio_menu = false;
	uint8_t setting_request_data[] = {0x2B, 0x45};
	AIData setting_request_msg(sizeof(setting_request_data), device_ai_id, ID_RADIO, setting_request_data);
	ai_driver->writeAIData(&setting_request_msg, parameter_list->radio_connected);
}

//Send the introductory menu message.
void HondaSourceHandler::startMenu(const bool audio, const uint8_t count, const uint8_t rows, const bool loop, String title) {
	uint8_t start_menu_data[title.length() + 12];
	
	unsigned int div = count/rows;
	if(count%rows != 0)
		div += 1;

	const uint16_t x = 0, y = audio ? 140 : 105, width = parameter_list->screen_w/div;
	
	start_menu_data[0] = 0x2B;
	start_menu_data[1] = audio ? 0x5A : 0x50;
	start_menu_data[2] = rows&0x7F;
	start_menu_data[3] = count;
	start_menu_data[4] = (x&0xFF00) >> 8;
	start_menu_data[5] = x&0xFF;
	start_menu_data[6] = (y&0xFF00) >> 8;
	start_menu_data[7] = y&0xFF;
	start_menu_data[8] = (width&0xFF00)>>8;
	start_menu_data[9] = width&0xFF;
	start_menu_data[10] = 0x0;
	start_menu_data[11] = audio ? 0x23 : 40;
	
	for(unsigned int i=0;i<title.length();i+=1)
		start_menu_data[i+12] = uint8_t(title.charAt(i));
	
	if(loop)
		start_menu_data[2] |= 0x80;
	
	AIData start_menu_msg(sizeof(start_menu_data), device_ai_id, ID_NAV_COMPUTER);
	start_menu_msg.refreshAIData(start_menu_data);
	
	ai_driver->writeAIData(&start_menu_msg, parameter_list->computer_connected);
}

//Request display control.
void HondaSourceHandler::requestControl() {
	this->requestControl(this->device_ai_id);
}

//Request display control.
void HondaSourceHandler::requestControl(const uint8_t id) {
	if(id == 0)
		return;

	uint8_t request_data[] = {0x77, id, 0x80};
	AIData request_msg(sizeof(request_data), this->device_ai_id, ID_NAV_SCREEN);
	request_msg.refreshAIData(request_data);

	const bool ack = ai_driver->writeAIData(&request_msg, parameter_list->screen_connected);
	if(parameter_list->screen_connected && !ack)
		parameter_list->screen_connected = false;
}

//Listen for IEBus data.
inline void HondaSourceHandler::listenForIEBus(const unsigned long wait, const bool single_msg) {
	if(!source_sel)
		return;

	elapsedMillis ie_timer;
	IE_Message new_msg;
	AIData ai_cache;

	while(ie_timer < wait) {
		if(ie_driver->getInputOn()) {
			ai_driver->clearSerial();
			const int res = ie_driver->readMessage(&new_msg, true, IE_ID_RADIO);
			if(res == 0 && ie_cache_vec.size() < ie_cache_vec.max_size()) {
				parameter_list->last_iebus_msg = 0;
				if(new_msg.sender != device_ie_id || new_msg.receiver != IE_ID_RADIO)
					continue;

				ie_driver->sendAcknowledgement(IE_ID_RADIO, new_msg.sender);

				if(new_msg.direct && new_msg.control == 0xF && new_msg.l > 0 && new_msg.data[0] == 0x60) {
					if(single_msg) {
						ie_cache_vec.push_back(new_msg);
						break;
					} else {
						handleIEBus(&new_msg, false);
					}
				} else {
					ie_cache_vec.push_back(new_msg);
				}
			}
			ai_driver->clearSerial();
		}

		if(allow_recursive_aibus) {
			if(device_ai_id != ID_IMID_SCR && parameter_list->imid_connected) {
				uint8_t id_l[] = {device_ai_id, ID_IMID_SCR};
				AIData pending = ie_driver->getAIBus(id_l, sizeof(id_l));
				if(pending.l > 0 && pending.receiver == device_ai_id) {
					allow_recursive_aibus = false;
					handleAIBus(&pending);
					allow_recursive_aibus = true;
					break;
				} else if(pending.l > 0 && pending.receiver == ID_IMID_SCR) {
					allow_recursive_aibus = false;
					handleIMIDAIBus(&pending);
					allow_recursive_aibus = true;
					break;
				}
			} else {
				uint8_t id_l[] = {device_ai_id};
				AIData pending = ie_driver->getAIBus(id_l, sizeof(id_l));
				if(pending.l > 0 && pending.receiver == device_ai_id) {
					allow_recursive_aibus = false;
					handleAIBus(&pending);
					allow_recursive_aibus = true;
					break;
				}
			}
		} else if(use_ai_cache) {
			uint8_t id_l[] = {device_ai_id};
			AIData pending = ie_driver->getAIBus(id_l, sizeof(id_l));
			if(pending.l > 0 && pending.receiver == device_ai_id) {
				ai_cache.refreshAIData(pending);
				break;
			}
		}
	}

	if(ai_cache.l > 0) {
		handleAIBus(&ai_cache);
	}
}

//Generic AIBus read function.
void HondaSourceHandler::handleAIBus(AIData* msg) {

}

//Generic IEBus read function.
void HondaSourceHandler::handleIEBus(IE_Message* msg, const bool listen) {

}

//IMID AIBus forward function.
void HondaSourceHandler::handleIMIDAIBus(AIData* msg) {

}

//Send an overlay message to the phone mirror.
void HondaSourceHandler::sendMirrorMessage(String text, const uint8_t index, const bool refresh) {
	if(!parameter_list->mirror_connected)
		return;

	uint8_t mirror_data[2 + text.length()];
	mirror_data[0] = 0x23;
	mirror_data[1] = 0x60|(index&0xF);

	if(refresh)
		mirror_data[1] |= 0x10;

	for(int i=0;i<text.length();i+=1)
		mirror_data[i+2] = uint8_t(text.charAt(i));

	AIData mirror_msg(sizeof(mirror_data), this->device_ai_id, ID_ANDROID_AUTO);
	mirror_msg.refreshAIData(mirror_data);

	parameter_list->mirror_connected = ai_driver->writeAIData(&mirror_msg, parameter_list->mirror_connected);
}

//Set the header on the nav computer.
void HondaSourceHandler::setNavHeader(String text) {
	if(!text_control)
		return;

	AIData header_msg(2 + text.length(), this->device_ai_id, ID_NAV_COMPUTER);
	header_msg.data[0] = 0x22;
	header_msg.data[1] = 0x61;

	for(int i=0;i<text.length();i+=1)
		header_msg.data[i+2] = uint8_t(text.charAt(i));
	
	ai_driver->writeAIData(&header_msg, parameter_list->computer_connected);
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
	
	AIData text_message(sizeof(text_data), sender, ID_NAV_COMPUTER, text_data);
	
	return text_message;
}
