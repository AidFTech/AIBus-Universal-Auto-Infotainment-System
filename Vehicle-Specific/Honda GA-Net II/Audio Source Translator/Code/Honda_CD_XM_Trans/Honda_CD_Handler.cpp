#include "Honda_CD_Handler.h"

HondaCDHandler::HondaCDHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list, HondaIMIDHandler *imid_handler) : HondaSourceHandler(ie_driver, ai_driver, parameter_list) {
	this->device_ie_id = IE_ID_CDC;
	this->device_ai_id = ID_CDC;

	this->imid_handler = imid_handler;
	this->clearCDText(true, true, true, true, true);

	getCDSettings(&this->autostart, &this->use_function_timer);
}

void HondaCDHandler::loop() {
	if(this->source_sel) {
		if(text_timer_enabled && text_timer >= TEXT_REFRESH_TIMER) {
			text_timer_enabled = false;

			if(text_mode == TEXT_MODE_WITH_TEXT) {
				text_timer_file = false;
				text_timer_folder = false;

				if(text_timer_song) {
					text_timer_song = false;
					sendCDTextMessage(TEXT_SONG, true);
				}
				if(text_timer_artist) {
					text_timer_artist = false;
					sendCDTextMessage(TEXT_ARTIST, true);
				}
				if(text_timer_album) {
					text_timer_album = false;
					sendCDTextMessage(TEXT_ALBUM, true);
				}

				if(strlen(song_title) <= 0 || strlen(artist) <= 0 || strlen(album) <= 0)
					sendTextRequest();
				
			} else if(text_mode == TEXT_MODE_MP3) {
				if(text_timer_folder) {
					text_timer_folder = false;
					sendCDTextMessage(0, true);
				}
				if(text_timer_file) {
					text_timer_file = false;
					sendCDTextMessage(1, true);
				}
				if(text_timer_song) {
					text_timer_song = false;
					sendCDTextMessage(2, true);
				}
				if(text_timer_album) {
					text_timer_album = false;
					sendCDTextMessage(3, true);
				}
				if(text_timer_artist) {
					text_timer_artist = false;
					sendCDTextMessage(4, true);
				}
			}

			if(text_timer_info_change) {
				text_timer_info_change = false;
				incrementInfo();
			}
		}
	
		if(text_mode == TEXT_MODE_MP3 && text_request_timer > MP3_REQUEST_TIMER && (strlen(song_title) <= 0 || strlen(artist) <= 0 || strlen(album) <= 0 || strlen(filename) <= 0 || strlen(folder) <= 0)) {
			text_request_timer = 0;
			sendTextRequest();
		}

		if(display_timer_enabled && display_timer >= DISPLAY_INFO_TIMER) {
			display_timer_enabled = false;
			scroll_timer = 0;
			scroll_state = 0;
			sendIMIDInfoMessage();
		}

		unsigned int scroll_limit = SCROLL_TIMER;
		if(scroll_state <= 0)
			scroll_limit =  SCROLL_TIMER_0;

		if(display_parameter != TEXT_NONE && !text_timer_enabled && !display_timer_enabled && scroll_timer >= scroll_limit) {
			scroll_timer = 0;
			scroll_state += 1;
			sendIMIDInfoMessage(false);
		}

		if(use_function_timer && text_control && text_mode != TEXT_MODE_BLANK && function_timer_enabled && function_timer >= FUNCTION_CHANGE_TIMER) {
			function_timer = 0;
			function_timer_enabled = false;
			if(parameter_list->imid_connected) {
				imid_handler->setIMIDSource(ID_CD, 0x0);
			}
			sendCDIMIDTrackandTimeMessage();

			text_timer_enabled = true;
			text_timer_album = true;
			text_timer_artist = true;
			text_timer_song = true;
			text_timer = 0;
		}

		if((ff || fr) && ff_fr_timer >= FRFF_INTERVAL) {
			ff_fr_timer = 0;
			if(ff)
				sendButtonMessage(BUTTON_FF);
			else if(fr)
				sendButtonMessage(BUTTON_FR);
		}

		if(disc_change_timer_enabled && disc_change_timer >= SHORT_CHANGE_TIMER) {
			disc_change_timer_enabled = false;
			sendButtonMessage(BUTTON_DISC_CHANGE, disc_change_disc);
		}
		
		if(info_change_enabled && info_change_timer >= SHORT_CHANGE_TIMER) {
			info_change_enabled = false;
			incrementInfo();
		}

		if(mode_timer_enabled && mode_timer > MODE_FLASH_TIMER) {
			mode_timer_enabled = false;

			if(use_function_timer && parameter_list->imid_connected && text_mode != TEXT_MODE_BLANK) {
				imid_handler->setIMIDSource(ID_CD, 0);
			} else if(parameter_list->imid_connected) {
				imid_handler->setIMIDSource(ID_CDC, 0);
			}

			sendCDIMIDTrackandTimeMessage();
			text_timer = 0;
			text_timer_enabled = true;
			text_timer_song = true;
			text_timer_artist = true;
			text_timer_album = true;
		}
	
		if(setting_changed) {
			setting_changed = false;
			setCDSettings(this->autostart, this->use_function_timer);
		}
	}
}

