#include "Text_Handler.h"

TextHandler::TextHandler(AIBusHandler* ai_handler, ParameterList* parameter_list) {
	this->ai_handler = ai_handler;
	this->parameter_list = parameter_list;
}

//Clear all text in the audio window.
void TextHandler::clearAllText() {
	clearAllText(false);
}

//Clear all text in the audio window. If refresh is true, send the signal to refresh the screen.
void TextHandler::clearAllText(const bool refresh) {
	uint8_t data[] = {0x20, 0x6F};
	if(refresh)
		data[1] |= 0x10;

	AIData clear_msg(sizeof(data), ID_RADIO, ID_NAV_COMPUTER);
	clear_msg.refreshAIData(data);

	ai_handler->writeAIData(&clear_msg, parameter_list->computer_connected);
}

//Set a preliminary header.
void TextHandler::setBlankHeader(String header) { 
	AIData header_change(header.length() + 3, ID_RADIO, ID_NAV_COMPUTER);

	header_change.data[0] = 0x23;
	header_change.data[1] = 0x70;
	header_change.data[2] = 0x0;

	for(uint8_t i=0;i<header.length();i+=1)
		header_change.data[i+3] = uint8_t(header.charAt(i));

	ai_handler->writeAIData(&header_change, parameter_list->computer_connected);
}

//Send a text control message to recipient, indicating source is in control.
void TextHandler::sendSourceTextControl(const uint8_t recipient, const uint8_t source) {
	uint8_t data[] = {0x40, 0x01, source};
	AIData text_control_msg(sizeof(data), ID_RADIO, recipient);
	text_control_msg.refreshAIData(data);

	ai_handler->writeAIData(&text_control_msg);
}

//Write a radio frequency to the screen.
void TextHandler::sendTunedFrequencyMessage(const uint16_t frequency, const bool mhz, const bool sub) {
	sendTunedFrequencyMessage(0, frequency, mhz, sub);
}

//Write a radio frequency to the screen.
void TextHandler::sendTunedFrequencyMessage(const uint8_t preset, const uint16_t frequency, const bool mhz, const bool sub) {
	last_frequency = frequency;
	String frequency_text = "";
	if(mhz) {
		if(frequency%100 >= 10)
			frequency_text = String(frequency/100) + "." + String(frequency%100) + " MHz";
		else
			frequency_text = String(frequency/100) + ".0" + String(frequency%100) + " MHz";
	} else
		frequency_text = String(frequency) + " kHz";
		
	uint8_t group = 0, area = 1;
	if(sub) {
		group = 1;
		area = 2;
	}
		
	AIData frequency_message = getTextMessage(frequency_text, group, area);
	frequency_message.data[1] |= 0x10;
	ai_handler->writeAIData(&frequency_message, parameter_list->computer_connected);

	this->sendIMIDFrequencyMessage(frequency, parameter_list->last_sub, preset);
}

//Write an FM Stereo indicator to the screen.
void TextHandler::sendStereoMessage(const bool stereo) {
	if(stereo) {
		AIData stereo_message = getTextMessage("St", 0x1, 0);
		stereo_message.data[1] |= 0x10;
		ai_handler->writeAIData(&stereo_message, parameter_list->computer_connected);
	} else {
		uint8_t data[] = {0x20, 0x71, 0x0};
		AIData stereo_message(sizeof(data), ID_RADIO, ID_NAV_COMPUTER);
		stereo_message.refreshAIData(data);
		
		ai_handler->writeAIData(&stereo_message, parameter_list->computer_connected);
	}
	this->sendIMIDFrequencyMessage(last_frequency, parameter_list->last_sub, parameter_list->current_preset);
}

//Send the short 8-character RDS message to the screen.
void TextHandler::sendShortRDSMessage(String text) {
	if(text.length() > 0) {
		for(int i=0;i<text.length();i+=1) {
			if(text.charAt(i) < 0x20)
				text.setCharAt(i, ' ');
		}

		AIData rds_message = getTextMessage(text, 0, 1);
		rds_message.data[1] |= 0x10;
		ai_handler->writeAIData(&rds_message, parameter_list->computer_connected);
	} else {
		uint8_t data[] = {0x20, 0x70, 0x1};
		AIData clear_message(sizeof(data), ID_RADIO, ID_NAV_COMPUTER);
		clear_message.refreshAIData(data);

		ai_handler->writeAIData(&clear_message, parameter_list->computer_connected);
	}
}

