#include "Honda_IMID_Handler.h"

HondaIMIDHandler::HondaIMIDHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list) : HondaSourceHandler(ie_driver, ai_driver, parameter_list) {
	this->device_ie_id = IE_ID_IMID;
	this->device_ai_id = ID_IMID_SCR;

	requestor_vec.setStorage(requestor_list, 0);
	ai_cache_vec.setStorage(ai_cache);

	getIMIDSettings(&display_rds, &display_volume, (uint8_t*)&char_count);

	switch(char_count) {
	case CHAR_COUNT_8:
		max_char = 8;
		break;
	case CHAR_COUNT_10:
		max_char = 10;
		break;
	case CHAR_COUNT_12:
		max_char = 12;
		break;
	default:
		break;
	}
}

void HondaIMIDHandler::loop() {
	if(!source_established)
		return;

	if(frequency_change_timer_enabled && frequency_change_timer > FREQUENCY_CHANGE_TIMER) {
		frequency_change_timer_enabled = false;
		writeIMIDRadioMessage(frequency, decimal, preset, stereo_mode);
	}

	if(setting_changed) {
		setting_changed = false;
		setIMIDSettings(display_rds, display_volume, char_count);
	}

	if(parameter_list->power_on && imid_next_source >= 0) {
		if(imid_next_subsource < 0)
			imid_next_subsource = 0;

		if(setIMIDSource(imid_next_source&0xFF, imid_next_subsource&0xFF, true)) {
			imid_next_source = -1;
			imid_next_subsource = -1;
		}
		//TODO: Re-ping the source for data.
	}

	if(parameter_list->power_on && imid_change_timer >= IMID_CHANGE_TIMER_LOCAL && ai_cache_vec.size() > 0) {
		readAIBusMessage(&ai_cache_vec[0]);
		ai_cache_vec.remove(0);

		imid_change_timer = 0;
	}
}

void HondaIMIDHandler::interpretIMIDMessage(IE_Message* the_message) {
	if(the_message->receiver != IE_ID_RADIO || the_message->sender != IE_ID_IMID)
		return;
		
	bool ack = true;

	if(the_message->l == 2 && the_message->data[0] == 0x80) //Acknowledgement.
		return;
	
	if(the_message->l == 4) { //Handshake message.
		if(the_message->data[0] == 0x1 && the_message->data[1] == 0x1) {
			ack = false;
			sendIEAckMessage(IE_ID_IMID);

			ie_driver->addID(ID_IMID_SCR);

			if(!this->parameter_list->first_imid) {
				this->parameter_list->first_imid = true;

				uint8_t handshake_data[] = {0x0, 0x30};
				IE_Message handshake_msg(sizeof(handshake_data), IE_ID_RADIO, 0x1FF, 0xF, false);
				handshake_msg.refreshIEData(handshake_data);
				
				delay(2);

				ie_driver->sendMessage(&handshake_msg, false, false);
			}
		} else if(the_message->data[0] == 0x5 && the_message->data[2] == 0x3) {
			ack = false;
			sendIEAckMessage(IE_ID_IMID);
			
			delay(2);
			
			const bool handshake_rec = sendHandshakeAckMessage();
			if(!source_established) {
				source_established = handshake_rec;
				
				if(source_established) {
					this->parameter_list->imid_connected = true;
					uint8_t ping_data[] = {0x1};
					AIData ping(sizeof(ping_data), ID_IMID_SCR, ID_RADIO);
					ping.refreshAIData(ping_data);

					ai_driver->writeAIData(&ping, parameter_list->radio_connected);

					ie_driver->addID(ID_IMID_SCR);
					
					writeScreenLayoutMessage();
					if(parameter_list->radio_connected)
						writeVolumeLimitMessage();

					//uint8_t audio_off_data[] = {0x0, 0x0};
					//sendFunctionMessage(ie_driver, true, IE_ID_IMID, audio_off_data, sizeof(audio_off_data));
				}
			}
			
		} 
	} else if(the_message->l >= 7 && the_message->data[0] == 0x22 && the_message->data[1] == 0x11) { //Steering wheel button.
		ack = false;
		sendIEAckMessage(IE_ID_IMID);
		
		const uint8_t ctrl = the_message->data[5];

		uint8_t ai_button_data[] = {0x30, 0x00, 0x00};

		switch(ctrl) {
			case 0x5: //Volume down.
				ai_button_data[1] = 0x6;
				ai_button_data[2] |= 0x2;
				break;
			case 0x4: //Volume up.
				ai_button_data[1] = 0x6;
				ai_button_data[2] |= 0x1;
				break;
			case 0x3: //Seek left.
				ai_button_data[1] = 0x24;
				break;
			case 0x2: //Seek right.
				ai_button_data[1] = 0x25;
				break;
			case 0x1: //Source.
				ai_button_data[1] = 0x23;
				break;
		}

		if(button_held != 0 && button_held != ctrl) {
			uint8_t release_data[] = {0x30, 0x00, 0x80};
			switch(button_held) {
				case 0x5: //Volume down.
					release_data[1] = 0x6;
					release_data[2] |= 0x2;
					break;
				case 0x4: //Volume up.
					release_data[1] = 0x6;
					release_data[2] |= 0x1;
					break;
				case 0x3: //Seek left.
					release_data[1] = 0x24;
					break;
				case 0x2: //Seek right.
					release_data[1] = 0x25;
					break;
				case 0x1: //Source.
					release_data[1] = 0x23;
					break;
			}

			AIData release_msg(sizeof(release_data), ID_STEERING_CTRL, ID_RADIO);
			release_msg.refreshAIData(release_data);
			ai_driver->writeAIData(&release_msg, parameter_list->radio_connected);
		} else if(button_held == ctrl && ctrl != 0) {
			ai_button_data[2] |= 0x40;
		}

		if(ctrl != 0 && button_held != ctrl) {
			AIData button_msg(sizeof(ai_button_data), ID_STEERING_CTRL, ID_RADIO);
			button_msg.refreshAIData(ai_button_data);
			ai_driver->writeAIData(&button_msg, parameter_list->radio_connected);
		}

		button_held = ctrl;
	}

	if(ack)
		sendIEAckMessage(IE_ID_IMID);
}

