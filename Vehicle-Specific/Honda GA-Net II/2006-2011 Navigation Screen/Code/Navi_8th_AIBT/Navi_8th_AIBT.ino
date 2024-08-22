#include <MCP23S08.h>
#include <SPI.h>

#include "AIBus_Handler.h"
#include "Light_Control.h"
#include "Button_Handler.h"
#include "Jog_Handler.h"
#include "Open_Close_Handler.h"
#include "AIBT_Parameters.h"

#ifndef __AVR__
#define TCNT2 _SFR_MEM8(0xB2)
#define TCCR2B _SFR_MEM8(0xB1)
#endif

#if defined(TCNT2)
#define TIMER TCNT2
#elif defined(TCNT2L)
#define TIMER TCNT2L
#else
#error Timer Not Defined
#endif

#if defined(TCCR2B)
#define TIMER_SCALER TCCR2B
#else
#error Scale Not Defined
#endif

#define KNOB_WAIT int(5e-3*F_CPU/1024)

#define KNOB_A 2
#define KNOB_B 3

#define AI_RX 4
#define ILL_CATHODE 6
#define MOTOR_STOP_OUT 7
#define CS_KNOB 8
#define CS_OUT 9
#define CS_IN 10

#define MOTOR_POSITION A0

#define KNOB_MCP_MOTOR_CLOSE 0
#define KNOB_MCP_MOTOR_OPEN 1
#define KNOB_MCP_MOTOR_STOP_IND 2
#define KNOB_MCP_ANODE 3
#define KNOB_MCP_CLOSE_ANODE 4
#define KNOB_MCP_BACKLIGHT 5
#define KNOB_MCP_POWER_ON 6
#define KNOB_MCP_VOL_PUSH 7

#define INPUT_MCP_COM6 0
#define INPUT_MCP_COM7 1
#define INPUT_MCP_COM8 2
#define INPUT_MCP_COM9 3
#define INPUT_MCP_COM10 4
#define INPUT_MCP_TOGGLE_LEFT 5
#define INPUT_MCP_TOGGLE_RIGHT 6
#define INPUT_MCP_TOGGLE_ENTER 7

#define OUTPUT_MCP_COM2 0
#define OUTPUT_MCP_COM3 1
#define OUTPUT_MCP_COM4 2
#define OUTPUT_MCP_COM5 3
#define OUTPUT_MCP_PUSH_OPEN 4
#define OUTPUT_MCP_PUSH_CLOSE 5
#define OUTPUT_MCP_TOGGLE_UP 6
#define OUTPUT_MCP_TOGGLE_DOWN 7

#define CONTROL_TIMER 7000
#define DOOR_TIMER 30000
#define VOL_REVERSE_TIMER 50

#define AISerial Serial

MCP23S08 mcp_knob(CS_KNOB);
MCP23S08 mcp_out(CS_OUT);
MCP23S08 mcp_in(CS_IN);

AIBusHandler ai_handler(&AISerial, AI_RX);

ParameterList parameters;
elapsedMillis all_timer, radio_timer, source_timer;
bool all_timer_enabled = false, radio_timer_enabled = false, source_timer_enabled = false;

elapsedMillis door_timer;
bool door_timer_enabled = false;

elapsedMillis vol_reverse_timer;

LightController* light_handler;

ButtonHandler* button_handler;
JogHandler* jog_handler;
OpenCloseHandler* open_handler;

volatile int32_t vol_steps = 0;
volatile int8_t last_int = 0;
volatile uint8_t vol_a_state, vol_b_state;

uint8_t key_position = 0, door_position = 0;
bool power_off_with_door = false; //Turn the power off when the door opens.

