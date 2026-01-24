#include "Honda_Tape_Handler.h"

HondaTapeHandler::HondaTapeHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list, HondaIMIDHandler* imid_handler) : HondaSourceHandler(ie_driver, ai_driver, parameter_list) {
	this->device_ie_id = IE_ID_TAPE;
	this->device_ai_id = ID_TAPE;
	this->imid_handler = imid_handler;

	getTapeSettings(&this->autostart, &this->fwd_start);

	ie_vec.setStorage(ie_cache, 0);
}

//Tape handler loop function.
void HondaTapeHandler::loop() {
	if(this->source_sel) {
		if(*active_menu == MENU_TAPE && setting_changed) {
			setting_changed = false;
			setTapeSettings(this->autostart, this->fwd_start);
		}

		if(nr_timer_enabled && nr_timer > MODE_FLASH_TIMER) {
			nr_timer_enabled = false;

			this->sendTapeTextMessage();
		}

		if(parameter_list->radio_ping_timer > RADIO_PING_WAIT) {
			parameter_list->radio_ping_timer = 0;
			uint8_t src_request_data[] = {0x60, 0x10};
			AIData src_request_msg(sizeof(src_request_data), this->device_ai_id, ID_RADIO, src_request_data);
			const bool ack = ai_driver->writeAIData(&src_request_msg, parameter_list->radio_connected);
			
			if(!ack)
				parameter_list->radio_connected = false;
		}

		if(text_ping_timer_enabled && text_ping_timer > TEXT_PING_TIMER) {
			text_ping_timer = 0;
			uint8_t text_request_data[] = {0x60, 0x11};
			AIData text_request_msg(sizeof(text_request_data), this->device_ai_id, ID_RADIO, text_request_data);
			const bool ack = ai_driver->writeAIData(&text_request_msg, parameter_list->radio_connected);
			
			if(!ack)
				parameter_list->radio_connected = false;
		}

		//if(ie_driver->getInputOn())
		//	this->listenForIEBus();

		while(ie_vec.size() > 0) {
			interpretTapeMessage(&ie_vec[0], false);
			ie_vec.remove(0);
		}
		ie_vec.clear();
	}
}

