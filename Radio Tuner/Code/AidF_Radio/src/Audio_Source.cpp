#include "Audio_Source.h"

SourceHandler::SourceHandler(AIBusHandler* ai_handler, TextHandler* text_handler, Si4735Controller* tuner_main, BackgroundTuneHandler* tuner_background, ParameterList* parameter_list, VolumeHandler* volume_handler, uint16_t source_count) {
	this->source_count = source_count;
	this->source_list = new AudioSource[source_count];

	for(int i=0;i<source_count;i+=1) {
		source_list[i].sub_id = 0;
		source_list[i].source_id = 0;
		source_list[i].source_name = "";
	}

	this->ai_handler = ai_handler;
	this->text_handler = text_handler;
	this->parameter_list = parameter_list;
	this->tuner_main = tuner_main;
	this->tuner_background = tuner_background;
	this->volume_handler = volume_handler;
}

SourceHandler::~SourceHandler() {
	delete[] source_list;
	delete[] imid_supported_sources;
}

//Get the ID of the currently-selected source.
uint8_t SourceHandler::getCurrentSourceID() {
	if(audio_on)
		return this->source_list[this->current_source].source_id;
	else
		return 0;
}

//Get the subsource ID of the currently-selected source.
uint8_t SourceHandler::getCurrentSourceSubID() {
	if(audio_on)
		return this->source_list[this->current_source].sub_id;
	else
		return 0;
}

//Get the index of the currently selected source.
uint16_t SourceHandler::getCurrentSource() {
	return this->current_source;
}

//Set the selected source by index.
void SourceHandler::setSource(const uint16_t source) {
	if(source >= source_count)
		return;
	this->audio_on = true;
	this->current_source = source;
}

//Set the selected source by ID. Returns false if that ID does not exist.
bool SourceHandler::setSourceID(const uint8_t source) {
	int new_source = getFirstOccurenceOf(source);
	if(new_source < 0)
		return false;
	else {
		this->current_source = new_source;
		this->audio_on = true;
		return true;
	}
}

//Create a new subsource.
void SourceHandler::createSubsource(const uint8_t id) {
	const int index = getFirstOccurenceOf(id);
	int new_index = getFirstAvailable(index+1);

	if(new_index < 0 || index < 0)
		return;
	
	uint8_t last_sub = 0;
	{
		int search_index = index;
		while(search_index > 0) {
			search_index = getFirstOccurenceOf(id, new_index + 1);
			if(search_index > 0) {
				new_index = search_index;
				last_sub = source_list[new_index].sub_id;
			}
		}
	}

	AudioSource new_source;
	new_source.source_id = id;
	new_source.source_name = "";
	new_source.sub_id = last_sub + 1;

	this->source_list[new_index] = new_source;
}

//Clear all subsources for this ID.
void SourceHandler::clearSubsources(const uint8_t id) {
	const int index = getFirstOccurenceOf(id);
	
	if(index < 0)
		return;
	
	int new_index = getFirstOccurenceOf(id, index+1);
	while(new_index > 0) {
		this->source_list[new_index].source_id = 0;
		this->source_list[new_index].source_name = "";
		this->source_list[new_index].sub_id = 0;
		new_index = getFirstOccurenceOf(id, new_index+1);
	}
}

//Check the sources for missing information.
void SourceHandler::checkSources() {
	bool resend = false;

	for(int i=0;i<this->source_count;i+=1) {
		AudioSource* source = &this->source_list[i];
		if(source->source_id != 0 && source->source_id != ID_RADIO && source->source_name.compareTo("") == 0) {
			const bool resp = sendSourceQuery(source->source_id);
			if(!resp && !resend)
				resend = true;
		}
	}

	if(resend) {
		parameter_list->handshake_timer_active = true;
		parameter_list->handshake_timer = 0;
	}
}

//Set the list of sources natively supported on the IMID.
void SourceHandler::setImidSupportedSources(const int l, uint8_t* source_list) {
	this->imid_supported_source_count = 0;
	delete[] this->imid_supported_sources;

	this->imid_supported_sources = new uint8_t[l];
	this->imid_supported_source_count = l;

	for(int i=0;i<l;i+=1)
		this->imid_supported_sources[i] = source_list[i];
}

//Get whether the source is natively supported on the connected IMID.
bool SourceHandler::getIMIDSourceSupported(const uint8_t source_id) {
	for(int i=0;i<this->imid_supported_source_count;i+=1) {
		if(imid_supported_sources[i] == source_id)
			return true;
	}

	return false;
}

//Turn the audio on or off.
void SourceHandler::setAudioOn(const bool audio_on) {
	this->audio_on = audio_on;
}

//Return whether audio is on.
bool SourceHandler::getAudioOn() {
	return this->audio_on;
}

//Get whether the source should be enabled with no delay.
bool SourceHandler::getForceSourceChanged() {
	const bool source_changed = this->force_source_changed;
	this->force_source_changed = false;
	return source_changed;
}

//Call if the car is turned on or off.
void SourceHandler::setPower(const bool power) {
	if(!power) {
		this->audio_on = false;
	} else {
		
	}
}

//Send the initial radio handshake message.
void SourceHandler::sendRadioHandshake() {
	uint8_t data[] = {0x4, 0xE6, 0x10};
	AIData handshake_msg(3, ID_RADIO, 0xFF, data);

	ai_handler->writeAIData(&handshake_msg);
}