void setup() {
	TIMER_SCALER &= 0xF8;
	TIMER_SCALER |= 0x2;
	TIMER = 0;

	AISerial.begin(AI_BAUD);
	pinMode(AI_RX, INPUT);

	pinMode(MOTOR_STOP_OUT, OUTPUT);
	digitalWrite(MOTOR_STOP_OUT, LOW);

	SPI.begin();

	pinMode(CS_KNOB, OUTPUT);
	pinMode(CS_IN, OUTPUT);
	pinMode(CS_OUT, OUTPUT);

	digitalWrite(CS_KNOB, HIGH);
	digitalWrite(CS_IN, HIGH);
	digitalWrite(CS_OUT, HIGH);

	delay(100);

	AIData init_msg(1, ID_NAV_SCREEN, 0xFF);
	init_msg.data[0] = 0x1;
	ai_handler.writeAIData(&init_msg, false);

	mcp_knob.begin();
	mcp_out.begin();
	mcp_in.begin();

	mcp_knob.pinModeIO(KNOB_MCP_MOTOR_CLOSE, OUTPUT);
	mcp_knob.pinModeIO(KNOB_MCP_MOTOR_OPEN, OUTPUT);
	mcp_knob.pinModeIO(KNOB_MCP_MOTOR_STOP_IND, INPUT);
	mcp_knob.pinModeIO(KNOB_MCP_ANODE, OUTPUT);
	mcp_knob.pinModeIO(KNOB_MCP_BACKLIGHT, OUTPUT);
	mcp_knob.pinModeIO(KNOB_MCP_CLOSE_ANODE, OUTPUT);
	mcp_knob.pinModeIO(KNOB_MCP_POWER_ON, OUTPUT);
	mcp_knob.pinModeIO(KNOB_MCP_VOL_PUSH, INPUT_PULLUP);

	mcp_out.pinModeIO(OUTPUT_MCP_COM2, OUTPUT);
	mcp_out.pinModeIO(OUTPUT_MCP_COM3, OUTPUT);
	mcp_out.pinModeIO(OUTPUT_MCP_COM4, OUTPUT);
	mcp_out.pinModeIO(OUTPUT_MCP_COM5, OUTPUT);
	mcp_out.pinModeIO(OUTPUT_MCP_PUSH_CLOSE, INPUT_PULLUP);
	mcp_out.pinModeIO(OUTPUT_MCP_PUSH_OPEN, INPUT_PULLUP);
	mcp_out.pinModeIO(OUTPUT_MCP_TOGGLE_UP, INPUT_PULLUP);
	mcp_out.pinModeIO(OUTPUT_MCP_TOGGLE_DOWN, INPUT_PULLUP);

	mcp_in.pinModeIO(INPUT_MCP_COM6, INPUT_PULLUP);
	mcp_in.pinModeIO(INPUT_MCP_COM7, INPUT_PULLUP);
	mcp_in.pinModeIO(INPUT_MCP_COM8, INPUT_PULLUP);
	mcp_in.pinModeIO(INPUT_MCP_COM9, INPUT_PULLUP);
	mcp_in.pinModeIO(INPUT_MCP_COM10, INPUT_PULLUP);
	mcp_in.pinModeIO(INPUT_MCP_TOGGLE_ENTER, INPUT_PULLUP);
	mcp_in.pinModeIO(INPUT_MCP_TOGGLE_LEFT, INPUT_PULLUP);
	mcp_in.pinModeIO(INPUT_MCP_TOGGLE_RIGHT, INPUT_PULLUP);

	pinMode(KNOB_A, INPUT_PULLUP);
	pinMode(KNOB_B, INPUT_PULLUP);
	vol_a_state = digitalRead(KNOB_A);
	vol_b_state = digitalRead(KNOB_B);

	//attachInterrupt(digitalPinToInterrupt(KNOB_A), volumeKnobIncrement, FALLING);

	mcp_out.digitalWriteIO(OUTPUT_MCP_COM2, HIGH);
	mcp_out.digitalWriteIO(OUTPUT_MCP_COM3, HIGH);
	mcp_out.digitalWriteIO(OUTPUT_MCP_COM4, HIGH);
	mcp_out.digitalWriteIO(OUTPUT_MCP_COM5, HIGH);

	mcp_knob.digitalWriteIO(KNOB_MCP_MOTOR_CLOSE, LOW);
	mcp_knob.digitalWriteIO(KNOB_MCP_MOTOR_OPEN, LOW);
	mcp_knob.digitalWriteIO(KNOB_MCP_ANODE, LOW);
	mcp_knob.digitalWriteIO(KNOB_MCP_CLOSE_ANODE, LOW);
	mcp_knob.digitalWriteIO(KNOB_MCP_POWER_ON, LOW);
	mcp_knob.digitalWriteIO(KNOB_MCP_BACKLIGHT, LOW);

	button_handler = new ButtonHandler(&mcp_in, &mcp_out, &ai_handler, &parameters, &mcp_knob, KNOB_MCP_VOL_PUSH);
	jog_handler = new JogHandler(&mcp_out, OUTPUT_MCP_TOGGLE_UP, &mcp_out, OUTPUT_MCP_TOGGLE_DOWN, &mcp_in, INPUT_MCP_TOGGLE_LEFT, &mcp_in, INPUT_MCP_TOGGLE_RIGHT, &mcp_in, INPUT_MCP_TOGGLE_ENTER, &ai_handler, &parameters);
	open_handler = new OpenCloseHandler(&mcp_out, OUTPUT_MCP_PUSH_OPEN, &mcp_out, OUTPUT_MCP_PUSH_CLOSE, &mcp_knob, KNOB_MCP_MOTOR_OPEN, &mcp_knob, KNOB_MCP_MOTOR_CLOSE, NULL, MOTOR_STOP_OUT, &mcp_knob, KNOB_MCP_MOTOR_STOP_IND, &mcp_knob, KNOB_MCP_CLOSE_ANODE, MOTOR_POSITION, &ai_handler, &parameters);
	light_handler = new LightController(ILL_CATHODE, KNOB_MCP_ANODE, &mcp_knob);
}