void HondaIMIDHandler::readAIBusMessage(AIData* the_message) {
	if(the_message->receiver != ID_IMID_SCR)
		return;

	if(!parameter_list->power_on)
		return;

	/*if(cache && imid_change_timer < IMID_CHANGE_TIMER_LOCAL && 
			ai_cache_vec.size() < ai_cache_vec.max_size() &&
			!(the_message->l >= 3 && the_message->data[0] == 0x40 && the_message->data[1] == 0x10) &&
			!(the_message->l >= 3 && the_message->data[0] == 0x62)) {
		ai_driver->sendAcknowledgement(ID_IMID_SCR, the_message->sender);	
		ai_cache_vec.push_back(*the_message);
	}*/

	if(the_message->l >= 3 && the_message->data[0] == 0x4 && the_message->data[1] == 0xE6 && the_message->data[2] == 0x3B) {
		writeScreenLayoutMessage(the_message->sender);

		bool requestor_found = false;
		for(int i=0;i<requestor_vec.size();i+=1) {
			if(requestor_vec[i] == the_message->sender) {
				requestor_found = true;
				break;
			}
		}

		if(!requestor_found)
			requestor_vec.push_back(the_message->sender);
	} else if(the_message->l >= 4 && the_message->data[0] == 0x23 && the_message->data[1] == 0x60) { //Write a custom message.
		if(the_message->data[3] > 1)
			return;

		if(imid_next_source >= 0) {
			ai_cache_vec.push_back(*the_message);
			return;
		}
		
		const uint8_t space = the_message->data[2];
	
		String new_text = "";

		for(uint8_t i=0;i<space;i+=1)
			new_text += " ";

		for(uint8_t i=4;i<the_message->l;i+=1)
			new_text += char(the_message->data[i]);

		if(!writeIMIDTextMessage(new_text)) {
			imid_change_timer = 0;
			ai_cache_vec.push_back(*the_message);
		}
	} else if(the_message->l >= 3 && the_message->data[0] == 0x40 && the_message->data[1] == 0x10 && the_message->sender == ID_RADIO) { //Function change.
		const uint8_t source = the_message->data[2];
		bool set = false;

		imid_next_subsource = -1;

		if(the_message->l >= 4) {
			const uint8_t subsource = the_message->data[3];
			imid_next_subsource = subsource;
			set = setIMIDSource(source, subsource);
		} else
			set = setIMIDSource(source, 0);

		if(!set) {
			imid_next_source = source;
			imid_change_timer = 0;
		}

		if(the_message->l >= 4 && source == ID_CDC && the_message->data[3] > 4) {
			uint8_t data_request_data[] = {0x60, 0x10};
			AIData data_request(sizeof(data_request_data), ID_IMID_SCR, ID_RADIO, data_request_data);

			ai_driver->writeAIData(&data_request, parameter_list->radio_connected);
		}
	} else if((the_message->data[0] == 0x39 || the_message->data[0] == 0x3B) && the_message->sender == ID_CDC) {
		if(the_message->l >= 8 && the_message->data[0] == 0x39) { //Track and disc.
			bool set = false;
			if(the_message->data[6] != disc)
				set = setIMIDSource(ID_CDC, 0);

			if(!set)
				imid_next_source = ID_CDC;
			
			ai_cd_mode = the_message->data[2];
			
			const uint8_t last_track = track, last_disc = disc;
			disc = the_message->data[6];
			track = the_message->data[7];
			
			const uint8_t cd_state = the_message->data[2];

			if(!writeIMIDCDCTrackMessage(disc, track, track_count, timer, getIECDStatus(ai_cd_mode), getIECDRepeat(ai_cd_mode))) {
				ai_cache_vec.push_back(*the_message);
				imid_change_timer = 0;
				return;
			}

			if(last_disc != disc || last_track != track) {
				if(!writeIMIDCDCTextMessage(1, "")) {
					ai_cache_vec.push_back(*the_message);
					imid_change_timer = 0;
					return;
				}

				if(!writeIMIDCDCTextMessage(2, ""))  {
					ai_cache_vec.push_back(*the_message);
					imid_change_timer = 0;
					return;
				}

				if(last_disc != disc) {
					if(!writeIMIDCDCTextMessage(0, ""))  {
						ai_cache_vec.push_back(*the_message);
						imid_change_timer = 0;
						return;
					}
				}
			}
		} else if(the_message->l >= 5 && the_message->data[0] == 0x3B) {
			timer = (the_message->data[3]<<8)|the_message->data[4];

			if(!writeIMIDCDCTrackMessage(disc, track, track_count, timer, getIECDStatus(ai_cd_mode), getIECDRepeat(ai_cd_mode))) {
				imid_change_timer = 0;
				ai_cache_vec.push_back(*the_message);
			}
		}
	} else if((the_message->sender == ID_CDC || the_message->sender == ID_CD) && the_message->l >= 2 && the_message->data[0] == 0x23) { //CD text message.
		uint8_t field = 0;

		switch(the_message->data[1]&0xF) {
			case 0x1: //Song title.
				field = 1;
				break;
			case 0x2: //Artist.
				field = 2;
				break;
			case 0x3: //Album.
				field = 0;
				break;
			default:
				return;
		}

		String text = "";
		for(int i=3;i<the_message->l;i+=1)
			text += char(the_message->data[i]);

		//TODO: Byte count.
		if(!writeIMIDCDCTextMessage(field, text)) {
			imid_change_timer = 0;
			ai_cache_vec.push_back(*the_message);
		}
	} else if(the_message->sender == ID_RADIO && the_message->l >= 6 && the_message->data[0] == 0x67) { //Frequency change message.	
		if(imid_next_source >= 0) {
			ai_cache_vec.push_back(*the_message);
			imid_change_timer = 0;
			return;
		}
		
		const uint16_t last_frequency = this->frequency;
		const uint16_t frequency = (the_message->data[1]<<8) | the_message->data[2];

		if(last_frequency != frequency)
			this->rds = false;

		/*if(frequency_change_timer_enabled && frequency_change_timer < 200) {
			this->frequency = frequency;
			this->decimal = the_message->data[3];
			this->preset = the_message->data[4]&0xF;
			this->stereo_mode = (the_message->data[4]&0xF0)>>4;

			if(frequency_change_timer > 150)
				frequency_change_timer = 150;
			frequency_change_timer_enabled = true;
			imid_change_timer = 0;
			return;
		}*/

		bool set = writeIMIDRadioMessage(frequency, the_message->data[3], the_message->data[4]&0xF, (the_message->data[4]&0xF0)>>4, false);
		imid_change_timer = 0;

		if(!set) {
			imid_change_timer = 0;
			ai_cache_vec.push_back(*the_message);
		} else {
			frequency_change_timer_enabled = true;
		}
	} else if(the_message->sender == ID_RADIO && the_message->l >= 2 && the_message->data[0] == 0x63) { //RDS.
		if(imid_next_source >= 0) {
			ai_cache_vec.push_back(*the_message);
			imid_change_timer = 0;
			return;
		}
		
		String rds_str = "";
		for(int i=2;i<the_message->l;i+=1)
			rds_str += char(the_message->data[i]);

		if(rds_str.equals(" ") || (rds_str.length() > 0 && rds_str[0] == 0x0))
			rds_str = "";
		
		if(the_message->data[1] == 0x61) { // True RDS.
			if(rds_str.length() > 0)
				this->rds = true;

			if(!writeIMIDRDSMessage(rds_str)) {
				imid_change_timer = 0;
				ai_cache_vec.push_back(*the_message);
			}
		} else if(the_message->data[1] == 0x60) { //Call sign.
			if(!writeIMIDCallsignMessage(rds_str)) {
				imid_change_timer = 0;
				ai_cache_vec.push_back(*the_message);
			}
		}
	} else if(the_message->sender == ID_RADIO && the_message->l >= 3 && the_message->data[0] == 0x62) { //Volume control display.
		if(!display_volume)
			return;

		if(imid_next_source >= 0) {
			ai_cache_vec.push_back(*the_message);
			imid_change_timer = 0;
			return;
		}
		
		const int max_vol = 40;
		const uint8_t new_vol = the_message->data[1]*max_vol/the_message->data[2];

		writeIMIDVolumeMessage(new_vol);
	} else if(the_message->sender == ID_XM && the_message->l >= 5 && the_message->data[0] == 0x39) { //XM channel change message.
		if(imid_next_source >= 0) {
			ai_cache_vec.push_back(*the_message);
			return;
		}
		
		const uint16_t channel = (the_message->data[2]<<8) | the_message->data[3];
		const uint8_t preset = the_message->data[4]&0x3F;

		if((the_message->data[4]&0xC0) == 0) {
			if(!writeIMIDSiriusNumberMessage(preset, channel, this->xm2)) {
				imid_change_timer = 0;
				ai_cache_vec.push_back(*the_message);
			}
		}
		//TODO: If signal is unavailable, send the appropriate message.
	} else if(the_message->sender == ID_XM && the_message->l >= 2 && the_message->data[0] == 0x23) { //Sirius text message.
		if(imid_next_source >= 0) {
			ai_cache_vec.push_back(*the_message);
			imid_change_timer = 0;
			return;
		}
		
		uint8_t field = 0;
		switch(the_message->data[1]&0xF) {
			case 1:
				field = 2;
				break;
			case 2:
				field = 1;
				break;
			case 3:
				field = 0;
				break;
			case 4:
				field = 3;
				break;
			default:
				return;
		}

		String text = "";
		for(unsigned int i=3;i<the_message->l;i+=1)
			text += char(the_message->data[i]);

		if(!writeIMIDSiriusTextMessage(field, text)) {
			imid_change_timer = 0;
			ai_cache_vec.push_back(*the_message);
		}
	} else if((the_message->sender == ID_PHONE || the_message->sender == ID_ANDROID_AUTO) && the_message->l >= 5 && the_message->data[0] == 0x3B) { //Bluetooth timer.
		if(imid_next_source >= 0) {
			ai_cache_vec.push_back(*the_message);
			imid_change_timer = 0;
			return;
		}
		
		const long time = the_message->data[4] | (the_message->data[3]<<8);
		if(!setBTTimer(time)) {
			ai_cache_vec.push_back(*the_message);
			imid_change_timer = 0;
		}
	} else if(the_message->sender == ID_PHONE && the_message->l >= 3 && the_message->data[0] == 0x23 && (the_message->data[1]&0xF0) == 0x60) { //Bluetooth text message.	
		if(imid_next_source >= 0) {
			ai_cache_vec.push_back(*the_message);
			imid_change_timer = 0;
			return;
		}
		
		uint8_t leader = 0xFF;

		switch(the_message->data[1]) {
		case 0x61:
			leader = 0x42;
			break;
		case 0x62:
			leader = 0x43;
			break;
		case 0x63:
			leader = 0x44;
			break;
		case 0x64:
			leader = 0x41;
			break;
		default:
			return;
		}

		String sent_text = "";
		for(int i=3;i<the_message->l;i+=1)
			sent_text += char(the_message->data[i]);

		if(!this->setBTText(leader, sent_text)) {
			imid_change_timer = 0;
			ai_cache_vec.push_back(*the_message);
		}
	} else if(the_message->sender == ID_ANDROID_AUTO && the_message->l >= 2 && the_message->data[0] == 0x30 && the_message->data[1] == 0x0) { //Phone disconnected.
		if(imid_next_source >= 0) {
			ai_cache_vec.push_back(*the_message);
			imid_change_timer = 0;
			return;
		}
		
		if(!this->setBTModeNotConnected()) {
			imid_change_timer = 0;
			ai_cache_vec.push_back(*the_message);
			return;
		}
	} else if(the_message->sender == ID_ANDROID_AUTO && the_message-> l >= 2 && the_message->data[0] == 0x23 && (the_message->data[1]&0xF0) == 0x60) { //Mirror text message.
		if(imid_next_source >= 0) {
			ai_cache_vec.push_back(*the_message);
			imid_change_timer = 0;
			return;
		}
		
		uint8_t leader = 0xFF;

		switch(the_message->data[1]) {
		case 0x61:
			leader = 0x42;
			break;
		case 0x62:
			leader = 0x43;
			break;
		case 0x63:
			leader = 0x44;
			break;
		case 0x64:
			leader = 0x45;
			break;
		default:
			return;
		}

		String sent_text = "";
		for(int i=2;i<the_message->l;i+=1)
			sent_text += char(the_message->data[i]);

		if(sent_text.length() >= 0 || leader != 0x45) {
			if(!this->setBTText(leader, sent_text)) {
				imid_change_timer = 0;
				ai_cache_vec.push_back(*the_message);
				return;
			}
		} else {
			
		}
	} else if(the_message->sender == ID_NAV_COMPUTER && the_message->l >= 2 && the_message->data[0] == 0x2B) { //Menu message.
		const uint8_t operation = the_message->data[1];

		if(operation == 0x55 || operation == 0x56 || operation == 0x57) { //Menu presence request.
			String menu_requested = "";
			for(int i=2;i<the_message->l;i+=1)
				menu_requested += char(the_message->data[i]);

			menu_requested.trim();

			if(menu_requested.equalsIgnoreCase("SETTING")) {
				const MenuList settings_menu = getMenu(MENU_INDEX_IMID_SETTINGS, parameter_list->locale);
				const String menu_name = settings_menu.title;

				if(operation == 0x55) //Display the menu.
					createIMIDSettingsMenu();
				else {
					uint8_t response_data[] = {0x2B, operation};
					AIData response_msg(sizeof(response_data) + (operation == 0x57 ? menu_name.length() : 0), ID_IMID_SCR, the_message->sender);

					for(int i=0;i<sizeof(response_data);i+=1)
						response_msg[i] = response_data[i];

					if(operation == 0x57) {
						for(int i=0;i<menu_name.length() && i+sizeof(response_data)<response_msg.l;i+=1)
							response_msg[i+sizeof(response_data)] = uint8_t(menu_name[i]);
					}

					ai_driver->writeAIData(&response_msg);
				}
			}
		} else if(operation == 0x45) { //Menu request.
			createIMIDSettingsMenu();
		} else if(operation == 0x40) { //Menu closed.
			if(*active_menu == MENU_IMID) {
				*active_menu = 0;

				uint8_t close_data[] = {0x2B, 0x40};
				AIData close_msg(sizeof(close_data), ID_IMID_SCR, the_message->sender, close_data);
				ai_driver->writeAIData(&close_msg);
			} else if(*active_menu == MENU_IMID_CHARACTER) {
				createIMIDSettingsMenu();
			}
		} else if(operation == 0x60) { //Selection.
			const int selected = the_message->data[2] - 1;
			if(selected < 0)
				return;

			if(*active_menu == MENU_IMID) {
				const MenuList settings_menu = getMenu(MENU_INDEX_IMID_SETTINGS, parameter_list->locale);
				switch(settings_menu.getGlobalIndex(selected)) {
				case MENU_INDEX_IMID_SETTINGS_RDS:
					display_rds = !display_rds;
					createIMIDSettingsMenuOption(selected);

					if((imid_mode&0xFF) == ID_RADIO)
						writeIMIDRadioMessage(frequency, decimal, preset, stereo_mode);
					
					setting_changed = true;
					break;
				case MENU_INDEX_IMID_SETTINGS_CD_TEXT:
					if(parameter_list->imid_cd_text != nullptr && parameter_list->imid_cd_text != NULL) {
						*parameter_list->imid_cd_text = !*parameter_list->imid_cd_text;
						parameter_list->cd_text_changed = true;

						createIMIDSettingsMenuOption(selected);
					}
					break;
				case MENU_INDEX_IMID_SETTINGS_VOLUME:
					display_volume = !display_volume;
					createIMIDSettingsMenuOption(selected);
					setting_changed = true;
					break;
				case MENU_INDEX_IMID_SETTINGS_LENGTH:
					createIMIDCharacterMenu();
					break;
				}
			} else if(*active_menu == MENU_IMID_CHARACTER) {
				char_count = (char_count_t)(selected&0b11);
				setting_changed = true;

				switch(char_count) {
				case CHAR_COUNT_8:
					max_char = 8;
					break;
				case CHAR_COUNT_10:
					max_char = 10;
					break;
				case CHAR_COUNT_12:
					max_char = 12;
					break;
				default:
					break;
				}

				writeScreenLayoutMessage(0xFF);
				for(int i=0;i<requestor_vec.size();i+=1)
					writeScreenLayoutMessage(requestor_vec[i]);

				createIMIDSettingsMenu();
			}
		}
	}
}