//Handle AIBus data.
bool SourceHandler::handleAIBus(AIData* ai_d) {
	bool ack = true;

	if(ai_d->l >= 3 && ai_d->data[0] == 0x1) { //Handshake message.
		if(ai_d->data[1] == 0x1) { //First message from this source.
			if(ai_d->sender != ai_d->data[2]) {
				ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
				return true;
			}
			
			const uint8_t new_id = ai_d->sender;
			if(getFirstOccurenceOf(new_id) >= 0) {
				if(ai_d->l >= 4) {
					const uint8_t sub_count = ai_d->data[3];
					uint16_t subsources[source_count];
					const uint16_t set_sub_count = getSubsourceIDs(new_id, subsources);
					if(set_sub_count != sub_count) {
						clearSubsources(new_id);

						for(int i=1;i<sub_count;i+=1)
							createSubsource(new_id);
					} else {
						for(int i=0;i<set_sub_count;i+=1)
							source_list[subsources[i]].connected = true;
					}
				}

				for(int i=0;i<source_count;i+=1) {
					if(source_list[i].source_id == new_id)
						source_list[i].connected = true;
				}

				ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
				return true;
			} else {
				const int new_index = getFirstAvailable();
				if(new_index < 0) {
					ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
					return true;
				}

				AudioSource new_source;
				new_source.source_id = new_id;
				new_source.source_name = "";
				new_source.source_short = "";
				new_source.sub_id = 0;
				new_source.connected = true;

				this->source_list[new_index] = new_source;

				if(ai_d->l >= 4) {
					const uint8_t sub_count = ai_d->data[3];
					for(int i=1;i<sub_count;i+=1)
						createSubsource(new_id);
				}
			}

			parameter_list->handshake_timer = 0;
			parameter_list->handshake_timer_active = true;
			parameter_list->handshake_sources.push_back(new_id);
			
			ack = false;
			ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);

			const uint8_t current_source_id = parameter_list->audio_on ? this->source_list[current_source].source_id : 0, sub_id = parameter_list->audio_on ? this->source_list[current_source].sub_id : 0;
			uint8_t function_data[] = {0x40, 0x10, current_source_id, sub_id};
			AIData function_msg(sizeof(function_data), ID_RADIO, new_id, function_data);
			ai_handler->writeAIData(&function_msg);

			if(current_source_id == new_id) {
				uint8_t text_function_data[] = {0x40, 0x1, current_source_id};
				AIData text_function_msg(sizeof(text_function_data), ID_RADIO, new_id, text_function_data);
				ai_handler->writeAIData(&text_function_msg);
			}
		} else if(ai_d->data[1] == 0x2) { //Sub-source.
			const uint8_t id = ai_d->sender;

			int index = getFirstOccurenceOf(id);
			int new_index = getFirstAvailable(index+1);

			if(new_index < 0 || index < 0) {
				ack = false;
				ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
				sendSourceQuery(ai_d->sender);
				return true;
			}

			createSubsource(id);

			uint8_t handshake_data[] = {0x5, id, 0x2};
			AIData handshake_msg(sizeof(handshake_data), ID_RADIO, ai_d->sender, handshake_data);
			ai_handler->writeAIData(&handshake_msg);
		} else if(ai_d->data[1] == 0x23 || ai_d->data[1] == 0x22) { //Source name or short name.
			const uint8_t sub = ai_d->data[2];
			int index = getFirstOccurenceOf(ai_d->sender);
			
			int count = 0;
			
			while(count < sub && index >= 0) {
				index = getFirstOccurenceOf(ai_d->sender, index+1);
				count += 1;
			}

			if(index < 0) {
				ack = false;
				ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
				sendSourceQuery(ai_d->sender);
				return true;
			}

			String name = "";
			for(int i=3;i<ai_d->l;i+=1)
				name += char(ai_d->data[i]);

			if(ai_d->data[1] == 0x23) {
				this->source_list[index].source_name = name;
				if(this->source_list[index].source_short.equals(""))
					this->source_list[index].source_short = name;
			} else 
				this->source_list[index].source_short = name;

			this->source_list[index].connected = true;
		}
		//ack = false;
		
		if(ack)
			ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
		
		return true;
		
	} else if(ai_d->l >= 1 && (ai_d->data[0] == 0x3B || ai_d->data[0] == 0x39 || ai_d->data[0] == 0x31)) { //Status message.
		ack = false;
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
		
		const uint8_t src = getCurrentSourceID();
		if(ai_d->sender != src) {
			uint8_t function_data[] = {0x40, 0x10, src};
			AIData function_msg(sizeof(function_data), ID_RADIO, ai_d->sender, function_data);
			ai_handler->writeAIData(&function_msg);
		}
	} else if(ai_d->l >= 2 && ai_d->data[0] == 0x60 && ai_d->data[1] == 0x10) { //Current source request.
		ack = false;
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
		
		const uint8_t src = getCurrentSourceID();
		if(ai_d->sender == src || ai_d->sender == ID_IMID_SCR || ai_d->sender == ID_NAV_COMPUTER) {
			const uint8_t sub_id = getCurrentSourceSubID();

			uint8_t function_data[] = {0x40, 0x10, src, sub_id};
			AIData function_msg(sizeof(function_data), ID_RADIO, ai_d->sender, function_data);
			ai_handler->writeAIData(&function_msg);
		} else {
			uint8_t function_data[] = {0x40, 0x10, src};
			AIData function_msg(sizeof(function_data), ID_RADIO, ai_d->sender, function_data);
			ai_handler->writeAIData(&function_msg);
		}
	} else if(ai_d->l >= 2 && ai_d->data[0] == 0x60 && ai_d->data[1] == 0x11) { //Text control request.
		ack = false;
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
		
		const uint8_t src = getCurrentSourceID();

		uint8_t function_data[] = {0x40, 0x1, src};
		AIData function_msg(sizeof(function_data), ID_RADIO, ai_d->sender, function_data);
		ai_handler->writeAIData(&function_msg);
	} else if(ai_d->l >= 3 && ai_d->data[0] == 0x10 && ai_d->data[1] == 0x10) { //Source request control.
		ack = false;
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
		
		const uint8_t new_src = ai_d->data[2];
		uint8_t new_sub = 0;
		
		if(ai_d->l >= 4)
			new_sub = ai_d->data[3];
		
		force_source_changed = true;
		setCurrentSource(new_src, new_sub);
	} else if(ai_d->sender == ID_NAV_SCREEN) { //Screen message.
		ack = false;
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
		if(ai_d->l >= 3 && ai_d->data[0] == 0x30) { //Button press.
			const uint8_t button = ai_d->data[1], state = ai_d->data[2]>>6;

			if(parameter_list->manual_tune_mode && (button != 0x7 && button != 0x2A && button != 0x2B)) {
				parameter_list->manual_tune_mode = false;
				sendManualTuneMessage();
				
				{
					uint8_t data[] = {0x77, parameter_list->last_control, 0x10};

					AIData screen_msg(sizeof(data), ID_RADIO, ID_NAV_SCREEN, data);
					ai_handler->writeAIData(&screen_msg, parameter_list->screen_connected);
				}
			}

			if(parameter_list->bass_adjust || parameter_list->treble_adjust || parameter_list->balance_adjust || parameter_list->fader_adjust) {
				if(button != 0x2A && button != 0x2B && state == 2) {
					if(menu_open == TONE_MENU) {
						if(parameter_list->bass_adjust) {
							parameter_list->bass_adjust = false;
							createToneMenuItem(0);
						}
						if(parameter_list->treble_adjust) {
							parameter_list->treble_adjust = false;
							createToneMenuItem(1);
						}
						if(parameter_list->balance_adjust) {
							parameter_list->balance_adjust = false;
							createToneMenuItem(2);
						}
						if(parameter_list->fader_adjust) {
							parameter_list->fader_adjust = false;
							createToneMenuItem(3);
						}
					} else {
						parameter_list->bass_adjust = false;
						parameter_list->treble_adjust = false;
						parameter_list->balance_adjust = false;
						parameter_list->fader_adjust = false;
					}

					{
						uint8_t data[] = {0x77, parameter_list->last_control, 0x10};

						AIData screen_msg(sizeof(data), ID_RADIO, ID_NAV_SCREEN, data);
						ai_handler->writeAIData(&screen_msg, parameter_list->screen_connected);
					}
				} else if(state == 2) {
					volume_handler->setAIBusParameter(ai_d);

					if(menu_open == TONE_MENU) {
						if(parameter_list->bass_adjust)
							createToneMenuItem(0);
						if(parameter_list->treble_adjust)
							createToneMenuItem(1);
						if(parameter_list->balance_adjust)
							createToneMenuItem(2);
						if(parameter_list->fader_adjust)
							createToneMenuItem(3);
					}
				}
				return true;
			}

			if(button == 0x6 && state == 2) //Press volume knob.
				audio_on = !audio_on;
			else if(button == 0x23 && state == 2) { //Source button.
				if(parameter_list->vehicle_speed <= 5 && parameter_list->computer_connected) { //Car is stopped.
					if(menu_open != SOURCE_MENU)
						createSourceMenu();
					else
						clearMenu();
				} else if(audio_on) {
					incrementSource();
				}
			} else if((button == 0x24 || button == 0x25) && state == 2) { //Seek up/down.
				if(source_list[current_source].source_id == ID_RADIO && source_list[current_source].sub_id != SUB_AM) {		
					this->tuner_main->startSeek(button == 0x25);
					parameter_list->tune_changed = true;
				}
			} else if((button == 0x2A || button == 0x2B) && state == 0) { //Left/right buttons.
				if((this->parameter_list->manual_tune_mode || !this->parameter_list->computer_connected) && source_list[current_source].source_id == ID_RADIO) {
					manualTuneIncrement(button == 0x2B, 1);
					//parameter_list->tune_changed = true;
				}
			} else if(button == 0x7 && state == 2) { //Enter button.
				parameter_list->manual_tune_mode = false;
				sendManualTuneMessage();

				{
					uint8_t data[] = {0x77, ID_RADIO, 0x10};
					if(parameter_list->manual_tune_mode)
						data[2] |= 0x20;
					else
						data[1] = parameter_list->last_control;

					AIData screen_msg(sizeof(data), ID_RADIO, ID_NAV_SCREEN, data);
					ai_handler->writeAIData(&screen_msg, parameter_list->screen_connected);
				}
			} else if(button == 0x53 && state == 2) { //Info button.
				if(parameter_list->imid_char > 0 && parameter_list->imid_lines == 1 && getCurrentSourceID() == ID_RADIO)
					parameter_list->info_mode = !parameter_list->info_mode;
				else if(getCurrentSourceID() == ID_RADIO) {
					if(parameter_list->info_mode)
						parameter_list->info_mode = false;
					String rds_header = parameter_list->rds_program_name;

					for(int c=0;c<rds_header.length();c+=1) {
						if(rds_header.charAt(c) < ' ') {
							rds_header = rds_header.substring(0,c);
							break;
						}
					}

					rds_header.replace("#", "##  ");
					this->text_handler->setOverlayHeader(rds_header);
				}
			} else if(button == 0x36 && state == 0x2) { //AM/FM button.
				const uint8_t source_id = getCurrentSourceID();
				if(source_id == ID_RADIO) { //Increment AM/FM.
					int new_src = getFirstOccurenceOf(ID_RADIO, current_source+1);
					if(new_src < 0)
						new_src = getFirstOccurenceOf(ID_RADIO);
					else if(source_list[new_src].sub_id > 2) {
						new_src = getFirstOccurenceOf(ID_RADIO, new_src+1);
						if(new_src < 0)
							new_src = getFirstOccurenceOf(ID_RADIO);
					}

					if(new_src >= 0 && new_src < source_count) {
						setCurrentSource(source_list[new_src].source_id, source_list[new_src].sub_id);
					}
				} else {
					int new_src = getFirstOccurenceOf(ID_RADIO);

					if(source_list[new_src].sub_id > 2)
						new_src = getFirstOccurenceOf(ID_RADIO, new_src+1);

					if(new_src >= 0 && new_src < source_count)
						setCurrentSource(source_list[new_src].source_id, source_list[new_src].sub_id);
				}
			} else if(button == 0x38 && state == 0x2) { //Media button.
				int new_src = current_source;
				int start = current_source;
				const uint8_t source_id = getCurrentSourceID();

				if(source_id == 0) {
					start = 0;
					new_src = -1;
				}

				for(int s=start + 1;s<source_count;s+=1) {
					bool source_found = false;

					if((source_list[s].source_id != ID_RADIO && source_list[s].source_id != 0 && source_list[s].connected) || (source_list[s].source_id == ID_RADIO && source_list[s].sub_id > 2)) {
						new_src = s;
						source_found = true;
					}

					if(source_found)
						break;

					if(s == start)
						break;
					if(s == source_count - 1)
						s = 0;
				}

				if(new_src > 0 && new_src < source_count)
					setCurrentSource(source_list[new_src].source_id, source_list[new_src].sub_id);
			} else if(button == 0x30 && state == 2) { //FM button.
				if(getCurrentSourceID() == 0) {
					setCurrentSource(ID_RADIO, 0);
				} else if(source_list[current_source].source_id == ID_RADIO) {
					if(source_list[current_source].sub_id == 0)
						setCurrentSource(ID_RADIO, 1);
					else
						setCurrentSource(ID_RADIO, 0);
				} else
					setCurrentSource(ID_RADIO, 0);
			} else if(button == 0x31 && state == 2) { //AM button.
				setCurrentSource(ID_RADIO, 2);
			} else if(button == 0x33 && state == 2) { //CD button.
				const uint8_t source_id = getCurrentSourceID();
				int new_src = -1;

				if(source_id == ID_CD) {
					new_src = getFirstOccurenceOf(ID_CDC);
				} else if(source_id == ID_CDC) {
					new_src = getFirstOccurenceOf(ID_CD);
				} else {
					new_src = getFirstOccurenceOf(ID_CD);
					if(new_src < 0)
						new_src = getFirstOccurenceOf(ID_CDC);
				}

				if(new_src >= 0 && new_src < source_count)
					setCurrentSource(source_list[new_src].source_id, source_list[new_src].sub_id);
			} else if(button == 0x32 && state == 2) { //Tape button.
				setCurrentSource(ID_TAPE, 0);
			} else if(button == 0x34 && state == 2) { //Aux button.
				setCurrentSource(ID_RADIO, 3);
				//TODO: USB and BTA.
			} else if(button == 0x35 && state == 2) { //XM button.
				const uint8_t source_id = getCurrentSourceID();
				int new_src = -1;

				if(source_id == ID_XM) {
					new_src = getFirstOccurenceOf(ID_XM, current_source + 1);
					if(new_src < 0)
						new_src = getFirstOccurenceOf(ID_XM);
				} else
					new_src = getFirstOccurenceOf(ID_XM);

				if(new_src >= 0 && new_src < source_count)
					setCurrentSource(source_list[new_src].source_id, source_list[new_src].sub_id);
			} else if(button == 0x37 && state == 2) { //Tape/CD button.
				int start = current_source;
				int new_src = current_source;
				const uint8_t source_id = getCurrentSourceID();

				if(source_id == 0) {
					start = 0;
					new_src = -1;
				}

				for(int s=start + 1;s<source_count;s+=1) {
					bool source_found = false;

					if(source_list[s].source_id == ID_TAPE && source_list[s].connected) {
						new_src = s;
						source_found = true;
					} else if((source_list[s].source_id == ID_CDC || source_list[s].source_id == ID_CD) && source_list[s].connected) {
						new_src = s;
						source_found = true;
					}

					if(source_found)
						break;

					if(s == start)
						break;
					if(s == source_count - 1)
						s = 0;
				}

				if(new_src > 0 && new_src < source_count && new_src != current_source)
					setCurrentSource(source_list[new_src].source_id, source_list[new_src].sub_id);
			} else if(button == 0x52 && state == 2) { //Tone button.
				if(!parameter_list->digital_amp) {
					if(menu_open != TONE_MENU)
						this->createToneMenu();
					else
						clearMenu();
				}
				//TODO: For a digital amp, ask the amp to create the tone menu.
			} else if(button >= 0x11 && button <= 0x16) { //Presets.
				const uint8_t preset = (button&0xF) - 1;
				if(audio_on && source_list[current_source].source_id == ID_RADIO && source_list[current_source].sub_id <= 2) {
					const uint8_t group = source_list[current_source].sub_id;
					if(state == 2) { //Recall preset.
						uint16_t freq = tuner_main->getFrequency();
						if(group == SUB_FM1)
							freq = parameter_list->fm1_presets[preset];
						else if(group == SUB_FM2)
							freq = parameter_list->fm2_presets[preset];
						else if(group == SUB_AM)
							freq = parameter_list->am_presets[preset];

						tuner_main->setFrequency(freq);
						if(group == SUB_FM1)
							parameter_list->fm1_tune = tuner_main->getFrequency();
						else if(group == SUB_FM2)
							parameter_list->fm2_tune = tuner_main->getFrequency();
						else if(group == SUB_AM)
							parameter_list->am_tune = tuner_main->getFrequency();

						parameter_list->preferred_preset = preset + 1;
						parameter_list->tune_changed = true;
					} else if(state == 1) { //Save preset.
						savePreset(tuner_main->getFrequency(), preset, group);
						parameter_list->preferred_preset = preset + 1;
						parameter_list->tune_changed = true;

						setEEPROMPresets(parameter_list);
					}
				}
			}
			
		} else if(ai_d->l >= 3 && ai_d->data[0] == 0x32) { //Knob turn.
			ack = false;
			ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);

			const uint8_t steps = ai_d->data[2]&0xF;
			const bool clockwise = (ai_d->data[2]&0x10) != 0;
			if(ai_d->data[1] == 0x7) { //Function knob.
				if((this->parameter_list->manual_tune_mode || !this->parameter_list->computer_connected) && source_list[current_source].source_id == ID_RADIO) {
					manualTuneIncrement(clockwise, steps);
					//parameter_list->tune_changed = true;
				} else if(parameter_list->bass_adjust || parameter_list->treble_adjust || parameter_list->balance_adjust || parameter_list->fader_adjust) {
					volume_handler->setAIBusParameter(ai_d);

					if(menu_open == TONE_MENU) {
						if(parameter_list->bass_adjust)
							createToneMenuItem(0);
						if(parameter_list->treble_adjust)
							createToneMenuItem(1);
						if(parameter_list->balance_adjust)
							createToneMenuItem(2);
						if(parameter_list->fader_adjust)
							createToneMenuItem(3);
					}
				}
			}
		}
		return true;
	} else if(ai_d->sender == ID_NAV_COMPUTER) { //Message from computer.
		ack = false;
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
		if(ai_d->l >= 2 && ai_d->data[0] == 0x2B) { //Menu related message.
			if(ai_d->data[1] == 0x40) { //A menu was cleared.
				const radio_menu_t last_menu = menu_open;
				
				menu_open = NO_MENU;
				tuner_background->setSeekMode(true);

				if(last_menu == RDS_FLASH_MENU)
					createRadioSettingsMenu();

				return true;
			} else if(ai_d->l >= 3 && ai_d->data[1] == 0x6A) { //Audio menu item selected.
				const uint8_t item = ai_d->data[2], source_id = this->getCurrentSourceID();
				if(source_id == ID_RADIO && source_list[current_source].sub_id <= 2) {
					switch(item) {
					case 1: //Manual tune.
						parameter_list->manual_tune_mode = !parameter_list->manual_tune_mode;

						{
							uint8_t data[] = {0x77, ID_RADIO, 0x10};
							if(parameter_list->manual_tune_mode)
								data[2] |= 0x20;
							else
								data[1] = parameter_list->last_control;

							AIData screen_msg(sizeof(data), ID_RADIO, ID_NAV_SCREEN, data);
							ai_handler->writeAIData(&screen_msg, parameter_list->screen_connected);
						}

						this->sendManualTuneMessage();
						break;
					case 2: //Preset list.
						if(menu_open == NO_MENU)
							createPresetMenu(source_list[current_source].sub_id);
						break;
					case 4: //Station list.
						if(menu_open == NO_MENU)
							createStationListMenu();
						break;
					}
				} else if(source_id != 0 && source_id != ID_RADIO) {
					AIData forward_msg(ai_d->l, ID_RADIO, source_id, ai_d->data);
					ai_handler->writeAIData(&forward_msg);
				}
				return true;
			} else if(ai_d->l >= 3 && ai_d->data[1] == 0x60) { //True menu item selected.
				const uint8_t selection = ai_d->data[2]-1;
				if(menu_open == SOURCE_MENU) {
					AudioSource source_list[source_count];
					getFilledSources(source_list);
					const uint8_t new_id = source_list[selection].source_id, new_sub = source_list[selection].sub_id;
					clearMenu();
					setCurrentSource(new_id, new_sub);
					force_source_changed = true;
				} else if(menu_open == PRESET_MENU) {
					if(source_list[current_source].source_id == ID_RADIO && source_list[current_source].sub_id <= 2) {
						const uint8_t group = source_list[current_source].sub_id;
						uint16_t freq = tuner_main->getFrequency();
						const uint8_t preset = selection;
						if(group == SUB_FM1)
							freq = parameter_list->fm1_presets[preset];
						else if(group == SUB_FM2)
							freq = parameter_list->fm2_presets[preset];
						else if(group == SUB_AM)
							freq = parameter_list->am_presets[preset];

						tuner_main->setFrequency(freq);
						if(group == SUB_FM1)
							parameter_list->fm1_tune = tuner_main->getFrequency();
						else if(group == SUB_FM2)
							parameter_list->fm2_tune = tuner_main->getFrequency();
						else if(group == SUB_AM)
							parameter_list->am_tune = tuner_main->getFrequency();

						parameter_list->preferred_preset = preset + 1;
						parameter_list->tune_changed = true;
					}
					clearMenu();
				} else if(menu_open == STATION_MENU) {
					if(source_list[current_source].source_id == ID_RADIO && source_list[current_source].sub_id <= 1) {
						const uint16_t new_freq = tuner_background->getStationFrequency(selection);
						const uint8_t group = source_list[current_source].sub_id;

						tuner_main->setFrequency(new_freq);
						if(group == SUB_FM1)
							parameter_list->fm1_tune = tuner_main->getFrequency();
						else if(group == SUB_FM2)
							parameter_list->fm2_tune = tuner_main->getFrequency();
						else if(group == SUB_AM)
							parameter_list->am_tune = tuner_main->getFrequency();

						parameter_list->tune_changed = true;

					}
					clearMenu();
				} else if(menu_open == TONE_MENU) {
					const int selection = ai_d->data[2]-1;

					if(selection < TONE_OPTION_SVC) {
						bool* switch_mode;
						const MenuList tone_menu = getMenu(MENU_INDEX_TONE, parameter_list->locale);

						switch(tone_menu.getGlobalIndex(selection)) {
						case MENU_INDEX_TONE_BASS:
							switch_mode = &parameter_list->bass_adjust;
							break;
						case MENU_INDEX_TONE_TREBLE:
							switch_mode = &parameter_list->treble_adjust;
							break;
						case MENU_INDEX_TONE_BALANCE:
							switch_mode = &parameter_list->balance_adjust;
							break;
						case MENU_INDEX_TONE_FADER:
							switch_mode = &parameter_list->fader_adjust;
							break;
						}

						*switch_mode = !*switch_mode;

						createToneMenuItem(selection);

						{
							uint8_t data[] = {0x77, ID_RADIO, 0x10};
							if(*switch_mode)
								data[2] |= 0x20;
							else
								data[1] = parameter_list->last_control;

							AIData screen_msg(sizeof(data), ID_RADIO, ID_NAV_SCREEN, data);
							ai_handler->writeAIData(&screen_msg, parameter_list->screen_connected);
						}
					}
				} else if(menu_open == RADIO_SETTINGS_MENU) {
					const int selection = ai_d->data[2]-1;
					const MenuList settings_menu = getMenu(MENU_INDEX_RADIO_SETTINGS, parameter_list->locale);
					
					switch(settings_menu.getGlobalIndex(selection)) {
					case MENU_INDEX_RADIO_SETTINGS_RDS_FLASH:
						if(parameter_list->imid_char <= 0 || parameter_list->imid_lines != 1) { //Checkbox.
							if(parameter_list->header_rds_setting == HEADER_RDS_OFF)
								parameter_list->header_rds_setting = HEADER_RDS_ALWAYS;
							else
								parameter_list->header_rds_setting = HEADER_RDS_OFF;

							createRadioSettingsMenuItem(settings_menu.getLocalIndex(MENU_INDEX_RADIO_SETTINGS_RDS_FLASH));
						} else
							createRDSFlashMenu();
						break;
					}
				} else if(menu_open == RDS_FLASH_MENU) {
					const int selection = ai_d->data[2]-1;
					const MenuList rds_menu = getMenu(MENU_INDEX_RDS_FLASH_SETTINGS, parameter_list->locale);

					switch(rds_menu.getGlobalIndex(selection)) {
					case MENU_INDEX_RDS_FLASH_SETTINGS_OFF:
						parameter_list->header_rds_setting = HEADER_RDS_OFF;
						break;
					case MENU_INDEX_RDS_FLASH_SETTINGS_INFO_MODE:
						parameter_list->header_rds_setting = HEADER_RDS_INFO_MODE;
						break;
					case MENU_INDEX_RDS_FLASH_SETTINGS_ALWAYS:
						parameter_list->header_rds_setting = HEADER_RDS_ALWAYS;
						break;
					}

					clearMenu();
					createRadioSettingsMenu();
				}
				return true;
			} else if(ai_d->l >= 2 && ai_d->data[1] == 0x4A) {
				if(source_list[current_source].source_id == 0)
					return true;
				
				if(menu_open != NO_MENU)
					clearMenu();
				const uint8_t src = source_list[current_source].source_id;
				
				if(src != ID_RADIO) {
					uint8_t request_data[] = {0x2B, 0x4A};
					AIData request_msg(sizeof(request_data), ID_RADIO, src, request_data);
					
					ai_handler->writeAIData(&request_msg);
					return true;
				} else {
					createRadioSettingsMenu();
					return true;
				}
			}
		}
	} else if(ai_d->sender == ID_STEERING_CTRL) { //Steering wheel control.
		ack = false;
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);

		if(ai_d->l >= 3 && ai_d->data[0] == 0x30) {
			this->handleSteeringControl(ai_d->data[1], ai_d->data[2]);
		}
		return true;
	}

	if(ack)
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);

	return false;
}

