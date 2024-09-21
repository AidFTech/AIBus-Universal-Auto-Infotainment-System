#include "Button_Handler.h"

ButtonHandler::ButtonHandler(MCP23S08* input_mcp, MCP23S08* output_mcp, AIBusHandler* ai_handler, ParameterList* parameters, MCP23S08* vol_mcp, const int8_t vol_pin) {
	this->ai_handler = ai_handler;
	
	this->input_mcp = input_mcp;
	this->output_mcp = output_mcp;
	this->vol_mcp = vol_mcp;

	this->vol_pin = vol_pin;

	this->parameters = parameters;

	this->button_states = parameters->button_states;
	this->button_timers = parameters->button_timers;

	for(int i=0;i<BUTTON_INDEX_SIZE;i+=1)
		button_states[i] = BUTTON_STATE_RELEASED;
}

void ButtonHandler::loop() {
	checkButtonPress();
	checkButtonHold();
}

void ButtonHandler::checkButtonPress() {
	for(uint8_t x=0;x<4;x+=1) { //Check each output COM.
		output_mcp->digitalWriteIO(x, LOW);
		
		for(uint8_t y=0;y<5;y+=1) { //Check each input COM.
			const bool state = !input_mcp->digitalReadIO(y);
			const int index = getButtonIndex(x, y);

			if(index < 0 || index >= BUTTON_INDEX_SIZE)
				continue;

			uint8_t* recipient = &parameters->all_dest;
			switch(index) {
			case BUTTON_INDEX_AMFM:
			case BUTTON_INDEX_CDXM:
				recipient = &parameters->audio_dest;
				break;
			case BUTTON_INDEX_SCAN:
			case BUTTON_INDEX_SKIPUP:
			case BUTTON_INDEX_SKIPDOWN:
			case BUTTON_INDEX_PRESET1:
			case BUTTON_INDEX_PRESET2:
			case BUTTON_INDEX_PRESET3:
			case BUTTON_INDEX_PRESET4:
			case BUTTON_INDEX_PRESET5:
			case BUTTON_INDEX_PRESET6:
			case BUTTON_INDEX_INFO:
				recipient = &parameters->source_dest;
				break;
			}

			if(index != BUTTON_INDEX_ZOOMIN && index != BUTTON_INDEX_ZOOMOUT) {
				if((index != BUTTON_INDEX_AMFM || !amfm_pressed) && (index != BUTTON_INDEX_CDXM || !cdxm_pressed)) {
					if(state && button_states[index] == BUTTON_STATE_RELEASED) { //Button pressed.
						button_timers[index] = 0;
						button_states[index] = BUTTON_STATE_PRESSED;
						sendButtonMessage(getButtonCode(index), BUTTON_STATE_PRESSED, *recipient);
					} else if(!state && button_states[index] != BUTTON_STATE_RELEASED) { //Button released.
						button_states[index] = BUTTON_STATE_RELEASED;
						sendButtonMessage(getButtonCode(index), BUTTON_STATE_RELEASED, *recipient);
					}
				} else {
					if(!state && index == BUTTON_INDEX_AMFM)
						amfm_pressed = false;
					else if(!state && index == BUTTON_INDEX_CDXM)
						cdxm_pressed = false;

					if(!state)
						button_states[index] = BUTTON_STATE_RELEASED;
				}
			} else {
				if(state && button_states[index] == BUTTON_STATE_RELEASED) {
					uint8_t scroll_data[] = {0x32, 0x7, 0x1};
					if(index == BUTTON_INDEX_ZOOMIN)
						scroll_data[2] |= 0x10;

					AIData scroll_msg(sizeof(scroll_data), ID_NAV_SCREEN, *recipient);
					scroll_msg.refreshAIData(scroll_data);

					bool ack = true;
					if(*recipient == ID_NAV_COMPUTER && !parameters->computer_connected)
						ack = false;
					else if(*recipient == ID_RADIO && !parameters->radio_connected)
						ack = false;
					
					ai_handler->writeAIData(&scroll_msg, ack);
					button_states[index] = BUTTON_STATE_PRESSED;
				} else if(!state && button_states[index] != BUTTON_STATE_RELEASED) {
					button_states[index] = BUTTON_STATE_RELEASED;
				}
			}
		}
		
		output_mcp->digitalWriteIO(x, HIGH);
	}
	
	//Check the volume button.
	bool state = false;
	if(vol_mcp != NULL && vol_pin >= 0)
		state = !vol_mcp->digitalReadIO(vol_pin);
	else if(vol_pin >= 0)
		state = digitalRead(vol_pin) == LOW;
	else return;

	if(state && vol_state == BUTTON_STATE_RELEASED) { //Button pressed.
		vol_timer = 0;
		vol_state = BUTTON_STATE_PRESSED;
		sendButtonMessage(0x6, BUTTON_STATE_PRESSED, parameters->audio_dest);
	} else if(!state && vol_state != BUTTON_STATE_RELEASED) { //Button released.
		vol_state = BUTTON_STATE_RELEASED;
		sendButtonMessage(0x6, BUTTON_STATE_RELEASED, parameters->audio_dest);
	}
}

