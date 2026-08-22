#include "Button_Handler.h"

ButtonHandler::ButtonHandler(AIBusHandler* ai_handler, OpenCloseHandler* open_close_handler, ParameterList* parameters) {
	this->ai_handler = ai_handler;
	this->open_close_handler = open_close_handler;
	this->parameters = parameters;

	for(int i=0;i<BUTTON_INDEX_SIZE;i+=1)
		button_states[i] = BUTTON_STATE_RELEASED;

	for(int i=0;i<TOGGLE_INDEX_SIZE;i+=1)
		toggle_states[i] = BUTTON_STATE_RELEASED;

	button_index[0] = BT_EJECT;
	button_index[1] = BT_SOURCE;
	button_index[2] = BT_AUDIO;
	button_index[3] = BT_FMAM;
	button_index[4] = BT_AUX;
	button_index[5] = BT_SKIPUP;
	button_index[6] = BT_SKIPDN;
	button_index[7] = BT_INFO;
	button_index[8] = BT_TONE;
	button_index[9] = BT_HOME;
	button_index[10] = BT_SETUP;
	button_index[11] = BT_BACK;

	button_index[12] = BT_F1;
	button_index[13] = BT_F2;
	button_index[14] = BT_F3;
	button_index[15] = BT_F4;
	button_index[16] = BT_F5;
	button_index[17] = BT_F6;
}

void ButtonHandler::loop() {
	ai_handler->cachePending(ID_NAV_SCREEN);

	checkButtonPress();
	checkButtonHold();

	ai_handler->cachePending(ID_NAV_SCREEN);
}

//Check buttons pressed.
void ButtonHandler::checkButtonPress() {
	for(int b=0;b<sizeof(button_index)/sizeof(int); b+=1) {
		const bool state = !digitalRead(button_index[b]);

		uint8_t recipient = parameters->all_dest;
		switch(button_index[b]) {
			case BT_SOURCE:
			case BT_FMAM:
			case BT_AUX:
			case BT_TONE:
				recipient = parameters->audio_dest;
				break;
			case BT_INFO:
			case BT_SKIPUP:
			case BT_SKIPDN:
			case BT_F1:
			case BT_F2:
			case BT_F3:
			case BT_F4:
			case BT_F5:
			case BT_F6:
				recipient = parameters->source_dest;
				break;
		}

		if(state && button_states[b] == BUTTON_STATE_RELEASED) { //Button pressed.
			button_timers[b] = 0;
			button_states[b] = BUTTON_STATE_PRESSED;
			last_button_rec[b] = recipient;
			if(button_index[b] != BT_EJECT)
				sendButtonMessage(getButtonCode(button_index[b]), BUTTON_STATE_PRESSED, recipient);
			else {
				//if(open_close_handler->getClosed())
					if(parameters->allow_open)
						open_close_handler->setOpen();
				//else
				//	open_close_handler->setClosed();
			}
		} else if(!state && button_states[b] != BUTTON_STATE_RELEASED) { //Button released.
			button_states[b] = BUTTON_STATE_RELEASED;
			if(button_index[b] != BT_EJECT)
				sendButtonMessage(getButtonCode(button_index[b]), BUTTON_STATE_RELEASED, last_button_rec[b]);
		}
	}

	if(debounce_timer < 50)
		return;

	debounce_timer = 0;

	for(int b=0;b<TOGGLE_INDEX_SIZE;b+=1) {
		const bool state = !toggle[b];

		uint8_t button_code = 0x0;
		switch(b) {
		case INDEX_NAV_PUSH:
			button_code = 0x7;
			break;
		case INDEX_NAV_UP:
			button_code = 0x28;
			break;
		case INDEX_NAV_DN:
			button_code = 0x29;
			break;
		case INDEX_NAV_LEFT:
			button_code = 0x2A;
			break;
		case INDEX_NAV_RIGHT:
			button_code = 0x2B;
			break;
		case INDEX_VOL_PUSH:
			button_code = 0x6;
			break;
		}

		if(state && toggle_states[b] == BUTTON_STATE_RELEASED) { //Toggle pressed.
			bool press = true;
			if(b == INDEX_NAV_PUSH) {
				if((!toggle[INDEX_NAV_UP])
					|| (!toggle[INDEX_NAV_DN])
					|| (!toggle[INDEX_NAV_LEFT])
					|| (!toggle[INDEX_NAV_RIGHT]))
					press = false;
			} else if(b != INDEX_VOL_PUSH) {
				if(toggle_states[INDEX_NAV_PUSH] != BUTTON_STATE_RELEASED) {
					toggle_states[INDEX_NAV_PUSH] = BUTTON_STATE_RELEASED;
					sendButtonMessage(0x7, BUTTON_STATE_RELEASED, parameters->all_dest);
				}
			}

			const uint8_t dest = b != INDEX_VOL_PUSH ? parameters->all_dest : parameters->audio_dest;

			if(press) {
				toggle_timers[b] = 0;
				toggle_states[b] = BUTTON_STATE_PRESSED;
				last_toggle_rec[b] = dest;
				sendButtonMessage(button_code, BUTTON_STATE_PRESSED, dest);
			}
		} else if(!state && toggle_states[b] != BUTTON_STATE_RELEASED) { //Toggle released.
			const uint8_t dest = b != INDEX_VOL_PUSH ? parameters->all_dest : parameters->audio_dest;
			toggle_states[b] = BUTTON_STATE_RELEASED;
			sendButtonMessage(button_code, BUTTON_STATE_RELEASED, last_toggle_rec[b]);
		}
	}
}