//Interpret a tape IEBus message.
void HondaTapeHandler::interpretTapeMessage(IE_Message* the_message, const bool listen) {
	if(the_message->receiver != IE_ID_RADIO || the_message->sender != IE_ID_TAPE)
		return;

	if(!source_established && (the_message->l > 4 || (the_message->l >= 1 && the_message->data[0] == 0x80))) {
		ie_driver->addID(ID_TAPE);
		source_established = true;
		if(parameter_list->radio_connected)
			sendSourceNameMessage();
	}
		
	bool ack = true;

	if(the_message->l == 2 && the_message->data[0] == 0x80) //Acknowledgement.
		return;
	
	if(the_message->l == 4) { //Handshake message.
		if(the_message->data[0] == 0x1 && the_message->data[1] == 0x1) {
			ack = false;
			sendIEAckMessage(IE_ID_TAPE);

			if(!this->parameter_list->first_tape) {
				this->parameter_list->first_tape = true;

				uint8_t handshake_data[] = {0x0, 0x30};
				IE_Message handshake_msg(sizeof(handshake_data), IE_ID_RADIO, 0x1FF, 0xF, false);
				handshake_msg.refreshIEData(handshake_data);
				
				delay(2);

				ie_driver->sendMessage(&handshake_msg, false, false);
				if(listen)
					listenForIEBus();
			}
		} else if(the_message->data[0] == 0x5 && the_message->data[2] == 0x2) {
			ack = false;
			sendIEAckMessage(IE_ID_TAPE);

			/*while(handshake_timer < HANDSHAKE_WAIT) {
				if(ie_driver->getInputOn() && ie_driver->readMessage(&new_msg, true, IE_ID_RADIO) == 0)
					sendIEAckMessage(new_msg.sender);
			}*/
			
			delay(2);
			
			const bool handshake_rec = sendHandshakeAckMessage();
			if(!source_established) {
				source_established = handshake_rec;
				
				if(source_established && parameter_list->radio_connected) {
					sendSourceNameMessage();
				}
			}
			
		} 
	} else if(the_message->l >= 0x9 && the_message->data[0] == 0x60 && the_message->data[1] == 0x13 && the_message->data[2] == 0x1) { //Display/status change message.
		uint8_t tape_state = the_message->data[4];
		if(tape_state == 0x12)
			tape_state = 0;

		if(listen && (tape_state == TAPE_MODE_LOAD || tape_state == TAPE_MODE_EJECT))
			listenForIEBus();
		
		const uint8_t direction_state = the_message->data[7]&(~8), track_state = the_message->data[6];
		const uint8_t last_mode = tape_mode, last_dir = getDirectionByte(!fwd, repeat_on, nr_on, cro2), last_track = track_count;
		
		if(tape_state != last_mode || direction_state != last_dir || track_state != last_track) {
			ack = false;
			sendIEAckMessage(the_message->sender);

			const bool last_nr = nr_on;
			
			fwd = (direction_state&(1<<6)) == 0;
			repeat_on = (direction_state&(1<<5)) != 0;
			nr_on = (direction_state&(1<<1)) != 0;
			cro2 = (direction_state&(1)) != 0;
			
			if(tape_state != 0x12)
				tape_mode = tape_state;
			else
				tape_mode = 0;
			
			track_count = track_state;
			
			uint8_t tape_update_data[] = {0x31, tape_mode, direction_state, track_count};
			AIData tape_update_message(sizeof(tape_update_data), ID_TAPE, ID_RADIO);
			tape_update_message.refreshAIData(tape_update_data);
			
			ai_driver->writeAIData(&tape_update_message, parameter_list->radio_connected);
			
			if(text_control && last_nr != nr_on) {
				this->nr_timer_enabled = true;
				this->nr_timer = 0;
			}
			
			if(text_control)
				sendTapeTextMessage();

			if(tape_mode == TAPE_MODE_LOAD && fwd_start && !fwd) {
				sendButtonMessage(HONDA_BUTTON_DIR);
				change_dir = true;
			}
			
			if(tape_mode == TAPE_MODE_PLAY && fwd_start && change_dir) {
				if(!fwd)
					sendButtonMessage(HONDA_BUTTON_DIR);
				else
					change_dir = false;
			}
		}
	} else if(the_message->l >= 6 && the_message->data[0] ==0x10 && the_message->data[3] == 0x10 && the_message->data[4] == 0x10 && !source_sel) { //Tape loading but not selected.
		uint8_t tape_load_data[] = {0x31, TAPE_MODE_LOAD, 0x2, 0x0};
		AIData tape_load_message(sizeof(tape_load_data), ID_TAPE, ID_RADIO);
		tape_load_message.refreshAIData(tape_load_data);
		
		ai_driver->writeAIData(&tape_load_message, parameter_list->radio_connected);
		if(text_control)
			sendTapeTextMessage();

		if(autostart && !source_sel)
			requestRadioControl();
	}
	
	if(ack)
		sendIEAckMessage(the_message->sender);
}