//Get the IMID change timer.
unsigned long HondaIMIDHandler::getIMIDChangeTimer() {
	return this->imid_change_timer;
}

//Write the volume limit message.
void HondaIMIDHandler::writeVolumeLimitMessage() {
	uint8_t vol_limit_data[] = {0x33, 0x6, VOL_LIMIT>>8, VOL_LIMIT&0xFF};
	AIData vol_limit_msg(sizeof(vol_limit_data), ID_IMID_SCR, ID_RADIO);

	vol_limit_msg.refreshAIData(vol_limit_data);
	ai_driver->writeAIData(&vol_limit_msg);
}

//Write the screen layout message.
void HondaIMIDHandler::writeScreenLayoutMessage() {
	uint8_t screen_field_data[] = {0x3B, 0x23, max_char, LINES};
	AIData screen_field_msg(sizeof(screen_field_data), ID_IMID_SCR, 0xFF);
	screen_field_msg.refreshAIData(screen_field_data);

	uint8_t screen_oem_data[] = {0x3B, 0x57, ID_RADIO, ID_CD, ID_CDC, ID_XM, ID_PHONE, ID_ANDROID_AUTO};
	AIData screen_oem_msg(sizeof(screen_oem_data), ID_IMID_SCR, 0xFF);
	screen_oem_msg.refreshAIData(screen_oem_data);

	ai_driver->writeAIData(&screen_field_msg, false);

	elapsedMillis wait_timer;
	while(wait_timer < 50) {
		ie_driver->cacheAIBus();
	}

	ai_driver->writeAIData(&screen_oem_msg, false);
}