//Check whether buttons are held.
void ButtonHandler::checkButtonHold() {
	for(int b=0;b<sizeof(button_index)/sizeof(int); b+=1) {
		if(button_states[b] == BUTTON_STATE_PRESSED && button_timers[b] > BUTTON_TIMER && button_index[b] != BT_EJECT) {
			button_states[b] = BUTTON_STATE_HELD;
			sendButtonMessage(getButtonCode(button_index[b]), BUTTON_STATE_HELD, last_button_rec[b]);
		}
	}

	for(int b=0;b<TOGGLE_INDEX_SIZE;b+=1) {
		if(toggle_states[b] == BUTTON_STATE_PRESSED && toggle_timers[b] > BUTTON_TIMER) {
			toggle_states[b] = BUTTON_STATE_HELD;

			uint8_t button_code = 0x0;
			switch(b) {
			case INDEX_NAV_PUSH:
				button_code = 0x7;
				break;
			case INDEX_NAV_UP:
				button_code = 0x28;
				break;
			case INDEX_NAV_DN:
				button_code = 0x29;
				break;
			case INDEX_NAV_LEFT:
				button_code = 0x2A;
				break;
			case INDEX_NAV_RIGHT:
				button_code = 0x2B;
				break;
			case INDEX_VOL_PUSH:
				button_code = 0x6;
				break;
			}

			sendButtonMessage(button_code, BUTTON_STATE_HELD, last_toggle_rec[b]);
		}
	}
}

//Send a button message to the relevant device.
void ButtonHandler::sendButtonMessage(const uint8_t button, const uint8_t state, const uint8_t recipient) {
	uint8_t new_state = 0x80;
	if(state == BUTTON_STATE_PRESSED)
		new_state = 0x0;
	else if(state == BUTTON_STATE_HELD)
		new_state = 0x40;
	
	uint8_t button_data[] = {0x30, button, new_state};
	AIData button_msg(sizeof(button_data), ID_NAV_SCREEN, recipient, button_data);

	bool ack = true;
	if(recipient == ID_NAV_COMPUTER && !parameters->computer_connected)
		ack = false;
	else if(recipient == ID_RADIO && !parameters->radio_connected)
		ack = false;
	else if(recipient == 0xFF || recipient == ID_NAV_SCREEN)
		ack = false;
	
	ai_handler->writeAIData(&button_msg, ack);
}
