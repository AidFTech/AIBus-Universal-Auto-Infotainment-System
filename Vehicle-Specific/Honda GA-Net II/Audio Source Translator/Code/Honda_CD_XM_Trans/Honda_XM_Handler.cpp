#include "Honda_XM_Handler.h"

HondaXMHandler::HondaXMHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list, HondaIMIDHandler* imid_handler) : HondaSourceHandler(ie_driver, ai_driver, parameter_list) {
	this->device_ie_id = IE_ID_SIRIUS;
	this->device_ai_id = ID_XM;
	this->imid_handler = imid_handler;
}

void HondaXMHandler::loop() {
	if(preset_received) {
		preset_request_increment += 1;
		preset_received = false;
	}

	if(source_sel) {
		if(text_timer_enabled && text_timer >= XM_TEXT_REFRESH_TIMER) {
			text_timer_enabled = false;
			if(parameter_list->imid_connected) {
				if(text_timer_channel) {
					imid_handler->writeIMIDSiriusTextMessage(N_CHANNEL_NAME, this->channel_name);
					text_timer_channel = false;
				} 
				if(text_timer_song) {
					imid_handler->writeIMIDSiriusTextMessage(N_SONG_NAME, this->song);
					text_timer_song = false;
				}
				if(text_timer_artist) { 
					imid_handler->writeIMIDSiriusTextMessage(N_ARTIST_NAME, this->artist);
					text_timer_artist = false;
				}
				if(text_timer_genre) {
					imid_handler->writeIMIDSiriusTextMessage(N_GENRE, this->genre);
					text_timer_genre = false;
				}
			} else {
				if(text_timer_channel) {
					setIMIDTextDisplay(N_CHANNEL_NAME, channel_name);
					text_timer_channel = false;
				}

				if(text_timer_song) {
					setIMIDTextDisplay(N_SONG_NAME, song);
					text_timer_song = false;
				}

				if(text_timer_artist) {
					setIMIDTextDisplay(N_ARTIST_NAME, artist);
					text_timer_artist = false;
				}

				if(text_timer_genre) {
					setIMIDTextDisplay(N_GENRE, genre);
					text_timer_genre = false;
				}
			}
		}

		if(display_timer_enabled && display_timer >= XM_DISPLAY_INFO_TIMER) {
			display_timer_enabled = false;
			scroll_timer = 0;
			scroll_state = 0;
			sendIMIDInfoMessage();
		}

		unsigned int scroll_limit = XM_SCROLL_TIMER;
		if(scroll_state <= 0)
			scroll_limit =  XM_SCROLL_TIMER_0;

		if(display_parameter != N_TEXT_NONE && !text_timer_enabled && !display_timer_enabled && scroll_timer >= scroll_limit) {
			scroll_timer = 0;
			scroll_state += 1;
			sendIMIDInfoMessage(false);
		}

		if(request_timer > XM_QUERY_TIMER
		&& (xm_state != XM_STATE_NOSIGNAL && xm_state != XM_STATE_ANTENNA)
		&& (song.length() <= 0 || artist.length() <= 0 || channel_name.length() <= 0 || genre.length() <= 0)) {
			request_timer = 0;
			sendTextRequest();
		}
	}

	if(source_established && preset_request_timer > preset_request_wait) {
		if(preset_request_increment < 1 || preset_request_increment > 12)
			preset_request_increment = 1;

		getPreset(preset_request_increment);

		preset_request_timer = 0;
		if(preset_request_increment <= 12)
			preset_request_wait = XM_QUERY_TIMER;
		else
			preset_request_wait =  PRESET_QUERY_TIMER;
	}
}