//Set the manual tuning message.
void SourceHandler::sendManualTuneMessage() {
	const MenuList main_menu = getMenu(MENU_INDEX_RADIO_MAIN_MENU, parameter_list->locale);
	if(parameter_list->manual_tune_mode) { //TODO Integrate the text handler.
		AIData change_msg = getTextMessage(String("< ") + main_menu.getLocalEntry(MENU_INDEX_RADIO_MAIN_MENU_MANUAL) + " >", 0xB, 0x0);
		ai_handler->writeAIData(&change_msg);
	} else {
		AIData change_msg = getTextMessage(main_menu.getLocalEntry(MENU_INDEX_RADIO_MAIN_MENU_MANUAL), 0xB, 0x0);
		ai_handler->writeAIData(&change_msg);
	}
}

//Adjust tuning manually.
void SourceHandler::manualTuneIncrement(const bool up, const uint8_t steps) {
	uint16_t* current_frequency;

	if(source_list[current_source].sub_id == SUB_FM1)
		current_frequency = &this->parameter_list->fm1_tune;
	else if(source_list[current_source].sub_id == SUB_FM2)
		current_frequency = &this->parameter_list->fm2_tune;
	else if(source_list[current_source].sub_id == SUB_AM)
		current_frequency = &this->parameter_list->am_tune;
	else
		return;

	uint8_t increment = parameter_list->fm_inc;
	uint16_t lower_limit = parameter_list->fm_lower_limit, upper_limit = parameter_list->fm_upper_limit;
	if(source_list[current_source].sub_id == SUB_AM) {
		increment = parameter_list->am_inc;
		lower_limit = parameter_list->am_lower_limit;
		upper_limit = parameter_list->am_upper_limit;
	}
	
	if(up) {
		for(int i=0;i<steps;i+=1) {
			*current_frequency += increment;
			if(*current_frequency < lower_limit || *current_frequency > upper_limit)
				*current_frequency = lower_limit;
		}
	} else {
		for(int i=0;i<steps;i+=1) {
			*current_frequency -= increment;
			if(*current_frequency < lower_limit || *current_frequency > upper_limit)
				*current_frequency = upper_limit;
		}
	}

	if(!this->tuner_main->getQueued()) {
		text_handler->sendStereoMessage(false);
		text_handler->sendShortRDSMessage("");
		text_handler->sendLongRDSMessage("");
		text_handler->sendIMIDCallsignMessage("");
		text_handler->sendMirrorMessage("", 3, false);

		parameter_list->rds_program_name = "";
		parameter_list->rds_station_name = "";
		parameter_list->fm_stereo = false;
		parameter_list->has_rds = false;

		uint8_t clear_data[] = {0x20, 0x71, 0x1};
		AIData clear_msg(sizeof(clear_data), ID_RADIO, ID_NAV_COMPUTER, clear_data);
		ai_handler->writeAIData(&clear_msg, parameter_list->computer_connected);
	}

	text_handler->sendTunedFrequencyMessage(*current_frequency, source_list[current_source].sub_id != SUB_AM, true);
	this->tuner_main->queueFrequency(*current_frequency);
}