//Interpret a tape AIBus message.
void HondaTapeHandler::readAIBusMessage(AIData* the_message) {
	if(the_message->receiver != ID_TAPE)
		return;

	if(the_message->l >= 1 && the_message->data[0] == 0x80) //Acknowledgement.
		return;

	if(!parameter_list->power_on)
		return;
	
	bool ack = true;
	const uint8_t sender = the_message->sender;
	
	if(the_message->l >= 3 && the_message->data[0] == 0x4 && the_message->data[1] == 0xE6 && the_message->data[2] == 0x10) { //Name request.
		ack = false;
		sendAIAckMessage(sender);
		sendSourceNameMessage(the_message->sender);
	} else if(the_message->l == 2 && the_message->data[0] == 0x28 && source_sel) {
		ack = false;
		sendAIAckMessage(sender);
		
		switch(the_message->data[1]) {
			case 0x1: //Direction change.
				this->sendButtonMessage(HONDA_BUTTON_DIR);
				break;
			case 0x9: //Toggle repeat.
				this->sendButtonMessage(HONDA_BUTTON_REPEAT);
				break;
			case 0xA: //Toggle NR.
				this->sendButtonMessage(HONDA_BUTTON_NR);
				break;
		}
	} else if(the_message->l == 3 && the_message->data[0] == 0x28 && the_message->data[1] == 0x4 && source_sel) {
		switch(the_message->data[2]) {
			case 0x0: //Fast Forward.
				this->sendButtonMessage(HONDA_BUTTON_FF);
				break;
			case 0x1: //Rewind.
				this->sendButtonMessage(HONDA_BUTTON_REW);
				break;
		}
	} else if(the_message->l == 4 && the_message->data[0] == 0x28 && the_message->data[1] == 0x7 && source_sel) {		
		uint8_t track_count = the_message->data[3];
		switch(the_message->data[2]) {
			case 0x0: //Forward search.
				if(track_count > 1)
					track_count = 1;
				this->sendButtonMessage(HONDA_BUTTON_SKIPFWD, track_count);
				break;
			case 0x1: //Reverse search.
				if(track_count > 2)
					track_count = 2;
				this->sendButtonMessage(HONDA_BUTTON_SKIPREV, track_count);
				break;
		}
	} else if(the_message->l >= 3 && the_message->data[0] == 0x40 && the_message->data[1] == 0x10 && sender == ID_RADIO) { //Function change.
		const uint8_t active_source = the_message->data[2];

		ack = false;
		sendAIAckMessage(sender);

		if(active_source == ID_TAPE) {
			text_ping_timer = 0;
			text_ping_timer_enabled = true;

			uint8_t function[] = {0x13, 0x0};
			source_sel = true;
			sendFunctionMessage(ie_driver, true, IE_ID_TAPE, function, sizeof(function));
			sendFunctionMessage(ie_driver, false, IE_ID_TAPE, function, sizeof(function));
			//getIEAckMessage(device_ie_id);
			listenForIEBus();
			
			sendTapeUpdateMessage(ID_RADIO);
			*parameter_list->screen_request_timer = SCREEN_REQUEST_TIMER;
			if(this->text_control) {
				if(!parameter_list->imid_connected && !parameter_list->external_imid_tape)
					clearExternalIMID();
				sendTapeTextMessage();
				sendFunctionTextMessage();
			}

			if(parameter_list->audio_pin >= 0)
				digitalWrite(parameter_list->audio_pin, HIGH);

		} else {
			if(source_sel) {
				source_sel = false;
				uint8_t function[] = {0x0, 0x1};
				sendFunctionMessage(ie_driver, true, IE_ID_TAPE, function, sizeof(function));
				getIEAckMessage(device_ie_id);
				requestControl(active_source);
			}

			*active_menu = 0;
			this->text_control = false;

			if(parameter_list->audio_pin >= 0)
				digitalWrite(parameter_list->audio_pin, LOW);
		}
	} else if(the_message->l >= 3 && the_message->data[0] == 0x40 && the_message->data[1] == 0x1 && sender == ID_RADIO) {
		ack = false;
		sendAIAckMessage(sender);

		text_ping_timer_enabled = false;
		
		this->text_control = the_message->data[2] == device_ai_id;
		if(this->text_control) {
			//if(!parameter_list->imid_connected && !parameter_list->external_imid_tape)
			//	clearExternalIMID();
			sendTapeTextMessage();
			sendFunctionTextMessage();
		}
	} else if(the_message->l >= 3 && the_message->data[0] == 0x70 && the_message->data[1] == 0x10 && sender == ID_RADIO) { //Function heartbeat.
		if(source_sel && !text_control) {
			ack = false;
			sendAIAckMessage(sender);

			uint8_t text_request_data[] = {0x60, 0x11};
			AIData text_request_msg(sizeof(text_request_data), ID_TAPE, ID_RADIO, text_request_data);
			ai_driver->writeAIData(&text_request_msg);
		}
	
	} else if(sender == ID_NAV_SCREEN) {
		if(!this->source_sel) {
			ack = false;
			sendAIAckMessage(sender);
			return;
		}

		if(the_message->l >= 3 && the_message->data[0] == 0x30 && source_sel) {
			const uint8_t button = the_message->data[1], state = (the_message->data[2]&0xC0)>>6;
			if(button == 0x11 && state == 0x2) //Preset 1. Repeat.
				sendButtonMessage(HONDA_BUTTON_REPEAT);
			else if(button == 0x12 && state == 0x2) //Preset 2. Switch direction.
				sendButtonMessage(HONDA_BUTTON_DIR);
			else if((button == 0x13 || button == 0x44) && state == 0x2) //Preset 3. Rewind.
				sendButtonMessage(HONDA_BUTTON_REW);
			else if((button == 0x14 || button == 0x45) && state == 0x2) //Preset 4. FF.
				sendButtonMessage(HONDA_BUTTON_FF);
			else if(button == 0x15 && state == 0x2) //Preset 5. NR.
				sendButtonMessage(HONDA_BUTTON_NR);
			else if(button == 0x24 && state == 0x2) //Reverse search. // @TODO: Skip multiple tracks.
				sendButtonMessage(HONDA_BUTTON_SKIPREV, 1);
			else if(button == 0x25 && state == 0x2) //FWD search.
				sendButtonMessage(HONDA_BUTTON_SKIPFWD, 1);
			else if(button == 0x53 && state == 0x2) { //Info.
				ack = false;
				sendAIAckMessage(sender);
				sendFullTapeNavOverlay();
			}
		}
	} else if(the_message->l >= 2 && the_message->data[0] == 0x2B && source_sel) {
		ack = false;
		sendAIAckMessage(sender);
		
		if((the_message->data[1] == 0x4A || the_message->data[1] == 0x45) && sender == ID_RADIO) {
			createTapeMenu();
		} else if(the_message->data[1] == 0x40) {
			//TODO: Change the active menu depending on what cleared it.
			*active_menu = 0;
		} else if(the_message->data[1] == 0x60 && the_message->l >= 3 && sender == ID_NAV_COMPUTER && *active_menu == MENU_TAPE) {
			const MenuList tape_menu = getMenu(MENU_INDEX_TAPE_SETTINGS, parameter_list->locale);
			
			switch(tape_menu.getGlobalIndex(int(the_message->data[2]) - 1)) {
			case MENU_INDEX_TAPE_SETTINGS_FWD_START: //Start in forward mode.
				this->fwd_start = !this->fwd_start;
				setting_changed = true;
				createTapeMenuOption(tape_menu.getLocalIndex(MENU_INDEX_TAPE_SETTINGS_FWD_START));
				break;
			case MENU_INDEX_TAPE_SETTINGS_AUTO_START: //Autostart.
				this->autostart = !this->autostart;
				setting_changed = true;
				createTapeMenuOption(tape_menu.getLocalIndex(MENU_INDEX_TAPE_SETTINGS_AUTO_START));
				break;
			case MENU_INDEX_TAPE_SETTINGS_AUDIO: //Radio settings.
				//TODO: Request radio settings.
				break;
			default:
				break;
			}
		}
	} else if(the_message->sender == ID_RADIO && the_message->l >= 3 && the_message->data[0] == 0x4 && the_message->data[1] == 0xE6 && the_message->data[2] == 0x10) {
		ack = false;
		sendAIAckMessage(sender);
		this->sendSourceNameMessage();
	}
	
	if(ack)
		sendAIAckMessage(sender);
}