//Write the screen layout message.
void HondaIMIDHandler::writeScreenLayoutMessage(const uint8_t receiver) {
	uint8_t screen_field_data[] = {0x3B, 0x23, max_char, LINES};
	AIData screen_field_msg(sizeof(screen_field_data), ID_IMID_SCR, receiver);
	screen_field_msg.refreshAIData(screen_field_data);

	uint8_t screen_oem_data[] = {0x3B, 0x57, ID_RADIO, ID_CD, ID_CDC, ID_XM, ID_PHONE, ID_ANDROID_AUTO};
	AIData screen_oem_msg(sizeof(screen_oem_data), ID_IMID_SCR, receiver);
	screen_oem_msg.refreshAIData(screen_oem_data);

	ai_driver->writeAIData(&screen_field_msg, receiver != 0xFF);
	ai_driver->writeAIData(&screen_oem_msg, receiver != 0xFF);
	
	uint8_t vol_limit_data[] = {0x33, 0x6, VOL_LIMIT>>8, VOL_LIMIT&0xFF};
	AIData vol_limit_msg(sizeof(vol_limit_data), ID_IMID_SCR, receiver);

	vol_limit_msg.refreshAIData(vol_limit_data);
	ai_driver->writeAIData(&vol_limit_msg, receiver != 0xFF);
}