//Increment source up.
void SourceHandler::incrementSource() {
	incrementSource(true);
}

//Increment source up if direction is true.
void SourceHandler::incrementSource(const bool direction) {
	if(direction) {
		uint16_t new_source;
		if(this->current_source < this->source_count - 1)
			new_source = this->current_source + 1;
		else
			new_source = 0;
		
		const uint16_t old_source = new_source;
		while(this->source_list[new_source].source_id == 0 || !this->source_list[new_source].connected) {
			new_source += 1;
			if(new_source >= this->source_count)
				new_source = 0;
			
			if(new_source == old_source)
				break;
		}

		this->current_source = new_source;
	} else {
		uint16_t new_source;
		if(this->current_source <= 0)
			new_source = this->source_count - 1;
		else
			new_source = current_source - 1;

		const uint16_t old_source = new_source;
		while(this->source_list[new_source].source_id == 0 || !this->source_list[new_source].connected) {
			if(new_source <= 0)
				new_source = this->source_count - 1;
			else
				new_source -= 1;

			if(new_source == old_source)
				break;
		}
		this->current_source = new_source;
	}
}

//Return the number of available sources.
uint16_t SourceHandler::getFilledSourceCount() {
	uint16_t filled_source_count = 0;
	for(int i=0;i<this->source_count;i+=1) {
		if(this->source_list[i].source_id != 0 && this->source_list[i].connected)
			filled_source_count += 1;
	}

	return filled_source_count;
}