//Refresh the source.
void HondaTapeHandler::refreshSource() {
	if(ie_driver->getInputOn())
		this->listenForIEBus();
}

//Send the AIBus handshake message to the radio.
void HondaTapeHandler::sendSourceNameMessage() {
	sendSourceNameMessage(ID_RADIO);
}

//Send the AIBus handshake message.
void HondaTapeHandler::sendSourceNameMessage(const uint8_t id) {
	uint8_t ai_handshake_data[] = {0x1, 0x1, 0x13};
	AIData ai_handshake_msg(sizeof(ai_handshake_data), ID_TAPE, id);

	ai_handshake_msg.refreshAIData(ai_handshake_data);
	
	ai_driver->writeAIData(&ai_handshake_msg, parameter_list->radio_connected);
	
	uint8_t ai_name_data[] = {0x1, 0x23, 0x0, 'T', 'a', 'p', 'e'};
	AIData ai_name_msg(sizeof(ai_name_data), ID_TAPE, id);

	ai_name_msg.refreshAIData(ai_name_data);
	ai_driver->writeAIData(&ai_name_msg, parameter_list->radio_connected);

	ai_name_data[1] = 0x22;
	ai_name_msg.refreshAIData(ai_name_data);
	ai_driver->writeAIData(&ai_name_msg, parameter_list->radio_connected);
}