//Send the full RDS message to the screen.
void TextHandler::sendLongRDSMessage(String text) {
	if(text.length() > 0) {
		for(int i=0;i<text.length();i+=1) {
			if(text.charAt(i) < 0x20)
				text.setCharAt(i, ' ');
		}
		
		String sub_text[5] = {"","","","",""};

		splitText(14, text, sub_text, 5);
		
		for(int i=0;i<5;i+=1) {
			AIData rds_message;
			if(sub_text[i].length() > 0) {
				rds_message.refreshAIData(getTextMessage(sub_text[i], 0, uint8_t(i+1)));
			} else {
				uint8_t clear_data[] = {0x20, 0x60, uint8_t(i+1)};
				rds_message.refreshAIData(sizeof(clear_data), ID_RADIO, ID_NAV_COMPUTER);
				rds_message.refreshAIData(clear_data);
			}
			
			if(i >= 4)
				rds_message.data[1] |= 0x10;
			
			ai_handler->writeAIData(&rds_message, parameter_list->computer_connected);
		}
	} else {
		for(int i=0;i<5;i+=1) {
			uint8_t clear_data[] = {0x20, 0x60, uint8_t(i+1)};
			if(i >= 4)
				clear_data[1] |= 0x10;

			AIData clear_msg(sizeof(clear_data), ID_RADIO, ID_NAV_COMPUTER);
			clear_msg.refreshAIData(clear_data);

			ai_handler->writeAIData(&clear_msg, parameter_list->computer_connected);
		}
	}
}

//Create a placeholder window for a phone call.
void TextHandler::createPhoneWindow() {
	clearAllText();
	AIData phone_header = getTextMessage("In Call", 0x0, 0x0);
	AIData phone_option = getTextMessage("Phone Menu", 0xB, 0x0);
	AIData end_option = getTextMessage("End", 0xB, 0x1);
	
	end_option.data[1] |= 0x10;
	
	ai_handler->writeAIData(&phone_header, parameter_list->computer_connected);
	ai_handler->writeAIData(&phone_option, parameter_list->computer_connected);
	ai_handler->writeAIData(&end_option, parameter_list->computer_connected);
}

//Write the radio-handling menu to screen.
void TextHandler::createRadioMenu(const uint8_t sub) {
	if(sub != SUB_AM && sub != SUB_FM1 && sub != SUB_FM2)
		return;

	String manual_tune_msg;
	if(parameter_list->manual_tune_mode)
		manual_tune_msg = F("< Manual Tune >");
	else
		manual_tune_msg = F("Manual Tune");

	String preset_msg = F("Presets");
	AIData manual_tune_aid = getTextMessage(manual_tune_msg, 0xB, 0);
	ai_handler->writeAIData(&manual_tune_aid, parameter_list->computer_connected);

	AIData preset_aid = getTextMessage(preset_msg, 0xB, 1);
	if(sub == SUB_AM)
		preset_aid.data[1] |= 0x10;

	ai_handler->writeAIData(&preset_aid, parameter_list->computer_connected);

	if(sub != SUB_AM) {
		String scan_msg = F("Scan"), list_msg = F("Station List");
		
		AIData scan_aid = getTextMessage(scan_msg, 0xB, 2);
		ai_handler->writeAIData(&scan_aid, parameter_list->computer_connected);

		AIData list_aid = getTextMessage(list_msg, 0xB, 3);
		list_aid.data[1] |= 0x10;
		ai_handler->writeAIData(&list_aid, parameter_list->computer_connected);
	}
}

//Send the IMID source control message.
void TextHandler::sendIMIDSourceMessage(const uint8_t source, const uint8_t subsource) {
	if(!parameter_list->imid_connected)
		return;
	
	uint8_t function_data[] = {0x40, 0x10, source, subsource};
	AIData function_msg(sizeof(function_data), ID_RADIO, ID_IMID_SCR);
	function_msg.refreshAIData(function_data);

	ai_handler->writeAIData(&function_msg);
}