void HondaXMHandler::interpretSiriusMessage(IE_Message* the_message) {
	if(the_message->receiver != IE_ID_RADIO || the_message->sender != IE_ID_SIRIUS)
		return;

	if(!source_established && the_message->l > 4)
		source_established = true;
		
	bool ack = true;

	if(the_message->l == 2 && the_message->data[0] == 0x80) //Acknowledgement.
		return;

	if(the_message->l == 4) { //Handshake message.
		if(the_message->data[0] == 0x1 && the_message->data[1] == 0x1) {
			ack = false;
			sendIEAckMessage(IE_ID_SIRIUS);

			if(!this->parameter_list->first_xm) {
				this->parameter_list->first_xm = true;

				uint8_t handshake_data[] = {0x0, 0x30};
				IE_Message handshake_msg(sizeof(handshake_data), IE_ID_RADIO, 0x1FF, 0xF, false);
				handshake_msg.refreshIEData(handshake_data);
				
				delay(2);

				ie_driver->sendMessage(&handshake_msg, false, false);
			}
		} else if(the_message->data[0] == 0x5 && the_message->data[2] == 0x2) {
			ack = false;
			sendIEAckMessage(IE_ID_SIRIUS);
			
			delay(2);
			
			const bool handshake_rec = sendHandshakeAckMessage();
			if(!source_established) {
				source_established = handshake_rec;
				
				if(source_established && parameter_list->radio_connected) {
					sendSourceNameMessage();
				}
			}
			
		} 
	} else if(the_message->data[0] == 0x10 && the_message->data[1] == 0x19 && the_message->l >= 6) { //No signal?
		if(the_message->data[4] == 0xD)
			xm_state = XM_STATE_NOSIGNAL;
		else
			xm_state = 0; //Decide state with next 60 message.
	} else if(the_message->data[0] == 0x60 && the_message->data[1] == 0x19 && the_message->l > 5) { //Screen change message.
		ack = false;
		sendIEAckMessage(IE_ID_SIRIUS);
		
		const bool last_xm2 = xm2;
		const uint16_t last_channel = *channel;
		const uint8_t last_preset = current_preset;

		const uint8_t str = the_message->data[5], new_preset = the_message->data[6]; //TODO: Is this BCD?

		bool song_changed = false, artist_changed = false, channel_changed = false, genre_changed = false;

		if(str == 0) { //Tuned channel number.
			if(full_xm) {
				if(new_preset > 6 && xm2 && new_preset != 0xF)
					current_preset = new_preset-6;
				else if(new_preset <= 6 && !xm2)
					current_preset = new_preset;
				else
					current_preset = 0;
			} else {
				if(new_preset != 0xF)
					current_preset = new_preset;
				else
					current_preset = 0;
			}
			
			xm_state = the_message->data[4];
			
			*channel = getByteFromBCD(the_message->data[9]) + 100*getByteFromBCD(the_message->data[8]);
		} else if(str >= 0x50 && str < 0x60) { //Text change message.
			const uint8_t text_n = the_message->data[5]&0xF;
			String new_msg = "";

			if(text_n <= 2) { //Start at byte 7.
				for(int i=0;i<16;i+=1)
					new_msg += char(the_message->data[i+7]);
			} else if(text_n <= 3) {
				for(int i=0;i<16;i+=1)
					new_msg += char(the_message->data[i+8]);
			} else if(text_n <= 6) {
				for(int i=0;i<16;i+=1)
					new_msg += char(the_message->data[i+11]);
			}

			new_msg.replace("#", "##  ");

			if(text_n < 6) {
				switch(text_n) {
					case N_SONG_NAME:
						song = new_msg;
						song_changed = true;
						break;
					case N_ARTIST_NAME:
						artist = new_msg;
						artist_changed = true;
						break;
					case N_CHANNEL_NAME:
						channel_name = new_msg;
						channel_changed = true;
						break;
					case N_GENRE:
						genre = new_msg;
						genre_changed = true;
						break;
				}
			} else if(text_n == 6) { //Preset.
				uint8_t defined_number = the_message->data[7];
				bool defined_xm2 = false;
				if(full_xm && defined_number > 6) {
					defined_number -= 6;
					defined_xm2 = true;
				}

				if(defined_number <= 6) {
					uint16_t* preset_set = xm1_preset;
					String* text_set = xm1_preset_name;

					if(defined_xm2) {
						preset_set = xm2_preset;
						text_set = xm2_preset_name;
					}

					preset_set[defined_number-1] = getByteFromBCD(the_message->data[8])*100 + getByteFromBCD(the_message->data[9]);
					text_set[defined_number-1] = new_msg;
				}

				preset_received = true;
			}
		}

		if(last_channel != *channel || last_preset != current_preset) {
			sendAINumberMessage(ID_RADIO);
			
			if(last_channel != *channel)
				clearXMText(true, true, true, true);
			
			if(text_control) {
				if(last_channel != *channel)
					clearAIXMText(true, true, true, true);
				
				setNumberDisplay();
			}

			clearXMIMIDText();

			request_timer = 0;
		}

		if(song_changed) {
			request_timer = 0;
			sendAITextMessage(ID_RADIO, 1);
			if(text_control)
				setTextDisplay(N_SONG_NAME);
		}
		
		if(artist_changed) {
			request_timer = 0;
			sendAITextMessage(ID_RADIO, 2);
			if(text_control)
				setTextDisplay(N_ARTIST_NAME);
		}
		
		if(channel_changed) {
			request_timer = 0;
			sendAITextMessage(ID_RADIO, 3);
			if(text_control)
				setTextDisplay(N_CHANNEL_NAME);
		}
		
		if(genre_changed) {
			request_timer = 0;
			sendAITextMessage(ID_RADIO, 4);
			if(text_control)
				setTextDisplay(N_GENRE);
		}
	}

	if(ack)
		sendIEAckMessage(IE_ID_SIRIUS);
}