//Get the AIBus direction byte.
uint8_t HondaTapeHandler::getDirectionByte(const bool rev, const bool repeat, const bool nr, const bool cr_o2) {
	uint8_t the_return = 0;
	if(rev)
		the_return |= (1<<6);
	
	if(repeat)
		the_return |= (1<<5);
		
	if(nr)
		the_return |= (1<<1);
	
	if(cr_o2)
		the_return |= (1);
	
	return the_return;
}

//Send a button IEBus message.
void HondaTapeHandler::sendButtonMessage(const uint8_t button) {
	uint8_t button_data[] = {0x30, 0x0, 0x13, 0x2, 0x13, button};
	IE_Message button_message(sizeof(button_data), IE_ID_RADIO, IE_ID_TAPE, 0xF, true);

	button_message.refreshIEData(button_data);
	
	ie_driver->sendMessage(&button_message, true, true);
	getIEAckMessage(device_ie_id);

	listenForIEBus();
}

//Send a button IEBus message with a track count.
void HondaTapeHandler::sendButtonMessage(const uint8_t button, const uint8_t tracks) {
	uint8_t button_data[] = {0x30, 0x0, 0x13, 0x2, 0x13, button, tracks};
	IE_Message button_message(sizeof(button_data), IE_ID_RADIO, IE_ID_TAPE, 0xF, true);

	button_message.refreshIEData(button_data);
	
	ie_driver->sendMessage(&button_message, true, true);
	getIEAckMessage(device_ie_id);

	listenForIEBus();
}

//Listen for IEBus data.
inline void HondaTapeHandler::listenForIEBus() {
	if(!source_sel)
		return;

	elapsedMillis ie_timer;

	while(ie_timer < 500) {
		if(ie_driver->getInputOn()) {
			IE_Message new_msg;
			const int res = ie_driver->readMessage(&new_msg, true, IE_ID_RADIO);
			if(res == 0) {
				if(new_msg.sender != device_ie_id || new_msg.receiver != IE_ID_RADIO)
					continue;

				if(ie_vec.size() < ie_vec.max_size()) {
					ie_driver->sendAcknowledgement(IE_ID_RADIO, new_msg.sender);
					ie_vec.push_back(new_msg);

					if(new_msg.direct && new_msg.control == 0xF && new_msg.l > 0 && new_msg.data[0] == 0x60)
						break;
				}
			}
		}

		ie_driver->cacheAIBus();
	}
}

//Determine whether the up/down symbol should be shown.
bool HondaTapeHandler::getDisplaySymbol() {
	return (tape_mode == TAPE_MODE_PLAY || tape_mode == TAPE_MODE_REW || tape_mode == TAPE_MODE_REVSKIP || tape_mode == TAPE_MODE_FF || tape_mode == TAPE_MODE_FWDSKIP);
}