//Return a list of available sources.
uint16_t SourceHandler::getFilledSources(AudioSource* source_list) {
	uint16_t filled_source_count = 0;
	for(int i=0;i<this->source_count;i+=1) {
		if(this->source_list[i].source_id != 0 && this->source_list[i].connected) {
			source_list[filled_source_count] = this->source_list[i];
			filled_source_count += 1;
		}
	}

	return filled_source_count;
}

//Return the names of available sources.
uint16_t SourceHandler::getSourceNames(String* source_list) {
	uint16_t filled_source_count = 0;
	for(int i=0;i<this->source_count;i+=1) {
		if(this->source_list[i].source_id != 0 && this->source_list[i].connected) {
			source_list[filled_source_count] = this->source_list[i].source_name;
			filled_source_count += 1;
		}
	}

	return filled_source_count;
}

//Return the IDs of available sources.
uint16_t SourceHandler::getSourceIDs(uint8_t* source_list) {
	uint16_t filled_source_count = 0;
	for(int i=0;i<this->source_count;i+=1) {
		if(this->source_list[i].source_id != 0 && this->source_list[i].sub_id == 0 && this->source_list[i].connected) {
			source_list[filled_source_count] = this->source_list[i].source_id;
			filled_source_count += 1;
		}
	}

	return filled_source_count;
}

