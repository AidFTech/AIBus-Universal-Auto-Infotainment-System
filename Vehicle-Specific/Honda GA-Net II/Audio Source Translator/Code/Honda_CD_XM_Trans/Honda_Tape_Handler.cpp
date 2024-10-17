#include "Honda_Tape_Handler.h"

HondaTapeHandler::HondaTapeHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list, HondaIMIDHandler* imid_handler) : HondaSourceHandler(ie_driver, ai_driver, parameter_list) {
	this->device_ie_id = IE_ID_TAPE;
	this->device_ai_id = ID_TAPE;
	this->imid_handler = imid_handler;
}

void HondaTapeHandler::interpretTapeMessage(IE_Message* the_message) {
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
		
		const uint8_t direction_state = the_message->data[7], track_state = the_message->data[6];
		const uint8_t last_mode = tape_mode, last_dir = getDirectionByte(!fwd, repeat_on, nr_on, cro2), last_track = track_count;
		
		if(tape_state != last_mode || direction_state != last_dir || track_state != last_track) {
			ack = false;
			sendIEAckMessage(the_message->sender);
			
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

void HondaTapeHandler::readAIBusMessage(AIData* the_message) {
	if(the_message->receiver != ID_TAPE)
		return;

	if(the_message->l >= 1 && the_message->data[0] == 0x80) //Acknowledgement.
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
	} else if(the_message->data[0] == 0x40 && the_message->data[1] == 0x10 && sender == ID_RADIO && the_message->l >= 3) { //Function change.
		const uint8_t active_source = the_message->data[2];

		ack = false;
		sendAIAckMessage(sender);

		if(active_source == ID_TAPE) {
			uint8_t function[] = {0x13, 0x0};
			source_sel = true;
			sendFunctionMessage(ie_driver, true, IE_ID_TAPE, function, sizeof(function));
			sendFunctionMessage(ie_driver, false, IE_ID_TAPE, function, sizeof(function));
			getIEAckMessage(device_ie_id);
			
			sendTapeUpdateMessage(ID_RADIO);
			*parameter_list->screen_request_timer = SCREEN_REQUEST_TIMER;
			if(this->text_control) {
				if(!parameter_list->imid_connected && !parameter_list->external_imid_tape)
					clearExternalIMID();
				sendTapeTextMessage();
				sendFunctionTextMessage();
			}
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
		}
	} else if(the_message->data[0] == 0x40 && the_message->data[1] == 0x1 && sender == ID_RADIO && the_message->l >= 3) {
		ack = false;
		sendAIAckMessage(sender);
		
		this->text_control = the_message->data[2] == device_ai_id;
		if(this->text_control) {
			//if(!parameter_list->imid_connected && !parameter_list->external_imid_tape)
			//	clearExternalIMID();
			sendTapeTextMessage();
			sendFunctionTextMessage();
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
			else if(button == 0x24 && state == 0x2) //Reverse search. //TODO: Skip multiple tracks.
				sendButtonMessage(HONDA_BUTTON_SKIPREV, 1);
			else if(button == 0x25 && state == 0x2) //FWD search.
				sendButtonMessage(HONDA_BUTTON_SKIPFWD, 1);
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
			switch(the_message->data[2]) {
			case 1: //Start in forward mode.
				this->fwd_start = !this->fwd_start;
				createTapeMenuOption(0);
				break;
			case 2: //Autostart.
				this->autostart = !this->autostart;
				createTapeMenuOption(1);
				break;
			case 3: //Radio settings.
				//TODO: Request radio settings.
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

void HondaTapeHandler::sendSourceNameMessage() {
	sendSourceNameMessage(ID_RADIO);
}

void HondaTapeHandler::sendSourceNameMessage(const uint8_t id) {
	uint8_t ai_handshake_data[] = {0x1, 0x1, 0x13};
	AIData ai_handshake_msg(sizeof(ai_handshake_data), ID_TAPE, id);

	ai_handshake_msg.refreshAIData(ai_handshake_data);
	
	ai_driver->writeAIData(&ai_handshake_msg, parameter_list->radio_connected);
	
	uint8_t ai_name_data[] = {0x1, 0x23, 0x0, 'T', 'a', 'p', 'e'};
	AIData ai_name_msg(sizeof(ai_name_data), ID_TAPE, id);

	ai_name_msg.refreshAIData(ai_name_data);
	
	ai_driver->writeAIData(&ai_name_msg, parameter_list->radio_connected);
}

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

void HondaTapeHandler::sendButtonMessage(const uint8_t button) {
	uint8_t button_data[] = {0x30, 0x0, 0x13, 0x2, 0x13, button};
	IE_Message button_message(sizeof(button_data), IE_ID_RADIO, IE_ID_TAPE, 0xF, true);

	button_message.refreshIEData(button_data);
	
	ie_driver->sendMessage(&button_message, true, true);
	getIEAckMessage(device_ie_id);
}

void HondaTapeHandler::sendButtonMessage(const uint8_t button, const uint8_t tracks) {
	uint8_t button_data[] = {0x30, 0x0, 0x13, 0x2, 0x13, button, tracks};
	IE_Message button_message(sizeof(button_data), IE_ID_RADIO, IE_ID_TAPE, 0xF, true);

	button_message.refreshIEData(button_data);
	
	ie_driver->sendMessage(&button_message, true, true);
	getIEAckMessage(device_ie_id);
}

bool HondaTapeHandler::getDisplaySymbol() {
	return (tape_mode == TAPE_MODE_PLAY || tape_mode == TAPE_MODE_REW || tape_mode == TAPE_MODE_REVSKIP || tape_mode == TAPE_MODE_FF || tape_mode == TAPE_MODE_FWDSKIP);
}

void HondaTapeHandler::sendTapeTextMessage() {
	if(!text_control)
		return;

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

	String imid_mode_msg = F("Tape ");
	if((parameter_list->external_imid_char < 11 || parameter_list->external_imid_lines > 1) && !imid_handler->getEstablished())
		imid_mode_msg = F("");
	
	switch(tape_mode) {
		case TAPE_MODE_PLAY:
			imid_mode_msg += "Play";
			break;
		case TAPE_MODE_REVSKIP:
		case TAPE_MODE_REW:
			imid_mode_msg += "Rew";
			break;
		case TAPE_MODE_FWDSKIP:
		case TAPE_MODE_FF:
			imid_mode_msg += "FF";
			break;
		case TAPE_MODE_LOAD:
			imid_mode_msg += "Load";
			break;
		case TAPE_MODE_EJECT:
			imid_mode_msg += "Eject";
			break;
	}

	if((parameter_list->external_imid_char >= 12) && (tape_mode == TAPE_MODE_REVSKIP || tape_mode == TAPE_MODE_FWDSKIP)) {
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
			uint8_t imid_tape_data[] = {0x23, 0x60, parameter_list->external_imid_char/2-2, parameter_list->external_imid_lines/2, 'T', 'a', 'p', 'e'};
			AIData imid_tape_msg(sizeof(imid_tape_data), ID_TAPE, ID_IMID_SCR);
			imid_tape_msg.refreshAIData(imid_tape_data);
			
			ai_driver->writeAIData(&imid_tape_msg);
		}

		AIData imid_text_msg(4 + imid_mode_msg.length(), ID_TAPE, ID_IMID_SCR);
		imid_text_msg.data[0] = 0x23;
		imid_text_msg.data[1] = 0x60;
		imid_text_msg.data[2] = parameter_list->external_imid_char/2-imid_mode_msg.length()/2;
		
		if(parameter_list->external_imid_lines == 1)
			imid_text_msg.data[3] = 1;
		else
			imid_text_msg.data[3] = parameter_list->external_imid_lines/2+1;

		for(int i=0;i<imid_mode_msg.length();i+=1)
			imid_text_msg.data[i+4] = uint8_t(imid_mode_msg.charAt(i));

		ai_driver->writeAIData(&imid_text_msg);
	}
}

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
}

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

void HondaTapeHandler::createTapeMenu() {
	const uint8_t menu_size = 3;
	startAudioMenu(menu_size, menu_size, false, "Tape Settings");

	elapsedMillis cancel_wait;
	while(cancel_wait < 20) {
		AIData ai_msg;
		if(ai_driver->dataAvailable() > 0) {
			if(ai_driver->readAIData(&ai_msg)) {
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

void HondaTapeHandler::createTapeMenuOption(const uint8_t option) {
	switch(option) {
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
	}
}