//Send the tape info messages as required.
void HondaTapeHandler::sendTapeTextMessage() {
	if(!text_control)
		return;

	if(!nr_timer_enabled) {
		String imid_mode_msg = F("Tape ");
		if(((parameter_list->external_imid_char < 11 && tape_mode != TAPE_MODE_IDLE) || parameter_list->external_imid_lines > 1) && !imid_handler->getEstablished())
			imid_mode_msg = F("");

		String header_mode_msg = F("Tape");
		
		switch(tape_mode) {
			case TAPE_MODE_PLAY:
				imid_mode_msg += "Play";
				header_mode_msg = "Play";
				break;
			case TAPE_MODE_REVSKIP:
			case TAPE_MODE_REW:
				imid_mode_msg += "Rew";
				header_mode_msg = "Rew";
				break;
			case TAPE_MODE_FWDSKIP:
			case TAPE_MODE_FF:
				imid_mode_msg += "FF";
				header_mode_msg = "FF";
				break;
			case TAPE_MODE_LOAD:
				imid_mode_msg += "Load";
				header_mode_msg = "Load";
				break;
			case TAPE_MODE_EJECT:
				imid_mode_msg += "Eject";
				header_mode_msg = "Eject";
				break;
		}

		if(parameter_list->external_imid_char >= 15 && tape_mode == TAPE_MODE_PLAY) {
			if(parameter_list->external_imid_char >= 18) {
				if(repeat_on)
					imid_mode_msg += " Repeat";
				else
					imid_mode_msg += "       ";
			} else {
				if(repeat_on)
					imid_mode_msg += " RPT";
				else
					imid_mode_msg += "    ";
			}
		} else if((parameter_list->external_imid_char >= 12) && (tape_mode == TAPE_MODE_REVSKIP || tape_mode == TAPE_MODE_FWDSKIP)) {
			imid_mode_msg += " " + String(track_count);
		}

		if(parameter_list->imid_connected) {
			if(getDisplaySymbol()) {
				if(fwd) 
					imid_mode_msg += " >";
				else
					imid_mode_msg += " <";
			}
			imid_handler->writeIMIDTextMessage(imid_mode_msg);
		} else if(parameter_list->external_imid_tape) {
			sendTapeUpdateMessage(ID_IMID_SCR);
		} else if(parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines > 0) {
			if(getDisplaySymbol()) {
				if(fwd) 
					imid_mode_msg += " #UP ";
				else
					imid_mode_msg += " #DN ";
			}
			
			if(parameter_list->external_imid_lines > 1) {
				uint8_t imid_tape_data[] = {0x23, 0x60, uint8_t(parameter_list->external_imid_char/2-2), uint8_t(parameter_list->external_imid_lines/2), 'T', 'a', 'p', 'e'};
				AIData imid_tape_msg(sizeof(imid_tape_data), ID_TAPE, ID_IMID_SCR);
				imid_tape_msg.refreshAIData(imid_tape_data);
				
				ai_driver->writeAIData(&imid_tape_msg);
			}

			int effective_length = imid_mode_msg.length();
			{
				String effective_string = imid_mode_msg;
				effective_string.replace("#UP ", "#");
				effective_string.replace("#DN ", "#");

				effective_length = effective_string.length();
			}

			AIData imid_text_msg(4 + imid_mode_msg.length(), ID_TAPE, ID_IMID_SCR);
			imid_text_msg.data[0] = 0x23;
			imid_text_msg.data[1] = 0x60;
			imid_text_msg.data[2] = parameter_list->external_imid_char/2-effective_length/2;
			
			if(parameter_list->external_imid_lines == 1)
				imid_text_msg.data[3] = 1;
			else
				imid_text_msg.data[3] = parameter_list->external_imid_lines/2+1;

			for(int i=0;i<imid_mode_msg.length();i+=1)
				imid_text_msg.data[i+4] = uint8_t(imid_mode_msg.charAt(i));

			ai_driver->writeAIData(&imid_text_msg);
		}

		if(tape_mode == TAPE_MODE_REVSKIP || tape_mode == TAPE_MODE_FWDSKIP)
			header_mode_msg += " " + String(track_count);

		if(getDisplaySymbol()) {
			if(fwd)
				header_mode_msg += " #UP ";
			else
				header_mode_msg += " #DN ";
		}

		setNavHeader(header_mode_msg);
	} else {
		String nr_mode_txt = "NR On";
		if(!nr_on)
			nr_mode_txt = "NR Off";

		setNavHeader(nr_mode_txt);

		if(parameter_list->imid_connected)
			imid_handler->writeIMIDTextMessage(nr_mode_txt);
		else if(parameter_list->external_imid_tape)
			sendTapeUpdateMessage(ID_IMID_SCR);
		else if(parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines > 0) {
			AIData imid_text_msg(4 + nr_mode_txt.length(), ID_TAPE, ID_IMID_SCR);
			imid_text_msg.data[0] = 0x23;
			imid_text_msg.data[1] = 0x60;
			imid_text_msg.data[2] = parameter_list->external_imid_char/2-nr_mode_txt.length()/2;
			
			if(parameter_list->external_imid_lines == 1)
				imid_text_msg.data[3] = 1;
			else
				imid_text_msg.data[3] = parameter_list->external_imid_lines/2+1;

			for(int i=0;i<nr_mode_txt.length();i+=1)
				imid_text_msg.data[i+4] = uint8_t(nr_mode_txt.charAt(i));

			ai_driver->writeAIData(&imid_text_msg);
		}
	}

	String mode_msg = " ";
	switch(tape_mode) {
		case TAPE_MODE_PLAY:
			mode_msg = "Play";
			if(repeat_on)
				mode_msg += " Repeat";
			break;
		case TAPE_MODE_REVSKIP:
		case TAPE_MODE_REW:
			mode_msg = "Rew";
			break;
		case TAPE_MODE_FWDSKIP:
		case TAPE_MODE_FF:
			mode_msg = "FF";
			break;
		case TAPE_MODE_LOAD:
			mode_msg = "Load";
			break;
		case TAPE_MODE_EJECT:
			mode_msg = "Eject";
			break;
		case TAPE_MODE_IDLE:
			mode_msg = "No Tape";
			break;
	}

	if(tape_mode == TAPE_MODE_FWDSKIP || tape_mode == TAPE_MODE_REVSKIP) {
		mode_msg += " ";
		mode_msg += String(this->track_count);
	}

	if(getDisplaySymbol()) {
		if(fwd) 
			mode_msg += " #UP ";
		else
			mode_msg += " #DN ";
	}

	String mode_symbol = " ";
	switch(tape_mode) {
		case TAPE_MODE_PLAY:
			if(fwd)
				mode_symbol = "#FWD";
			else
				mode_symbol = "#REV";
			break;
		case TAPE_MODE_FF:
		case TAPE_MODE_FWDSKIP:
			if(fwd)
				mode_symbol = "#FF ";
			else
				mode_symbol = "#REW";
			break;
		case TAPE_MODE_REW:
		case TAPE_MODE_REVSKIP:
			if(fwd)
				mode_symbol = "#REW";
			else
				mode_symbol = "#FF ";
	}

	String nr_msg = " ";
	if(nr_on)
		nr_msg = "NR";

	String bias_msg = " ";
	if(tape_mode != TAPE_MODE_EJECT && tape_mode != TAPE_MODE_LOAD && tape_mode != TAPE_MODE_IDLE) {
		if(cro2)
			bias_msg = "High Bias";
		else
			bias_msg = "Type I Bias";
	}

	AIData text_msg1 = getTextMessage(ID_TAPE, mode_msg, 0, 1, false);
	AIData text_msg2 = getTextMessage(ID_TAPE, mode_symbol, 0, 3, false);

	AIData sub_msg1 = getTextMessage(ID_TAPE, nr_msg, 1, 0, false);
	AIData sub_msg2 = getTextMessage(ID_TAPE, bias_msg, 1, 1, true);

	ai_driver->writeAIData(&text_msg1, parameter_list->computer_connected);
	ai_driver->writeAIData(&text_msg2, parameter_list->computer_connected);
	ai_driver->writeAIData(&sub_msg1, parameter_list->computer_connected);
	ai_driver->writeAIData(&sub_msg2, parameter_list->computer_connected);

	this->sendMirrorMessage(mode_msg, 1, true);
	this->sendMirrorMessage(nr_msg, 4, true);
}