//Get the first occurrence of a source ID.
int SourceHandler::getFirstOccurenceOf(const uint8_t source) {
	return getFirstOccurenceOf(source, 0);
}

//Get the first occurrence of a source ID after index s.
int SourceHandler::getFirstOccurenceOf(const uint8_t source, const uint16_t s) {
	for(int i=s;i<this->source_count;i+=1) {
		if(this->source_list[i].source_id == source)
			return i;
	}
	return -1;
}

//Get the indices of all sources with ID source.
int SourceHandler::getSubsourceIDs(const uint8_t source, uint16_t* index) {
	int ptr = 0;
	for(int i=0;i<source_count;i+=1) {
		if(source_list[i].source_id == source) {
			index[ptr] = i;
			ptr += 1;
		}
	}

	return ptr;
}

//Get the first available source index.
int SourceHandler::getFirstAvailable() {
	return getFirstAvailable(0);
}

//Get the first available source index after index s.
int SourceHandler::getFirstAvailable(const uint16_t s) {
	for(int i=s;i<this->source_count;i+=1) {
		if(this->source_list[i].source_id == 0)
			return i;
	}
	return -1;
}

//Send a query to a missed source.
bool SourceHandler::sendSourceQuery(const uint8_t source) {
	if(query)
		return true;

	query = true;

	uint8_t query_data[] = {0x4, 0xE6, 0x10};
	AIData query_msg(sizeof(query_data), ID_RADIO, source, query_data);

	ai_handler->writeAIData(&query_msg, false);

	bool source_responded = false;

	elapsedMillis source_timer;
	while(source_timer < 100) {
		if(ai_handler->dataAvailable(false) > 0) {
			AIData ai_d;
			if(ai_handler->readAIData(&ai_d, false)) {
				if(ai_d.receiver == ID_RADIO) {
					handleAIBus(&ai_d);
					if(ai_d.sender == source)
						source_responded = true;
				} else if(ai_d.receiver == 0xFF)
					ai_handler->cacheMessage(&ai_d);

				//source_timer = 0;
			}
		}
	}

	query = false;
	return source_responded;
}

//Clear any open audio menu.
bool SourceHandler::clearMenu() {
	uint8_t data[] = {0x2B, 0x4A};
	AIData clear_msg(sizeof(data), ID_RADIO, ID_NAV_COMPUTER, data);

	const bool ack = ai_handler->writeAIData(&clear_msg);
	if(!ack)
		return false;

	elapsedMillis clear_wait;
	while(clear_wait < 50) {
		AIData clear_msg;
		if(ai_handler->readAIData(&clear_msg)) {
			if(clear_msg.receiver == ID_RADIO && clear_msg.sender == ID_NAV_COMPUTER &&
											clear_msg.l >= 2 &&
											clear_msg[0] == 0x2B && clear_msg[1] == 0x40) { //Clear message.
				ai_handler->sendAcknowledgement(ID_RADIO, ID_NAV_COMPUTER);
				menu_open = NO_MENU;
				tuner_background->setSeekMode(true);
				return true;
			}
		}
	}

	return false;
}

//Send the initial request to create a menu. Return whether creation is allowed.
bool SourceHandler::createMenu(const String title, const int items) {
	uint8_t menu_header_data[12 + title.length()];

	const uint16_t width = parameter_list->screen_w;

	menu_header_data[0] = 0x2B;
	menu_header_data[1] = 0x5A;
	menu_header_data[2] = items&0xFF;
	menu_header_data[3] = items&0xFF;
	menu_header_data[4] = 0x0;
	menu_header_data[5] = 0x0;
	menu_header_data[6] = 0x0;
	menu_header_data[7] = 0x8C;
	menu_header_data[8] = (width&0xFF00)>>8;
	menu_header_data[9] = width&0xFF;
	menu_header_data[10] = (parameter_list->option_height&0xFF00)>>8;
	menu_header_data[11] = parameter_list->option_height&0xFF;
	for(int i=0;i<title.length();i+=1)
		menu_header_data[i+12] = uint8_t(title.charAt(i));

	AIData menu_header(sizeof(menu_header_data), ID_RADIO, ID_NAV_COMPUTER, menu_header_data);
	bool ack = ai_handler->writeAIData(&menu_header, parameter_list->computer_connected);

	if(!ack)
		return false;

	//Confirm that the nav computer does not respond with a "no menu" message.
	elapsedMillis no_draw;
	while(no_draw < 50) {
		AIData no_msg;
		if(ai_handler->readAIData(&no_msg)) {
			if(no_msg.receiver == ID_RADIO && no_msg.sender == ID_NAV_COMPUTER &&
											no_msg.l >= 2 &&
											no_msg.data[0] == 0x2B && no_msg.data[1] == 0x40) { //No menu message.
				ai_handler->sendAcknowledgement(ID_RADIO, ID_NAV_COMPUTER);
				return false;
			} else if(no_msg.receiver == ID_RADIO) {
				ai_handler->sendAcknowledgement(ID_RADIO, no_msg.sender);
				ai_handler->cacheMessage(&no_msg);
			}
		}
	}

	return true;
}

//Create the source menu.
void SourceHandler::createSourceMenu() {
	AudioSource active_list[source_count];
	const uint16_t active_count = getFilledSources(active_list);
	
	if(menu_open != NO_MENU) {
		if(!clearMenu())
			return;
	}

	if(!createMenu(getMenu(MENU_INDEX_SOURCES, parameter_list->locale).title, active_count))
		return;
	
	for(int i=0;i<active_count;i+=1) {
		uint8_t option_data[3 + active_list[i].source_name.length()];
		option_data[0] = 0x2B;
		option_data[1] = 0x51;
		option_data[2] = i&0xFF;
		for(int j=0;j<active_list[i].source_name.length();j+=1)
			option_data[j+3] = uint8_t(active_list[i].source_name.charAt(j));

		AIData option_msg(sizeof(option_data), ID_RADIO, ID_NAV_COMPUTER, option_data);
		bool ack = ai_handler->writeAIData(&option_msg);
		if(!ack)
			return;
	}

	uint8_t display_data[] = {0x2B, 0x52, 0x1};
	AIData display_msg(sizeof(display_data), ID_RADIO, ID_NAV_COMPUTER, display_data);
	bool ack = ai_handler->writeAIData(&display_msg);
	if(ack)
		menu_open = SOURCE_MENU;
}

//Create a menu for presets.
void SourceHandler::createPresetMenu(const uint8_t group) {
	uint16_t* preset_list;
	if(group == SUB_FM1)
		preset_list = parameter_list->fm1_presets;
	else if(group == SUB_FM2)
		preset_list = parameter_list->fm2_presets;
	else if(group == SUB_AM)
		preset_list = parameter_list->am_presets;
	else
		return;

	if(menu_open != NO_MENU) {
		if(!clearMenu())
			return;
	}

	if(!createMenu(getMenu(MENU_INDEX_RADIO_MAIN_MENU, parameter_list->locale).getLocalEntry(MENU_INDEX_RADIO_MAIN_MENU_PRESETS), 6))
		return;

	for(int i=0;i<6;i+=1) {
		String preset = String(i+1) + ". ";
		if(group == SUB_AM)
			preset += String(preset_list[i]) + " kHz";
		else {
			if(preset_list[i]%100 >= 10)
				preset += String(preset_list[i]/100) + "." + String(preset_list[i]%100) + " MHz";
			else
				preset += String(preset_list[i]/100) + ".0" + String(preset_list[i]%100) + " MHz";
		}

		uint8_t option_data[3 + preset.length()];
		option_data[0] = 0x2B;
		option_data[1] = 0x51;
		option_data[2] = i&0xFF;
		for(int j=0;j<preset.length();j+=1)
			option_data[j+3] = uint8_t(preset.charAt(j));

		AIData option_msg(sizeof(option_data), ID_RADIO, ID_NAV_COMPUTER, option_data);
		ai_handler->writeAIData(&option_msg);
	}

	uint8_t display_data[] = {0x2B, 0x52, 0x1};
	AIData display_msg(sizeof(display_data), ID_RADIO, ID_NAV_COMPUTER, display_data);
	bool ack = ai_handler->writeAIData(&display_msg);
	if(ack)
		menu_open = PRESET_MENU;
}