void loop() {
	AIData msg;

	elapsedMillis ai_timer;
	
	do {
		bool message_read = false;
		if(AISerial.available() > 0) {
			if(ai_handler.readAIData(&msg)) {
				if(msg.sender == ID_NAV_SCREEN)
					continue;

				if(msg.receiver == ID_NAV_SCREEN && msg.l >= 1 && msg.data[0] != 0x80)
					message_read = true;
				else if(msg.receiver == 0xFF && msg.l >= 1 && msg.data[0] == 0xA1) {
					if(msg.l >= 0x4 && msg.data[1] == 0x10) { //Light control.
						if((msg.data[3]&0x1) != 0)
							light_handler->lightOn(msg.data[2]);
						else
							light_handler->lightOff();
					} else if(msg.l >= 3 && msg.data[1] == 0x2) { //Key position.
						if((msg.data[2]&0x7) != 0) {
							mcp_knob.digitalWriteIO(KNOB_MCP_POWER_ON, HIGH);
							mcp_knob.digitalWriteIO(KNOB_MCP_BACKLIGHT, HIGH);
							door_timer_enabled = false;
						}

						if((key_position&0xF) != 0 && msg.data[2] == 0) {
							if((door_position&0xF) == 0)
								power_off_with_door = true;
							else {
								power_off_with_door = false;
								mcp_knob.digitalWriteIO(KNOB_MCP_POWER_ON, LOW);
								mcp_knob.digitalWriteIO(KNOB_MCP_BACKLIGHT, LOW);
							}
						}

						key_position = msg.data[2];
					} else if(msg.l >= 3 && msg.data[1] == 0x43) { //Door opened or closed.
						door_position = msg.data[2];
						if((msg.data[2]&0xF) != 0) { //Door opened.
							if(power_off_with_door && key_position == 0) {
								power_off_with_door = false;
								mcp_knob.digitalWriteIO(KNOB_MCP_POWER_ON, LOW);
								mcp_knob.digitalWriteIO(KNOB_MCP_BACKLIGHT, LOW);
							} else if((key_position&0xF) == 0) {
								mcp_knob.digitalWriteIO(KNOB_MCP_POWER_ON, HIGH);
								door_timer_enabled = true;
								door_timer = 0;
							}
						}
					}
				}
				ai_timer = 0;
			}
		}

		if(message_read) {
			bool ack = true;

			if(msg.sender == ID_NAV_SCREEN)
				continue;

			if(msg.l >= 2 && msg.data[0] == 0x31 && msg.data[1] == 0x30) { //Button request.
				ack = false;
				ai_handler.sendAcknowledgement(ID_NAV_SCREEN, msg.sender);
				sendButtonsPresent(msg.sender);
			} else if(msg.l >= 3 && msg.data[0] == 0x77) { //Control change.
				const uint8_t controls = msg.data[2], receiver = msg.data[1];
				if((controls&0x10) != 0) { //Full screen control.
					all_timer_enabled = true;
					all_timer = 0;
					parameters.all_dest = receiver;

					if(!radio_timer_enabled || parameters.audio_dest == receiver) {
						radio_timer_enabled = true;
						radio_timer = 0;
						parameters.audio_dest = receiver;

						if(!source_timer_enabled || parameters.source_dest == receiver) {
							source_timer_enabled = true;
							source_timer = 0;
							parameters.source_dest = receiver;
						}
					}
				}

				if((controls&0x20) != 0) { //Audio control.
					radio_timer_enabled = true;
					radio_timer = 0;
					parameters.audio_dest = receiver;

					if(!source_timer_enabled || parameters.source_dest == receiver) {
						source_timer_enabled = true;
						source_timer = 0;
						parameters.source_dest = receiver;
					}
				}

				if((controls&0x80) != 0) { //Source control.
					source_timer_enabled = true;
					source_timer = 0;
					parameters.source_dest = receiver;
				}
			} else if(msg.l >= 2 && msg.data[0] == 0x38) {
				if(msg.data[1] == 0x0)
					open_handler->forceClose();
				else if(msg.data[1] == 0x2)
					open_handler->forceOpen();
			}

			if(ack)
				ai_handler.sendAcknowledgement(ID_NAV_SCREEN, msg.sender);
		}
	} while(ai_timer < 50);

	button_handler->loop();
	jog_handler->loop();
	open_handler->loop();

	if(vol_reverse_timer > VOL_REVERSE_TIMER) {
		handleVolumeKnob();
		if(last_int == 0)
			vol_reverse_timer = 0;
	}

	if(all_timer_enabled && all_timer > CONTROL_TIMER) {
		all_timer_enabled = false;
		parameters.all_dest = ID_NAV_COMPUTER;
	}

	if(radio_timer_enabled && radio_timer > CONTROL_TIMER) {
		radio_timer_enabled = false;
		parameters.audio_dest = ID_NAV_COMPUTER;

		if(all_timer_enabled)
			parameters.audio_dest = parameters.all_dest;
		else
			parameters.audio_dest = ID_NAV_COMPUTER;
	}

	if(source_timer_enabled && source_timer > CONTROL_TIMER) {
		source_timer_enabled = false;
		if(radio_timer_enabled)
			parameters.source_dest = parameters.audio_dest;
		else if(all_timer_enabled)
			parameters.source_dest = parameters.all_dest;
		else
			parameters.source_dest = ID_NAV_COMPUTER;
	}

	if(door_timer_enabled && door_timer > DOOR_TIMER) {
		door_timer_enabled = false;
		mcp_knob.digitalWriteIO(KNOB_MCP_POWER_ON, LOW);
	}

	if(last_int != 0 && vol_reverse_timer > VOL_REVERSE_TIMER)
		last_int = 0;
}