//Set the function buttons on the main screen.
void HondaTapeHandler::sendFunctionTextMessage() {
	AIData title = getTextMessage(ID_TAPE, F("Tape"), 0x0, 0, false);
	AIData function1 = getTextMessage(ID_TAPE, F("Repeat"), 0x2, 0, false);
	AIData function2 = getTextMessage(ID_TAPE, F("#REV #FWD"), 0x2, 1, false);
	AIData function3 = getTextMessage(ID_TAPE, F("#REW"), 0x2, 2, false);
	AIData function4 = getTextMessage(ID_TAPE, F("#FF "), 0x2, 3, false);
	AIData function5 = getTextMessage(ID_TAPE, F("NR"), 0x2, 4, true);

	ai_driver->writeAIData(&title, parameter_list->computer_connected);
	ai_driver->writeAIData(&function1, parameter_list->computer_connected);
	ai_driver->writeAIData(&function2, parameter_list->computer_connected);
	ai_driver->writeAIData(&function3, parameter_list->computer_connected);
	ai_driver->writeAIData(&function4, parameter_list->computer_connected);
	ai_driver->writeAIData(&function5, parameter_list->computer_connected);
	this->sendMirrorMessage("Tape", 0, true);
}

//Send a tape status AIBus message.
void HondaTapeHandler::sendTapeUpdateMessage(const uint8_t receiver) {
	uint8_t tape_update_data[] = {0x31, tape_mode, getDirectionByte(!this->fwd, this->repeat_on, this->nr_on, this->cro2), track_count};
	AIData tape_update_message(sizeof(tape_update_data), ID_TAPE, receiver);
	tape_update_message.refreshAIData(tape_update_data);
	
	bool ack = true;
	if(receiver == ID_RADIO && !parameter_list->radio_connected)
		ack = false;
	else if(receiver == ID_NAV_COMPUTER && !parameter_list->computer_connected)
		ack = false;
	
	ai_driver->writeAIData(&tape_update_message, ack);
}

