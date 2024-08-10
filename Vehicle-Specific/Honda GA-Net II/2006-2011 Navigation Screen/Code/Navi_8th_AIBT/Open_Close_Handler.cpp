#include "Open_Close_Handler.h"

OpenCloseHandler::OpenCloseHandler(MCP23S08* open_mcp, const int8_t open_pin,
					MCP23S08* close_mcp, const int8_t close_pin,
					MCP23S08* open_motor_mcp, const int8_t open_motor_pin,
					MCP23S08* close_motor_mcp, const int8_t close_motor_pin,
					MCP23S08* stop_motor_mcp, const int8_t stop_motor_pin,
					MCP23S08* stop_motor_ind_mcp, const int8_t stop_motor_ind_pin,
					MCP23S08* close_ind_mcp, const int8_t close_ind_pin,
					const int8_t motor_pos, AIBusHandler* ai_handler, ParameterList* parameters) {
						this->open_mcp = open_mcp;
						this->close_mcp = close_mcp;
						this->open_motor_mcp = open_motor_mcp;
						this->close_motor_mcp = close_motor_mcp;
						this->stop_motor_mcp = stop_motor_mcp;
						this->stop_motor_ind_mcp = stop_motor_ind_mcp;
						this->close_ind_mcp = close_ind_mcp;

						this->open_pin = open_pin;
						this->close_pin = close_pin;
						this->open_motor_pin = open_motor_pin;
						this->close_motor_pin = close_motor_pin;
						this->stop_motor_pin = stop_motor_pin;
						this->stop_motor_ind_pin = stop_motor_ind_pin;
						this->close_ind_pin = close_ind_pin;
						
						this->motor_pos_pin = motor_pos;
						this->ai_handler = ai_handler;
						this->parameters = parameters;
					}

void OpenCloseHandler::loop() {
	checkButtonPress();
	getMotorPosition();

	bool stop_motor_ind = false;
	if(stop_motor_ind_mcp != NULL && stop_motor_ind_pin >= 0)
		stop_motor_ind = stop_motor_ind_mcp->digitalReadIO(stop_motor_ind_pin);
	else if(stop_motor_ind_pin >= 0)
		stop_motor_ind = digitalRead(stop_motor_ind_pin) == HIGH;

	if(stop_motor_ind) {
		if(open_motor_mcp != NULL && open_motor_pin >= 0)
			open_motor_mcp->digitalWriteIO(open_motor_pin, false);
		else if(open_motor_pin >= 0)
			digitalWrite(open_motor_pin, LOW);

		if(close_motor_mcp != NULL && close_motor_pin >= 0)
			close_motor_mcp->digitalWriteIO(close_motor_pin, false);
		else if(close_motor_pin >= 0)
			digitalWrite(close_motor_pin, LOW);
	}

	if(parameters->open_pulse_enabled && parameters->pulse_timer > OPEN_CLOSE_TIME) {
		parameters->open_pulse_enabled = false;

		if(open_motor_mcp != NULL && open_motor_pin >= 0)
			open_motor_mcp->digitalWriteIO(open_motor_pin, false);
		else if(open_motor_pin >= 0)
			digitalWrite(open_motor_pin, LOW);
	}

	if(parameters->close_pulse_enabled && parameters->pulse_timer > OPEN_CLOSE_TIME) {
		if(close_motor_mcp != NULL && close_motor_pin >= 0)
			close_motor_mcp->digitalWriteIO(close_motor_pin, false);
		else if(close_motor_pin >= 0)
			digitalWrite(close_motor_pin, LOW);
	}

	if(motor_position < 40 && !close_led) {
		if(close_ind_mcp != NULL && close_ind_pin >= 0)
			close_ind_mcp->digitalWriteIO(close_ind_pin, true);
		else if(close_ind_pin >= 0)
			digitalWrite(close_ind_pin, HIGH);
	} else if(motor_position >= 40 && close_led) {
		if(close_ind_mcp != NULL && close_ind_pin >= 0)
			close_ind_mcp->digitalWriteIO(close_ind_pin, false);
		else if(close_ind_pin >= 0)
			digitalWrite(close_ind_pin, LOW);
	}
}

void OpenCloseHandler::forceOpen() {
	if(open_motor_pin >= 0) {
		parameters->open_pulse_enabled = true;
		parameters->pulse_timer = 0;

		if(open_motor_mcp != NULL)
			open_motor_mcp->digitalWriteIO(open_motor_pin, true);
		else
			digitalWrite(open_motor_pin, HIGH);
	}
}

void OpenCloseHandler::forceClose() {
	if(close_motor_pin >= 0) {
		parameters->close_pulse_enabled = true;
		parameters->pulse_timer = 0;

		if(close_motor_mcp != NULL)
			close_motor_mcp->digitalWriteIO(close_motor_pin, true);
		else
			digitalWrite(close_motor_pin, HIGH);
	}
}

void OpenCloseHandler::checkButtonPress() {
	bool open_state = false;
	if(open_mcp != NULL && open_pin >= 0)
		open_state = open_mcp->digitalReadIO(open_pin);
	else if(open_pin >= 0)
		open_state = digitalRead(open_pin) == HIGH;

	bool close_state = false;
	if(close_mcp != NULL && close_pin >= 0)
		close_state = close_mcp->digitalReadIO(close_pin);
	else if(close_pin >= 0)
		close_state = digitalRead(close_pin) == HIGH;

	const bool screen_open = this->motor_position < 40, screen_closed = this->motor_position > 984;
	if(open_state != open_pressed && open_pressed) { //Open button pressed.
		if(screen_closed && !parameters->open_pulse_enabled) {
			if(open_motor_pin >= 0) {
				parameters->open_pulse_enabled = true;
				parameters->pulse_timer = 0;

				if(open_motor_mcp != NULL)
					open_motor_mcp->digitalWriteIO(open_motor_pin, true);
				else
					digitalWrite(open_motor_pin, HIGH);
			}
		} else if(!screen_closed && !parameters->close_pulse_enabled) {
			if(close_motor_pin >= 0) {
				parameters->close_pulse_enabled = true;
				parameters->pulse_timer = 0;

				if(close_motor_mcp != NULL)
					close_motor_mcp->digitalWriteIO(close_motor_pin, true);
				else
					digitalWrite(close_motor_pin, HIGH);
			}
		}
	}

	if(close_state != close_pressed && close_pressed) { //Close button pressed.
		if(screen_open) {
			if(close_motor_pin >= 0) {
				parameters->close_pulse_enabled = true;
				parameters->pulse_timer = 0;

				if(close_motor_mcp != NULL)
					close_motor_mcp->digitalWriteIO(close_motor_pin, true);
				else
					digitalWrite(close_motor_pin, HIGH);
			}
		}
	}

	open_pressed = open_state;
	close_pressed = close_state;
}

void OpenCloseHandler::getMotorPosition() {
	//Lower voltage indicates screen open.
	this->motor_position = analogRead(this->motor_pos_pin);
}