void HondaIMIDHandler::writeTimeAndDayMessage(uint8_t hour, const uint8_t minute, const uint8_t month, const uint8_t day, const uint16_t year, const bool display_24h) {
	//TODO: Day of week.
	if(!display_24h) {
		if(hour > 12)
			hour -= 12;
		else if(hour == 0)
			hour = 12;
	}

	uint8_t time_data[] = {0x60, 0xD, 0x11, 0x0, 0x1, 0x40, uint8_t(getBCDFromByte(hour)), uint8_t(getBCDFromByte(minute)), 0x0, 0x1, uint8_t((getBCDFromByte(year)>>8)&0xFF), uint8_t(getBCDFromByte(year)&0xFF), month, day, 0xF};

	if(!display_24h)
		time_data[9] = 0x2;

	IE_Message time_msg(sizeof(time_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	time_msg.refreshIEData(time_data);
	ie_driver->sendMessage(&time_msg, true, true);
	getIEAckMessage(device_ie_id);
}

//Write the volume to the screen.
bool HondaIMIDHandler::writeIMIDVolumeMessage(const uint8_t volume) {
	uint8_t volume_data[] = {0x60, 0x2, 0x11, 0x0, 0x2, 0x0, volume, 0x10, 0x0, 0x30, 0x0, 0x0, 0x9, 0x0, 0x0, 0x0};

	IE_Message volume_msg(sizeof(volume_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	volume_msg.refreshIEData(volume_data);

	ie_driver->sendMessage(&volume_msg, true, true);
	return getIEAckMessage(&volume_msg, device_ie_id);
}

bool HondaIMIDHandler::writeIMIDTextMessage(String text) {
	uint8_t bta_trigger_data[] = {0x40, 0x0, 0x11, 0x2, 0x23, 0x0};
	uint8_t bta_data1[] = {0x60, 0x23, 0x11, 0x0, 0xD1, 0x10, 0x0, 0x7A};
	uint8_t bta_data2[] = {0x60, 0x23, 0x11, 0x0, 0xD1, 0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t bta_data3[] = {0x60, 0x23, 0x11, 0x0, 0xD1, 0x0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0, 0xFF, 0xF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	uint8_t bta_text_data1[] = {0x60, 0x23, 0x11, 0x0, 0x1, 0x41, 0x2, 0x0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	uint16_t len = text.length();
	if(len > 16)
		len = 16;

	if(len > INT_CHAR_LIMIT)
		len = INT_CHAR_LIMIT;

	for(int i=0;i<len;i+=1) {
		bta_text_data1[i + 8] = uint8_t(text.charAt(i));
	}

	IE_Message bta_trigger_msg(sizeof(bta_trigger_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message bta_msg1(sizeof(bta_data1), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message bta_msg2(sizeof(bta_data2), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message bta_msg3(sizeof(bta_data3), IE_ID_RADIO, IE_ID_IMID, 0xF, true);

	IE_Message bta_text1(sizeof(bta_text_data1), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	
	bta_trigger_msg.refreshIEData(bta_trigger_data);
	bta_msg1.refreshIEData(bta_data1);
	bta_msg2.refreshIEData(bta_data2);
	bta_msg3.refreshIEData(bta_data3);
	
	bta_text1.refreshIEData(bta_text_data1);

	if(imid_mode != 0x23) {
		ie_driver->sendMessage(&bta_trigger_msg, true, true);
		if(!getIEAckMessage(&bta_trigger_msg, device_ie_id)) {
			imid_next_source = 0x23;
			return false;
		}
		
		delay(10);
		ie_driver->sendMessage(&bta_msg1, true, true);
		if(!getIEAckMessage(&bta_msg1, device_ie_id)) {
			imid_next_source = 0x23;
			return false;
		}
		
		delay(10);
		ie_driver->sendMessage(&bta_msg2, true, true);
		if(!getIEAckMessage(&bta_msg2, device_ie_id)) {
			imid_next_source = 0x23;
			return false;
		}
		
		delay(10);
		ie_driver->sendMessage(&bta_msg3, true, true);
		if(!getIEAckMessage(&bta_msg3, device_ie_id)) {
			imid_next_source = 0x23;
			return false;
		}

		imid_mode = 0x23;
	}
	
	delay(10);
	ie_driver->sendMessage(&bta_text1, true, true);
	if(!getIEAckMessage(&bta_text1, device_ie_id))
		return false;
	
	return true;
	
}

//Set the source displayed by the IMID.
bool HondaIMIDHandler::setIMIDSource(const uint8_t source, const uint8_t subsource) {
	return setIMIDSource(source, subsource, false);
}

//Set the source displayed by the IMID.
bool HondaIMIDHandler::setIMIDSource(const uint8_t source, const uint8_t subsource, const bool force_set) {
	bool new_source = true;
	if(!force_set && parameter_list->imid_connected && source != 0 && this->imid_mode == (source | (subsource<<8)))
		new_source = false;
	
	this->imid_mode = source | (subsource << 8);

	if(source == 0) {
		uint8_t source_data[] = {0x0, 0x1};
		sendFunctionMessage(ie_driver, new_source, IE_ID_IMID, source_data, sizeof(source_data));
		if(!getIEAckMessage(device_ie_id)) {
			IE_Message function_message = getFunctionMessage(new_source, IE_ID_IMID, source_data, sizeof(source_data));
			return getIEAckMessage(&function_message, device_ie_id);
		}
		return true;
	} else if(source == ID_RADIO) {
		uint8_t source_data[] = {0x7, 0x0, 0x1};
		source_data[0] = 0x7;
		source_data[2] = 0x1;
		switch(subsource) {
			case 1: //FM2
				source_data[2] = 0x2;
				break;
			case 2: //AM
				source_data[2] = 0x11;
				break;
			case 3: //Aux.
				source_data[0] = 0x24;
				source_data[2] = 0x0;
		}

		sendFunctionMessage(ie_driver, new_source, IE_ID_IMID, source_data, sizeof(source_data));
		if(!getIEAckMessage(device_ie_id)) {
			IE_Message function_message = getFunctionMessage(new_source, IE_ID_IMID, source_data, sizeof(source_data));
			return getIEAckMessage(&function_message, device_ie_id);
		} else
			return true;
	} else if(source == ID_CD || source == ID_CDC) {
		bool set = true;
		
		uint8_t source_data[] = {source, 0x0};
		sendFunctionMessage(ie_driver, new_source, IE_ID_IMID, source_data, sizeof(source_data));
		if(!getIEAckMessage(device_ie_id)) {
			IE_Message function_message = getFunctionMessage(new_source, IE_ID_IMID, source_data, sizeof(source_data));
			set &= getIEAckMessage(&function_message, device_ie_id);
		}
		
		if(new_source) {
			if(source == ID_CDC) {
				uint8_t cd_data_1[] = {0x60, 0x6, 0x11, 0x0, 0x10, 0x10, 0x6, 0x3A};
				uint8_t cd_data_2[] = {0x60, 0x6, 0x11, 0x0, 0x15, 0x1, 0x80, 0x0, 0xA0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
				
				IE_Message cd_msg_1(sizeof(cd_data_1), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
				cd_msg_1.refreshIEData(cd_data_1);
				
				IE_Message cd_msg_2(sizeof(cd_data_2), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
				cd_msg_2.refreshIEData(cd_data_2);
				
				ie_driver->sendMessage(&cd_msg_1, true, true);
				set &= getIEAckMessage(&cd_msg_1, device_ie_id);
				
				ie_driver->sendMessage(&cd_msg_2, true, true);
				set &= getIEAckMessage(&cd_msg_2, device_ie_id);
			} else {
				uint8_t cd_data_1[] = {0x60, 0x4, 0x11, 0x0, 0x15, 0x10, 0x1, 0x2A, 0x80, 0xE0, 0x1};
				uint8_t cd_data_2[] = {0x60, 0x4, 0x11, 0x0, 0x15, 0x1, 0x80, 0x0, 0xA0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
				
				IE_Message cd_msg_1(sizeof(cd_data_1), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
				cd_msg_1.refreshIEData(cd_data_1);
				
				IE_Message cd_msg_2(sizeof(cd_data_2), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
				cd_msg_2.refreshIEData(cd_data_2);
				
				ie_driver->sendMessage(&cd_msg_1, true, true);
				set &= getIEAckMessage(&cd_msg_1, device_ie_id);
				
				ie_driver->sendMessage(&cd_msg_2, true, true);
				set &= getIEAckMessage(&cd_msg_2, device_ie_id);
			}
		}
		
		return set;
	} else if(source == ID_XM) {
		uint8_t source_data[] = {0x19, 0x0, uint8_t(subsource + 1)};
		sendFunctionMessage(ie_driver, new_source, IE_ID_IMID, source_data, sizeof(source_data));
		if(!getIEAckMessage(device_ie_id)) {
			IE_Message function_message = getFunctionMessage(new_source, IE_ID_IMID, source_data, sizeof(source_data));
			return getIEAckMessage(&function_message, device_ie_id);
		}
		return true;
	} else if(source == ID_ANDROID_AUTO) {
		/*uint8_t source_data[] = {0x23, 0x0, 0x0};
		sendFunctionMessage(ie_driver, new_source, IE_ID_IMID, source_data, sizeof(source_data));
		getIEAckMessage(device_ie_id);

		if(new_source)
			setBTMode();

		return true;*/
		return writeIMIDTextMessage("Mirror");
	} else if (source == ID_PHONE) {
		bool set = true;
		
		uint8_t source_data[] = {0x23, 0x0, 0x0};
		sendFunctionMessage(ie_driver, new_source, IE_ID_IMID, source_data, sizeof(source_data));
		if(!getIEAckMessage(device_ie_id)) {
			IE_Message function_message = getFunctionMessage(new_source, IE_ID_IMID, source_data, sizeof(source_data));
			set &= getIEAckMessage(&function_message, device_ie_id);
		}
		if(new_source) {
			set &= setBTMode();
			if(!set)
				imid_next_source = ID_PHONE;
		}

		return set;
	} else
		return true;
}

//Set the screen to show "iPod."
void HondaIMIDHandler::setUSBMode() {
	uint8_t ipod_data_0[] = {0x10, 0x12, 0x11, 0x0, 0x0};
	uint8_t ipod_data_1[] = {0x60, 0x2, 0x11, 0x0, 0x1, 0x0, 0xB, 0x10, 0x0, 0x30, 0x10, 0x0, 0x9, 0x0, 0x0, 0x0};
	uint8_t ipod_data_2[] = {0x60, 0x12, 0x11, 0x0, 0x0, 0x1, 0x0, 0x0};
	uint8_t ipod_data_3[] = {0x60, 0x22, 0x11, 0x0, 0x3, 0xA2, 0x7, 0x1};
	uint8_t ipod_data_4[] = {0x60, 0x22, 0x11, 0x0, 0x3, 0xA1, 0x0, 0x0};
	uint8_t ipod_data_5[] = {0x60, 0x22, 0x11, 0x0, 0x3, 0xA0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF1, 0xFF, 0xFF, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	IE_Message ipod_msg_0(sizeof(ipod_data_0), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message ipod_msg_1(sizeof(ipod_data_1), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message ipod_msg_2(sizeof(ipod_data_2), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message ipod_msg_3(sizeof(ipod_data_3), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message ipod_msg_4(sizeof(ipod_data_4), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message ipod_msg_5(sizeof(ipod_data_5), IE_ID_RADIO, IE_ID_IMID, 0xF, true);

	ipod_msg_0.refreshIEData(ipod_data_0);
	ipod_msg_1.refreshIEData(ipod_data_1);
	ipod_msg_2.refreshIEData(ipod_data_2);
	ipod_msg_3.refreshIEData(ipod_data_3);
	ipod_msg_4.refreshIEData(ipod_data_4);
	ipod_msg_5.refreshIEData(ipod_data_5);

	ie_driver->sendMessage(&ipod_msg_0, true, true);
	if(!getIEAckMessage(&ipod_msg_0, device_ie_id))
		return;

	ie_driver->sendMessage(&ipod_msg_1, true, true);
	if(!getIEAckMessage(&ipod_msg_1, device_ie_id))
		return;

	ie_driver->sendMessage(&ipod_msg_2, true, true);
	if(!getIEAckMessage(&ipod_msg_2, device_ie_id))
		return;

	ie_driver->sendMessage(&ipod_msg_3, true, true);
	if(!getIEAckMessage(&ipod_msg_3, device_ie_id))
		return;

	ie_driver->sendMessage(&ipod_msg_4, true, true);
	if(!getIEAckMessage(&ipod_msg_4, device_ie_id))
		return;

	ie_driver->sendMessage(&ipod_msg_5, true, true);
	if(!getIEAckMessage(&ipod_msg_5, device_ie_id))
		return;
		
	clearUSBText(0xBD);
	clearUSBText(0xB8);
	clearUSBText(0xBA);
	clearUSBText(0xB9);
	clearUSBText(0xBF);
		
	setUSBText(0xBD, "");
	setUSBText(0xB8, "");
	setUSBText(0xBA, "");
	setUSBText(0xB9, "");
	setUSBText(0xBF, "");
}

//Clear the iPod text fields.
void HondaIMIDHandler::clearUSBText(const uint8_t field) {
	uint8_t text_data[] = {0x60, 0x22, 0x11, 0x0, 0x3, field, 0x2, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};


	IE_Message text_msg(sizeof(text_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	text_msg.refreshIEData(text_data);

	ie_driver->sendMessage(&text_msg, true, true);
	getIEAckMessage(&text_msg, device_ie_id);
	
	text_data[7] = 0x12;
	text_msg.refreshIEData(text_data);
	ie_driver->sendMessage(&text_msg, true, true);
	getIEAckMessage(&text_msg, device_ie_id);
}

//Set an iPod/USB(?) text field.
void HondaIMIDHandler::setUSBText(const uint8_t field, String text) {
	uint8_t text_data[] = {0x60, 0x22, 0x11, 0x0, 0x3, field, 0x2, 0x2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	
	int limit = text.length();
	if(limit > 16)
		limit = 16;

	for(int i=0;i<limit;i+=1)
		text_data[i+8] = uint8_t(text.charAt(i));

	IE_Message text_msg(sizeof(text_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	text_msg.refreshIEData(text_data);

	ie_driver->sendMessage(&text_msg, true, true);
	getIEAckMessage(&text_msg, device_ie_id);
}

//Set the screen to show full BTA.
bool HondaIMIDHandler::setBTMode() {
	this->imid_mode = ID_PHONE;

	uint8_t bta_data1[] = {0x60, 0x23, 0x11, 0x0, 0xD1, 0x10, 0x0, 0x7A};
	uint8_t bta_data2[] = {0x60, 0x23, 0x11, 0x0, 0xD1, 0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t bta_data3[] = {0x60, 0x23, 0x11, 0x0, 0xD1, 0x0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0, 0xFF, 0xF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t bta_data4[] = {0x60, 0x23, 0x11, 0x0, 0x1, 0x0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0, 0xFF, 0xF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t bta_data5[] = {0x60, 0x23, 0x11, 0x0, 0x1, 0x1, 0xFF, 0x3, 0x14, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1}; //Byte [10] indicates BT connected.
	uint8_t bta_data6[] = {0x60, 0x23, 0x11, 0x0, 0x1, 0x0, 0xFF, 0xFF, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0, 0xFF, 0xF0, 0x0, 0xFF, 0xF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	IE_Message bta_msg1(sizeof(bta_data1), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message bta_msg2(sizeof(bta_data2), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message bta_msg3(sizeof(bta_data3), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message bta_msg4(sizeof(bta_data4), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message bta_msg5(sizeof(bta_data5), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message bta_msg6(sizeof(bta_data6), IE_ID_RADIO, IE_ID_IMID, 0xF, true);

	bta_msg1.refreshIEData(bta_data1);
	bta_msg2.refreshIEData(bta_data2);
	bta_msg3.refreshIEData(bta_data3);
	bta_msg4.refreshIEData(bta_data4);
	bta_msg5.refreshIEData(bta_data5);
	bta_msg6.refreshIEData(bta_data6);

	ie_driver->sendMessage(&bta_msg1, true, true);
	if(!getIEAckMessage(&bta_msg1, device_ie_id))
		return false;

	ie_driver->sendMessage(&bta_msg2, true, true);
	if(!getIEAckMessage(&bta_msg2, device_ie_id))
		return false;

	ie_driver->sendMessage(&bta_msg3, true, true);
	if(!getIEAckMessage(&bta_msg3, device_ie_id))
		return false;

	ie_driver->sendMessage(&bta_msg4, true, true);
	if(!getIEAckMessage(&bta_msg4, device_ie_id))
		return false;

	ie_driver->sendMessage(&bta_msg5, true, true);
	if(!getIEAckMessage(&bta_msg5, device_ie_id))
		return false;

	ie_driver->sendMessage(&bta_msg6, true, true);
	if(!getIEAckMessage(&bta_msg6, device_ie_id))
		return false;

	return true;
}

//Set the screen to show BTA with "device not connected."
bool HondaIMIDHandler::setBTModeNotConnected() {
	this->imid_mode = 0x100 | ID_PHONE;

	uint8_t bta_data1[] = {0x60, 0x23, 0x11, 0x0, 0xD1, 0x10, 0x0, 0x7A};
	uint8_t bta_data2[] = {0x60, 0x23, 0x11, 0x0, 0xD1, 0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	//TODO: Additional messages as needed.

	IE_Message bta_msg1(sizeof(bta_data1), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	IE_Message bta_msg2(sizeof(bta_data2), IE_ID_RADIO, IE_ID_IMID, 0xF, true);

	bta_msg1.refreshIEData(bta_data1);
	bta_msg2.refreshIEData(bta_data2);

	ie_driver->sendMessage(&bta_msg1, true, true);
	if(!getIEAckMessage(&bta_msg1, device_ie_id))
		return false;

	ie_driver->sendMessage(&bta_msg2, true, true);
	return getIEAckMessage(&bta_msg2, device_ie_id);
}

//Set a BTA timer.
bool HondaIMIDHandler::setBTTimer(const long time) {
	if((this->imid_mode&0xFF) != ID_PHONE)
		return false; //this->setBTMode();

	uint8_t timer_data[] = {0x60, 0x23, 0x11, 0x0, 0x1, 0x0, 0xFF, 0xFF, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0, 0xFF, 0xF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	const int sec = getBCDFromByte(time%60), min = getBCDFromByte(time/60);

	timer_data[17] = uint8_t(sec&0xFF);
	if(min >= 0x10)
		timer_data[16] = uint8_t(min&0xFF);
	else
		timer_data[16] = uint8_t((min&0xFF) | 0xF0);

	IE_Message timer_msg(sizeof(timer_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	timer_msg.refreshIEData(timer_data);

	ie_driver->sendMessage(&timer_msg, true, true);
	const bool set = getIEAckMessage(&timer_msg, device_ie_id);
	
	if(set)
		imid_change_timer = 0;

	return set;
}

//Set a BTA text field.
bool HondaIMIDHandler::setBTText(const uint8_t field, String text) {
	if(this->imid_mode != ID_PHONE) {
		const bool set = this->setBTMode();
		if(!set) {
			imid_next_source = ID_PHONE;
			imid_change_timer = 0;
			return false;
		}
	}

	uint8_t text_data[] = {0x60, 0x23, 0x11, 0x0, 0x1, field, 0x6, 0x1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t text_data2[] = {0x60, 0x23, 0x11, 0x0, 0x1, field, 0x6, 0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	int limit = text.length();
	if(limit > 16)
		limit = 16;

	for(int i=0;i<limit;i+=1)
		text_data[i+8] = uint8_t(text.charAt(i));

	IE_Message text_msg(sizeof(text_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	text_msg.refreshIEData(text_data);

	ie_driver->sendMessage(&text_msg, true, true);
	if(!getIEAckMessage(&text_msg, device_ie_id))
		return false;

	if(text.length() > 16) {
		text = text.substring(16);

		limit = text.length();
		if(limit > 16)
			limit = 16;

		for(int i=0;i<limit;i+=1)
			text_data2[i+8] = uint8_t(text.charAt(i));
	}

	text_msg.refreshIEData(text_data2);

	ie_driver->sendMessage(&text_msg, true, true);
	return getIEAckMessage(&text_msg, device_ie_id);
}

bool HondaIMIDHandler::writeIMIDRadioMessage(const uint16_t frequency, const int8_t decimal, const uint8_t preset, const uint8_t stereo_mode, const bool acknowledge) {
	if((this->imid_mode&0xFF) != ID_RADIO)
		return true;

	this->frequency = frequency;
	this->decimal = decimal;
	this->preset = preset;
	this->stereo_mode = stereo_mode;

	uint8_t subsource_byte, stereo_byte, hd_byte;
	uint8_t frequency_bytes[] = {0xFF, 0xFF, 0xF0};
	const bool valid = getTuningMessage(frequency_bytes, &subsource_byte, &stereo_byte, &hd_byte);
	if(!valid)
		return true;

	if(this->display_rds)
		hd_byte = 0;

	const uint8_t rds_byte = rds ? 0x8 : 0x0, display_rds = this->display_rds? 0x8 : 0x0;

	uint8_t tuning_data[] = {0x60,
							0x7,
							0x11,
							0x0,
							0x1, 
							0x4,
							preset,
							subsource_byte,
							frequency_bytes[0],
							frequency_bytes[1],
							frequency_bytes[2],
							0x0,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							stereo_byte,
							rds_byte,
							display_rds,
							hd_byte,
							0x1,
							0x0,
							0x0,
							0x0,
							0x0};

	tuning_data[12] = rds ? 0x87 : 0x83;

	IE_Message tuning_msg(sizeof(tuning_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	tuning_msg.refreshIEData(tuning_data);
	ie_driver->sendMessageStrict(&tuning_msg, true, true);

	if(acknowledge) {
		bool set = getIEAckMessage(&tuning_msg, device_ie_id);
		return set;
	}
	return true;
}

bool HondaIMIDHandler::writeIMIDCallsignMessage(String msg) {
	uint8_t callsign_data[] = {0x60,
								0x7,
								0x11,
								0x0,
								0x1,
								0x11,
								0x0, //Length?
								0x0,
								0x0,
								0x0,
								0x0,
								0x0,
								0x0,
								0x0,
								0x0};
								
	int len = msg.length();
	if(len > 8)
		len = 8;
	
	callsign_data[6] = len&0xFF;
	
	for(int i=0;i<len;i+=1)
		callsign_data[i+7] = uint8_t(msg.charAt(i));
	
	IE_Message callsign_msg(sizeof(callsign_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	callsign_msg.refreshIEData(callsign_data);
	ie_driver->sendMessage(&callsign_msg, true, true);
	return getIEAckMessage(&callsign_msg, device_ie_id);
}

bool HondaIMIDHandler::writeIMIDRDSMessage(String msg) {
	if((this->imid_mode&0xFF) != ID_RADIO)
		return true;

	uint8_t subsource_byte, stereo_byte, hd_byte;
	uint8_t frequency_bytes[] = {0xFF, 0xFF, 0xF0};
	const bool valid = getTuningMessage(frequency_bytes, &subsource_byte, &stereo_byte, &hd_byte);

	if(!valid)
		return true;

	if(this->display_rds)
		hd_byte = 0;

	const uint8_t rds_byte = rds ? 0x8 : 0x0, display_rds = (this->display_rds && msg.length() > 0) ? 0x8 : 0x0;

	uint8_t tuning_data[] = {0x60,
							0x7,
							0x11,
							0x0,
							0x1, 
							0x4,
							preset,
							subsource_byte,
							frequency_bytes[0],
							frequency_bytes[1],
							frequency_bytes[2],
							0x0,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							stereo_byte,
							rds_byte,
							display_rds,
							hd_byte,
							0x1,
							0x0,
							0x0,
							0x0,
							0x0};

	tuning_data[13] = rds && msg.length() > 0 ? 0x87 : 0x83;
	
	if(msg.length() > 0) {
		for(uint8_t i=0;i<8;i+=1) {
			if(i < msg.length())
				tuning_data[i+14] = uint8_t(msg.charAt(i));
			else
				tuning_data[i+14] = 0x20;
		}
	}

	IE_Message tuning_msg(sizeof(tuning_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	tuning_msg.refreshIEData(tuning_data);
	ie_driver->sendMessage(&tuning_msg, true, true);
	return getIEAckMessage(&tuning_msg, device_ie_id);
}

bool HondaIMIDHandler::getTuningMessage(uint8_t* frequency_bytes, uint8_t* subsource_byte, uint8_t* stereo_byte, uint8_t* hd_byte) {
	unsigned int div = 1;

	unsigned int frequency_ms = 0, frequency_ls = 0;

	if(decimal >= 0) {
		for(int i=0;i<decimal;i+=1)
			div *= 10;

		frequency_ms = frequency/div;
		frequency_ls = frequency%div;
	} else {
		for(int i=0;i<-decimal;i+=1)
			div *= 10;

		frequency_ms = frequency*div;
		frequency_ls = 0;
	}

	const uint8_t subsource = (imid_mode&0xFF00) >> 8;
	if(subsource > 2)
		return false;

	frequency_bytes[0] = 0xFF;
	frequency_bytes[1] = 0xFF;
	frequency_bytes[2] = 0xF0;

	if(subsource == 0 || subsource == 1) { //FM.
		frequency_bytes[0] = getBCDFromByte(frequency_ms/10);
		if(frequency_ms/100 == 0)
			frequency_bytes[0] |= 0xF0;

		frequency_bytes[1] = getBCDFromByte(frequency_ms%10)<<4;
		if(div >= 10)
			frequency_bytes[1] |= getBCDFromByte((frequency_ls/(div/10))%10);
		
		frequency_bytes[2] = 0xF0;
		if(div >= 100) {
			frequency_bytes[2] &= 0xF;
			frequency_bytes[2] |= getBCDFromByte((frequency_ls/(div/100))%10) << 4;
		}
	} else {
		frequency_bytes[0] = getBCDFromByte(frequency_ms/100);
		if(frequency_ms/1000 == 0)
			frequency_bytes[0] |= 0xF0;
		
		frequency_bytes[1] = getBCDFromByte(frequency_ms%100);
		frequency_bytes[2] = 0xF0;
	}

	*subsource_byte = 0x1;
	switch(subsource) {
		case 1:
			*subsource_byte = 0x2;
			break;
		case 2:
			*subsource_byte = 0x11;
			break;
	}
	
	*stereo_byte = 0x0;
	if(stereo_mode&0x1)
		*stereo_byte = 0x10;
	
	*hd_byte = 0x0;
	if((stereo_mode&0x8) != 0)
		*hd_byte = 0x90;
	else if((stereo_mode&0x6) != 0)
		*hd_byte = 0xD0;

	return true;
}

bool HondaIMIDHandler::writeIMIDSiriusNumberMessage(const uint8_t preset, const uint16_t channel, const bool xm2) {
	//TODO: No signal/unavailable signal.
	if((imid_mode&0xFF) != ID_XM) {
		/*if(xm2 || preset > 6)
			setIMIDSource(ID_XM, 1);
		else
			setIMIDSource(ID_XM, 0);*/
		return true;
	}
	
	uint8_t number_data[] = {0x60, 0x19, 0x11, 0x0, 0x1, 0x0, preset, uint8_t(((imid_mode&0xFF00)>>8) + 1), 0xFF, 0xFF, 0x4, 0x1, 0x1};
	const int bcd_channel = getBCDFromByte(channel);

	if((bcd_channel & 0xF000) != 0)
		number_data[8] = (bcd_channel&0xFF00)>>8;
	else
		number_data[8] = ((bcd_channel&0xF00)>>8)|0xF0;
	
	number_data[9] = bcd_channel&0xFF;

	IE_Message number_msg(sizeof(number_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	number_msg.refreshIEData(number_data);
	ie_driver->sendMessage(&number_msg, true, true);

	imid_change_timer = 0;
	return getIEAckMessage(&number_msg, device_ie_id);
}

bool HondaIMIDHandler::writeIMIDSiriusTextMessage(const uint8_t position, String text) {
	if((imid_mode&0xFF) != ID_XM)
		return true;
	
	if((position&0xF) <= 2) {
		uint8_t text_data[] = {0x60, 0x19, 0x11, 0x0, 0x1, uint8_t(0x50|(position&0xF)), 0x0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0, 0x0};
		unsigned int len = text.length();
		if(len > 16)
			len = 16;
		for(int i=0;i<len;i+=1)
			text_data[i+7] = uint8_t(text.charAt(i));

		IE_Message text_msg(sizeof(text_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
		text_msg.refreshIEData(text_data);
		ie_driver->sendMessage(&text_msg, true, true);

		imid_change_timer = 0;
		return getIEAckMessage(&text_msg, device_ie_id);
	} else {
		uint8_t text_data[] = {0x60, 0x19, 0x11, 0x0, 0x1, uint8_t(0x50|(position&0xF)), 0x0, 0x1, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
		unsigned int len = text.length();
		if(len > 16)
			len = 16;
		for(int i=0;i<len;i+=1)
			text_data[i+8] = uint8_t(text.charAt(i));

		IE_Message text_msg(sizeof(text_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
		text_msg.refreshIEData(text_data);
		ie_driver->sendMessage(&text_msg, true, true);

		imid_change_timer = 0;
		return getIEAckMessage(&text_msg, device_ie_id);
	}
}

bool HondaIMIDHandler::writeIMIDCDCTrackMessage(const uint8_t disc, const uint8_t track, const uint8_t track_count, const uint16_t time, const uint8_t state1, const uint8_t state2) {
	if(imid_mode != ID_CD && imid_mode != ID_CDC) {
		//setIMIDSource(ID_CDC, 0);
		return true;
	}

	bool change_disc = false, change_track = false;
	if(disc != this->disc)
		change_disc = true;

	if(track != this->track)
		change_track = true;

	this->disc = disc;
	this->track = track;
	
	ai_cd_mode = getAICDStatus(state1, state2);

	uint8_t rpt_state = state2;
	if(imid_mode == ID_CD && state2 == 0x20)
		rpt_state = 0x10;

	uint8_t text_state = 0x20;
	if(cd_text_mode == TEXT_MODE_WITH_TEXT)
		text_state = 0x22;
	else if(cd_text_mode == TEXT_MODE_MP3)
		text_state = 0x60;
	
	uint8_t cd_data[] = {0x60,
						uint8_t(imid_mode&0xFF),
						0x11,
						0x0,
						state1,
						0x0,
						uint8_t(getBCDFromByte(disc)),
						uint8_t(getBCDFromByte(time/60)),
						uint8_t(getBCDFromByte(time%60)),
						0xF,
						uint8_t(getBCDFromByte(track)),
						0x0,
						rpt_state,
						text_state,
						0xF,
						0xFF,
						0xF,
						0xFF,
						0xF,
						0xFF,//track_count
						0xFF,
						0xFF,
						0xF,
						uint8_t(getBCDFromByte(track))};
	
	IE_Message cd_message(sizeof(cd_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	cd_message.refreshIEData(cd_data);

	ie_driver->sendMessage(&cd_message, true, true);
	bool set = getIEAckMessage(&cd_message, device_ie_id);
	if(!set) {
		imid_change_timer = 0;
		return false;
	}

	if(change_disc)
		cd_text_mode = TEXT_MODE_BLANK;

	if((change_disc || change_track) && state1 < 0x10 && cd_text_mode != TEXT_MODE_BLANK) {
		set = writeIMIDCDCTextMessage(1, " ");
		if(!set) {
			imid_change_timer = 0;
			return false;
		}

		set = writeIMIDCDCTextMessage(2, " ");
		if(!set) {
			imid_change_timer = 0;
			return false;
		}

		if(change_disc || cd_text_mode == TEXT_MODE_MP3) {
			set = writeIMIDCDCTextMessage(0, " ");
			if(!set) {
				imid_change_timer = 0;
				return false;
			}
		}
	}

	return true;
}

bool HondaIMIDHandler::writeIMIDCDCTextMessage(const uint8_t position, String text) {
	if(!parameter_list->imid_connected)
		return true;

	if(imid_mode != ID_CD && imid_mode != ID_CDC) {
		//setIMIDSource(ID_CD, 0);
		return true;
	}

	if(cd_text_mode == TEXT_MODE_BLANK)
		cd_text_mode = TEXT_MODE_WITH_TEXT;

	//TODO: Why are we doing this?
	//writeIMIDCDCTrackMessage(disc, track, 0xFF, timer, getIECDStatus(ai_cd_mode), getIECDRepeat(ai_cd_mode));

	uint8_t text_data1[] = {0x60,
							uint8_t(imid_mode&0xFF),
							0x11,
							0x0,
							0x3,
							uint8_t(0x80|(position&0xF)),
							0x4,
							0x1,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF};

	uint8_t text_data2[] = {0x60,
							uint8_t(imid_mode&0xFF),
							0x11,
							0x0,
							0x3,
							uint8_t(0x80|(position&0xF)),
							0x4,
							0x11,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF,
							0xFF};

	int len = text.length();
	if(len > 16)
		len = 16;

	for(int i=0;i<len;i+=1)
		text_data1[i+8] = uint8_t(text.charAt(i));

	len = text.length() - 16;
	if(len > 16)
		len = 16;

	for(int i=0;i<len;i+=1)
		text_data2[i+8] = uint8_t(text.charAt(i+16));

	IE_Message text_msg1(sizeof(text_data1), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	text_msg1.refreshIEData(text_data1);

	IE_Message text_msg2(sizeof(text_data2), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	text_msg2.refreshIEData(text_data2);

	ie_driver->sendMessage(&text_msg1, true, true);
	if(!getIEAckMessage(&text_msg1, device_ie_id)) {
		imid_change_timer = 0;
		return false;
	}

	delay(10);
	
	imid_change_timer = 0;
	ie_driver->sendMessage(&text_msg2, true, true);
	return getIEAckMessage(&text_msg2, device_ie_id);
}

//Clear the CD text.
bool HondaIMIDHandler::clearIMIDCDText() {
	bool set = true;

	set &= writeIMIDCDCTextMessage(0, "");
	set &= writeIMIDCDCTextMessage(1, "");
	set &= writeIMIDCDCTextMessage(2, "");

	if(!set)
		imid_change_timer = 0;

	return set;
}

//Get the current IMID mode.
uint16_t HondaIMIDHandler::getMode() {
	return this->imid_mode;
}

//Send a text request to the source.
void HondaIMIDHandler::sendSourceRequest(const uint8_t source) {
	uint8_t request_data[] = {0x60, 0x10};
	AIData request_msg(sizeof(request_data), ID_IMID_SCR, source);
	request_msg.refreshAIData(request_data);

	ai_driver->writeAIData(&request_msg);
}

//Create the IMID settings menu.
void HondaIMIDHandler::createIMIDSettingsMenu() {
	const MenuList settings_menu = getMenu(MENU_INDEX_IMID_SETTINGS, parameter_list->locale);

	startSettingsMenu(settings_menu.size(), settings_menu.size(), false, settings_menu.title);

	elapsedMillis cancel_wait;
	while(cancel_wait < 20) {
		AIData ai_msg;
		if(ai_driver->dataAvailable(false) > 0) {
			if(ai_driver->readAIData(&ai_msg, false)) {
				if(ai_msg.l >= 2 && ai_msg.sender == ID_NAV_COMPUTER && ai_msg.data[0] == 0x2B && ai_msg.data[1] == 0x40) { //No menu available.
					return;
				}
			}
		}
	}

	*active_menu = MENU_IMID;

	for(int i=0;i<settings_menu.size();i+=1)
		createIMIDSettingsMenuOption(i);

	displayMenu(1);
}

//Add an option to the settings menu.
void HondaIMIDHandler::createIMIDSettingsMenuOption(const unsigned int index) {
	const MenuList settings_menu = getMenu(MENU_INDEX_IMID_SETTINGS, parameter_list->locale);
	String option = "";

	switch(settings_menu.getGlobalIndex(index)) {
	case MENU_INDEX_IMID_SETTINGS_RDS:
		option = display_rds ? "#RON " : "#ROF ";
		break;
	case MENU_INDEX_IMID_SETTINGS_CD_TEXT:
		if(parameter_list->imid_cd_text == NULL || parameter_list->imid_cd_text == nullptr)
			return;

		option = *parameter_list->imid_cd_text ? "#RON " : "#ROF ";
		break;
	case MENU_INDEX_IMID_SETTINGS_VOLUME:
		option = display_volume ? "#RON " : "#ROF ";
		break;
	default:
		break;
	}

	option += settings_menu[index];
	appendMenu(index, option);
}

//Create the IMID character count setting menu.
void HondaIMIDHandler::createIMIDCharacterMenu() {
	const MenuList character_menu = getMenu(MENU_INDEX_IMID_CHARACTER, parameter_list->locale);

	startSettingsMenu(character_menu.size(), character_menu.size(), false, character_menu.title);

	elapsedMillis cancel_wait;
	while(cancel_wait < 20) {
		AIData ai_msg;
		if(ai_driver->dataAvailable(false) > 0) {
			if(ai_driver->readAIData(&ai_msg, false)) {
				if(ai_msg.l >= 2 && ai_msg.sender == ID_NAV_COMPUTER && ai_msg.data[0] == 0x2B && ai_msg.data[1] == 0x40) { //No menu available.
					return;
				}
			}
		}
	}

	*active_menu = MENU_IMID_CHARACTER;

	for(int i=0;i<character_menu.size();i+=1)
		createIMIDCharacterMenuOption(i);

	displayMenu(1);
}

//Add an option to the character menu.
void HondaIMIDHandler::createIMIDCharacterMenuOption(const unsigned int index) {
	const MenuList character_menu = getMenu(MENU_INDEX_IMID_CHARACTER, parameter_list->locale);
	String option = this->char_count == index ? "#CON " : "#COF ";

	option += character_menu[index];
	appendMenu(index, option);
}