void HondaCDHandler::interpretCDMessage(IE_Message* the_message) {
	if(the_message->receiver != IE_ID_RADIO || the_message->sender != IE_ID_CDC)
		return;

	if(!source_established && (the_message->l > 4 || (the_message->l >= 1 && the_message->data[0] == 0x80))) {
		ie_driver->addID(ID_CDC);
		source_established = true;
		if(parameter_list->radio_connected)
			sendSourceNameMessage();
	}

	bool ack = true;
	
	if(the_message->l == 4) { //Handshake message.
		if(the_message->data[0] == 0x1 && the_message->data[1] == 0x1) {
			ack = false;
			sendIEAckMessage(IE_ID_CDC);

			if(this->parameter_list->first_cd) {
				this->parameter_list->first_cd = true;

				uint8_t handshake_data[] = {0x0, 0x30};
				IE_Message handshake_msg(sizeof(handshake_data), IE_ID_RADIO, 0x1FF, 0xF, false);
				handshake_msg.refreshIEData(handshake_data);
				
				delay(2);

				ie_driver->sendMessage(&handshake_msg, false, false);
			}
		} else if(the_message->data[0] == 0x5 && the_message->data[2] == 0x3) {
			ack = false;
			sendIEAckMessage(IE_ID_CDC);

			delay(2);
			
			const bool handshake_rec = sendHandshakeAckMessage();
			if(!source_established) {
				source_established = handshake_rec;

				if(source_established && parameter_list->radio_connected)
					sendSourceNameMessage();
			}
		}
	} else if(the_message->l >= 0xF && the_message->data[0] == 0x60 && the_message->data[1] == 0x6) { //Display change message.
		ack = false;
		sendIEAckMessage(IE_ID_CDC);

		if(!source_sel) {
			uint8_t function[] = {0x0, 0x1};
			sendFunctionMessage(ie_driver, true, IE_ID_CDC, function, sizeof(function));
			getIEAckMessageStrict(device_ie_id);
			return;
		}

		const uint8_t message_type = the_message->data[4], str = the_message->data[5], last_status = getIECDStatus(ai_cd_status), last_repeat = getIECDRepeat(ai_cd_status);
		const uint8_t last_folder = folder_num;
			
		if(str == 0x0) {
			ai_cd_status = getAICDStatus(message_type, the_message->data[12]);

			const uint8_t last_text_mode = text_mode;

			switch(the_message->data[13]) {
				case 0x22:
					text_mode = TEXT_MODE_WITH_TEXT;
					break;
				case 0x60:
					text_mode = TEXT_MODE_MP3;
					break;
				default:
					text_mode = TEXT_MODE_BLANK;
					break;
			}

			folder_num = getByteFromBCD(the_message->data[15]);

			if(last_text_mode == TEXT_MODE_BLANK && text_mode != TEXT_MODE_BLANK && use_function_timer) {
				function_timer = 0;
				function_timer_enabled = true;
			}

			const uint16_t disc_and_track = (disc << 8) | track;
		
			disc = getByteFromBCD(the_message->data[6]);
			track = getByteFromBCD(the_message->data[10]);

			if(text_control) {
				text_timer = 0;
				text_timer_enabled = true;
			}
			
			if(disc_and_track != ((disc << 8) | track) || getIECDStatus(ai_cd_status) != last_status || getIECDRepeat(ai_cd_status) != last_repeat) {
				uint8_t track_data[] = {0x39, 0x0, ai_cd_status, 0x0, 0x3F, 0x0, disc, track};

				AIData track_msg(sizeof(track_data), ID_CDC, ID_RADIO);
				track_msg.refreshAIData(track_data);

				ai_driver->writeAIData(&track_msg, parameter_list->radio_connected);

				if(disc != ((disc_and_track&0xFF00)>>8)) {
					display_parameter = TEXT_NONE;
					if(use_function_timer && parameter_list->imid_connected && text_control) {
						imid_handler->setIMIDSource(ID_CDC, 0);
						function_timer = 0;
						function_timer_enabled = true;
					}
				}

				clearCDText(track != (disc_and_track&0xFF), track != (disc_and_track&0xFF), disc != ((disc_and_track&0xFF00)>>8) || text_mode == TEXT_MODE_MP3, folder_num != last_folder && text_mode == TEXT_MODE_MP3,  track != (disc_and_track&0xFF) && text_mode==TEXT_MODE_MP3);
				text_request_timer = 0;
				if(text_control) {		
					clearAICDText(track != (disc_and_track&0xFF), track != (disc_and_track&0xFF), disc != ((disc_and_track&0xFF00)>>8) || text_mode == TEXT_MODE_MP3, folder_num != last_folder && text_mode == TEXT_MODE_MP3, track != (disc_and_track&0xFF) && text_mode==TEXT_MODE_MP3);
					
					if((disc != ((disc_and_track&0xFF00)>>8)) || text_mode == TEXT_MODE_MP3) 
						clearExternalCDIMID(true);
					
					sendCDTrackMessage();
				}
			}
		
			const int16_t last_timer = timer;
			timer = getByteFromBCD(the_message->data[7])*60 + getByteFromBCD(the_message->data[8]);
			
			if(timer != last_timer) {
				uint8_t timer_data[] = {0x3B, 0x0, 0x0, (timer&0xFF00) >> 8, (timer&0xFF)};

				AIData timer_msg(sizeof(timer_data), ID_CDC, ID_RADIO);
				timer_msg.refreshAIData(timer_data);
				
				ai_driver->writeAIData(&timer_msg, parameter_list->radio_connected);
				if(text_control)
					sendCDTimeMessage();
			}

			if(text_control) {
				if(timer != last_timer || disc_and_track != ((disc<<8) | track) || getIECDStatus(ai_cd_status) != last_status || getIECDRepeat(ai_cd_status) != last_repeat) {
					if(getIECDRepeat(ai_cd_status) != last_repeat &&
					parameter_list->imid_connected || (parameter_list->external_imid_char > 0 && parameter_list-> external_imid_char < 16 && parameter_list->external_imid_lines == 1 && !parameter_list->external_imid_cd)) {
						this->mode_timer_enabled = true;
						this->mode_timer = 0;
					}

					sendCDIMIDTrackandTimeMessage();
				}

				sendCDLoadWaitMessage(message_type);
			}
		} else if((str == 0x80 || str == 0x81 || str == 0x82 || (str >= 0x70 && str < 0x80)) && (message_type == 3 || message_type == 2)) {
			char* affected;
			uint8_t* stage;
			unsigned int len = 0;

			if(text_mode == TEXT_MODE_WITH_TEXT) {
				switch(str) {
					case 0x80:
						affected = this->album;
						stage = &this->album_stage;
						len = sizeof(this->album)/sizeof(char);
						break;
					case 0x81:
						affected = this->song_title;
						stage = &this->song_title_stage;
						len = sizeof(this->song_title)/sizeof(char);
						break;
					case 0x82:
						affected = this->artist;
						stage = &this->artist_stage;
						len = sizeof(this->artist)/sizeof(char);
						break;
					default:
						return;
				}
			} else if(text_mode == TEXT_MODE_MP3) {
				switch(str) {
					case 0x70:
						affected = this->folder;
						stage = &this->folder_stage;
						len = sizeof(this->folder)/sizeof(char);
						break;
					case 0x71:
						affected = this->filename;
						stage = &this->filename_stage;
						len = sizeof(this->filename)/sizeof(char);
						break;
					case 0x72:
						affected = this->song_title;
						stage = &this->song_title_stage;
						len = sizeof(this->song_title)/sizeof(char);
						break;
					case 0x73:
						affected = this->album;
						stage = &this->album_stage;
						len = sizeof(this->album)/sizeof(char);
						break;
					case 0x74:
						affected = this->artist;
						stage = &this->artist_stage;
						len = sizeof(this->artist)/sizeof(char);
						break; 
				}
			}

			uint8_t increment = the_message->data[6], msg_stage = the_message->data[7]>>4;
			if(increment > 2)
				increment = 1;
			
			unsigned int start = 0;
			if(msg_stage > 0)
				start = 16;
			else
				len = 16;

			for(unsigned int i=start;i<len;i+=1) {
				const unsigned int d = (i-start)*increment + (increment-1) + 8;
				if(d > the_message->l - 1)
					break;

				if(the_message->data[d] != 0xFF)
					affected[i] = the_message->data[d];
				else
					affected[i] = 0;
			}

			if(msg_stage == 0)
				*stage = 1;
			else
				*stage = 2;
			
			if(*stage >= 2) {
				*stage = 0;
				sendAICDTextMessage(ID_RADIO, str&0xF);
				if(text_control) {
					startTextTimer(str&0xF);
				}
			}
		}
	}
	
	if(ack)
		ie_driver->sendAcknowledgement(the_message->receiver, the_message->sender);
}