void sendButtonsPresent(const uint8_t receiver) {
	uint8_t button_data[] = {0x31, 0x30, 0x4, 0x6, 0x36, 0x38, 0x26, 0x39, 0x25, 0x24, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x27, 0x51, 0x20, 0x55, 0x53, 0x28, 0x29, 0x2A, 0x2B, 0x7};
	AIData button_msg(sizeof(button_data), ID_NAV_SCREEN, receiver);
	button_msg.refreshAIData(button_data);

	ai_handler.writeAIData(&button_msg);
}

void volumeKnobIncrement() {
	/*noInterrupts();
	TIMER_SCALER &= 0xF8;
	TIMER_SCALER |= 0x5;
	TIMER = 0;

	const uint8_t last_vol_a = vol_a_state, last_vol_b = vol_b_state;

	vol_a_state = digitalRead(KNOB_A);
	vol_b_state = digitalRead(KNOB_B);
	TIMER = 0;

	if(vol_a_state == last_vol_a && vol_b_state == last_vol_b)
		return;
	
	if(vol_a_state == HIGH && vol_b_state == HIGH) {
		while(TIMER < KNOB_WAIT);
		vol_a_state = digitalRead(KNOB_A);
		vol_b_state = digitalRead(KNOB_B);
		TIMER = 0;
	}

	int32_t steps = 0;

	if(vol_a_state != last_vol_a && vol_b_state != last_vol_b) {
		if(last_int < 0)
			steps += -2;
		else if(last_int > 0)
			steps += 2;
	} else {
		int8_t increment = 0;
		if(vol_a_state != last_vol_a) {
			if(vol_b_state == LOW)
				increment = 1;
			else
				increment = -1;

			if(vol_a_state == HIGH && last_vol_a == LOW)
				steps += increment;
			else
				steps -= increment;
		} 
		
		if(vol_b_state != last_vol_b) {
			if(vol_a_state == HIGH)
				increment = 1;
			else
				increment = -1;

			if(vol_b_state == HIGH && last_vol_b == LOW)
				steps += increment;
			else
				steps -= increment;
		}
	}

	while(TIMER < KNOB_WAIT);
	vol_a_state = digitalRead(KNOB_A);
	vol_b_state = digitalRead(KNOB_B);

	vol_steps += steps;
	interrupts();
	TIMER_SCALER &= 0xF8;
	TIMER_SCALER |= 0x2;
	TIMER = 0;*/

	noInterrupts();
	TIMER_SCALER &= 0xF8;
	TIMER_SCALER |= 0x5;
	TIMER = 0;

	while(TIMER < KNOB_WAIT);

	vol_a_state = digitalRead(KNOB_A);
	vol_b_state = digitalRead(KNOB_B);

	if(vol_b_state == HIGH)
		vol_steps += 1;
	else
		vol_steps -= 1;

	TIMER_SCALER &= 0xF8;
	TIMER_SCALER |= 0x2;
	interrupts();
}

void handleVolumeKnob() {
	if(vol_steps != 0) {
		if(last_int == 0) {
			if(vol_steps < 0)
				last_int = -1;
			else if(vol_steps > 0)
				last_int = 1;
			else
				last_int = 0;
		}

		if((vol_steps < 0 && last_int > 0) || (vol_steps > 0 && last_int < 0)) {
			vol_steps = 0;
			return;
		}

		vol_reverse_timer = 0;

		uint8_t vol_knob_data[] = {0x32, 0x6, abs(vol_steps)&0xF};
		if(vol_steps > 0)
			vol_knob_data[2] |= 0x10;

		AIData vol_knob_msg(sizeof(vol_knob_data), ID_NAV_SCREEN, parameters.audio_dest);
		vol_knob_msg.refreshAIData(vol_knob_data);

		bool ack = true;
		if(parameters.audio_dest == ID_NAV_COMPUTER && !parameters.computer_connected)
			ack = false;
		else if(parameters.audio_dest == ID_RADIO && !parameters.radio_connected)
			ack = false;
		else if(parameters.audio_dest == 0xFF)
			ack = false;

		ai_handler.writeAIData(&vol_knob_msg, ack);
	}
	vol_steps = 0;
}
