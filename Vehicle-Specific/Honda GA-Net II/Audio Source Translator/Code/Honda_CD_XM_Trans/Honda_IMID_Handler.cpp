#include "Honda_IMID_Handler.h"

HondaIMIDHandler::HondaIMIDHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list) : HondaSourceHandler(ie_driver, ai_driver, parameter_list) {
	this->device_ie_id = IE_ID_IMID;
	this->device_ai_id = ID_IMID_SCR;
}

void HondaIMIDHandler::loop() {

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
}

void HondaIMIDHandler::readAIBusMessage(AIData* the_message) {
	if(the_message->receiver != ID_IMID_SCR)
		return;

	if(the_message->l >= 1 && the_message->data[0] == 0x80) //Acknowledgement.
		return;

	if(!parameter_list->power_on)
		return;

	bool ack = true;

	if(the_message->l >= 3 && the_message->data[0] == 0x4 && the_message->data[1] == 0xE6 && the_message->data[2] == 0x3B) {
		ack = false;
		ai_driver->sendAcknowledgement(ID_IMID_SCR, the_message->sender);
		
		uint8_t screen_field_data[] = {0x3B, 0x23, max_char, LINES};
		AIData screen_field_msg(sizeof(screen_field_data), ID_IMID_SCR, the_message->sender);
		screen_field_msg.refreshAIData(screen_field_data);

		uint8_t screen_oem_data[] = {0x3B, 0x57, ID_RADIO, ID_CD, ID_CDC, ID_XM, ID_ANDROID_AUTO};
		AIData screen_oem_msg(sizeof(screen_oem_data), ID_IMID_SCR, the_message->sender);
		screen_oem_msg.refreshAIData(screen_oem_data);

		ai_driver->writeAIData(&screen_field_msg);
		ai_driver->writeAIData(&screen_oem_msg);
	} else if(the_message->data[0] == 0x23 && the_message->data[1] == 0x60) { //Write a custom message.
		if(the_message->data[3] > 1)
			return;
		
		const uint8_t space = the_message->data[2];
	
		String new_text = "";

		for(uint8_t i=0;i<space;i+=1)
			new_text += " ";

		for(uint8_t i=4;i<the_message->l;i+=1)
			new_text += char(the_message->data[i]);

		writeIMIDTextMessage(new_text);
	} else if(the_message->data[0] == 0x40 && the_message->data[1] == 0x10 && the_message->sender == ID_RADIO) {
		ack = false;
		ai_driver->sendAcknowledgement(ID_IMID_SCR, the_message->sender);
		
		const uint8_t source = the_message->data[2];
		if(the_message->l >= 4) {
			const uint8_t subsource = the_message->data[3];
			setIMIDSource(source, subsource);
		} else
			setIMIDSource(source, 0);

		/*uint8_t data_request_data[] = {0x60, 0x10};
		AIData data_request(sizeof(data_request_data), ID_IMID_SCR, ID_RADIO);
		data_request.refreshAIData(data_request_data);

		ai_driver->writeAIData(&data_request, parameter_list->radio_connected);*/
	} else if((the_message->data[0] == 0x39 || the_message->data[0] == 0x3B) && the_message->sender == ID_CDC) {
		if(the_message->data[0] == 0x39 && the_message->l >= 8) { //Track and disc.
			if(the_message->data[6] != disc)
				setIMIDSource(ID_CDC, 0);
			
			ai_cd_mode = the_message->data[2];
			
			const uint8_t last_track = track, last_disc = disc;
			disc = the_message->data[6];
			track = the_message->data[7];
			
			const uint8_t cd_state = the_message->data[2];

			writeIMIDCDCTrackMessage(disc, track, track_count, timer, getIECDStatus(ai_cd_mode), getIECDRepeat(ai_cd_mode));
			if(last_disc != disc || last_track != track) {
				writeIMIDCDCTextMessage(1, "");
				writeIMIDCDCTextMessage(2, "");
				if(last_disc != disc)
					writeIMIDCDCTextMessage(0, "");
			}
		} else if(the_message->data[0] == 0x3B && the_message->l >= 0x5) {
			timer = (the_message->data[3]<<8)|the_message->data[4];

			writeIMIDCDCTrackMessage(disc, track, track_count, timer, getIECDStatus(ai_cd_mode), getIECDRepeat(ai_cd_mode));
		}

		ack = false;
		ai_driver->sendAcknowledgement(ID_IMID_SCR, the_message->sender);
	} else if((the_message->sender == ID_CDC || the_message->sender == ID_CD) && the_message->data[0] == 0x23) { //CD text message.
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
		writeIMIDCDCTextMessage(field, text);
		ack = false;
		ai_driver->sendAcknowledgement(ID_IMID_SCR, the_message->sender);
	} else if(the_message->sender == ID_RADIO && the_message->l >= 6 && the_message->data[0] == 0x67) { //Frequency change message.		
		const uint16_t frequency = (the_message->data[1]<<8) | the_message->data[2];
		writeIMIDRadioMessage(frequency, the_message->data[3], the_message->data[4]&0xF, (the_message->data[4]&0xF0)>>4);
	} else if(the_message->sender == ID_RADIO && the_message->data[0] == 0x63 && the_message->l >= 3) { //RDS.
		ack = false;
		ai_driver->sendAcknowledgement(ID_IMID_SCR, the_message->sender);
		
		String rds_str = "";
		for(int i=2;i<the_message->l;i+=1)
			rds_str += char(the_message->data[i]);
		
		if(the_message->data[1] == 0x61) // True RDS.
			writeIMIDRDSMessage(rds_str);
		else if(the_message->data[1] == 0x60) //Call sign.
			writeIMIDCallsignMessage(rds_str);
	} else if(the_message->sender == ID_RADIO && the_message->l >= 3 && the_message->data[0] == 0x62) { //Volume control display.
		const int max_vol = 40;
		const uint8_t new_vol = the_message->data[1]*max_vol/the_message->data[2];

		writeIMIDVolumeMessage(new_vol);
	} else if(the_message->sender == ID_XM && the_message->data[0] == 0x39) { //XM channel change message.
		const uint16_t channel = (the_message->data[2]<<8) | the_message->data[3];
		const uint8_t preset = the_message->data[4]&0x3F;

		if((the_message->data[4]&0xC0) == 0)
			writeIMIDSiriusNumberMessage(preset, channel, this->xm2);
		//TODO: If signal is unavailable, send the appropriate message.

		ack = false;
		ai_driver->sendAcknowledgement(ID_IMID_SCR, the_message->sender);

	} else if(the_message->sender == ID_XM && the_message->data[0] == 0x23) { //Sirius text message.
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

		writeIMIDSiriusTextMessage(field, text);
		ack = false;
		ai_driver->sendAcknowledgement(ID_IMID_SCR, the_message->sender);
	} else if(the_message->sender == ID_ANDROID_AUTO && the_message-> l >= 2 && the_message->data[0] == 0x23 && (the_message->data[1]&0xF0) == 0x60) { //Mirror text message.
		ack = false;
		ai_driver->sendAcknowledgement(ID_IMID_SCR, the_message->sender);
		
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

		this->setBTText(leader, sent_text);
	}

	if(ack)
		ai_driver->sendAcknowledgement(ID_IMID_SCR, the_message->sender);
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

	uint8_t screen_oem_data[] = {0x3B, 0x57, ID_RADIO, ID_CD, ID_CDC, ID_XM, ID_ANDROID_AUTO};
	AIData screen_oem_msg(sizeof(screen_oem_data), ID_IMID_SCR, 0xFF);
	screen_oem_msg.refreshAIData(screen_oem_data);

	ai_driver->writeAIData(&screen_field_msg, false);

	elapsedMillis wait_timer;
	while(wait_timer < 50) {
		ie_driver->cacheAIBus();
	}

	ai_driver->writeAIData(&screen_oem_msg, false);
}