void HondaCDHandler::readAIBusMessage(AIData* the_message) {
	if(the_message->receiver != ID_CDC)
		return;

	if(the_message->l >= 1 && the_message->data[0] == 0x80) //Acknowledgement.
		return;
	
	bool ack = true;
	const uint8_t sender = the_message->sender;
	
	if(the_message->l >= 3 && the_message->data[0] == 0x4 && the_message->data[1] == 0xE6 && the_message->data[2] == 0x10) { //Name request.
		ack = false;
		sendAIAckMessage(sender);
		sendSourceNameMessage(the_message->sender);
	} else if(the_message->l >= 3 && the_message->data[0] == 0x40 && the_message->data[1] == 0x10 && sender == ID_RADIO && the_message->l >= 3) { //Function change.
		const uint8_t active_source = the_message->data[2];

		ack = false;
		sendAIAckMessage(sender);

		if(active_source == ID_CDC) {

			uint8_t function[] = {0x6, 0x0, 0x1};
			source_sel = true;
			sendFunctionMessage(ie_driver, true, IE_ID_CDC, function, sizeof(function));
			
			elapsedMillis function_timer;
			while(!getIEAckMessage(device_ie_id) && function_timer < 200)
				sendFunctionMessage(ie_driver, true, IE_ID_CDC, function, sizeof(function));

			sendFunctionMessage(ie_driver, false, IE_ID_CDC, function, sizeof(function));

			function_timer = 0;
			while(!getIEAckMessage(device_ie_id) && function_timer < 200)
				sendFunctionMessage(ie_driver, false, IE_ID_CDC, function, sizeof(function));
			
			*parameter_list->screen_request_timer = SCREEN_REQUEST_TIMER;
			sendAICDStatusMessage(ID_RADIO);
			if(this->text_control) {
				sendCDTrackMessage();
				sendCDTimeMessage();

				if(!parameter_list->imid_connected && !parameter_list->external_imid_cd)
					clearExternalIMID();

				sendCDIMIDTrackandTimeMessage();

				startTextTimer(TEXT_SONG);
				startTextTimer(TEXT_ALBUM);
				startTextTimer(TEXT_ARTIST);

				sendFunctionTextMessage();

				sendCDLoadWaitMessage(getIECDStatus(ai_cd_status));

				if(use_function_timer) {
					function_timer = 0;
					function_timer_enabled = true;
				}
			}
		} else {
			if(source_sel) {
				source_sel = false;
				uint8_t function[] = {0x0, 0x1};
				sendFunctionMessage(ie_driver, true, IE_ID_CDC, function, sizeof(function));
				getIEAckMessageStrict(device_ie_id);
				requestControl(active_source);
			}

			display_parameter = TEXT_NONE;
			*active_menu = 0;
			this->text_control = false;
			clearCDText(true, true, true, true, true);
		}
	} else if (the_message->data[0] == 0x40 && the_message->data[1] == 0x1 && sender == ID_RADIO && the_message->l >= 3) {
		ack = false;
		sendAIAckMessage(sender);
	
		this->text_control = the_message->data[2] == device_ai_id;
		if(this->text_control) {
			sendCDTrackMessage();
			sendCDTimeMessage();

			if(!parameter_list->imid_connected && !parameter_list->external_imid_cd)
				clearExternalIMID();

			sendCDIMIDTrackandTimeMessage();

			startTextTimer(TEXT_SONG);
			startTextTimer(TEXT_ALBUM);
			startTextTimer(TEXT_ARTIST);

			sendFunctionTextMessage();

			sendCDLoadWaitMessage(getIECDStatus(ai_cd_status));

			if(use_function_timer) {
				function_timer = 0;
				function_timer_enabled = true;
			}
		}
	} else if(sender == ID_NAV_SCREEN) {
		if(!this->source_sel) {
			ack = false;
			sendAIAckMessage(sender);
			return;
		}

		if(the_message->l >= 3 && the_message->data[0] == 0x30 && source_sel) { //Button pressed.
			const uint8_t button = the_message->data[1], state = (the_message->data[2]&0xC0)>>6;
			if(button == 0x53 && state == 0x2) { //Info button.
				info_change_timer = 0;
				info_change_enabled = true;
			} else if(button == 0x11 && state == 0x2) { //Preset 1, Repeat
				sendButtonMessage(BUTTON_REPEAT);
			} else if(button == 0x12 && state == 0x2) { //Preset 2, Random
				sendButtonMessage(BUTTON_RANDOM);
			} else if((button == 0x13 || button == 0x44) && (state == 0x0 || state == 0x1)) { //Preset 3, FR Pressed
				this->fr = true;
				ff_fr_timer = 0;
				sendButtonMessage(BUTTON_FR);
			} else if((button == 0x13 || button == 0x44) && state == 0x2) { //Preset 3, FR Released
				this->fr = false;
			} else if((button == 0x14 || button == 0x45) && (state == 0x0 || state == 0x1)) { //Preset 4, FF Pressed
				this->ff = true;
				ff_fr_timer = 0;
				sendButtonMessage(BUTTON_FF);
			} else if((button == 0x14 || button == 0x45) && state == 0x2) { //Preset 4, FF Released
				this->ff = false;
			} else if(button == 0x15 && state == 0x2) { //Preset 5, Disc -
				sendButtonMessage(BUTTON_DOWN);
			} else if(button == 0x16 && state == 0x2) { //Preset 6, Disc +
				sendButtonMessage(BUTTON_UP);
			} else if(button == 0x25 && state == 0x2) { //Track up.
				sendNextTrackMessage();
			} else if(button == 0x24 && state == 0x2) { //Track down.
				sendPrevTrackMessage();
			}
		}
	} else if(the_message->l == 3 && (the_message->data[0] == 0x30 || the_message->data[0] == 0x38)) {
		switch(the_message->data[0]) {
			case 0x30:
				switch(the_message->data[1]) {
					case 0x0:
						sendAICDStatusMessage(sender);
						break;
					case 0x1:
						sendButtonMessage(BUTTON_PAUSE_RESUME);
						break;
					case 0x3:
						fr = false;
						ff = false;
						break;
				}
				break;
			case 0x38:
				switch(the_message->data[1]) {
					case 0x4:
						if(the_message->data[2] == 0x00) {
							ff = true;
							fr = false;
						} else if(the_message->data[2] == 0x01) {
							fr = true;
							ff = false;
						} else {
							fr = false;
							ff = false;
						}
						break;
					case 0x6:
						if(the_message->data[2] == 0x10)
							sendButtonMessage(BUTTON_UP);
						else if(the_message->data[2] == 0x20)
							sendButtonMessage(BUTTON_DOWN);
						else
							sendButtonMessage(BUTTON_DISC_CHANGE, the_message->data[2]&0xF);
						break;
					case 0x7:
						if(the_message->data[2] == 0x10)
							sendButtonMessage(BUTTON_SCAN);
						break;
					case 0x8:
						if(the_message->data[2] == 0x10)
							sendButtonMessage(BUTTON_RANDOM);
						break;
					case 0x9:
						if(the_message->data[2] == 0x10)
							sendButtonMessage(BUTTON_REPEAT);
						break;
					case 0xA:
						if(the_message->data[2] == 0x0)
							sendButtonMessage(BUTTON_TRACK_NEXT);
						else
							sendButtonMessage(BUTTON_TRACK_PREV);
						break;
					case 0xB:
						const uint8_t new_track = the_message->data[2];
						if(new_track <= 0x99)
							sendButtonMessage(BUTTON_TRACK_CHANGE, getBCDFromByte(new_track)&0xFF);
						break;
				}
				break;
		}
	} else if(the_message->l >= 2 && the_message->data[0] == 0x2B && source_sel) {
		ack = false;
		sendAIAckMessage(sender);

		if((the_message->data[1] == 0x4A || the_message->data[1] == 0x45) && sender == ID_RADIO) {
			createCDMainMenu();
		} else if(the_message->data[1] == 0x40) {
			//TODO: Change the active menu depending on what cleared it.
			*active_menu = 0;
		} else if(the_message->data[1] == 0x60 && the_message->l >= 3 && sender == ID_NAV_COMPUTER) {
			if(*active_menu == MENU_CD) {
				switch(the_message->data[2]) {
				case 1: //Track list.
					//TODO: Figure this out.
					break;
				case 2: //Change disc.
					createCDChangeDiscMenu();
					break;
				case 3: //Autostart.
					this->autostart = !this->autostart;
					createCDMainMenuOption(2);
					setting_changed = true;
					break;
				case 4: //CD text.
					this->use_function_timer = !this->use_function_timer;
					setting_changed = true;

					if(parameter_list->imid_connected) {
						if(use_function_timer) {
							function_timer_enabled = true;
							function_timer = FUNCTION_CHANGE_TIMER - 10;
						} else {
							imid_handler->setIMIDSource(ID_CDC, 0);
							imid_handler->writeIMIDCDCTrackMessage(disc, track, 0xFF, timer, getIECDStatus(ai_cd_status), getIECDRepeat(ai_cd_status));
						}
					}

					createCDMainMenuOption(3);
					break;
				case 5: //Radio settings.
					//TODO: Request radio settings.
					break;
				}
			} else if(*active_menu == MENU_SELECT_DISC) {
				uint8_t clear_data[] = {0x2B, 0x4A};
				AIData clear_msg(sizeof(clear_data), device_ai_id, ID_NAV_COMPUTER);
				clear_msg.refreshAIData(clear_data);

				ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);

				disc_change_timer_enabled = true;
				disc_change_timer = 0;
				disc_change_disc = the_message->data[2];
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

void HondaCDHandler::sendSourceNameMessage() {
	sendSourceNameMessage(ID_RADIO);
}

void HondaCDHandler::sendSourceNameMessage(const uint8_t id) {
	uint8_t ai_handshake_data[] = {0x1, 0x1, 0x6};
	AIData ai_handshake_msg(sizeof(ai_handshake_data), ID_CDC, id);

	ai_handshake_msg.refreshAIData(ai_handshake_data);
	
	ai_driver->writeAIData(&ai_handshake_msg, parameter_list->radio_connected);

	uint8_t ai_name_data[] = {0x1, 0x23, 0x0, 'C', 'D', 'C'};
	AIData ai_name_msg(sizeof(ai_name_data), ID_CDC, id);

	ai_name_msg.refreshAIData(ai_name_data);
	
	ai_driver->writeAIData(&ai_name_msg, parameter_list->radio_connected);
}

void HondaCDHandler::sendNextTrackMessage() {
	this->sendButtonMessage(BUTTON_TRACK_NEXT);
}

void HondaCDHandler::sendPrevTrackMessage() {
	this->sendButtonMessage(BUTTON_TRACK_PREV);
}

void HondaCDHandler::sendPauseMessage() {
	this->sendButtonMessage(BUTTON_PAUSE_RESUME);
}

void HondaCDHandler::sendAICDStatusMessage(const uint8_t recipient) {
	uint8_t cd_status_data[] = {0x39, 0x0, ai_cd_status, 0x0, 0x3F, 0x0, this->disc, this->track};
	AIData cd_status_message(sizeof(cd_status_data), ID_CDC, ID_RADIO);
	cd_status_message.refreshAIData(cd_status_data);
	
	bool ack = true;
	if(recipient == ID_RADIO && !parameter_list->radio_connected)
		ack = false;
	else if(recipient == ID_NAV_COMPUTER && !parameter_list->computer_connected)
		ack = false;

	ai_driver->writeAIData(&cd_status_message, ack);
}

void HondaCDHandler::sendAICDTextMessage(const uint8_t recipient, const uint8_t field) {
	uint8_t ai_field = 1;
	char* affected;
	unsigned int len = 0;

	if(text_mode == TEXT_MODE_WITH_TEXT) {
		switch(field) {
			case TEXT_SONG:
				affected = song_title;
				len = sizeof(song_title)/sizeof(char);
				ai_field = 1;
				break;
			case TEXT_ARTIST:
				affected = artist;
				len = sizeof(artist)/sizeof(char);
				ai_field = 2;
				break;
			case TEXT_ALBUM:
				affected = album;
				len = sizeof(album)/sizeof(char);
				ai_field = 3;
				break;
			default:
				return;
		}
	} else if(text_mode == TEXT_MODE_MP3) {
		switch(field) {
			case 0:
				affected = folder;
				len = sizeof(folder)/sizeof(char);
				ai_field = 4;
				break;
			case 1:
				affected = filename;
				len = sizeof(filename)/sizeof(char);
				ai_field = 5;
				break;
			case 2:
				affected = song_title;
				len = sizeof(song_title)/sizeof(char);
				ai_field = 1;
				break;
			case 3:
				affected = album;
				len = sizeof(album)/sizeof(char);
				ai_field = 3;
				break;
			case 4:
				affected = artist;
				len = sizeof(artist)/sizeof(char);
				ai_field = 2;
				break;
		}
	}

	String meta_text = F("");
	for(int i=0;i<len;i+=1) {
		if(uint8_t(affected[i]) >= 0x20)
			meta_text += affected[i];
		else if(affected[i] == 0)
			break;
	}
	
	meta_text.replace("#","##  ");

	AIData text_msg(3+meta_text.length(), ID_CDC, recipient);
	text_msg.data[0] = 0x23;
	text_msg.data[1] = 0x60|ai_field;
	text_msg.data[2] = 0x1;
	for(int i=0;i<meta_text.length();i+=1)
		text_msg.data[i+3] = uint8_t(meta_text.charAt(i));

	bool ack = true;
	if(recipient == ID_RADIO && !parameter_list->radio_connected)
		ack = false;
	else if(recipient == ID_NAV_COMPUTER && !parameter_list->computer_connected)
		ack = false;

	ai_driver->writeAIData(&text_msg, ack);
}

void HondaCDHandler::sendButtonByte(const uint8_t byte_msg) {
	this->sendButtonMessage(byte_msg);
}

void HondaCDHandler::sendButtonMessage(const uint8_t button) {
	uint8_t button_data[] = {0x30, 0x0, 0x6, 0x2, 0x6, button};
	IE_Message button_message(sizeof(button_data), IE_ID_RADIO, IE_ID_CDC, 0xF, true);

	button_message.refreshIEData(button_data);
	ie_driver->sendMessage(&button_message, true, true);
	getIEAckMessage(device_ie_id);
}

void HondaCDHandler::sendButtonMessage(const uint8_t button, const uint8_t state) {
	uint8_t button_data[] = {0x30, 0x0, 0x6, 0x2, 0x6, button, state};
	IE_Message button_message(sizeof(button_data), IE_ID_RADIO, IE_ID_CDC, 0xF, true);

	button_message.refreshIEData(button_data);
	ie_driver->sendMessage(&button_message, true, true);
	getIEAckMessage(device_ie_id);
}

void HondaCDHandler::sendCDTrackMessage() {
	this->sendCDTrackMessage(this->track > 0 && this-> track <= 99);
}

//Send the disc and track to the nav screen.
void HondaCDHandler::sendCDTrackMessage(const bool track) {
	String track_text = "CD" + String(int(this->disc));
	if(track)
		track_text += "-" + String(int(this->track));

	AIData track_msg = getTextMessage(ID_CDC, track_text, 0, 0, true);
	ai_driver->writeAIData(&track_msg, parameter_list->computer_connected);

	if((ai_cd_status&AI_CD_REPEAT_T) != 0) {
		AIData rpt_message = getTextMessage(ID_CDC, "Repeat T", 0x1, 1, true);
		ai_driver->writeAIData(&rpt_message, parameter_list->computer_connected);
	} else if((ai_cd_status&AI_CD_REPEAT_D) != 0) {
		AIData rpt_message = getTextMessage(ID_CDC, "Repeat D", 0x1, 1, true);
		ai_driver->writeAIData(&rpt_message, parameter_list->computer_connected);
	} else if((ai_cd_status&AI_CD_RANDOM_D) != 0) {
		AIData rnd_message = getTextMessage(ID_CDC, "Random D", 0x1, 1, true);
		ai_driver->writeAIData(&rnd_message, parameter_list->computer_connected);
	} else if((ai_cd_status&AI_CD_RANDOM_A) != 0) {
		AIData rnd_message = getTextMessage(ID_CDC, "Random A", 0x1, 1, true);
		ai_driver->writeAIData(&rnd_message, parameter_list->computer_connected);
	} else {
		uint8_t clear_data[] = {0x20, 0x71, 1};
		AIData clear_msg(sizeof(clear_data), ID_CDC, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	}

	if(text_mode == TEXT_MODE_MP3) {
		AIData folder_msg = getTextMessage(ID_CDC, "Folder " + String(int(folder_num)), 0x1, 0, true);
		ai_driver->writeAIData(&folder_msg, parameter_list->computer_connected);
	} else {
		uint8_t clear_data[] = {0x20, 0x71, 0};
		AIData clear_msg(sizeof(clear_data), ID_CDC, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	}
}

//Send the time to the nav screen.
void HondaCDHandler::sendCDTimeMessage() {
	if(!text_control)
		return;

	String time_text = "-:--";
	if(this->timer >= 0) {
		time_text = String(int(this->timer/60)) + ":";
		if(this->timer%60 >= 10)
			time_text += String(int(this->timer%60));
		else
			time_text += "0" + String(int(this->timer%60));
	}
	AIData time_msg = getTextMessage(ID_CDC, time_text, 1, 2, true);
	ai_driver->writeAIData(&time_msg, parameter_list->computer_connected);
}

//Send the song title to the nav screen.
void HondaCDHandler::sendCDTextMessage(const uint8_t field, const bool refresh) {
	if(!text_control)
		return;

	uint8_t ai_field = 1;
	char* affected;
	unsigned int len = 0;

	if(text_mode == TEXT_MODE_WITH_TEXT) {
		switch(field) {
			case TEXT_SONG:
				affected = song_title;
				len = sizeof(song_title)/sizeof(char);
				ai_field = 1;
				break;
			case TEXT_ARTIST:
				affected = artist;
				len = sizeof(artist)/sizeof(char);
				ai_field = 2;
				break;
			case TEXT_ALBUM:
				affected = album;
				len = sizeof(album)/sizeof(char);
				ai_field = 3;
				break;
			default:
				return;
		}
	} else if(text_mode == TEXT_MODE_MP3) {
		switch(field) {
			case 0:
				affected = folder;
				len = sizeof(folder)/sizeof(char);
				ai_field = 4;
				break;
			case 1:
				affected = filename;
				len = sizeof(filename)/sizeof(char);
				ai_field = 5;
				break;
			case 2:
				affected = song_title;
				len = sizeof(song_title)/sizeof(char);
				ai_field = 1;
				break;
			case 3:
				affected = album;
				len = sizeof(album)/sizeof(char);
				ai_field = 3;
				break;
			case 4:
				affected = artist;
				len = sizeof(artist)/sizeof(char);
				ai_field = 2;
				break;
		}
	}

	String meta_text = String(affected);
	if(meta_text.length() > len)
		meta_text = meta_text.substring(0, len);

	String meta_pound = meta_text;
	meta_pound.replace("#", "##  ");

	AIData meta_msg = getTextMessage(ID_CDC, meta_pound, 0, ai_field, refresh);
	ai_driver->writeAIData(&meta_msg, parameter_list->computer_connected);

	sendCDIMIDTextMessage(field, meta_text);
}

void HondaCDHandler::sendCDLoadWaitMessage(const uint8_t message_type) {
	if(message_type == 0x16) { //Wait.
		clearCDText(true, true, true, true, true);
		clearAICDText(false, true, true, true, true);
		AIData wait_msg = getTextMessage(ID_CDC, "Wait", 0, 1, true);
		ai_driver->writeAIData(&wait_msg, parameter_list->computer_connected);
	} else if(message_type == 0x17) { //Load.
		clearCDText(true, true, true, true, true);
		clearAICDText(false, true, true, true, true);
		AIData wait_msg = getTextMessage(ID_CDC, "Load", 0, 1, true);
		ai_driver->writeAIData(&wait_msg, parameter_list->computer_connected);
	} else if(message_type == 0x18) { //Loading.
		clearCDText(true, true, true, true, true);
		clearAICDText(false, true, true, true, true);
		AIData wait_msg = getTextMessage(ID_CDC, "Loading...", 0, 1, true);
		ai_driver->writeAIData(&wait_msg, parameter_list->computer_connected);
	} else if(message_type == 0x10) { //Reading
		clearCDText(true, true, true, true, true);
		clearAICDText(false, true, true, true, true);
		AIData wait_msg = getTextMessage(ID_CDC, "Reading disc...", 0, 1, true);
		ai_driver->writeAIData(&wait_msg, parameter_list->computer_connected);
	} else if(message_type == 0x1A) { //Ejecting
		clearCDText(true, true, true, true, true);
		clearAICDText(false, true, true, true, true);
		AIData wait_msg = getTextMessage(ID_CDC, "Eject", 0, 1, true);
		ai_driver->writeAIData(&wait_msg, parameter_list->computer_connected);
	} else if(message_type == 0x1B) { //Disc is hanging
		clearCDText(true, true, true, true, true);
		clearAICDText(false, true, true, true, true);
		AIData wait_msg = getTextMessage(ID_CDC, "Ejected", 0, 1, true);
		ai_driver->writeAIData(&wait_msg, parameter_list->computer_connected);
	} else if(message_type == 0xF3) { //No disc.
		clearCDText(true, true, true, true, true);
		clearAICDText(false, true, true, true, true);
		AIData wait_msg = getTextMessage(ID_CDC, "No Disc", 0, 1, true);
		ai_driver->writeAIData(&wait_msg, parameter_list->computer_connected);
	}
}

void HondaCDHandler::startTextTimer(const uint8_t field) {
	text_timer = 0;
	text_timer_enabled = true;

	if(text_mode == TEXT_MODE_WITH_TEXT) {
		switch(field) {
			case TEXT_ALBUM:
				text_timer_album = true;
				break;
			case TEXT_ARTIST:
				text_timer_artist = true;
				break;
			case TEXT_SONG:
				text_timer_song = true;
				break;
		}
	} else if(text_mode == TEXT_MODE_MP3) {
		text_request_timer = 0;
		switch(field) {
			case 3:
				text_timer_album = true;
				break;
			case 4:
				text_timer_artist = true;
				break;
			case 2:
				text_timer_song = true;
				break;
			case 0:
				text_timer_folder = true;
				break;
			case 1:
				text_timer_file = true;
				break;
		}
	}
}

void HondaCDHandler::sendTextRequest() {
	if(!source_sel)
		return;

	uint8_t function_data[] = {0x6, 0x0};
	sendFunctionMessage(ie_driver, true, IE_ID_CDC, function_data, sizeof(function_data));
}

//Send text to the IMID.
void HondaCDHandler::sendCDIMIDTextMessage(const uint8_t field, String meta_text) {
	if(!text_control)
		return;

	if(parameter_list->imid_connected) {
		if(display_parameter == TEXT_NONE) {
			if(text_mode == TEXT_MODE_WITH_TEXT) {
				imid_handler->writeIMIDCDCTextMessage(field, meta_text);
			} else if(text_mode == TEXT_MODE_MP3) {
				switch(field) {
					case 2:
						imid_handler->writeIMIDCDCTextMessage(TEXT_SONG, meta_text);
						break;
					case 4:
						imid_handler->writeIMIDCDCTextMessage(TEXT_ARTIST, meta_text);
						break;
					case 3:
						imid_handler->writeIMIDCDCTextMessage(TEXT_ALBUM, meta_text);
						break;
				}
			}
		} else {
			display_timer_enabled = false;
			scroll_state = 0;
			sendIMIDInfoMessage();
		}
	} else if(parameter_list->external_imid_cd) {
		sendAICDTextMessage(ID_IMID_SCR, field);
	} else if(parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines >= 2) {
		if(display_parameter == TEXT_NONE) {
			if(meta_text.length() >= parameter_list->external_imid_char)
				meta_text = meta_text.substring(0, parameter_list->external_imid_char);
		
			meta_text.replace("#","##  ");
			
			AIData text_msg(4+meta_text.length(), ID_CDC, ID_IMID_SCR);
			text_msg.data[0] = 0x23;
			text_msg.data[1] = 0x60;
			text_msg.data[2] = parameter_list->external_imid_char/2-meta_text.length()/2;
			if(text_mode == TEXT_MODE_WITH_TEXT) {
				switch(field) {
					case TEXT_SONG:
						text_msg.data[3] = 2;
						break;
					case TEXT_ARTIST:
						if(parameter_list->external_imid_lines >= 3)
							text_msg.data[3] = 3;
						else
							return;
						break;
					case TEXT_ALBUM:
						if(parameter_list->external_imid_lines >= 4)
							text_msg.data[3] = 4;
						else
							return;
						break;
				}
			} else if(text_mode == TEXT_MODE_MP3) {
				switch(field) {
					case 2:
						text_msg.data[3] = 2;
						break;
					case 4:
						if(parameter_list->external_imid_lines >= 3)
							text_msg.data[3] = 3;
						else
							return;
						break;
					case 3:
						if(parameter_list->external_imid_lines >= 4)
							text_msg.data[3] = 4;
						else
							return;
						break;
				}
			}
			for(int i=0;i<meta_text.length();i+=1)
				text_msg.data[i+4] = uint8_t(meta_text.charAt(i));

			ai_driver->writeAIData(&text_msg);
		} else {
			display_timer_enabled = false;
			scroll_state = 0;
			sendIMIDInfoMessage();
		}
	}
}

//Increment the info screen.
void HondaCDHandler::incrementInfo() {
	if(text_timer_enabled) {
		text_timer_info_change = true;
		return;
	}

	mode_timer_enabled = false;

	if(text_mode == TEXT_MODE_WITH_TEXT) {
		switch(display_parameter) {
			case TEXT_NONE:
				display_parameter = TEXT_SONG;
				break;
			case TEXT_SONG:
				display_parameter = TEXT_ARTIST;
				break;
			case TEXT_ARTIST:
				display_parameter = TEXT_ALBUM;
				break;
			case TEXT_ALBUM:
			default:
				display_parameter = TEXT_NONE;
				break;
		}
	} else if(text_mode == TEXT_MODE_MP3) {
		switch(display_parameter) {
			case TEXT_NONE:
				display_parameter = TEXT_SONG;
				break;
			case TEXT_SONG:
				display_parameter = TEXT_ARTIST;
				break;
			case TEXT_ARTIST:
				display_parameter = TEXT_ALBUM;
				break;
			case TEXT_ALBUM:
				display_parameter = TEXT_FOLDER;
				break;
			case TEXT_FOLDER:
				display_parameter = TEXT_FILE;
				break;
			case TEXT_FILE:
			default:
				display_parameter = TEXT_NONE;
				break;
		}
	} else {
		display_parameter = TEXT_NONE;
	}
	
	if(display_parameter != TEXT_NONE) {
		display_timer = 0;
		if(parameter_list->external_imid_lines > 1)
			display_timer_enabled = false;
		else
			display_timer_enabled = true;
		scroll_state = 0;
		
		if(parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines > 0)
			clearExternalIMID();
		sendIMIDInfoMessage();

	} else {
		if(use_function_timer && parameter_list->imid_connected && text_mode != TEXT_MODE_BLANK) {
			imid_handler->setIMIDSource(ID_CD, 0);
		} else if(parameter_list->imid_connected) {
			imid_handler->setIMIDSource(ID_CDC, 0);
		}

		sendCDIMIDTrackandTimeMessage();
		text_timer = 0;
		text_timer_enabled = true;
		text_timer_song = true;
		text_timer_artist = true;
		text_timer_album = true;
	}
}

//Send the track and time message to the IMID.
void HondaCDHandler::sendCDIMIDTrackandTimeMessage() {
	if(!text_control)
		return;

	if(!mode_timer_enabled) {
		if(parameter_list->imid_connected) {
			if(display_parameter == TEXT_NONE) {
				imid_handler->writeIMIDCDCTrackMessage(disc, track, 0xFF, timer, getIECDStatus(ai_cd_status), getIECDRepeat(ai_cd_status));
			}
		} else if(parameter_list->external_imid_cd) {
			sendAICDStatusMessage(ID_IMID_SCR);
			uint8_t timer_data[] = {0x3B, 0x0, 0x0, (timer&0xFF00) >> 8, (timer&0xFF)};

			AIData timer_msg(sizeof(timer_data), ID_CDC, ID_IMID_SCR);
			timer_msg.refreshAIData(timer_data);
			
			ai_driver->writeAIData(&timer_msg);
		} else if(parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines > 0) {
			if(display_parameter == TEXT_NONE) {
				if(parameter_list->external_imid_lines >= 2 || (ai_cd_status&AI_CD_STATUS_PLAY) == AI_CD_STATUS_PLAY) {
					String function_text = "";
					if(parameter_list->external_imid_char > 10) {
						function_text = "CD" + String(int(disc));
						if(track > 0 && track <= 99)
							function_text += "-" + String(int(track));
						
						if(parameter_list->external_imid_char >= 16) {
							for(int i=0;i<(parameter_list->external_imid_char - 15)/2;i+=1)
								function_text += " ";
							
							if((ai_cd_status&AI_CD_REPEAT_T) == AI_CD_REPEAT_T)
								function_text += "RPT T";
							else if((ai_cd_status&AI_CD_REPEAT_D) == AI_CD_REPEAT_D)
								function_text += "RPT D";
							else if((ai_cd_status&AI_CD_RANDOM_D) == AI_CD_RANDOM_D)
								function_text += "RND D";
							else if((ai_cd_status&AI_CD_RANDOM_A) == AI_CD_RANDOM_A)
								function_text += "RND A";
							else if((ai_cd_status&AI_CD_SCAN_D) == AI_CD_SCAN_D)
								function_text += "SCN D";
							else if((ai_cd_status&AI_CD_SCAN_A) == AI_CD_SCAN_A)
								function_text += "SCN A";
							else
								function_text += "     ";
							
							for(int i=0;i<(parameter_list->external_imid_char - 15)/2;i+=1)
								function_text += " ";
						} else { 	
							for(int i=0;i<parameter_list->external_imid_char - 10;i+=1)
								function_text += " ";
						}
					} else {
						if(track > 0 && track <= 99)
							function_text += String(int(track));
						else
							function_text += "D" + String(int(disc));

						function_text += " ";
					}
					
					if(timer >= 0) {
						function_text += String(int(timer/60)) + ":";
						if(timer%60 >= 10)
							function_text += String(int(timer%60));
						else
							function_text += "0" + String(int(timer%60));
					} else
						function_text += "-:--";

					AIData function_msg(4+function_text.length(), ID_CDC, ID_IMID_SCR);
					function_msg.data[0] = 0x23;
					function_msg.data[1] = 0x60;
					function_msg.data[2] = parameter_list->external_imid_char/2-function_text.length()/2;
					function_msg.data[3] = 1;
					for(int i=0;i<function_text.length();i+=1)
						function_msg.data[i+4] = uint8_t(function_text.charAt(i));

					ai_driver->writeAIData(&function_msg);
				}
			}
		}
	} else if(mode_timer_enabled && ((!parameter_list->external_imid_cd && parameter_list->external_imid_char > 0 && parameter_list->external_imid_char < 16 && parameter_list->external_imid_lines == 1) || parameter_list->imid_connected)) {
		String function_text;
		if((ai_cd_status&AI_CD_REPEAT_T) == AI_CD_REPEAT_T)
			function_text = "REPEAT T";
		else if((ai_cd_status&AI_CD_REPEAT_D) == AI_CD_REPEAT_D)
			function_text = "REPEAT D";
		else if((ai_cd_status&AI_CD_RANDOM_D) == AI_CD_RANDOM_D)
			function_text = "RANDOM D";
		else if((ai_cd_status&AI_CD_RANDOM_A) == AI_CD_RANDOM_A)
			function_text = "RANDOM A";
		else if((ai_cd_status&AI_CD_SCAN_D) == AI_CD_SCAN_D)
			function_text = "SCAN D";
		else if((ai_cd_status&AI_CD_SCAN_A) == AI_CD_SCAN_A)
			function_text = "SCAN A";
		else
			function_text = "CD PLAY";

		if(parameter_list->imid_connected) {
			imid_handler->writeIMIDTextMessage(function_text);
		} else {
			AIData function_msg(4+function_text.length(), ID_CDC, ID_IMID_SCR);
			function_msg.data[0] = 0x23;
			function_msg.data[1] = 0x60;
			function_msg.data[2] = parameter_list->external_imid_char/2-function_text.length()/2;
			function_msg.data[3] = 1;
			for(int i=0;i<function_text.length();i+=1)
				function_msg.data[i+4] = uint8_t(function_text.charAt(i));

			ai_driver->writeAIData(&function_msg);
		}
	}
}

//Send the message for the "info" button to the IMID. Always resend the message.
void HondaCDHandler::sendIMIDInfoMessage() {
	sendIMIDInfoMessage(true);
}

//Send the message for the "info" button to the IMID. If resend is true, always resend the message.
void HondaCDHandler::sendIMIDInfoMessage(const bool resend) {
	if(!text_control || display_parameter == TEXT_NONE)
		return;
	
	String text_to_send = F(" ");
	
	if(display_timer_enabled) {
		if(display_parameter == TEXT_SONG)
			text_to_send = F("TRACK");
		else if(display_parameter == TEXT_ARTIST)
			text_to_send = F("ARTIST");
		else if(display_parameter == TEXT_ALBUM)
			text_to_send = F("ALBUM");
		else if(display_parameter == TEXT_FOLDER)
			text_to_send = F("FOLDER");
		else if(display_parameter == TEXT_FILE)
			text_to_send = F("FILE");
	} else {
		if(display_parameter == TEXT_SONG)
			text_to_send = String(song_title);
		else if(display_parameter == TEXT_ARTIST)
			text_to_send = String(artist);
		else if(display_parameter == TEXT_ALBUM)
			text_to_send = String(album);
		else if(display_parameter == TEXT_FOLDER)
			text_to_send = String(folder);
		else if(display_parameter == TEXT_FILE)
			text_to_send = String(filename);
	}

	if(text_to_send.length() <= 0 || text_to_send.compareTo(" ") == 0)
		text_to_send = F("No Data");

	if(text_to_send.length() > 32) {
		text_to_send = text_to_send.substring(0, 32);
	}

	/*if(resend && (parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines > 0))
		clearExternalIMID();*/

	bool scroll_changed = true;
	uint8_t max_length = parameter_list->external_imid_char;
	if(parameter_list->imid_connected)
		max_length = 11;

	if(text_to_send.length() > max_length) {
		if(text_to_send.length() - scroll_state < max_length) {
			scroll_changed = false;
			display_timer = 0;
			display_timer_enabled = true;
		} else {
			text_to_send = text_to_send.substring(scroll_state, text_to_send.length());
		}
	} else
		scroll_changed = false;
	
	if(resend && !parameter_list->imid_connected && parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines >= 2) {
		switch(display_parameter) {
			case TEXT_SONG:
				sendIMIDInfoHeader("TRACK");
				break;
			case TEXT_ARTIST:
				sendIMIDInfoHeader("ARTIST");
				break;
			case TEXT_ALBUM:
				sendIMIDInfoHeader("ALBUM");
				break;
			case TEXT_FOLDER:
				sendIMIDInfoHeader("FOLDER");
				break;
			case TEXT_FILE:
				sendIMIDInfoHeader("FILE");
				break;
		}
	}
	
	if((resend || scroll_changed)) {
		if(text_timer_enabled) {
			while (text_timer < TEXT_REFRESH_TIMER);
		}
		sendIMIDInfoMessage(text_to_send);
	}
}

void HondaCDHandler::sendIMIDInfoMessage(String text) {
	if(parameter_list->imid_connected) {
		imid_handler->writeIMIDTextMessage(text);
	} else if(parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines > 0) {
		uint8_t line = 1;
		if(parameter_list->external_imid_lines > 1)
			line = parameter_list->external_imid_lines/2+1;

		if(text.length() > parameter_list->external_imid_char)
			text = text.substring(0, parameter_list->external_imid_char);
			
		text.replace("#","##  ");

		AIData imid_msg(text.length() + 4, ID_CDC, ID_IMID_SCR);
		imid_msg.data[0] = 0x23;
		imid_msg.data[1] = 0x60;
		imid_msg.data[2] = parameter_list->external_imid_char/2-text.length()/2;
		imid_msg.data[3] = line;

		for(unsigned int i=0;i<text.length();i+=1)
			imid_msg.data[i+4] = uint8_t(text.charAt(i));
			
		ai_driver->writeAIData(&imid_msg);
	}
}

void HondaCDHandler::sendIMIDInfoHeader(String text) {
	if(parameter_list->external_imid_char <= 0 || parameter_list->external_imid_lines < 2)
		return;

	uint8_t line = parameter_list->external_imid_lines/2;

	if(text.length() > parameter_list->external_imid_char)
		text = text.substring(0, parameter_list->external_imid_char);
		
	text.replace("#","##  ");

	AIData imid_msg(text.length() + 4, ID_CDC, ID_IMID_SCR);
	imid_msg.data[0] = 0x23;
	imid_msg.data[1] = 0x60;
	imid_msg.data[2] = parameter_list->external_imid_char/2-text.length()/2;
	imid_msg.data[3] = line;

	for(unsigned int i=0;i<text.length();i+=1)
		imid_msg.data[i+4] = uint8_t(text.charAt(i));
		
	ai_driver->writeAIData(&imid_msg);
}

void HondaCDHandler::clearCDText(const bool song_title, const bool artist, const bool album, const bool folder, const bool file) {
	if(song_title) {
		for(int i=0;i<sizeof(this->song_title)/sizeof(char);i+=1)
			this->song_title[i] = 0;
	}

	if(album) {
		for(int i=0;i<sizeof(this->album)/sizeof(char);i+=1)
			this->album[i] = 0;
	}

	if(artist) {
		for(int i=0;i<sizeof(this->artist)/sizeof(char);i+=1)
			this->artist[i] = 0;
	}

	if(file) {
		for(int i=0;i<sizeof(this->filename)/sizeof(char);i+=1)
			this->filename[i] = 0;
	}
	
	if(folder) {
		for(int i=0;i<sizeof(this->folder)/sizeof(char);i+=1)
			this->folder[i] = 0;
	}
}

void HondaCDHandler::clearAICDText(const bool song_title, const bool artist, const bool album, const bool folder, const bool file) {
	if(song_title) {
		uint8_t clear_data[] = {0x20, 0x60, 0x1};
		if(!artist && !album)
			clear_data[1] |= 0x10;

		AIData clear_msg(sizeof(clear_data), ID_CDC, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	}

	if(artist) {
		uint8_t clear_data[] = {0x20, 0x60, 0x2};
		if(!album)
			clear_data[1] |= 0x10;

		AIData clear_msg(sizeof(clear_data), ID_CDC, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	}

	if(album) {
		uint8_t clear_data[] = {0x20, 0x60, 0x3};
		if(!folder)
			clear_data[1] |= 0x10;

		AIData clear_msg(sizeof(clear_data), ID_CDC, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	}

	if(folder) {
		uint8_t clear_data[] = {0x20, 0x60, 0x4};
		if(!file)
			clear_data[1] |= 0x10;

		AIData clear_msg(sizeof(clear_data), ID_CDC, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	}

	if(file) {
		uint8_t clear_data[] = {0x20, 0x60, 0x5};

		AIData clear_msg(sizeof(clear_data), ID_CDC, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	}
}

void HondaCDHandler::clearExternalCDIMID(const bool album) {
	if(parameter_list->external_imid_lines <= 1)
		return;
	
	unsigned int msg_l = parameter_list->external_imid_lines;
	if(album)
		msg_l += 1;
		
	if(msg_l < 2)
		msg_l = 2;
	
	uint8_t clear_data[msg_l];

	clear_data[0] = 0x20;
	clear_data[1] = 0x60;

	if(album) {
		for(unsigned int i=1;i<parameter_list->external_imid_lines;i+=1)
			clear_data[i+1] = i+1;
	} else {
		unsigned int first_limit = 3;
		if(parameter_list->external_imid_lines < first_limit)
			first_limit = parameter_list->external_imid_lines;
		
		for(unsigned int i=2;i<first_limit;i+=1)
			clear_data[i] = i;
		
		for(unsigned int i=first_limit + 1;i<parameter_list->external_imid_lines;i+=1)
			clear_data[i] = i+1;
	}

	AIData clear_msg(sizeof(clear_data), ID_CDC, ID_IMID_SCR);
	clear_msg.refreshAIData(clear_data);

	ai_driver->writeAIData(&clear_msg);
}

void HondaCDHandler::sendFunctionTextMessage() {
	AIData function1 = getTextMessage(ID_CDC, F("Repeat"), 0x2, 0, false);
	AIData function2 = getTextMessage(ID_CDC, F("Random"), 0x2, 1, false);
	AIData function3 = getTextMessage(ID_CDC, F("#REW"), 0x2, 2, false);
	AIData function4 = getTextMessage(ID_CDC, F("#FF "), 0x2, 3, false);
	AIData function5 = getTextMessage(ID_CDC, F("Disc -"), 0x2, 4, false);
	AIData function6 = getTextMessage(ID_CDC, F("Disc +"), 0x2, 5, true);

	ai_driver->writeAIData(&function1, parameter_list->computer_connected);
	ai_driver->writeAIData(&function2, parameter_list->computer_connected);
	ai_driver->writeAIData(&function3, parameter_list->computer_connected);
	ai_driver->writeAIData(&function4, parameter_list->computer_connected);
	ai_driver->writeAIData(&function5, parameter_list->computer_connected);
	ai_driver->writeAIData(&function6, parameter_list->computer_connected);
}

void HondaCDHandler::createCDMainMenu() {
	const uint8_t menu_size = 6;
	startAudioMenu(menu_size, menu_size, false, "CDC Settings");

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

	*active_menu = MENU_CD;

	for(uint8_t i=0;i<menu_size;i+=1)
		createCDMainMenuOption(i);
	displayAudioMenu(1);
}

void HondaCDHandler::createCDMainMenuOption(const uint8_t option) {
	switch(option) {
	case 0:
		appendAudioMenu(0, "Track List");
		break;
	case 1:
		appendAudioMenu(1, "Change Disc");
		break;
	case 2:
		if(!autostart)
			appendAudioMenu(2, "#ROF Auto Start");
		else
			appendAudioMenu(2, "#RON Auto Start");
		break;
	case 3:
		if(parameter_list->imid_connected) {
			if(!use_function_timer)
				appendAudioMenu(3, "#ROF CD Text on IMID");
			else
				appendAudioMenu(3, "#RON CD Text on IMID");
		}
		break;
	case 4:
		appendAudioMenu(4, "Audio Settings");
		break;
	}
}

void HondaCDHandler::createCDChangeDiscMenu() {
	uint8_t clear_data[] = {0x2B, 0x4A};
	AIData clear_msg(sizeof(clear_data), device_ai_id, ID_NAV_COMPUTER);
	clear_msg.refreshAIData(clear_data);

	ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);

	bool canceled = false;
	elapsedMillis cancel_wait;
	while(cancel_wait < 20) {
		AIData ai_msg;
		if(ai_driver->dataAvailable() > 0) {
			if(ai_driver->readAIData(&ai_msg)) {
				if(ai_msg.l >= 2 && ai_msg.sender == ID_NAV_COMPUTER && ai_msg.data[0] == 0x2B && ai_msg.data[1] == 0x40) { //Menu cleared.
					sendAIAckMessage(ai_msg.sender);
					canceled = true;
					break;
				}
			}
		}
	}

	if(!canceled)
		return;
	
	const uint8_t menu_size = 6;
	startAudioMenu(menu_size, menu_size, false, "Select Disc");

	for(uint8_t i=0;i<6;i+=1)
		createCDChangeDiscMenuOption(i);

	if(disc > 0 && disc <= 6)
		displayAudioMenu(disc);
	else
		displayAudioMenu(1);

	*active_menu = MENU_SELECT_DISC;
}

void HondaCDHandler::createCDChangeDiscMenuOption(const uint8_t option) {
	appendAudioMenu(option, "Disc " + String(option+1));
}