//Send the overlay to the nav screen.
void HondaTapeHandler::sendFullTapeNavOverlay() {
	String tape_msg = "";

	switch(tape_mode) {
	case TAPE_MODE_PLAY:
		tape_msg += "Play";
		break;
	case TAPE_MODE_REVSKIP:
	case TAPE_MODE_REW:
		tape_msg += "Rew";
		break;
	case TAPE_MODE_FWDSKIP:
	case TAPE_MODE_FF:
		tape_msg += "FF";
		break;
	case TAPE_MODE_LOAD:
		tape_msg += "Load";
		break;
	case TAPE_MODE_EJECT:
		tape_msg += "Eject";
		break;
	default:
		tape_msg += "Tape";
		break;
	}

	if(getDisplaySymbol()) {
		if(fwd)
			tape_msg += " #UP ";
		else
			tape_msg += " #DN ";
	}

	if(repeat_on)
		tape_msg += "   Repeat";

	if(nr_on)
		tape_msg += "   NR";

	setNavHeader(tape_msg);
}

//Create the tape main menu.
void HondaTapeHandler::createTapeMenu() {
	const MenuList main_menu = getMenu(MENU_INDEX_TAPE_SETTINGS, parameter_list->locale);
	const uint8_t menu_size = main_menu.size();
	startAudioMenu(menu_size, menu_size, false, main_menu.title);

	elapsedMillis cancel_wait;
	while(cancel_wait < 20) {
		AIData ai_msg;
		if(ai_driver->dataAvailable(false) > 0) {
			if(ai_driver->readAIData(&ai_msg, false)) {
				if(ai_msg.l >= 2 && ai_msg.sender == ID_NAV_COMPUTER && ai_msg.data[0] == 0x2B && ai_msg.data[1] == 0x40) { //No menu available.
					sendAIAckMessage(ai_msg.sender);
					return;
				}
			}
		}
	}

	*active_menu = MENU_TAPE;

	for(uint8_t i=0;i<menu_size;i+=1)
		createTapeMenuOption(i);
	displayAudioMenu(1);
}

//Create a tape main menu option.
void HondaTapeHandler::createTapeMenuOption(const uint8_t option) {
	const MenuList main_menu = getMenu(MENU_INDEX_TAPE_SETTINGS, parameter_list->locale);

	String item_txt = "";

	if(option==main_menu.getLocalIndex(MENU_INDEX_TAPE_SETTINGS_AUTO_START))
		item_txt = autostart ? "#RON " : "#ROF ";
	else if(option==main_menu.getLocalIndex(MENU_INDEX_TAPE_SETTINGS_FWD_START))
		item_txt = fwd_start ? "#RON " : "#ROF ";

	item_txt += main_menu[option];
	appendAudioMenu(option, item_txt);

	/*switch(option) {
	case 0:
		if(fwd_start)
			appendAudioMenu(0, "#RON Start in Forward Mode");
		else
			appendAudioMenu(0, "#ROF Start in Forward Mode");
		break;
	case 1:
		if(autostart)
			appendAudioMenu(1, "#RON Auto Select");
		else
			appendAudioMenu(1, "#ROF Auto Select");
		break;
	case 2:
		appendAudioMenu(2, "Audio Settings");
		break;
	}*/
}