//Send the frequency message to the IMID.
void TextHandler::sendIMIDFrequencyMessage(const uint16_t frequency, const uint8_t subsrc, const uint8_t preset) {
	if(!parameter_list->imid_connected || parameter_list->info_mode)
		return;

	if(parameter_list->imid_radio) {
		uint8_t dec = 2, freq_letter = 0x4D;
		if(subsrc == SUB_AM) {
			dec = 0;
			freq_letter = 0x6B;
		}

		uint8_t freq_data[] = {0x67, (frequency&0xFF00)>>8, frequency&0xFF, dec, 0, freq_letter};
		if(parameter_list->fm_stereo)
			freq_data[4] |= 0x10;

		if(preset > 0)
			freq_data[4] |= (preset&0xF);

		AIData freq_msg(sizeof(freq_data), ID_RADIO, ID_IMID_SCR);
		freq_msg.refreshAIData(freq_data);

		ai_handler->writeAIData(&freq_msg);
	} else if(parameter_list->imid_lines > 0 && parameter_list->imid_char > 0) {
		const uint8_t line = (parameter_list->imid_lines+1)/2;
		
		String freq_text = "";
		if(parameter_list->imid_char >= 15) {
			if(subsrc == SUB_FM1)
				freq_text = "FM1";
			else if(subsrc == SUB_FM2)
				freq_text = "FM2";
			else if(subsrc == SUB_AM)
				freq_text = "AM";

			if(preset > 0)
				freq_text += "-" + String(preset) + " ";
			else
				freq_text += "   ";
		}
			
		if(subsrc != SUB_AM) {
			if(frequency%100 >= 10)
				freq_text += String(frequency/100) + "." + String(frequency%100) + "MHz";
			else
				freq_text += String(frequency/100) + ".0" + String(frequency%100) + "MHz";
		} else
			freq_text += String(frequency) + "kHz";

		if(freq_text.length() + 4 < parameter_list->imid_char && parameter_list->fm_stereo)
			freq_text += " St.";
		
		int16_t imid_char = parameter_list->imid_char/2 - freq_text.length()/2;
		if(imid_char < 0)
			imid_char = 0;
		
		AIData freq_msg(freq_text.length() + 4, ID_RADIO, ID_IMID_SCR);
		freq_msg.data[0] = 0x23;
		freq_msg.data[1] = 0x60;
		freq_msg.data[2] = imid_char&0xFF;
		freq_msg.data[3] = line;
		for(unsigned int i=0;i<freq_text.length();i+=1)
			freq_msg.data[i+4] = uint8_t(freq_text.charAt(i));
		
		ai_handler->writeAIData(&freq_msg);
	}
}

//Send an RDS message to the IMID. This is not the message that is displayed when Info is pressed.
void TextHandler::sendIMIDRDSMessage(String text) {
	sendIMIDRDSMessage(last_frequency, text);
}

//Send an RDS message to the IMID. This is not the message that is displayed when Info is pressed.
void TextHandler::sendIMIDRDSMessage(const uint16_t frequency, String text) {
	if(!parameter_list->imid_connected || parameter_list->info_mode)
		return;

	if(parameter_list->imid_radio) {
		AIData rds_msg(2 + text.length(), ID_RADIO, ID_IMID_SCR);

		rds_msg.data[0] = 0x63;
		rds_msg.data[1] = 0x61;

		for(unsigned int i=0;i<text.length();i+=1)
			rds_msg.data[i+2] = uint8_t(text.charAt(i));

		ai_handler->writeAIData(&rds_msg);
	} else if(parameter_list->imid_lines >= 2 && parameter_list->imid_char > 0) {
		const uint8_t line = (parameter_list->imid_lines+1)/2 + 1;

		int16_t imid_char = parameter_list->imid_char/2 - text.length()/2;
		if(imid_char < 0)
			imid_char = 0;

		AIData rds_msg(text.length() + 4, ID_RADIO, ID_IMID_SCR);
		rds_msg.data[0] = 0x23;
		rds_msg.data[1] = 0x60;
		rds_msg.data[2] = imid_char&0xFF;
		rds_msg.data[3] = line;
		for(unsigned int i=0;i<text.length();i+=1)
			rds_msg.data[i+4] = uint8_t(text.charAt(i));

		ai_handler->writeAIData(&rds_msg);
	}
}