void ButtonHandler::checkButtonHold() {
	for(int i=0;i<BUTTON_INDEX_SIZE;i+=1) {
		uint8_t* recipient = &parameters->all_dest;
		
		if(i == BUTTON_INDEX_ZOOMIN || i == BUTTON_INDEX_ZOOMOUT)
			continue;

		if(button_states[i] == BUTTON_STATE_PRESSED) {
			if(button_timers[i] >= HOLD_TIME) {
				if(i == BUTTON_INDEX_AMFM || i == BUTTON_INDEX_CDXM) {
					const long long am_fm_timer = button_timers[BUTTON_INDEX_AMFM], cd_timer = button_timers[BUTTON_INDEX_CDXM];

					if(button_states[BUTTON_INDEX_AMFM] == BUTTON_STATE_PRESSED && button_states[BUTTON_INDEX_CDXM] == BUTTON_STATE_PRESSED
					&& abs(am_fm_timer - cd_timer) <= 500) {
						amfm_pressed = true;
						cdxm_pressed = true;
						
						button_states[BUTTON_INDEX_AMFM] = BUTTON_STATE_HELD;
						button_states[BUTTON_INDEX_CDXM] = BUTTON_STATE_HELD;

						sendButtonMessage(0x23, BUTTON_STATE_PRESSED, parameters->audio_dest);
						sendButtonMessage(0x23, BUTTON_STATE_RELEASED, parameters->audio_dest);
						continue;
					}
				}

				button_states[i] = BUTTON_STATE_HELD;

				switch(i) {
				case BUTTON_INDEX_AMFM:
				case BUTTON_INDEX_CDXM:
					recipient = &parameters->audio_dest;
					break;
				case BUTTON_INDEX_SCAN:
				case BUTTON_INDEX_SKIPUP:
				case BUTTON_INDEX_SKIPDOWN:
				case BUTTON_INDEX_PRESET1:
				case BUTTON_INDEX_PRESET2:
				case BUTTON_INDEX_PRESET3:
				case BUTTON_INDEX_PRESET4:
				case BUTTON_INDEX_PRESET5:
				case BUTTON_INDEX_PRESET6:
				case BUTTON_INDEX_INFO:
					recipient = &parameters->source_dest;
					break;
				}

				sendButtonMessage(getButtonCode(i), BUTTON_STATE_HELD, *recipient);
			}
		}
	}

	if(vol_state == BUTTON_STATE_PRESSED) {
		if(vol_timer >= HOLD_TIME) {
			vol_state = BUTTON_STATE_HELD;
			sendButtonMessage(0x6, BUTTON_STATE_HELD, parameters->audio_dest);
		}
	}
}

void ButtonHandler::sendButtonMessage(const uint8_t button, const uint8_t state, const uint8_t recipient) {
	uint8_t new_state = 0x80;
	if(state == BUTTON_STATE_PRESSED)
		new_state = 0x0;
	else if(state == BUTTON_STATE_HELD)
		new_state = 0x40;
	
	uint8_t button_data[] = {0x30, button, new_state};
	AIData button_msg(sizeof(button_data), ID_NAV_SCREEN, recipient);
	button_msg.refreshAIData(button_data);

	bool ack = true;
	if(recipient == ID_NAV_COMPUTER && !parameters->computer_connected)
		ack = false;
	else if(recipient == ID_RADIO && !parameters->radio_connected)
		ack = false;
	
	ai_handler->writeAIData(&button_msg, ack);
}

int getButtonIndex(const uint8_t low_com, const uint8_t high_com) {
	if(low_com == 0) { //Com 2.
		switch(high_com) {
		case 0: //Com 6. Cancel.
			return BUTTON_INDEX_CANCEL;
		case 1: //Com 7. Scan.
			return BUTTON_INDEX_SCAN;
		case 2: //Com 8. Preset 1.
			return BUTTON_INDEX_PRESET1;
		case 3: //Com 9. Preset 4.
			return BUTTON_INDEX_PRESET4;
		case 4: //Com 10. Zoom in.
			return BUTTON_INDEX_ZOOMIN;
		default:
			return -1;
		}
	} else if(low_com == 1) { //Com 3.
		switch(high_com) {
		case 0: //Com 6. Setup.
			return BUTTON_INDEX_SETUP;
		case 1: //Com 7. Audio.
			return BUTTON_INDEX_AUDIO;
		case 2: //Com 8. Seek down.
			return BUTTON_INDEX_SKIPDOWN;
		case 3: //Com 9. Preset 3.
			return BUTTON_INDEX_PRESET3;
		case 4: //Com 10. Zoom out.
			return BUTTON_INDEX_ZOOMOUT;
		default:
			return -1;
		}
	} else if(low_com == 2) { //Com 4.
		switch(high_com) {
		case 0: //Com 6. Menu.
			return BUTTON_INDEX_MENU;
		case 1: //Com 7. CD/Aux.
			return BUTTON_INDEX_CDXM;
		case 2: //Com 8. Seek up.
			return BUTTON_INDEX_SKIPUP;
		case 3: //Com 9. Preset 2.
			return BUTTON_INDEX_PRESET2;
		case 4: //Com 10. Preset 6.
			return BUTTON_INDEX_PRESET6;
		default:
			return -1;
		}
	} else if(low_com == 3) { //Com 5.
		switch(high_com) {
		case 0: //Com 6. Map/guide.
			return BUTTON_INDEX_MAP;
		case 1: //Com 7. AM/FM.
			return BUTTON_INDEX_AMFM;
		case 2: //Com 8. Info.
			return BUTTON_INDEX_INFO;
		case 4: //Com 10. Preset 5.
			return BUTTON_INDEX_PRESET5;
		default:
			return -1;
		}
	} else
		return -1;
}
