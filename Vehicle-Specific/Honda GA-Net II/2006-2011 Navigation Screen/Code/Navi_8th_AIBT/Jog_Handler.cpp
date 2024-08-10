#include "Jog_Handler.h"

JogHandler::JogHandler(MCP23S08* up_mcp, const int8_t up_pin,
				MCP23S08* down_mcp, const int8_t down_pin,
				MCP23S08* left_mcp, const int8_t left_pin,
				MCP23S08* right_mcp, const int8_t right_pin,
				MCP23S08* enter_mcp, const int8_t enter_pin,
				AIBusHandler* ai_handler, ParameterList* parameters) {
					this->up_mcp = up_mcp;
					this->down_mcp = down_mcp;
					this->left_mcp = left_mcp;
					this->right_mcp = right_mcp;
					this->enter_mcp = enter_mcp;

					this->up_pin = up_pin;
					this->down_pin = down_pin;
					this->left_pin = left_pin;
					this->right_pin = right_pin;
					this->enter_pin = enter_pin;

					this->ai_handler = ai_handler;
					this->parameters = parameters;
				}

void JogHandler::loop() {
	checkJogPress();
	checkJogHold();
}

void JogHandler::checkJogPress() {
	for(uint8_t i=BUTTON_INDEX_JOGUP;i<=BUTTON_INDEX_JOGENTER;i+=1) {
		bool state = false;

		switch(i) {
		case BUTTON_INDEX_JOGUP:
			if(up_mcp != NULL && up_pin >= 0)
				state = up_mcp->digitalReadIO(up_pin);
			else if(up_pin >= 0)
				state = digitalRead(up_pin) == HIGH;
			break;
		case BUTTON_INDEX_JOGDOWN:
			if(down_mcp != NULL && down_pin >= 0)
				state = down_mcp->digitalReadIO(down_pin);
			else if(down_pin >= 0)
				state = digitalRead(down_pin) == HIGH;
			break;
		case BUTTON_INDEX_JOGLEFT:
			if(left_mcp != NULL && left_pin >= 0)
				state = left_mcp->digitalReadIO(left_pin);
			else if(left_pin >= 0)
				state = digitalRead(left_pin) == HIGH;
			break;
		case BUTTON_INDEX_JOGRIGHT:
			if(right_mcp != NULL && right_pin >= 0)
				state = right_mcp->digitalReadIO(right_pin);
			else if(right_pin >= 0)
				state = digitalRead(right_pin) == HIGH;
			break;
		case BUTTON_INDEX_JOGENTER:
			if(enter_mcp != NULL && enter_pin >= 0)
				state = enter_mcp->digitalReadIO(enter_pin);
			else if(enter_pin >= 0)
				state = digitalRead(enter_pin) == HIGH;
			break;
		}

		if(!state && parameters->button_states[i] == BUTTON_STATE_RELEASED) {
			parameters->button_timers[i] = 0;
			parameters->button_states[i] = BUTTON_STATE_PRESSED;
			sendButtonMessage(getButtonCode(i), BUTTON_STATE_PRESSED, parameters->all_dest);
		} else if(state && parameters->button_states[i] != BUTTON_STATE_RELEASED) {
			parameters->button_states[i] = BUTTON_STATE_RELEASED;
			sendButtonMessage(getButtonCode(i), BUTTON_STATE_RELEASED, parameters->all_dest);
		}
	}
}

void JogHandler::checkJogHold() {
	for(uint8_t i=BUTTON_INDEX_JOGUP;i<=BUTTON_INDEX_JOGENTER;i+=1) {
		if(parameters->button_states[i] == BUTTON_STATE_PRESSED) {
			if(parameters->button_timers[i] >= HOLD_TIME) {
				parameters->button_states[i] = BUTTON_STATE_HELD;
				sendButtonMessage(getButtonCode(i), BUTTON_STATE_HELD, parameters->all_dest);
			}
		}
	}
}

void JogHandler::sendButtonMessage(const uint8_t button, const uint8_t state, const uint8_t recipient) {
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