void HondaIMIDHandler::writeTimeAndDayMessage(const uint8_t hour, const uint8_t minute, const uint8_t month, const uint8_t day, const uint16_t year) {
	//TODO: Day of week.
	uint8_t time_data[] = {0x60, 0xD, 0x11, 0x0, 0x1, 0x40, getBCDFromByte(hour), getBCDFromByte(minute), 0x0, 0x1, (getBCDFromByte(year)>>8)&0xFF, getBCDFromByte(year)&0xFF, month, day, 0xF};
	IE_Message time_msg(sizeof(time_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	time_msg.refreshIEData(time_data);
	ie_driver->sendMessage(&time_msg, true, true);
	getIEAckMessage(device_ie_id);
}

//Write the volume to the screen.
void HondaIMIDHandler::writeIMIDVolumeMessage(const uint8_t volume) {
	uint8_t volume_data[] = {0x60, 0x2, 0x11, 0x0, 0x2, 0x0, volume, 0x10, 0x0, 0x30, 0x0, 0x0, 0x9, 0x0, 0x0, 0x0};

	IE_Message volume_msg(sizeof(volume_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	volume_msg.refreshIEData(volume_data);

	ie_driver->sendMessage(&volume_msg, true, true);
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

	if(len > max_char)
		len = max_char;

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

	elapsedMillis timeout_del = 0;

	if(imid_mode != 0x23) {
		imid_mode = 0x23;
		ie_driver->sendMessage(&bta_trigger_msg, true, true);
		
		if(!getIEAckMessage(device_ie_id))
			return false;
		
		ie_driver->sendMessage(&bta_msg1, true, true);
		
		timeout_del = 0;
		if(!getIEAckMessage(device_ie_id))
			return false;
		
		ie_driver->sendMessage(&bta_msg2, true, true);
		
		timeout_del = 0;
		if(!getIEAckMessage(device_ie_id))
			return false;
		
		ie_driver->sendMessage(&bta_msg3, true, true);
		
		timeout_del = 0;
		if(!getIEAckMessage(device_ie_id))
			return false;
	}
	
	ie_driver->sendMessage(&bta_text1, true, true);
	
	timeout_del = 0;
	if(!getIEAckMessage(device_ie_id))
		return false;
	
	return true;
	
}

bool HondaIMIDHandler::setIMIDSource(const uint8_t source, const uint8_t subsource) {
	bool new_source = true;
	if(parameter_list->imid_connected && source != 0 && this->imid_mode == (source | (subsource<<8)))
		new_source = false;
	
	this->imid_mode = source | (subsource << 8);

	if(source == 0) {
		uint8_t source_data[] = {0x0, 0x1};
		sendFunctionMessage(ie_driver, new_source, IE_ID_IMID, source_data, sizeof(source_data));
		getIEAckMessage(device_ie_id);
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
		return getIEAckMessage(device_ie_id);
	} else if(source == ID_CD || source == ID_CDC) {
		uint8_t source_data[] = {source, 0x0};
		sendFunctionMessage(ie_driver, new_source, IE_ID_IMID, source_data, sizeof(source_data));
		getIEAckMessage(device_ie_id);
		
		if(new_source) {
			if(source == ID_CDC) {
				uint8_t cd_data_1[] = {0x60, 0x6, 0x11, 0x0, 0x10, 0x10, 0x6, 0x3A};
				uint8_t cd_data_2[] = {0x60, 0x6, 0x11, 0x0, 0x15, 0x1, 0x80, 0x0, 0xA0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
				
				IE_Message cd_msg_1(sizeof(cd_data_1), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
				cd_msg_1.refreshIEData(cd_data_1);
				
				IE_Message cd_msg_2(sizeof(cd_data_2), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
				cd_msg_2.refreshIEData(cd_data_2);

				bool ack = false;
				elapsedMillis wait_timer;
				
				while(!ack && wait_timer < 5000) {
					ie_driver->sendMessage(&cd_msg_1, true, true);
					ack = getIEAckMessage(device_ie_id);
				}
			
				if(!ack)
					return false;

				ack = false;
				wait_timer = 0;
				
				while(!ack && wait_timer < 5000) {
					ie_driver->sendMessage(&cd_msg_2, true, true);
					ack = getIEAckMessage(device_ie_id);
				}
			
				if(!ack)
					return false;
			} else {
				uint8_t cd_data_1[] = {0x60, 0x4, 0x11, 0x0, 0x15, 0x10, 0x1, 0x2A, 0x80, 0xE0, 0x1};
				uint8_t cd_data_2[] = {0x60, 0x4, 0x11, 0x0, 0x15, 0x1, 0x80, 0x0, 0xA0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
				
				IE_Message cd_msg_1(sizeof(cd_data_1), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
				cd_msg_1.refreshIEData(cd_data_1);
				
				IE_Message cd_msg_2(sizeof(cd_data_2), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
				cd_msg_2.refreshIEData(cd_data_2);
				
				ie_driver->sendMessage(&cd_msg_1, true, true);
			
				elapsedMillis timeout_del = 0;
				if(!getIEAckMessage(device_ie_id))
					return false;
				
				ie_driver->sendMessage(&cd_msg_2, true, true);
			
				timeout_del = 0;
				if(!getIEAckMessage(device_ie_id))
					return false;
			}
		}
		
		return true;
	} else if(source == ID_XM) {
		uint8_t source_data[] = {0x19, 0x0, subsource + 1};
		sendFunctionMessage(ie_driver, new_source, IE_ID_IMID, source_data, sizeof(source_data));
		getIEAckMessage(device_ie_id);
		return true;
	} else if(source == ID_ANDROID_AUTO) {
		uint8_t source_data[] = {0x23, 0x0, 0x0};
		sendFunctionMessage(ie_driver, new_source, IE_ID_IMID, source_data, sizeof(source_data));
		getIEAckMessage(device_ie_id);

		if(new_source)
			setBTMode();

		return true;
	} else
		return false;
}

//Set the screen to show "iPod."
void HondaIMIDHandler::setIPodMode() {
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
	if(!getIEAckMessage(device_ie_id))
		return;

	ie_driver->sendMessage(&ipod_msg_1, true, true);
	if(!getIEAckMessage(device_ie_id))
		return;

	ie_driver->sendMessage(&ipod_msg_2, true, true);
	if(!getIEAckMessage(device_ie_id))
		return;

	ie_driver->sendMessage(&ipod_msg_3, true, true);
	if(!getIEAckMessage(device_ie_id))
		return;

	ie_driver->sendMessage(&ipod_msg_4, true, true);
	if(!getIEAckMessage(device_ie_id))
		return;

	ie_driver->sendMessage(&ipod_msg_5, true, true);
	if(!getIEAckMessage(device_ie_id))
		return;
		
	clearIPodText(0xBD);
	clearIPodText(0xB8);
	clearIPodText(0xBA);
	clearIPodText(0xB9);
	clearIPodText(0xBF);
		
	setIPodText(0xBD, "");
	setIPodText(0xB8, "");
	setIPodText(0xBA, "");
	setIPodText(0xB9, "");
	setIPodText(0xBF, "");
}

//Clear the iPod text fields.
void HondaIMIDHandler::clearIPodText(const uint8_t field) {
	uint8_t text_data[] = {0x60, 0x22, 0x11, 0x0, 0x3, field, 0x2, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};


	IE_Message text_msg(sizeof(text_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	text_msg.refreshIEData(text_data);

	ie_driver->sendMessage(&text_msg, true, true);
	
	text_data[7] = 0x12;
	text_msg.refreshIEData(text_data);
	ie_driver->sendMessage(&text_msg, true, true);
}

//Set an iPod/USB(?) text field.
void HondaIMIDHandler::setIPodText(const uint8_t field, String text) {
	uint8_t text_data[] = {0x60, 0x22, 0x11, 0x0, 0x3, field, 0x2, 0x2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	
	int limit = text.length();
	if(limit > 16)
		limit = 16;

	for(int i=0;i<limit;i+=1)
		text_data[i+8] = uint8_t(text.charAt(i));

	IE_Message text_msg(sizeof(text_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	text_msg.refreshIEData(text_data);

	ie_driver->sendMessage(&text_msg, true, true);
}

//Set the screen to show full BTA.
void HondaIMIDHandler::setBTMode() {
	this->imid_mode = ID_ANDROID_AUTO;

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
	if(!getIEAckMessage(device_ie_id))
		return;

	ie_driver->sendMessage(&bta_msg2, true, true);
	if(!getIEAckMessage(device_ie_id))
		return;

	ie_driver->sendMessage(&bta_msg3, true, true);
	if(!getIEAckMessage(device_ie_id))
		return;

	ie_driver->sendMessage(&bta_msg4, true, true);
	if(!getIEAckMessage(device_ie_id))
		return;

	ie_driver->sendMessage(&bta_msg5, true, true);
	if(!getIEAckMessage(device_ie_id))
		return;

	ie_driver->sendMessage(&bta_msg6, true, true);
}

//Set a BTA text field.
void HondaIMIDHandler::setBTText(const uint8_t field, String text) {
	if((this->imid_mode&0xFF) != ID_ANDROID_AUTO)
		this->setBTMode();

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
	if(!getIEAckMessage(device_ie_id))
		return;

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
	if(!getIEAckMessage(device_ie_id))
	return;
}

void HondaIMIDHandler::writeIMIDRadioMessage(const uint16_t frequency, const int8_t decimal, const uint8_t preset, const uint8_t stereo_mode) {
	if((this->imid_mode&0xFF) != ID_RADIO)
		return;

	this->frequency = frequency;
	this->decimal = decimal;
	this->preset = preset;
	this->stereo_mode = stereo_mode;

	uint8_t subsource_byte, stereo_byte, hd_byte;
	uint8_t frequency_bytes[] = {0xFF, 0xFF, 0xF0};
	const bool valid = getTuningMessage(frequency_bytes, &subsource_byte, &stereo_byte, &hd_byte);
	if(!valid)
		return;

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
							0x0,
							0x0,
							hd_byte,
							0x1,
							0x0,
							0x0,
							0x0,
							0x0};

	IE_Message tuning_msg(sizeof(tuning_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	tuning_msg.refreshIEData(tuning_data);
	ie_driver->sendMessage(&tuning_msg, true, true);
}

void HondaIMIDHandler::writeIMIDCallsignMessage(String msg) {
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
}

void HondaIMIDHandler::writeIMIDRDSMessage(String msg) {
	if((this->imid_mode&0xFF) != ID_RADIO)
		return;

	uint8_t subsource_byte, stereo_byte, hd_byte;
	uint8_t frequency_bytes[] = {0xFF, 0xFF, 0xF0};
	const bool valid = getTuningMessage(frequency_bytes, &subsource_byte, &stereo_byte, &hd_byte);
	if(!valid)
		return;

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
							0x0,
							0x0,
							hd_byte,
							0x1,
							0x0,
							0x0,
							0x0,
							0x0};

	tuning_data[12] = 0x83;
	for(uint8_t i=0;i<8;i+=1) {
		if(i < msg.length())
			tuning_data[i+13] = uint8_t(msg.charAt(i));
		else
			tuning_data[i+13] = 0x20;
	}

	IE_Message tuning_msg(sizeof(tuning_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	tuning_msg.refreshIEData(tuning_data);
	ie_driver->sendMessage(&tuning_msg, true, true);
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
			frequency_bytes[2] |= getBCDFromByte((frequency_ls%(div/100))%10) << 4;
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

void HondaIMIDHandler::writeIMIDSiriusNumberMessage(const uint8_t preset, const uint16_t channel, const bool xm2) {
	//TODO: No signal/unavailable signal.
	if((imid_mode&0xFF) != ID_XM) {
		/*if(xm2 || preset > 6)
			setIMIDSource(ID_XM, 1);
		else
			setIMIDSource(ID_XM, 0);*/
		return;
	}
	
	uint8_t number_data[] = {0x60, 0x19, 0x11, 0x0, 0x1, 0x0, preset, ((imid_mode&0xFF00)>>8) + 1, 0xFF, 0xFF, 0x4, 0x1, 0x1};
	const int bcd_channel = getBCDFromByte(channel);

	if((bcd_channel && 0xF000) != 0)
		number_data[8] = (bcd_channel&0xFF00)>>8;
	else
		number_data[8] = ((bcd_channel&0xF00)>>8)|0xF0;
	
	number_data[9] = bcd_channel&0xFF;

	IE_Message number_msg(sizeof(number_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	number_msg.refreshIEData(number_data);
	ie_driver->sendMessage(&number_msg, true, true);
	getIEAckMessage(device_ie_id);
}

void HondaIMIDHandler::writeIMIDSiriusTextMessage(const uint8_t position, String text) {
	if((imid_mode&0xFF) != ID_XM)
		return;
	
	if((position&0xF) <= 2) {
		uint8_t text_data[] = {0x60, 0x19, 0x11, 0x0, 0x1, 0x50|(position&0xF), 0x0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0, 0x0};
		unsigned int len = text.length();
		if(len > 16)
			len = 16;
		for(int i=0;i<len;i+=1)
			text_data[i+7] = uint8_t(text.charAt(i));

		IE_Message text_msg(sizeof(text_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
		text_msg.refreshIEData(text_data);
		ie_driver->sendMessage(&text_msg, true, true);
		getIEAckMessage(device_ie_id);
	} else {
		uint8_t text_data[] = {0x60, 0x19, 0x11, 0x0, 0x1, 0x50|(position&0xF), 0x0, 0x1, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
		unsigned int len = text.length();
		if(len > 16)
			len = 16;
		for(int i=0;i<len;i+=1)
			text_data[i+8] = uint8_t(text.charAt(i));

		IE_Message text_msg(sizeof(text_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
		text_msg.refreshIEData(text_data);
		ie_driver->sendMessage(&text_msg, true, true);
		getIEAckMessage(device_ie_id);
	}
}

void HondaIMIDHandler::writeIMIDCDCTrackMessage(const uint8_t disc, const uint8_t track, const uint8_t track_count, const uint16_t time, const uint8_t state1, const uint8_t state2) {
	if(imid_mode != ID_CD && imid_mode != ID_CDC) {
		//setIMIDSource(ID_CDC, 0);
		return;
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
						imid_mode,
						0x11,
						0x0,
						state1,
						0x0,
						getBCDFromByte(disc),
						getBCDFromByte(time/60),
						getBCDFromByte(time%60),
						0xF,
						getBCDFromByte(track),
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
						getBCDFromByte(track)};
	
	IE_Message cd_message(sizeof(cd_data), IE_ID_RADIO, IE_ID_IMID, 0xF, true);
	cd_message.refreshIEData(cd_data);

	bool ack = false;
	elapsedMillis wait_timer;

	while(!ack && wait_timer < 5000) {
		ie_driver->sendMessage(&cd_message, true, true);
		ack = this->getIEAckMessage(IE_ID_RADIO);
	}

	if(change_disc)
		cd_text_mode = TEXT_MODE_BLANK;

	if((change_disc || change_track) && state1 < 0x10 && cd_text_mode != TEXT_MODE_BLANK) {
		writeIMIDCDCTextMessage(1, " ");
		writeIMIDCDCTextMessage(2, " ");
		if(change_disc)
			writeIMIDCDCTextMessage(0, " ");
	}
}

void HondaIMIDHandler::writeIMIDCDCTextMessage(const uint8_t position, String text) {
	if(!parameter_list->imid_connected)
		return;

	if(imid_mode != ID_CD && imid_mode != ID_CDC) {
		//setIMIDSource(ID_CD, 0);
		return;
	}

	if(cd_text_mode == TEXT_MODE_BLANK)
		cd_text_mode = TEXT_MODE_WITH_TEXT;

	writeIMIDCDCTrackMessage(disc, track, 0xFF, timer, getIECDStatus(ai_cd_mode), getIECDRepeat(ai_cd_mode));

	uint8_t text_data1[] = {0x60,
							imid_mode,
							0x11,
							0x0,
							0x3,
							0x80|(position&0xF),
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
							imid_mode,
							0x11,
							0x0,
							0x3,
							0x80|(position&0xF),
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

	bool ack = false;
	elapsedMillis wait_timer;

	while(!ack && wait_timer < 5000) {
		ie_driver->sendMessage(&text_msg1, true, true);
		ack = getIEAckMessage(IE_ID_RADIO);
	}

	ack = false;
	wait_timer = 0;
	while(!ack && wait_timer < 5000) {
		ie_driver->sendMessage(&text_msg2, true, true);
		ack = getIEAckMessage(IE_ID_RADIO);
	}
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