void HondaXMHandler::readAIBusMessage(AIData* the_message) {
	if(the_message->receiver != ID_XM)
		return;
	
	bool ack = true;
	const uint8_t sender = the_message->sender;
	
	if(the_message->l >= 3 && the_message->data[0] == 0x4 && the_message->data[1] == 0xE6 && the_message->data[2] == 0x10) { //Name request.
		ack = false;
		sendAIAckMessage(sender);
		sendSourceNameMessage(the_message->sender);
	} else if(sender == ID_NAV_SCREEN) {
		if(!this->source_sel) {
			ack = false;
			sendAIAckMessage(sender);
			return;
		}

		if(the_message->l >= 3 && the_message->data[0] == 0x30 && source_sel) { //Button pressed.
			const uint8_t button = the_message->data[1], state = (the_message->data[2]&0xC0)>>6;

			if(button != 0x2A && button != 0x2B && manual_tune) {
				manual_tune = false;
				if(*active_menu == MENU_XM)
					createXMMainMenuOption(1);
			}

			if(button == 0x53 && state == 0x2) { //Info button.
				if(text_timer_enabled)
					return;

				switch(display_parameter) {
					case N_TEXT_NONE:
						display_parameter = N_SONG_NAME;
						break;
					case N_SONG_NAME:
						display_parameter = N_ARTIST_NAME;
						break;
					case N_ARTIST_NAME:
						display_parameter = N_CHANNEL_NAME;
						break;
					case N_CHANNEL_NAME:
						display_parameter = N_GENRE;
						break;
					case N_GENRE:
					default:
						display_parameter = N_TEXT_NONE;
						break;
				}
				
				if(display_parameter != N_TEXT_NONE) {
					display_timer = 0;
					display_timer_enabled = true;
					scroll_state = 0;
					sendIMIDInfoMessage();
				} else {
					if(parameter_list->imid_connected) {
						if(xm2)
							imid_handler->setIMIDSource(ID_XM, 1);
						else
							imid_handler->setIMIDSource(ID_XM, 0);
					}
					sendIMIDNumberMessage();
					text_timer = 0;
					text_timer_enabled = true;
					text_timer_song = true;
					text_timer_artist = true;
					text_timer_channel = true;
					text_timer_genre = true;
				}
			} else if(button >= 0x11 && button <= 0x16) { //Preset.
				if(state == 2)
					changeToPreset(button&0xF);
				else if(state == 1)
					savePreset(button&0xF);
			} else if(button == 0x25 && state == 0x2) { //Increment channel.
				uint8_t channel_data[] = {0x30, 0x0, 0x19, 0x2, 0x19, 0xF2, 0x1};
				IE_Message channel_msg(sizeof(channel_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
				channel_msg.refreshIEData(channel_data);
				ie_driver->sendMessage(&channel_msg, true, true);
				getIEAckMessage(device_ie_id);
			} else if(button == 0x24 && state == 0x2) { //Decrement channel.
				uint8_t channel_data[] = {0x30, 0x0, 0x19, 0x2, 0x19, 0xF2, 0xFF};
				IE_Message channel_msg(sizeof(channel_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
				channel_msg.refreshIEData(channel_data);
				ie_driver->sendMessage(&channel_msg, true, true);
				getIEAckMessage(device_ie_id);
			} else if(button == 0x2A && state == 0x2 && manual_tune) {
				uint8_t channel_data[] = {0x30, 0x0, 0x19, 0x2, 0x19, 0xF2, 0xFF};
				IE_Message channel_msg(sizeof(channel_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
				channel_msg.refreshIEData(channel_data);
				ie_driver->sendMessage(&channel_msg, true, true);
				getIEAckMessage(device_ie_id);
			} else if(button == 0x2B && state == 0x2 && manual_tune) {
				uint8_t channel_data[] = {0x30, 0x0, 0x19, 0x2, 0x19, 0xF2, 0x1};
				IE_Message channel_msg(sizeof(channel_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
				channel_msg.refreshIEData(channel_data);
				ie_driver->sendMessage(&channel_msg, true, true);
				getIEAckMessage(device_ie_id);
			}
		} else if(the_message->l >= 3 && the_message->data[0] == 0x32 && the_message->data[1] == 0x7 && manual_tune && source_sel) {
			int8_t sign = 1;
			if((the_message->data[2]&0x10) == 0)
				sign = -1;

			const int new_channel = *channel + sign*(the_message->data[2]&0xF);
			if(new_channel >= 0 && new_channel <= 255) {
				setChannel(new_channel);
			} else if(new_channel < 0) {
				setChannel(255);
			} else if(new_channel > 255) {
				setChannel(0);
			}

		}
	} else if(the_message->data[0] == 0x30 && the_message->data[1] == 0x0 && source_sel) {
		ack = false;
		sendAIAckMessage(sender);
		sendAINumberMessage(sender);

		for(uint8_t i=1;i<=4;i+=1)
			sendAITextMessage(sender, i);
	} else if(the_message->data[0] == 0x38 && the_message->data[1] == 0x04 && the_message->l >= 3 && source_sel) { //Channel increment/decrement.
		ack = false;
		sendAIAckMessage(sender);

		uint8_t channel_data[] = {0x30, 0x0, 0x19, 0x2, 0x19, 0xF2, 0xFF};
		if(the_message->data[2] == 0)
			channel_data[6] = 0x1;
		else if(the_message->data[2] != 0x1)
			return;
		
		IE_Message channel_msg(sizeof(channel_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
		channel_msg.refreshIEData(channel_data);
		ie_driver->sendMessage(&channel_msg, true, true);
		getIEAckMessage(device_ie_id);
	} else if(the_message->data[0] == 0x38 && the_message->data[1] == 0xA && the_message->l >= 3 && source_sel) { //Category increment/decrement.
		ack = false;
		sendAIAckMessage(sender);

		uint8_t channel_data[] = {0x30, 0x0, 0x19, 0x2, 0x19, 0xF5, 0x0};
		if(the_message->data[2] == 0)
			channel_data[5] = 0xF4;
		else if(the_message->data[2] != 0x1)
			return;
		
		IE_Message channel_msg(sizeof(channel_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
		channel_msg.refreshIEData(channel_data);
		ie_driver->sendMessage(&channel_msg, true, true);
		getIEAckMessage(device_ie_id);
	} else if(the_message->data[0] == 0x38 && the_message->data[1] == 0x5 && the_message->l >= 4 && source_sel) { //Goto channel.
		if(the_message->data[2] > 0)
			return;

		uint8_t channel_data[] = {0x30, 0x0, 0x19, 0x2, 0x19, 0xF0, the_message->data[3]};
		IE_Message channel_msg(sizeof(channel_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
		channel_msg.refreshIEData(channel_data);
		ie_driver->sendMessage(&channel_msg, true, true);
		getIEAckMessage(device_ie_id);
	} else if(the_message->data[0] == 0x38 && the_message->data[1] == 0x6 && the_message->l >= 3 && source_sel) { //Preset change.
		ack = false;
		sendAIAckMessage(sender);
		changeToPreset(the_message->data[2]);
	} else if(the_message->data[0] == 0x38 && the_message->data[1] == 0x7 && the_message->l >= 3 && source_sel) { //Preset increment.
		ack = false;
		sendAIAckMessage(sender);
		
		uint8_t new_preset = current_preset;
		if(current_preset == 0) {
			if(the_message->data[2] == 0)
				new_preset = 1;
			else if (the_message->data[2] == 1)
				new_preset = 6;
		} else if(the_message->data[2] == 0) {
			new_preset += 1;
			if(new_preset > 6)
				new_preset = 1;
		} else if(the_message->data[2] == 1) {
			new_preset -= 1;
			if(new_preset <= 0)
				new_preset = 6;
		} else return;

		changeToPreset(new_preset);
	} else if(the_message->data[0] == 0x38 && the_message->data[1] == 0x16 && the_message->l >= 3 && source_sel) { //Preset save.
		ack = false;
		sendAIAckMessage(sender);
		savePreset(the_message->data[2]);
	} else if(the_message->data[0] == 0x40 && the_message->data[1] == 0x10 && sender == ID_RADIO) { //Function change.
		ack = false;
		sendAIAckMessage(sender);
		
		const uint8_t active_source = the_message->data[2];
		if(active_source == ID_XM) {
			uint8_t function[] = {0x19, 0x0, 0x41};

			if(the_message->l >= 4) {
				function[2] = the_message->data[3]+1;

				if(the_message->data[3] == 0)
					xm2 = false;
				else if(the_message->data[3] == 1)
					xm2 = true;
			}

			source_sel = true;
			sendFunctionMessage(ie_driver, true, IE_ID_SIRIUS, function, sizeof(function));
			sendFunctionMessage(ie_driver, false, IE_ID_SIRIUS, function, sizeof(function));
			getIEAckMessage(device_ie_id);

			bool full_xm_ack = false;

			xm_timer = 0;
			xm_timer_enabled = true;
			while(xm_timer < XM_FULL_TIMER && !full_xm_ack) {
				if(ie_driver->getInputOn()) {
					IE_Message new_msg;
					if(ie_driver->readMessage(&new_msg, true, IE_ID_RADIO) == 0) {
						if(new_msg.sender == IE_ID_SIRIUS) {
							sendIEAckMessage(new_msg.sender);
							if(new_msg.l >= 6 && new_msg.data[0] == 0x10 && new_msg.data[1] == 0x19) {
								full_xm = false;
								full_xm_ack = true;
								break;
							}
						}
					}
				}
			}

			if(!full_xm_ack) {
				function[2] = 0x41;
				sendFunctionMessage(ie_driver, true, IE_ID_SIRIUS, function, sizeof(function));
				sendFunctionMessage(ie_driver, false, IE_ID_SIRIUS, function, sizeof(function));
				getIEAckMessage(device_ie_id);
				full_xm = true;
			}
			//TODO: Change the channel if XM2 is selected.

			sendAINumberMessage(ID_RADIO);
			for(uint8_t i=0;i<=3;i+=1)
				sendAITextMessage(ID_RADIO, i);

			*parameter_list->screen_request_timer = 0;
			
			if(this->text_control) {
				if(parameter_list->imid_connected) {
					if(xm2)
						imid_handler->setIMIDSource(ID_XM, 1);
					else
						imid_handler->setIMIDSource(ID_XM, 0);
				}

				setNumberDisplay();
				for(uint8_t i=1;i<=4;i+=1)
					setTextDisplay(i);
			}
		} else {
			if(source_sel) {
				source_sel = false;
				uint8_t function[] = {0x0, 0x1};
				sendFunctionMessage(ie_driver, true, IE_ID_SIRIUS, function, sizeof(function));
				getIEAckMessage(device_ie_id);
				*parameter_list->screen_request_timer = 0;
			}

			this->text_control = false;
			this->manual_tune = false;
		}
	} else if(the_message->data[0] == 0x40 && the_message->data[1] == 0x1 && sender == ID_RADIO) {
		ack = false;
		sendAIAckMessage(sender);

		this->text_control = the_message->data[2] == device_ai_id;
		if(this->text_control) {
			if(parameter_list->imid_connected) {
				if(xm2)
					imid_handler->setIMIDSource(ID_XM, 1);
				else
					imid_handler->setIMIDSource(ID_XM, 0);
			}

			setNumberDisplay();
			for(uint8_t i=0;i<=3;i+=1)
				setTextDisplay(i);
		}
	} else if(the_message->l >= 2 && the_message->data[0] == 0x2B && source_sel) {
		ack = false;
		sendAIAckMessage(sender);

		if((the_message->data[1] == 0x4A || the_message->data[1] == 0x45) && sender == ID_RADIO) {
			createXMMainMenu();
		} else if(the_message->data[1] == 0x40) {
			manual_tune = false;
			direct_num = 0;
			//TODO: Change the active menu depending on what cleared it.
			*active_menu = 0;
			
			setNumberDisplay();
			setTextDisplay(N_CHANNEL_NAME);
			setTextDisplay(N_GENRE);
		} else if(the_message->data[1] == 0x60 && the_message->l >= 3 && sender == ID_NAV_COMPUTER) {
			if(*active_menu == MENU_XM) {
				switch(the_message->data[2]) {
				case 1: //Preset list.
					createXMPresetMenu();
					break;
				case 2: //Direct tune.
					manual_tune = !manual_tune;
					createXMMainMenuOption(1);
					break;
				case 3: //Channel list.
					//TODO: This.
					break;
				case 4: //Manual entry.
					createXMDirectMenu();
					break;
				case 5: //Audio settings.
					//TODO: Audio menu from radio.
					break;
				}
			} else if(*active_menu == MENU_PRESET_LIST) {
				uint8_t clear_data[] = {0x2B, 0x4A};
				AIData clear_msg(sizeof(clear_data), device_ai_id, ID_NAV_COMPUTER);
				clear_msg.refreshAIData(clear_data);

				ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);

				changeToPreset(the_message->data[2]);
			} else if(*active_menu == MENU_DIRECT_TUNE) {
				appendDirectNumber(the_message->data[2]);
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

void HondaXMHandler::sendSourceNameMessage() {
	sendSourceNameMessage(ID_RADIO);
}

void HondaXMHandler::sendSourceNameMessage(const uint8_t id) {
	uint8_t ai_handshake_data[] = {0x1, 0x1, 0x19, 0x2};
	AIData ai_handshake_msg(sizeof(ai_handshake_data), ID_XM, id);

	ai_handshake_msg.refreshAIData(ai_handshake_data);
	ai_driver->writeAIData(&ai_handshake_msg, parameter_list->radio_connected);
	
	//uint8_t ai_subs_data[] = {0x1, 0x2};
	//AIData ai_subs_msg(sizeof(ai_subs_data), ID_XM, id);
	//ai_subs_msg.refreshAIData(ai_subs_data);
	//ai_driver->writeAIData(&ai_subs_msg, parameter_list->radio_connected);
	
	uint8_t ai_name_data[] = {0x1, 0x23, 0x0, 'X', 'M', '1'};
	AIData ai_name_msg(sizeof(ai_name_data), ID_XM, id);

	ai_name_msg.refreshAIData(ai_name_data);
	ai_driver->writeAIData(&ai_name_msg, parameter_list->radio_connected);
	
	ai_name_data[2] = 0x1;
	ai_name_data[5] = uint8_t('2');
	ai_name_msg.refreshAIData(ai_name_data);
	ai_driver->writeAIData(&ai_name_msg, parameter_list->radio_connected);
}

bool HondaXMHandler::getXM2() {
	return this->xm2;
}

void HondaXMHandler::setNumberDisplay() {
	if(!text_control)
		return;
	
	if(*active_menu != MENU_DIRECT_TUNE) {
		String preset_text = "XM";
		if(xm2)
			preset_text += "2";
		else
			preset_text += "1";

		if(current_preset > 0)
			preset_text += "-" + String(int(current_preset));

		AIData preset_msg = getTextMessage(ID_XM, preset_text, 0x0, 0x0, false);
		ai_driver->writeAIData(&preset_msg, parameter_list->computer_connected);

		AIData channel_msg = getTextMessage(ID_XM, "CH " + String(int(*channel)), 0x1, 0x2, true);
		ai_driver->writeAIData(&channel_msg,parameter_list->computer_connected);
	}

	if(display_parameter == N_TEXT_NONE)
		sendIMIDNumberMessage();
}

void HondaXMHandler::setTextDisplay(const uint8_t field) {
	if(!text_control)
		return;

	uint8_t nav_group = 0, nav_area = 1;

	String selected;
	switch(field) {
		case N_SONG_NAME:
			selected = String(song);
			nav_group = 0;
			nav_area = 1;
			break;
		case N_ARTIST_NAME:
			selected = String(artist);
			nav_group = 0;
			nav_area = 2;
			break;
		case N_CHANNEL_NAME:
			selected = String(channel_name);
			nav_group = 1;
			nav_area = 1;
			break;
		case N_GENRE:
			selected = String(genre);
			nav_group = 1;
			nav_area = 0;
			break;
		default:
			return;
	}

	unsigned int pop_limit = selected.length();
	for(unsigned int i=pop_limit - 1;i>=0;i-=1) {
		if(selected.charAt(i) != ' ' && selected.charAt(i) != 0) {
			pop_limit = i;
			break;
		}

		if(i <= 0)
			break;
	}

	selected = selected.substring(0, pop_limit + 1);

	if(*active_menu != MENU_DIRECT_TUNE || nav_group != 1) {
		AIData text_msg = getTextMessage(ID_XM, selected, nav_group, nav_area, true);
		ai_driver->writeAIData(&text_msg, parameter_list->computer_connected);
	}

	setIMIDTextDisplay(field, selected);
}

void HondaXMHandler::sendTextRequest() {
	if(!source_sel)
		return;

	uint8_t function_data[] = {0x19, 0x0, 0x1};
	if(xm2)
		function_data[2] = 0x2;
	else if(full_xm)
		function_data[2] = 0x41;

	sendFunctionMessage(ie_driver, true, IE_ID_SIRIUS, function_data, sizeof(function_data));
	getIEAckMessage(device_ie_id);
}

void HondaXMHandler::setIMIDTextDisplay(const uint8_t field, String selected) {
	if(!text_control)
		return;

	if(parameter_list->imid_connected) {
		if(display_parameter == N_TEXT_NONE) {
			text_timer = 0;
			text_timer_enabled = true;
			switch(field) {
				case N_CHANNEL_NAME:
					text_timer_channel = true;
					break;
				case N_SONG_NAME:
					text_timer_song = true;
					break;
				case N_ARTIST_NAME:
					text_timer_artist = true;
					break;
				case N_GENRE:
					text_timer_genre = true;
					break;
			}
		} else {
			display_timer_enabled = false;
			scroll_state = 0;
			sendIMIDInfoMessage();
		}
	} else if(parameter_list->external_imid_xm) {
		uint8_t ai_field = 0;
		switch(field) {
			case N_SONG_NAME:
				ai_field = 1;
				break;
			case N_ARTIST_NAME:
				ai_field = 2;
				break;
			case N_CHANNEL_NAME:
				ai_field = 3;
				break;
			case N_GENRE:
				ai_field = 4;
				break;
			default:
				return;
		}

		sendAITextMessage(ID_IMID_SCR, ai_field);
	} else if(parameter_list->external_imid_char > 0 && parameter_list-> external_imid_lines > 0) {
		if(display_parameter == N_TEXT_NONE) {
			uint8_t line = 0;
			switch(field) {
				case N_SONG_NAME:
					line = 3;
					break;
				case N_ARTIST_NAME:
					line = 4;
					break;
				case N_CHANNEL_NAME:
					line = 2;
					break;
				case N_GENRE:
					line = 5;
					break;
				default:
					return;
			}

			if(line > parameter_list->external_imid_lines)
				return;
			
			if(selected.length() >= parameter_list->external_imid_char)
				selected = selected.substring(0, parameter_list->external_imid_char);

			AIData text_msg(4+selected.length(), ID_XM, ID_IMID_SCR);
			text_msg.data[0] = 0x23;
			text_msg.data[1] = 0x60;
			text_msg.data[2] = parameter_list->external_imid_char/2-selected.length()/2;
			text_msg.data[3] = line;

			for(int i=0;i<selected.length();i+=1)
				text_msg.data[i+4] = uint8_t(selected.charAt(i));

			ai_driver->writeAIData(&text_msg);
		} else {
			display_timer_enabled = false;
			scroll_state = 0;
			sendIMIDInfoMessage();
		}
	}
}

//Erase everything but the first line of an external IMID.
void HondaXMHandler::clearXMIMIDText() {
	if(!text_control)
		return;

	if(parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines > 1) {
		uint8_t clear_data[parameter_list->external_imid_lines + 1];

		clear_data[0] = 0x20;
		clear_data[1] = 0x60;
		for(int i=1;i<parameter_list->external_imid_lines;i+=1)
			clear_data[i+1] = i+1;

		AIData clear_msg(sizeof(clear_data), ID_XM, ID_IMID_SCR);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg);
	}
}

//Set a channel.
void HondaXMHandler::setChannel(uint16_t channel) {
	uint8_t channel_data[] = {0x30, 0x0, 0x19, 0x2, 0x19, 0xF0, channel&0xFF};
	IE_Message channel_msg(sizeof(channel_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
	channel_msg.refreshIEData(channel_data);
	ie_driver->sendMessage(&channel_msg, true, true);	
	getIEAckMessage(device_ie_id);
}

//Set the radio to a preset.
void HondaXMHandler::changeToPreset(uint8_t preset) {
	if(full_xm && xm2 && preset <= 6)
		preset = preset + 6;

	uint8_t preset_request_data[] = {0x30, 0x0, 0x19, 0x2, 0x19, 0xF1};
	IE_Message preset_request(sizeof(preset_request_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
	preset_request.refreshIEData(preset_request_data);

	bool ie_ack = false;

	while(!ie_ack) {
		ie_driver->sendMessage(&preset_request, true, true);
		ie_ack = this->getIEAckMessage(device_ie_id);
	}

	uint8_t preset_set_data[] = {0x30, 0x0, 0x19, 0x2, 0x19, 0xE7, 0x1};
	preset_set_data[6] = preset;
	IE_Message preset_set(sizeof(preset_set_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
	preset_set.refreshIEData(preset_set_data);
	ie_driver->sendMessage(&preset_set, true, true);
	getIEAckMessage(device_ie_id);
}

//Save a preset.
void HondaXMHandler::savePreset(uint8_t preset) {
	if(full_xm && xm2 && preset <= 6)
		preset = preset + 6;

	uint8_t preset_request_data[] = {0x30, 0x0, 0x19, 0x2, 0x19, 0xF1};
	IE_Message preset_request(sizeof(preset_request_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
	preset_request.refreshIEData(preset_request_data);

	bool ie_ack = false;

	while(!ie_ack) {
		ie_driver->sendMessage(&preset_request, true, true);
		ie_ack = this->getIEAckMessage(device_ie_id);
	}

	uint8_t preset_set_data[] = {0x30, 0x0, 0x19, 0x2, 0x19, 0xE8, 0x1};
	preset_set_data[6] = preset;
	IE_Message preset_set(sizeof(preset_set_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
	preset_set.refreshIEData(preset_set_data);
	ie_driver->sendMessage(&preset_set, true, true);

	ie_ack = false;
	while(!ie_ack) {
		ie_driver->sendMessage(&preset_set, true, true);
		ie_ack = this->getIEAckMessage(device_ie_id);
	}

	getPreset(preset);
}

void HondaXMHandler::sendAINumberMessage(const uint8_t receiver) {
	uint8_t number_data[] = {0x39, 0x00, (*channel&0xFF00)>>8, *channel&0xFF, current_preset};
	AIData number_msg(sizeof(number_data), ID_XM, receiver);
	number_msg.refreshAIData(number_data);
	
	bool ack = true;
	if(receiver == ID_RADIO && !parameter_list->radio_connected)
		ack = false;
	else if(receiver == ID_NAV_COMPUTER && !parameter_list->computer_connected)
		ack = false;
	
	
	ai_driver->writeAIData(&number_msg, ack);
}

void HondaXMHandler::sendAITextMessage(const uint8_t receiver, const uint8_t field) {
	String* selected;
	if(field < 1 || field > 4)
		return;

	switch(field) {
		case 1:
			selected = &song;
			break;
		case 2:
			selected = &artist;
			break;
		case 3:
			selected = &channel_name;
			break;
		case 4:
			selected = &genre;
			break;
		default:
			return;
	}

	unsigned int pop_limit = selected->length();
	for(unsigned int i=pop_limit - 1;i>=0;i-=1) {
		if(selected->charAt(i) != ' ' && selected->charAt(i) != 0) {
			pop_limit = i;
			break;
		}

		if(i <= 0)
			break;
	}

	*selected = selected->substring(0, pop_limit + 1);

	uint8_t text_data[3+selected->length()];
	text_data[0] = 0x23;
	text_data[1] = 0x60|field;
	text_data[2] = 0x1;
	for(int i=0;i<selected->length();i+=1)
		text_data[i+3] = uint8_t(selected->charAt(i));

	AIData text_msg(sizeof(text_data), ID_XM, receiver);
	text_msg.refreshAIData(text_data);
	
	bool ack = true;
	if(receiver == ID_RADIO && !parameter_list->radio_connected)
		ack = false;
	else if(receiver == ID_NAV_COMPUTER && !parameter_list->computer_connected)
		ack = false;
	
	ai_driver->writeAIData(&text_msg, ack);
}

void HondaXMHandler::sendIMIDNumberMessage() {
	if(parameter_list->imid_connected) {
		if(display_parameter == N_TEXT_NONE)
			imid_handler->writeIMIDSiriusNumberMessage(current_preset, *channel, xm2);
	} else if(parameter_list->external_imid_xm) {
		sendAINumberMessage(ID_IMID_SCR);
	} else if(parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines > 0) {
		if(parameter_list->external_imid_char > 8) {
			String xm_text = F("XM");
			if(xm2)
				xm_text += "2";
			else
				xm_text += "1";
			
			if(current_preset > 0) {
				xm_text += "-";
				xm_text += int(current_preset);
			}

			xm_text += " CH";
			xm_text += int(*channel);

			uint8_t xm_data[4 + xm_text.length()];
			xm_data[0] = 0x23;
			xm_data[1] = 0x60;
			xm_data[2] = parameter_list->external_imid_char/2 - xm_text.length()/2;
			xm_data[3] = 0x1;
			for(int i=0;i<xm_text.length();i+=1)
				xm_data[i+4] = uint8_t(xm_text.charAt(i));

			AIData xm_msg(sizeof(xm_data), ID_XM, ID_IMID_SCR);
			xm_msg.refreshAIData(xm_data);
			ai_driver->writeAIData(&xm_msg);
		}
	}
}

//Request a preset from the tuner.
void HondaXMHandler::getPreset(const uint8_t preset) {
	uint8_t preset_request_data[] = {0x50, 0x1, 0x19, 0x0, 0x1, 0xA, 0x0, preset};
	IE_Message preset_request(sizeof(preset_request_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
	preset_request.refreshIEData(preset_request_data);

	ie_driver->sendMessage(&preset_request, true, true);
	getIEAckMessage(device_ie_id);
}

//Request a channel name.
void HondaXMHandler::getChannel(const uint16_t channel) {
	uint8_t channel_request_data[] = {0x50, 0x1, 0x19, 0x0, 0x1, 0x8, 0x0, 0x1, 0xFF, 0x0, 0xFF, 0x0, 0x1};

	if(channel < 1) {
		channel_request_data[12] = 0;
		channel_request_data[10] = 0;
	} else {
		channel_request_data[10] = getBCDFromByte(channel - 1)&0xFF;
		channel_request_data[9] = getBCDFromByte(channel - 1)&0xFF00;
	}

	IE_Message channel_request(sizeof(channel_request_data), IE_ID_RADIO, IE_ID_SIRIUS, 0xF, true);
	channel_request.refreshIEData(channel_request_data);

	ie_driver->sendMessage(&channel_request, true, true);
	getIEAckMessage(device_ie_id);
}

//Send the message for the "info" button to the IMID. Always resend the message.
void HondaXMHandler::sendIMIDInfoMessage() {
	sendIMIDInfoMessage(true);
}

//Send the message for the "info" button to the IMID. If resend is true, always resend the message.
void HondaXMHandler::sendIMIDInfoMessage(const bool resend) {
	if(!text_control || display_parameter == N_TEXT_NONE)
		return;
	
	String text_to_send = F(" ");
	
	if(display_timer_enabled) {
		if(display_parameter == N_SONG_NAME)
			text_to_send = F("SONG");
		else if(display_parameter == N_ARTIST_NAME)
			text_to_send = F("ARTIST");
		else if(display_parameter == N_CHANNEL_NAME)
			text_to_send = F("CHANNEL");
		else if(display_parameter == N_GENRE)
			text_to_send = F("GENRE");
	} else {
		if(display_parameter == N_SONG_NAME)
			text_to_send = String(song);
		else if(display_parameter == N_ARTIST_NAME)
			text_to_send = String(artist);
		else if(display_parameter == N_CHANNEL_NAME)
			text_to_send = String(channel_name);
		else if(display_parameter == N_GENRE)
			text_to_send = String(genre);
	}

	if(text_to_send.length() <= 0 || text_to_send.compareTo(" ") == 0)
		text_to_send = F("No Data");

	unsigned int pop_limit = text_to_send.length();
	for(unsigned int i=pop_limit - 1;i>=0;i-=1) {
		if(text_to_send.charAt(i) != ' ' && text_to_send.charAt(i) != 0) {
			pop_limit = i;
			break;
		}

		if(i <= 0)
			break;
	}

	text_to_send = text_to_send.substring(0, pop_limit + 1);

	if(resend && (parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines > 0))
		clearExternalIMID();

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
	
	if((resend || scroll_changed) && !text_timer_enabled) {
		sendIMIDInfoMessage(text_to_send);
	}
}

void HondaXMHandler::sendIMIDInfoMessage(String text) {
	if(parameter_list->imid_connected) {
		imid_handler->writeIMIDTextMessage(text);
	} else if(parameter_list->external_imid_char > 0 && parameter_list->external_imid_lines > 0) {
		uint8_t line = 1;
		if(parameter_list->external_imid_lines > 1)
			line = parameter_list->external_imid_lines/2+1;

		if(text.length() > parameter_list->external_imid_char)
			text = text.substring(0, parameter_list->external_imid_char);

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

void HondaXMHandler::clearXMText(const bool song_title, const bool artist, const bool channel, const bool genre) {
	if(song_title)
		this->song = String("");
	
	if(artist)
		this->artist = String("");

	if(channel)
		this->channel_name = String("");
	
	if(genre)
		this->genre = String("");
}

void HondaXMHandler::clearAIXMText(const bool song_title, const bool artist, const bool channel, const bool genre) {
	if(song_title) {
		uint8_t clear_data[] = {0x20, 0x60, 0x1};
		if(!artist && !channel && !genre)
			clear_data[1] |= 0x10;

		AIData clear_msg(sizeof(clear_data), ID_XM, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	}

	if(artist) {
		uint8_t clear_data[] = {0x20, 0x60, 0x2};
		if(!channel && !genre)
			clear_data[1] |= 0x10;

		AIData clear_msg(sizeof(clear_data), ID_XM, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	}

	if(channel) {
		uint8_t clear_data[] = {0x20, 0x61, 0x1};
		if(!genre)
			clear_data[1] |= 0x10;

		AIData clear_msg(sizeof(clear_data), ID_XM, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	}

	if(genre) {
		uint8_t clear_data[] = {0x20, 0x71, 0x0};
		AIData clear_msg(sizeof(clear_data), ID_XM, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	}
}

void HondaXMHandler::createXMMainMenu() {
	const uint8_t menu_size = 255;
	startAudioMenu(menu_size, menu_size, false, "XM Tuner Settings");

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

	*active_menu = MENU_XM;

	for(uint8_t i=0;i<menu_size;i+=1)
		createXMMainMenuOption(i);
	displayAudioMenu(1);
}

void HondaXMHandler::createXMMainMenuOption(const uint8_t option) {
	switch(option) {
	case 0:
		appendAudioMenu(0, "Presets");
		break;
	case 1:
		if(!manual_tune)
			appendAudioMenu(1, "Direct Tune");
		else
			appendAudioMenu(1, "< Direct Tune >");
		break;
	case 2:
		appendAudioMenu(2, "Channel List");
		break;
	case 3:
		appendAudioMenu(3, "Manual Entry");
		break;
	case 4:
		appendAudioMenu(4, "Audio Settings");
		break;
	}
}

void HondaXMHandler::createXMPresetMenu() {
	bool clear = false;

	uint8_t clear_data[] = {0x2B, 0x4A};
	AIData clear_msg(sizeof(clear_data), device_ai_id, ID_NAV_COMPUTER);
	clear_msg.refreshAIData(clear_data);

	ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	
	elapsedMillis cancel_wait;
	while(cancel_wait < 20) {
		AIData ai_msg;
		if(ai_driver->dataAvailable() > 0) {
			if(ai_driver->readAIData(&ai_msg)) {
				if(ai_msg.l >= 2 && ai_msg.sender == ID_NAV_COMPUTER && ai_msg.data[0] == 0x2B && ai_msg.data[1] == 0x40) { //No menu available.
					sendAIAckMessage(ai_msg.sender);
					clear = true;
					break;
				}
			}
		}
	}

	if(!clear)
		return;

	startAudioMenu(6, 6, false, "Select Preset");
	for(uint8_t i=0;i<6;i+=1)
		createXMPresetMenuOption(i);
	
	*active_menu = MENU_PRESET_LIST;

	if(current_preset > 0 && current_preset <= 6)
		displayAudioMenu(current_preset);
	else
		displayAudioMenu(1);
}

void HondaXMHandler::createXMPresetMenuOption(const uint8_t option) {
	uint16_t preset_ch = xm1_preset[option];
	String preset_name = xm1_preset_name[option];
	if(xm2) {
		preset_ch = xm2_preset[option];
		preset_name = xm2_preset_name[option];
	}

	appendAudioMenu(option, String(option + 1) + ". CH" + String(preset_ch) + ": " + preset_name);
}

void HondaXMHandler::createXMDirectMenu() {
	bool clear = false;

	uint8_t clear_data[] = {0x2B, 0x4A};
	AIData clear_msg(sizeof(clear_data), device_ai_id, ID_NAV_COMPUTER);
	clear_msg.refreshAIData(clear_data);

	ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	
	elapsedMillis cancel_wait;
	while(cancel_wait < 20) {
		AIData ai_msg;
		if(ai_driver->dataAvailable() > 0) {
			if(ai_driver->readAIData(&ai_msg)) {
				if(ai_msg.l >= 2 && ai_msg.sender == ID_NAV_COMPUTER && ai_msg.data[0] == 0x2B && ai_msg.data[1] == 0x40) { //No menu available.
					sendAIAckMessage(ai_msg.sender);
					clear = true;
					break;
				}
			}
		}
	}

	if(!clear)
		return;

	startAudioMenu(12, 2, false, "Enter Channel Number");

	*active_menu = MENU_DIRECT_TUNE;
	direct_num = 0;
	clearUpperField();
	appendDirectNumber(0);

	for(uint8_t i=0;i<12;i+=1)
		createXMDirectMenuOption(i);
	
	displayAudioMenu(1);
}

void HondaXMHandler::createXMDirectMenuOption(const uint8_t option) {
	if(option < 10)
		appendAudioMenu(option, String((option + 1)%10));
	else {
		switch(option) {
		case 10:
			appendAudioMenu(option, "#REV   ");
			break;
		case 11:
			appendAudioMenu(option, "Enter");
			break;
		}
	}
}

void HondaXMHandler::clearUpperField() {
	if(!text_control)
		return;

	AIData clear_msg(3, ID_XM, ID_NAV_COMPUTER);
	clear_msg.data[0] = 0x20;
	clear_msg.data[1] = 0x60;
	clear_msg.data[2] = 0x0;

	ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);

	clear_msg.data[1] = 0x61;
	for(uint8_t i=0;i<=2;i+=1) {
		clear_msg.data[2] = i;
		if(i >= 2)
			clear_msg.data[1] |= 0x10;

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);
	}
}

void HondaXMHandler::appendDirectNumber(const uint8_t num) {
	if(num > 0 && num <= 10) {
		if(direct_num <= 255) {
			direct_num*=10;
			direct_num += int(num)%10;
		}
	} else if(num == 11) { //Backspace.
		if(direct_num > 0)
			direct_num /= 10;
		else {
			uint8_t clear_data[] = {0x2B, 0x4A};
			AIData clear_msg(sizeof(clear_data), device_ai_id, ID_NAV_COMPUTER);
			clear_msg.refreshAIData(clear_data);

			ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);

			*active_menu = 0;
		}
	} else if(num == 12) { //Enter.
		uint8_t clear_data[] = {0x2B, 0x4A};
		AIData clear_msg(sizeof(clear_data), device_ai_id, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		ai_driver->writeAIData(&clear_msg, parameter_list->computer_connected);

		*active_menu = 0;
		setChannel(direct_num);
		direct_num = 0;
	}

	if(*active_menu == MENU_DIRECT_TUNE) {
		String direct_text = "CH ";
		direct_text += String(direct_num);

		AIData direct_msg(direct_text.length() + 3, ID_XM, ID_NAV_COMPUTER);
		direct_msg.data[0] = 0x23;
		direct_msg.data[1] = 0x70;
		direct_msg.data[2] = 0x0;

		for(unsigned int i=0;i<direct_text.length();i+=1)
			direct_msg.data[i+3] = uint8_t(direct_text.charAt(i));

		ai_driver->writeAIData(&direct_msg, parameter_list->computer_connected);
	}
}
