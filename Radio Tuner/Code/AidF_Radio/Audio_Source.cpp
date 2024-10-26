#include "Audio_Source.h"

SourceHandler::SourceHandler(AIBusHandler* ai_handler, Si4735Controller* tuner_main, BackgroundTuneHandler* tuner_background, ParameterList* parameter_list, uint16_t source_count) {
	this->source_count = source_count;
	this->source_list = new AudioSource[source_count];

	for(uint16_t i=0;i<source_count;i+=1) {
		source_list[i].sub_id = 0;
		source_list[i].source_id = 0;
		source_list[i].source_name = "";
	}

	this->ai_handler = ai_handler;
	this->parameter_list = parameter_list;
	this->tuner_main = tuner_main;
	this->tuner_background = tuner_background;
}

SourceHandler::~SourceHandler() {
	delete[] source_list;
}

//Get the ID of the currently-selected source.
uint8_t SourceHandler::getCurrentSourceID() {
	if(audio_on)
		return this->source_list[this->current_source].source_id;
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
	this->current_source = source;
}

//Set the selected source by ID. Returns false if that ID does not exist.
bool SourceHandler::setSourceID(const uint8_t source) {
	int new_source = getFirstOccurenceOf(source);
	if(new_source < 0)
		return false;
	else {
		this->current_source = new_source;
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

//Call if the car is turned on or off.
void SourceHandler::setPower(const bool power) {
	if(!power) {
		this->audio_on = false;
	} else {
		
	}
}

//Send the initial radio handshake message.
void SourceHandler::sendRadioHandshake() {
	AIData handshake_msg(3, ID_RADIO, 0xFF);
	uint8_t data[] = {0x4, 0xE6, 0x10};
	handshake_msg.refreshAIData(data);

	ai_handler->writeAIData(&handshake_msg);
}

//Handle AIBus data.
bool SourceHandler::handleAIBus(AIData* ai_d) {
	bool ack = true;

	if(ai_d->data[0] == 0x1 && ai_d->l >= 2) { //Handshake message.
		if(ai_d->data[1] == 0x1) { //First message from this source.
			if(ai_d->sender != ai_d->data[2]) {
				ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
				return true;
			}

			const int new_index = getFirstAvailable();
			if(new_index < 0) {
				ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
				return true;
			}
			
			const uint8_t new_id = ai_d->sender;
			if(getFirstOccurenceOf(new_id) >= 0) {
				ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
				//TODO: Clear subsources?
				return true;
			}

			parameter_list->handshake_timer = 0;
			parameter_list->handshake_timer_active = true;
			parameter_list->handshake_sources.push_back(new_id);

			AudioSource new_source;
			new_source.source_id = new_id;
			new_source.source_name = "";
			new_source.sub_id = 0;

			this->source_list[new_index] = new_source;

			if(ai_d->l >= 4) {
				const uint8_t sub_count = ai_d->data[3];
				for(uint8_t i=1;i<sub_count;i+=1)
					createSubsource(new_id);
			}

			//const uint8_t current_source_id = this->source_list[current_source].source_id, sub_id = this->source_list[current_source].sub_id;
			//uint8_t function_data[] = {0x40, 0x10, current_source_id, sub_id};
			//AIData function_msg(sizeof(function_data), ID_RADIO, new_id);
			//function_msg.refreshAIData(function_data);
			//ai_handler->writeAIData(&function_msg);
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
			AIData handshake_msg(sizeof(handshake_data), ID_RADIO, ai_d->sender);
			handshake_msg.refreshAIData(handshake_data);
			ai_handler->writeAIData(&handshake_msg);
		} else if(ai_d->data[1] == 0x23) { //Source name.
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

			this->source_list[index].source_name = name;
		}
		ack = false;
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
		return true;
		
	} else if(ai_d->l >= 3 && ai_d->data[0] == 0x10 && ai_d->data[1] == 0x10) { //Source request control.
		ack = false;
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
		
		const uint8_t new_src = ai_d->data[2];
		uint8_t new_sub = 0;
		
		if(ai_d->l >= 4)
			new_sub = ai_d->data[3];
		
		setCurrentSource(new_src, new_sub);
	} else if(ai_d->sender == ID_NAV_SCREEN) { //Screen message.
		ack = false;
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
		if(ai_d->data[0] == 0x30) { //Button press.
			const uint8_t button = ai_d->data[1], state = ai_d->data[2]>>6;

			if(parameter_list->manual_tune_mode && (button != 0x7 && button != 0x2A && button != 0x2B)) {
				parameter_list->manual_tune_mode = false;
				sendManualTuneMessage();
				
				{
					uint8_t data[] = {0x77, parameter_list->last_control, 0x10};

					AIData screen_msg(sizeof(data), ID_RADIO, ID_NAV_SCREEN);
					screen_msg.refreshAIData(data);
					ai_handler->writeAIData(&screen_msg, parameter_list->screen_connected);
				}
			}

			if(button == 0x6 && state == 2) //Press volume knob.
				audio_on = !audio_on;
			else if(button == 0x23 && state == 2) { //Source button.
				if(parameter_list->vehicle_speed <= 5 && parameter_list->computer_connected) { //Car is stopped.
					createSourceMenu();
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
					parameter_list->tune_changed = true;
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

					AIData screen_msg(sizeof(data), ID_RADIO, ID_NAV_SCREEN);
					screen_msg.refreshAIData(data);
					ai_handler->writeAIData(&screen_msg, parameter_list->screen_connected);
				}
			} else if(button == 0x53 && state == 2) { //Info button.
				if(parameter_list->imid_char > 0 && parameter_list->imid_lines == 1 && getCurrentSourceID() == ID_RADIO)
					parameter_list->info_mode = !parameter_list->info_mode;
				else
					parameter_list->info_mode = false;
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

					if((source_list[s].source_id != ID_RADIO && source_list[s].source_id != 0) || (source_list[s].source_id == ID_RADIO && source_list[s].sub_id > 2)) {
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

					if(source_list[s].source_id == ID_TAPE) {
						new_src = s;
						source_found = true;
					} else if(source_list[s].source_id == ID_CDC || source_list[s].source_id == ID_CD) {
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
			} else if(button >= 0x11 && button <= 0x16) { //Presets.
				const uint8_t preset = (button&0xF) - 1;
				if(source_list[current_source].source_id == ID_RADIO && source_list[current_source].sub_id <= 2) {
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
					}
				}
			}
			
		} else if(ai_d->data[0] == 0x32) { //Knob turn.
			const uint8_t steps = ai_d->data[2]&0xF;
			const bool clockwise = (ai_d->data[2]&0x10) != 0;
			if(ai_d->data[1] == 0x7) { //Function knob.
				if((this->parameter_list->manual_tune_mode || !this->parameter_list->computer_connected) && source_list[current_source].source_id == ID_RADIO) {
					manualTuneIncrement(clockwise, steps);
					parameter_list->tune_changed = true;
				}
			}
		}
		return true;
	} else if(ai_d->sender == ID_NAV_COMPUTER) { //Message from computer.
		ack = false;
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);
		if(ai_d->data[0] == 0x2B && ai_d->l >= 2) { //Menu related message.
			if(ai_d->data[1] == 0x40) { //A menu was cleared.
				menu_open = NO_MENU;
				return true;
			} if(ai_d->data[1] == 0x6A && ai_d->l >= 3) { //Audio menu item selected.
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

							AIData screen_msg(sizeof(data), ID_RADIO, ID_NAV_SCREEN);
							screen_msg.refreshAIData(data);
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
					AIData forward_msg(ai_d->l, ID_RADIO, source_id);
					forward_msg.refreshAIData(ai_d->data);
					ai_handler->writeAIData(&forward_msg);
				}
				return true;
			} else if(ai_d->data[1] == 0x60 && ai_d->l >= 3) { //True menu item selected.
				const uint8_t selection = ai_d->data[2]-1;
				if(menu_open == SOURCE_MENU) {
					AudioSource source_list[source_count];
					getFilledSources(source_list);
					const uint8_t new_id = source_list[selection].source_id, new_sub = source_list[selection].sub_id;
					clearMenu();
					setCurrentSource(new_id, new_sub);
				} else if(menu_open == PRESET_MENU) {
					if(source_list[current_source].source_id == ID_RADIO && source_list[current_source].sub_id <= 2) {
						const uint8_t group = source_list[current_source].sub_id;
						uint16_t freq = tuner_main->getFrequency();
						const uint8_t preset = selection + 1;
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
				}
				return true;
			} else if(ai_d->data[1] == 0x4A) {
				if(source_list[current_source].source_id == 0)
					return true;
				
				if(menu_open != NO_MENU)
					clearMenu();
				const uint8_t src = source_list[current_source].source_id;
				
				if(src != ID_RADIO) {
					uint8_t request_data[] = {0x2B, 0x4A};
					AIData request_msg(sizeof(request_data), ID_RADIO, src);
					request_msg.refreshAIData(request_data);
					
					ai_handler->writeAIData(&request_msg);
				}
			}
		}
	} else if(ai_d->sender == ID_STEERING_CTRL) { //Steering wheel control.
		ack = false;
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);

		if(ai_d->l >= 3 && ai_d->data[0] == 0x30) {
			this->handleSteeringControl(ai_d->data[1], ai_d->data[2]);
		}
	}

	if(ack)
		ai_handler->sendAcknowledgement(ID_RADIO, ai_d->sender);

	return false;
}

//Set the manual tuning message.
void SourceHandler::sendManualTuneMessage() {
	if(parameter_list->manual_tune_mode) { //TODO Integrate the text handler.
		AIData change_msg = getTextMessage("< Manual Tune >", 0xB, 0x0);
		ai_handler->writeAIData(&change_msg);
	} else {
		AIData change_msg = getTextMessage("Manual Tune", 0xB, 0x0);
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
	
	if(up)
		*current_frequency = this->tuner_main->incrementFrequency(steps);
	else
		*current_frequency = this->tuner_main->decrementFrequency(steps);
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
		while(this->source_list[new_source].source_id == 0) {
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
		while(this->source_list[new_source].source_id == 0) {
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
	for(uint16_t i=0;i<this->source_count;i+=1) {
		if(this->source_list[i].source_id != 0)
			filled_source_count += 1;
	}

	return filled_source_count;
}

//Return a list of available sources.
uint16_t SourceHandler::getFilledSources(AudioSource* source_list) {
	uint16_t filled_source_count = 0;
	for(uint16_t i=0;i<this->source_count;i+=1) {
		if(this->source_list[i].source_id != 0) {
			source_list[filled_source_count] = this->source_list[i];
			filled_source_count += 1;
		}
	}

	return filled_source_count;
}

//Return the names of available sources.
uint16_t SourceHandler::getSourceNames(String* source_list) {
	uint16_t filled_source_count = 0;
	for(uint16_t i=0;i<this->source_count;i+=1) {
		if(this->source_list[i].source_id != 0) {
			source_list[filled_source_count] = this->source_list[i].source_name;
			filled_source_count += 1;
		}
	}

	return filled_source_count;
}

//Return the IDs of available sources.
uint16_t SourceHandler::getSourceIDs(uint8_t* source_list) {
	uint16_t filled_source_count = 0;
	for(uint16_t i=0;i<this->source_count;i+=1) {
		if(this->source_list[i].source_id != 0 && this->source_list[i].sub_id == 0) {
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
	for(uint16_t i=s;i<this->source_count;i+=1) {
		if(this->source_list[i].source_id == source)
			return i;
	}
	return -1;
	
}

//Get the first available source index.
int SourceHandler::getFirstAvailable() {
	return getFirstAvailable(0);
}

//Get the first available source index after index s.
int SourceHandler::getFirstAvailable(const uint16_t s) {
	for(uint16_t i=s;i<this->source_count;i+=1) {
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
	AIData query_msg(sizeof(query_data), ID_RADIO, source);
	query_msg.refreshAIData(query_data);

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

				source_timer = 0;
			}
		}
	}

	query = false;
	return source_responded;
}

//Clear any open audio menu.
void SourceHandler::clearMenu() {
	uint8_t data[] = {0x2B, 0x4A};
	AIData clear_msg(sizeof(data), ID_RADIO, ID_NAV_COMPUTER);
	clear_msg.refreshAIData(data);

	const bool ack = ai_handler->writeAIData(&clear_msg);
	if(!ack)
		return;

	elapsedMillis clear_wait;
	while(clear_wait < 50) {
		AIData clear_msg;
		if(ai_handler->readAIData(&clear_msg)) {
			if(clear_msg.receiver == ID_RADIO && clear_msg.sender == ID_NAV_COMPUTER &&
											clear_msg.l >= 2 &&
											clear_msg.data[0] == 0x2B && clear_msg.data[1] == 0x40) { //Clear message.
				ai_handler->sendAcknowledgement(ID_RADIO, ID_NAV_COMPUTER);
				menu_open = NO_MENU;
				break;
			}
		}
	}
	tuner_background->setSeekMode(true);
}

//Create the source menu.
void SourceHandler::createSourceMenu() {
	AudioSource active_list[source_count];
	const uint16_t active_count = getFilledSources(active_list);
	
	String source_title = F("Source");
	uint8_t source_data[12 + source_title.length()];

	const uint16_t width = parameter_list->screen_w;

	source_data[0] = 0x2B;
	source_data[1] = 0x5A;
	source_data[2] = active_count&0xFF;
	source_data[3] = active_count&0xFF;
	source_data[4] = 0x0;
	source_data[5] = 0x0;
	source_data[6] = 0x0;
	source_data[7] = 0x8C;
	source_data[8] = (width&0xFF00)>>8;
	source_data[9] = width&0xFF;
	source_data[10] = 0x0;
	source_data[11] = 0x23;
	for(uint8_t i=0;i<source_title.length();i+=1)
		source_data[i+12] = uint8_t(source_title.charAt(i));

	AIData menu_header(sizeof(source_data), ID_RADIO, ID_NAV_COMPUTER);
	menu_header.refreshAIData(source_data);
	bool ack = ai_handler->writeAIData(&menu_header);

	if(!ack)
		return;

	//Confirm that the nav computer does not respond with a "no menu" message.
	elapsedMillis no_draw;
	while(no_draw < 50) {
		AIData no_msg;
		if(ai_handler->readAIData(&no_msg)) {
			if(no_msg.receiver == ID_RADIO && no_msg.sender == ID_NAV_COMPUTER &&
											no_msg.l >= 2 &&
											no_msg.data[0] == 0x2B && no_msg.data[1] == 0x40) { //No menu message.
				ai_handler->sendAcknowledgement(ID_RADIO, ID_NAV_COMPUTER);
				return;
			} else if(no_msg.receiver == ID_RADIO) {
				ai_handler->sendAcknowledgement(ID_RADIO, no_msg.sender);
				ai_handler->cacheMessage(&no_msg);
			}
		}
	}
	
	for(uint16_t i=0;i<active_count;i+=1) {
		uint8_t option_data[3 + active_list[i].source_name.length()];
		option_data[0] = 0x2B;
		option_data[1] = 0x51;
		option_data[2] = i&0xFF;
		for(uint8_t j=0;j<active_list[i].source_name.length();j+=1)
			option_data[j+3] = uint8_t(active_list[i].source_name.charAt(j));

		AIData option_msg(sizeof(option_data), ID_RADIO, ID_NAV_COMPUTER);
		option_msg.refreshAIData(option_data);
		ack = ai_handler->writeAIData(&option_msg);
		if(!ack)
			return;
	}

	uint8_t display_data[] = {0x2B, 0x52, 0x1};
	AIData display_msg(sizeof(display_data), ID_RADIO, ID_NAV_COMPUTER);
	display_msg.refreshAIData(display_data);
	ack = ai_handler->writeAIData(&display_msg);
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

	String source_title = F("Presets");
	uint8_t source_data[12 + source_title.length()];

	const uint16_t width = parameter_list->screen_w;

	source_data[0] = 0x2B;
	source_data[1] = 0x5A;
	source_data[2] = 0x6;
	source_data[3] = 0x6;
	source_data[4] = 0x0;
	source_data[5] = 0x0;
	source_data[6] = 0x0;
	source_data[7] = 0x8C;
	source_data[8] = (width&0xFF00)>>8;
	source_data[9] = width&0xFF;
	source_data[10] = 0x0;
	source_data[11] = 0x23;
	for(uint8_t i=0;i<source_title.length();i+=1)
		source_data[i+12] = uint8_t(source_title.charAt(i));

	AIData menu_header(sizeof(source_data), ID_RADIO, ID_NAV_COMPUTER);
	menu_header.refreshAIData(source_data);
	bool ack = ai_handler->writeAIData(&menu_header);

	if(!ack)
		return;

	//Confirm that the nav computer does not respond with a "no menu" message.
	elapsedMillis no_draw;
	while(no_draw < 50) {
		AIData no_msg;
		if(ai_handler->readAIData(&no_msg)) {
			if(no_msg.receiver == ID_RADIO && no_msg.sender == ID_NAV_COMPUTER &&
											no_msg.l >= 2 &&
											no_msg.data[0] == 0x2B && no_msg.data[1] == 0x40) { //No menu message.
				ai_handler->sendAcknowledgement(ID_RADIO, ID_NAV_COMPUTER);
				return;
			} else if(no_msg.receiver == ID_RADIO) {
				ai_handler->sendAcknowledgement(ID_RADIO, no_msg.sender);
				ai_handler->cacheMessage(&no_msg);
			}
		}
	}

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
		for(uint8_t j=0;j<preset.length();j+=1)
			option_data[j+3] = uint8_t(preset.charAt(j));

		AIData option_msg(sizeof(option_data), ID_RADIO, ID_NAV_COMPUTER);
		option_msg.refreshAIData(option_data);
		ai_handler->writeAIData(&option_msg);
	}

	uint8_t display_data[] = {0x2B, 0x52, 0x1};
	AIData display_msg(sizeof(display_data), ID_RADIO, ID_NAV_COMPUTER);
	display_msg.refreshAIData(display_data);
	ack = ai_handler->writeAIData(&display_msg);
	if(ack)
		menu_open = PRESET_MENU;
}

//Create the station list menu.
void SourceHandler::createStationListMenu() {
	String station_list[MAXIMUM_FREQUENCY_COUNT];
	const int station_count = tuner_background->getStationNames(station_list);

	if(station_count <= 0)
		return;

	String source_title = F("Stations");
	uint8_t source_data[12 + source_title.length()];

	const uint16_t width = parameter_list->screen_w;

	source_data[0] = 0x2B;
	source_data[1] = 0x5A;
	source_data[2] = station_count&0xFF;
	source_data[3] = station_count&0xFF;
	source_data[4] = 0x0;
	source_data[5] = 0x0;
	source_data[6] = 0x0;
	source_data[7] = 0x8C;
	source_data[8] = (width&0xFF00)>>8;
	source_data[9] = width&0xFF;
	source_data[10] = 0x0;
	source_data[11] = 0x23;
	for(uint8_t i=0;i<source_title.length();i+=1)
		source_data[i+12] = uint8_t(source_title.charAt(i));

	AIData menu_header(sizeof(source_data), ID_RADIO, ID_NAV_COMPUTER);
	menu_header.refreshAIData(source_data);
	bool ack = ai_handler->writeAIData(&menu_header);

	if(!ack)
		return;

	//Confirm that the nav computer does not respond with a "no menu" message.
	elapsedMillis no_draw;
	while(no_draw < 50) {
		AIData no_msg;
		if(ai_handler->readAIData(&no_msg)) {
			if(no_msg.receiver == ID_RADIO && no_msg.sender == ID_NAV_COMPUTER &&
											no_msg.l >= 2 &&
											no_msg.data[0] == 0x2B && no_msg.data[1] == 0x40) { //No menu message.
				ai_handler->sendAcknowledgement(ID_RADIO, ID_NAV_COMPUTER);
				return;
			} else if(no_msg.receiver == ID_RADIO) {
				ai_handler->sendAcknowledgement(ID_RADIO, no_msg.sender);
				ai_handler->cacheMessage(&no_msg);
			}
		}
	}

	tuner_background->setSeekMode(false);

	for(int i=0;i<station_count;i+=1) {
		String station_name = station_list[i];

		uint8_t option_data[3 + station_name.length()];
		option_data[0] = 0x2B;
		option_data[1] = 0x51;
		option_data[2] = i&0xFF;
		for(uint8_t j=0;j<station_name.length();j+=1)
			option_data[j+3] = uint8_t(station_name.charAt(j));

		AIData option_msg(sizeof(option_data), ID_RADIO, ID_NAV_COMPUTER);
		option_msg.refreshAIData(option_data);
		ai_handler->writeAIData(&option_msg);
	}

	uint8_t display_data[] = {0x2B, 0x52, 0x1};
	AIData display_msg(sizeof(display_data), ID_RADIO, ID_NAV_COMPUTER);
	display_msg.refreshAIData(display_data);
	ack = ai_handler->writeAIData(&display_msg);
	if(ack)
		menu_open = STATION_MENU;
}

//Set the current source to the desired ID and sub ID.
void SourceHandler::setCurrentSource(const uint8_t id, const uint8_t sub_id) {
	if(!audio_on)
		audio_on = true;

	int index = -1;
	for(uint16_t i=0;i<source_count;i+=1) {
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

	if(command == 0x6) { //Volume.
		if(knob_state == 0x1) { //Volume up.
			//TODO: Send volume up command to amp.
		} else if(knob_state == 0x2) { //Volume down.
			//TODO: Send volume down command to amp.
		}
	} else if(command == 0x23 && button_state == 0x0) { //Source button.
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

			AIData command_msg(sizeof(command_data), ID_RADIO, ID_TAPE);
			command_msg.refreshAIData(command_data);
			ai_handler->writeAIData(&command_msg);
		} else if(source != 0) {
			uint8_t command_data[] = {0x38, 0xA, 0x0};

			if(source == ID_XM)
				command_data[1] = 0x7;

			if(command == 0x24)
				command_data[2] = 0x1;
			
			AIData command_msg(sizeof(command_data), ID_RADIO, source);
			command_msg.refreshAIData(command_data);
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