//Create the station list menu.
void SourceHandler::createStationListMenu() {
	String station_list[MAXIMUM_FREQUENCY_COUNT];
	const int station_count = tuner_background->getStationNames(station_list);

	if(station_count <= 0)
		return;

	if(menu_open != NO_MENU) {
		if(!clearMenu())
			return;
	}

	if(!createMenu(getMenu(MENU_INDEX_RADIO_MAIN_MENU, parameter_list->locale).getLocalEntry(MENU_INDEX_RADIO_MAIN_MENU_STATION_LIST), station_count))
		return;

	tuner_background->setSeekMode(false);

	for(int i=0;i<station_count;i+=1) {
		String station_name = station_list[i];

		uint8_t option_data[3 + station_name.length()];
		option_data[0] = 0x2B;
		option_data[1] = 0x51;
		option_data[2] = i&0xFF;
		for(int j=0;j<station_name.length();j+=1)
			option_data[j+3] = uint8_t(station_name.charAt(j));

		AIData option_msg(sizeof(option_data), ID_RADIO, ID_NAV_COMPUTER, option_data);
		ai_handler->writeAIData(&option_msg);
	}

	uint8_t display_data[] = {0x2B, 0x52, 0x1};
	AIData display_msg(sizeof(display_data), ID_RADIO, ID_NAV_COMPUTER, display_data);
	bool ack = ai_handler->writeAIData(&display_msg);
	if(ack)
		menu_open = STATION_MENU;
}

//Create the tone menu.
void SourceHandler::createToneMenu() {
	if(menu_open != NO_MENU) {
		if(!clearMenu())
			return;
	}

	if(!createMenu(getMenu(MENU_INDEX_TONE, parameter_list->locale).title, 5))
		return;
	
	for(int i=0;i<5;i+=1)
		createToneMenuItem(i);

	uint8_t display_data[] = {0x2B, 0x52, 0x1};
	AIData display_msg(sizeof(display_data), ID_RADIO, ID_NAV_COMPUTER, display_data);
	bool ack = ai_handler->writeAIData(&display_msg);
	if(ack)
		menu_open = TONE_MENU;
}

//Create an item in the tone menu.
void SourceHandler::createToneMenuItem(const int item) {
	MenuList tone_menu = getMenu(MENU_INDEX_TONE, parameter_list->locale);
	const uint8_t slider_max = DEFAULT_SLIDER_RANGE;
	uint8_t slider_pos = 0xFF;
	
	int slider_parameter = 0;
	String option_text = "";

	switch(item) {
	case TONE_OPTION_BASS:
		option_text = tone_menu.getLocalEntry(MENU_INDEX_TONE_BASS);
		slider_parameter = volume_handler->getBass();
		break;
	case TONE_OPTION_TREBLE:
		option_text = tone_menu.getLocalEntry(MENU_INDEX_TONE_TREBLE);
		slider_parameter = volume_handler->getTreble();
		break;
	case TONE_OPTION_BALANCE:
		option_text = tone_menu.getLocalEntry(MENU_INDEX_TONE_BALANCE);
		slider_parameter = volume_handler->getBalance();
		break;
	case TONE_OPTION_FADER:
		option_text = tone_menu.getLocalEntry(MENU_INDEX_TONE_FADER);
		slider_parameter = volume_handler->getFader();
		break;
	case TONE_OPTION_SVC:
		option_text = tone_menu.getLocalEntry(MENU_INDEX_TONE_SVC);
		break;
	}

	String slider_text = "";

	if(item == TONE_OPTION_BASS || item == TONE_OPTION_TREBLE) {
		slider_pos = slider_parameter*slider_max/DEFAULT_TONE_RANGE;
		const long slider_parameter_long = slider_parameter;

		if((DEFAULT_TONE_RANGE - slider_parameter_long)*MAX_ATTENUATION*10/DEFAULT_TONE_RANGE > 0)
			slider_text += "-";
		slider_text += String((DEFAULT_TONE_RANGE - slider_parameter_long)*MAX_ATTENUATION/DEFAULT_TONE_RANGE) + ".";
		slider_text += ((DEFAULT_TONE_RANGE - slider_parameter_long)*10*MAX_ATTENUATION/DEFAULT_TONE_RANGE%10);

		slider_text += "dB";
	} else if(item == TONE_OPTION_BALANCE || item == TONE_OPTION_FADER) {
		if(slider_parameter < 0) {
			if(item == TONE_OPTION_BALANCE)
				slider_text += "L";
			else
				slider_text += "B";
		} else if(slider_parameter > 0) {
			if(item == TONE_OPTION_BALANCE)
				slider_text += "R";
			else
				slider_text += "F";
		} else
			slider_text += "C";

		if(slider_parameter != 0)
			slider_text += abs(slider_parameter)*slider_max/DEFAULT_TONE_RANGE;

		slider_parameter += DEFAULT_TONE_RANGE/2;
		slider_pos = slider_parameter*slider_max/DEFAULT_TONE_RANGE;
	}

	uint8_t option_data[3 + option_text.length()];
	option_data[0] = 0x2B;
	option_data[1] = 0x51;
	option_data[2] = item&0xFF;
	for(int i=0;i<option_text.length();i+=1)
		option_data[i+3] = uint8_t(option_text.charAt(i));

	AIData option_msg(sizeof(option_data), ID_RADIO, ID_NAV_COMPUTER, option_data);
	ai_handler->writeAIData(&option_msg, parameter_list->computer_connected);
	
	if(item < TONE_OPTION_SVC) {
		uint8_t slider_data[6 + slider_text.length()];
		slider_data[0] = 0x2B;
		slider_data[1] = 0x54;
		slider_data[2] = item&0xFF;
		slider_data[3] = slider_pos;
		slider_data[4] = slider_max;
		slider_data[5] = 0;

		switch(item) {
		case TONE_OPTION_BASS:
			if(parameter_list->bass_adjust)
				slider_data[5] = 1;
			break;
		case TONE_OPTION_TREBLE:
			if(parameter_list->treble_adjust)
				slider_data[5] = 1;
			break;
		case TONE_OPTION_BALANCE:
			if(parameter_list->balance_adjust)
				slider_data[5] = 1;
			break;
		case TONE_OPTION_FADER:
			if(parameter_list->fader_adjust)
				slider_data[5] = 1;
			break;
		}
		
		for(int i=0;i<slider_text.length();i+=1)
			slider_data[i+6] = uint8_t(slider_text.charAt(i));

		AIData slider_msg(sizeof(slider_data), ID_RADIO, ID_NAV_COMPUTER, slider_data);

		ai_handler->writeAIData(&slider_msg, parameter_list->computer_connected);
	}
}