//Send an info message.
void TextHandler::sendIMIDInfoMessage(String text) {
	if(!parameter_list->info_mode)
		return;

	if(parameter_list->imid_char <= 0 || parameter_list->imid_lines <= 0)
		return;

	const uint8_t line = (parameter_list->imid_lines+1)/2;
	text = text.substring(0, parameter_list->imid_char);

	AIData info_msg(text.length() + 4, ID_RADIO, ID_IMID_SCR);
	info_msg.data[0] = 0x23;
	info_msg.data[1] = 0x60;
	if(parameter_list->imid_char >= 16)
		info_msg.data[2] = parameter_list->imid_char/2 - text.length()/2;
	else
		info_msg.data[2] = 0;
	info_msg.data[3] = line;
	for(unsigned int i=0;i<text.length();i+=1)
		info_msg.data[i+4] = uint8_t(text.charAt(i));

	ai_handler->writeAIData(&info_msg);
}

//Send the time message.
void TextHandler::sendTime() {
	if(!parameter_list->send_time)
		return;
	
	int16_t full_minute = parameter_list->hour*60 + parameter_list->min + parameter_list->offset*30;
						
	/*while(hour < 0)
		hour += 24;
	while(hour > 23)
		hour -= 24;
	
	while(minute < 0)
		minute += 60;
	while(minute > 59)
		minute -= 60;*/
	
	while(full_minute >= 1440)
		full_minute -= 1440;
	while(full_minute < 0)
		full_minute += 1440;

	const int8_t hour = full_minute/60, minute = full_minute%60;

	uint8_t clock_data[] = {0xA1, 0x1F, 0x1, hour, minute, parameter_list->minute_timer/1000};
	AIData clock_msg(sizeof(clock_data), ID_RADIO, 0xFF);
	clock_msg.refreshAIData(clock_data);
	
	ai_handler->writeAIData(&clock_msg, false);
}

//Get a text message from a string.
AIData getTextMessage(String text, const uint8_t group, const uint8_t area) {
	text.replace("#","##  ");
	
	uint8_t text_data[3 + text.length()];
	text_data[0] = 0x23;
	text_data[1] = 0x60 | (group&0xF);
	text_data[2] = area;
	
	for(uint16_t i=0;i<text.length();i+=1)
		text_data[i+3] = uint8_t(text.charAt(i));
	
	AIData text_message(sizeof(text_data), ID_RADIO, ID_NAV_COMPUTER);
	text_message.refreshAIData(text_data);
	
	return text_message;
	
}

//Split text by spaces.
void splitText(const uint16_t len, String text, String* sub_text, const int num) {
	if(text.length() > 0) {
		int text_end = text.length();
		for(int i=text.length()-1; i >= 0; i-= 1) {
			if(text.charAt(i) > ' ')
				break;
			else
				text_end = i;
		}
		text = text.substring(0, text_end);

		for(int i=0;i<text.length();i+=1) {
			if(text.charAt(i) < 0x20 && text.charAt(i) != 0) {
				text.remove(i);
				i -= 1;
			}
		}

		text += " ";
	}

	for(int i=0;i<num;i+=1) {
		if(text.length() > 0) {
			int last_space = -1, space = -1;

			if(text.charAt(0) == ' ')
				text.remove(0);
			
			do {
				last_space = space;
				space = text.indexOf(' ', last_space + 1);
				
				if(space >= len || space < 0 || space >= text.length() - 1)
					break;
			} while(space >= 0);

			if(space >= text.length() - 1 && i >= num - 1)
				text.remove(text.length() - 1);
			
			if(last_space >= 0 && last_space < len && space > len) {
				sub_text[i] = text.substring(0, last_space);
				text = text.substring(last_space + 1, text.length());
			} else if(space == len) {
				sub_text[i] = text.substring(0,len);
				text = text.substring(len + 1,text.length());
			} else {
				if(text.length() >= len) {
					sub_text[i] = text.substring(0,len);
					text = text.substring(len,text.length());
				} else {
					sub_text[i] = text;
					text = "";
				}
			}
		}
	}
}