//Create the radio settings menu.
void SourceHandler::createRadioSettingsMenu() {
	if(menu_open != NO_MENU) {
		if(!clearMenu())
			return;
	}
	
	const MenuList settings_menu = getMenu(MENU_INDEX_RADIO_SETTINGS, parameter_list->locale);
	if(!createMenu(settings_menu.title, settings_menu.size()))
		return;

	for(int i=0;i<settings_menu.size();i+=1)
		createRadioSettingsMenuItem(i);

	uint8_t display_data[] = {0x2B, 0x52, 0x1};
	AIData display_msg(sizeof(display_data), ID_RADIO, ID_NAV_COMPUTER, display_data);
	bool ack = ai_handler->writeAIData(&display_msg);
	if(ack)
		menu_open = RADIO_SETTINGS_MENU;
}

//Create an item in the radio settings menu.
void SourceHandler::createRadioSettingsMenuItem(const int item) {
	const MenuList settings_menu = getMenu(MENU_INDEX_RADIO_SETTINGS, parameter_list->locale);
	String setting = settings_menu[item];
	if(item==settings_menu.getLocalIndex(MENU_INDEX_RADIO_SETTINGS_RDS_FLASH)) {
		if(parameter_list->imid_char <= 0 || parameter_list->imid_lines != 1)
			setting = parameter_list->header_rds_setting != HEADER_RDS_OFF ? "#ROF" : "#RON" + (" " + setting);
	}

	uint8_t setting_data[3+setting.length()];
	setting_data[0] = 0x2B;
	setting_data[1] = 0x51;
	setting_data[2] = uint8_t(item);
	for(int i=0;i<setting.length();i+=1)
		setting_data[i+3] = uint8_t(setting[i]);

	AIData setting_msg(sizeof(setting_data), ID_RADIO, ID_NAV_COMPUTER, setting_data);
	ai_handler->writeAIData(&setting_msg, parameter_list->computer_connected);
}

//Create the RDS flash settings menu.
void SourceHandler::createRDSFlashMenu() {
	if(menu_open != NO_MENU) {
		if(!clearMenu())
			return;
	}
	
	if(!clearMenu())
		return;

	const MenuList rds_flash_menu = getMenu(MENU_INDEX_RDS_FLASH_SETTINGS, parameter_list->locale);
	if(!createMenu(rds_flash_menu.title, rds_flash_menu.size()))
		return;

	for(int i=0;i<rds_flash_menu.size();i+=1) {
		String setting = rds_flash_menu[i];
		bool setting_sel = false;
		switch(rds_flash_menu.getGlobalIndex(i)) {
		case MENU_INDEX_RDS_FLASH_SETTINGS_OFF:
			if(parameter_list->header_rds_setting == HEADER_RDS_OFF)
				setting_sel = true;
			break;
		case MENU_INDEX_RDS_FLASH_SETTINGS_INFO_MODE:
			if(parameter_list->header_rds_setting == HEADER_RDS_INFO_MODE)
				setting_sel = true;
			break;
		case MENU_INDEX_RDS_FLASH_SETTINGS_ALWAYS:
			if(parameter_list->header_rds_setting == HEADER_RDS_ALWAYS)
				setting_sel = true;
			break;
		}

		if(setting_sel)
			setting = "#CON " + setting;
		else
			setting = "#COF " + setting;

		AIData setting_msg(setting.length() + 3, ID_RADIO, ID_NAV_COMPUTER);
		setting_msg[0] = 0x2B;
		setting_msg[1] = 0x51;
		setting_msg[2] = uint8_t(i);
		for(int d=0;d<setting.length();d+=1)
			setting_msg[d+3] = uint8_t(setting[d]);

		ai_handler->writeAIData(&setting_msg, parameter_list->computer_connected);
	}

	uint8_t display_data[] = {0x2B, 0x52, 0x1};
	AIData display_msg(sizeof(display_data), ID_RADIO, ID_NAV_COMPUTER, display_data);
	bool ack = ai_handler->writeAIData(&display_msg);
	if(ack)
		menu_open = RDS_FLASH_MENU;
}

//Set the current source to the desired ID and sub ID.
void SourceHandler::setCurrentSource(const uint8_t id, const uint8_t sub_id) {
	if(!audio_on)
		audio_on = true;

	int index = -1;
	for(int i=0;i<source_count;i+=1) {
		if(source_list[i].source_id == id && source_list[i].sub_id == sub_id) {
			index = i;
			break;
		}
	}

	if(index >= 0)
		current_source = index;
}

//Handle a steering wheel audio control.
void SourceHandler::handleSteeringControl(const uint8_t command, const uint8_t state) {
	const uint8_t button_state = state>>6, knob_state = state&0x3F;

	if(command == 0x6 && button_state == 0 && parameter_list->audio_on) { //Volume.
		//TODO: Send the command to the amp if relevant.
		const uint16_t volume = volume_handler->getVolume();
		if(knob_state == 0x1 && volume < volume_handler->getVolRange())
			volume_handler->setVolume(volume + 1);
		else if(knob_state == 0x2 && volume > 0)
			volume_handler->setVolume(volume - 1);
	} else if(command == 0x23 && button_state == 0x0 && parameter_list->audio_on) { //Source button.
		incrementSource();
	} else if((command == 0x25 || command == 0x24) && button_state == 0x0) { //Increment/decrement.
		const uint8_t source = getCurrentSourceID();
		if(source == ID_RADIO) {
			const uint8_t sub = source_list[current_source].sub_id;
			int8_t new_preset = parameter_list->current_preset;
			
			if(command == 0x24)
				new_preset -= 1;
			else
				new_preset += 1;
				
			if(new_preset <= 0)
				new_preset = PRESET_COUNT;
			if(new_preset > PRESET_COUNT)
				new_preset = 1;

			parameter_list->preferred_preset = new_preset;
				
			if(sub == SUB_FM1) {
				tuner_main->setFrequency(parameter_list->fm1_presets[new_preset - 1]);
				parameter_list->fm1_tune = tuner_main->getFrequency();
			} else if(sub == SUB_FM2) {
				tuner_main->setFrequency(parameter_list->fm2_presets[new_preset - 1]);
				parameter_list->fm2_tune = tuner_main->getFrequency();
			} else if(sub == SUB_AM) {
				tuner_main->setFrequency(parameter_list->am_presets[new_preset - 1]);
				parameter_list->am_tune = tuner_main->getFrequency();
			}
			
		} else if(source == ID_TAPE) {
			uint8_t command_data[] = {0x28, 0x7, 0x0, 0x1}; //TODO: Track count.
			
			if(command == 0x24)
				command_data[2] = 0x1;

			AIData command_msg(sizeof(command_data), ID_RADIO, ID_TAPE, command_data);
			ai_handler->writeAIData(&command_msg);
		} else if(source != 0) {
			uint8_t command_data[] = {0x38, 0xA, 0x0};

			if(source == ID_XM)
				command_data[1] = 0x7;

			if(command == 0x24)
				command_data[2] = 0x1;
			
			AIData command_msg(sizeof(command_data), ID_RADIO, source, command_data);
			ai_handler->writeAIData(&command_msg);
		}
	}
}

//Save frequency freq to preset to FM1 (group = 0), FM2 (group = 1), or AM (group = 2).
void SourceHandler::savePreset(const uint16_t freq, const uint8_t preset, const uint8_t group) {
	if(preset >= PRESET_COUNT)
		return;

	if(group == SUB_FM1)
		parameter_list->fm1_presets[preset] = freq;
	else if(group == SUB_FM2)
		parameter_list->fm2_presets[preset] = freq;
	else if(group == SUB_AM)
		parameter_list->am_presets[preset] = freq;
		
	parameter_list->current_preset = 0;
}
