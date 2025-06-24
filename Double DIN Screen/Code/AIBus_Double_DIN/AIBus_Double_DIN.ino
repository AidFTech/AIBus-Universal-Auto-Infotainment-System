#include <elapsedMillis.h>
#include <MCP23S08.h>
#include <MCP4251.h>
#include <SPI.h>

#include "AIBus.h"
#include "AIBus_Handler.h"
#include "AIBT_Parameters.h"
#include "Button_Handler.h"
#include "Open_Close_Handler.h"

#ifdef MEGACOREX
#define AI_RX PIN_PA7
#define ILL_CS PIN_PB0
#define ILL_ON PIN_PB1
#define FULL_POWER_ON PIN_PB2
#define MCP_RESET PIN_PB5

#define OPEN_CLOSE_CS PIN_PB3
#define BL_ON PIN_PB4

#define VOL_CLK PIN_PE2
#define VOL_UP PIN_PE3

#define NAV_CLK PIN_PF2
#define NAV_UP PIN_PF3

#define NAV_CS PIN_PF4

#else
#define AI_RX 4
#define ILL_CS 5
#define ILL_ON 6
#define FULL_POWER_ON 7
#define MCP_RESET 8

#define OPEN_CLOSE_CS 9
#define BL_ON 10

#define VOL_CLK 27
#define VOL_UP 28

#define NAV_CLK 29
#define NAV_UP 30

#define NAV_CS 31

#endif

#define NAV_MCP_NAV_PUSH 0
#define NAV_MCP_NAV_UP 1
#define NAV_MCP_NAV_DN 2
#define NAV_MCP_NAV_LEFT 3
#define NAV_MCP_NAV_RIGHT 4
#define NAV_MCP_ILL_AIDF 6
#define NAV_MCP_VOL_PUSH 7

#define AISerial Serial

#define VOL_TIMER 20
#define DOOR_TIMER 30000

AIBusHandler ai_handler(&AISerial, AI_RX);
MCP4251 ill_mcp4251(ILL_CS, 100000, 0, 100000, 0);
MCP23S08 nav_mcp(NAV_CS);
MCP23S08 open_close_mcp(OPEN_CLOSE_CS);

ParameterList parameters;
OpenCloseHandler open_handler(&open_close_mcp, &parameters);
ButtonHandler button_handler(&ai_handler, &open_handler, &parameters);

elapsedMillis all_timer, radio_timer, source_timer;
bool all_timer_enabled = false, radio_timer_enabled = false, source_timer_enabled = false;

elapsedMillis vol_timer = 0;
int vol_steps;
bool vol_turned = false;

elapsedMillis nav_timer = 0;
int nav_steps;
bool nav_turned = false;

elapsedMillis door_timer;
bool door_timer_enabled = false;

void setup() {
	pinMode(ILL_CS, OUTPUT);
	pinMode(ILL_ON, OUTPUT);
	pinMode(FULL_POWER_ON, OUTPUT);
	pinMode(MCP_RESET, OUTPUT);

	pinMode(OPEN_CLOSE_CS, OUTPUT);
	pinMode(BL_ON, OUTPUT);
	
	pinMode(BT_EJECT, INPUT_PULLUP);
	pinMode(BT_SOURCE, INPUT_PULLUP);
	pinMode(BT_AUDIO, INPUT_PULLUP);
	pinMode(BT_FMAM, INPUT_PULLUP);
	pinMode(BT_AUX, INPUT_PULLUP);
	pinMode(BT_SKIPUP, INPUT_PULLUP);
	pinMode(BT_SKIPDN, INPUT_PULLUP);
	pinMode(BT_INFO, INPUT_PULLUP);
	pinMode(BT_TONE, INPUT_PULLUP);
	pinMode(BT_HOME, INPUT_PULLUP);
	pinMode(BT_SETUP, INPUT_PULLUP);
	pinMode(BT_BACK, INPUT_PULLUP);

	pinMode(BT_F1, INPUT_PULLUP);
	pinMode(BT_F2, INPUT_PULLUP);
	pinMode(BT_F3, INPUT_PULLUP);
	pinMode(BT_F4, INPUT_PULLUP);
	pinMode(BT_F5, INPUT_PULLUP);
	pinMode(BT_F6, INPUT_PULLUP);

	pinMode(VOL_CLK, INPUT);
	pinMode(VOL_UP, INPUT);
	pinMode(NAV_CLK, INPUT);
	pinMode(NAV_UP, INPUT);
	pinMode(NAV_CS, OUTPUT);
	
	digitalWrite(ILL_CS, HIGH);
	digitalWrite(ILL_ON, LOW);
	digitalWrite(FULL_POWER_ON, LOW);
	digitalWrite(MCP_RESET, HIGH);
	digitalWrite(NAV_CS, HIGH);
	digitalWrite(OPEN_CLOSE_CS, HIGH);
	digitalWrite(BL_ON, LOW);
	
	AISerial.begin(AI_BAUD);
	
	SPI.begin();
	
	ill_mcp4251.begin();
	ill_mcp4251.DigitalPotSetWiperPosition(0,0);
	ill_mcp4251.DigitalPotSetWiperPosition(1,256);

	nav_mcp.begin();
	nav_mcp.pinModeIO(NAV_MCP_NAV_PUSH, INPUT_PULLUP);
	nav_mcp.pinModeIO(NAV_MCP_NAV_UP, INPUT_PULLUP);
	nav_mcp.pinModeIO(NAV_MCP_NAV_DN, INPUT_PULLUP);
	nav_mcp.pinModeIO(NAV_MCP_NAV_LEFT, INPUT_PULLUP);
	nav_mcp.pinModeIO(NAV_MCP_NAV_RIGHT, INPUT_PULLUP);
	nav_mcp.pinModeIO(NAV_MCP_VOL_PUSH, INPUT_PULLUP);
	
	nav_mcp.pinModeIO(NAV_MCP_ILL_AIDF, OUTPUT);
	nav_mcp.digitalWriteIO(NAV_MCP_ILL_AIDF, false);

	open_close_mcp.begin();
	open_close_mcp.pinModeIO(OC_MCP_OPEN_IND, INPUT_PULLUP);
	open_close_mcp.pinModeIO(OC_MCP_CLOSE_IND, INPUT_PULLUP);
	open_close_mcp.pinModeIO(OC_MCP_OPEN_TOG, OUTPUT);
	open_close_mcp.pinModeIO(OC_MCP_CLOSE_TOG, OUTPUT);

	open_close_mcp.digitalWriteIO(OC_MCP_OPEN_TOG, false);
	open_close_mcp.digitalWriteIO(OC_MCP_CLOSE_TOG, false);
}

void loop() {
	AIData msg;
	elapsedMillis ai_timer;
	
	do {
		bool message_read = false;
		if(AISerial.available() > 0) {
			if(msg.sender == ID_RADIO && !parameters.radio_connected)
				parameters.radio_connected = true;
			
			if(msg.sender == ID_NAV_COMPUTER && !parameters.computer_connected)
				parameters.computer_connected = true;

			if(ai_handler.readAIData(&msg)) {
				if(msg.sender == ID_NAV_SCREEN)
					continue;

				if(msg.receiver == ID_NAV_SCREEN && msg.l >= 1 && msg.data[0] != 0x80)
					message_read = true;
				else if(msg.receiver == 0xFF && msg.l >= 1 && msg.data[0] == 0xA1) {
					if(msg.sender == ID_CANSLATOR && msg.l >= 0x4 && msg.data[1] == 0x10) { //Light control.
						const uint8_t brightness = msg.data[2];
						const bool light = (msg.data[3]&0x1) != 0;

						digitalWrite(ILL_ON, light ? HIGH : LOW);
						ill_mcp4251.DigitalPotSetWiperPosition(0, brightness < 255 ? brightness : 256);
						ill_mcp4251.DigitalPotSetWiperPosition(1, brightness < 255 ? brightness : 256);
					} else if(msg.sender == ID_CANSLATOR && msg.l >= 3 && msg.data[1] == 0x2) { //Key position.
						const uint8_t last_key = parameters.key_position;
						parameters.key_position = msg.data[2]&0xF;

						if(parameters.key_position != last_key) {
							if(parameters.key_position != 0) {
								digitalWrite(FULL_POWER_ON, HIGH);
								digitalWrite(BL_ON, HIGH);
							} else {
								if((parameters.door_position&0xC) != 0)
									digitalWrite(FULL_POWER_ON, LOW);
								else {
									door_timer_enabled = true;
									door_timer = 0;
								}
							} 
						}
					} else if(msg.sender == ID_CANSLATOR && msg.l >= 3 && msg.data[1] == 0x43) { //Door opened or closed.
						const uint8_t last_door = parameters.door_position;
						parameters.door_position = msg.data[2];
						const uint8_t front_door_position = parameters.door_position&0xC;

						if(parameters.key_position == 0 && front_door_position != (last_door&0xC)) {
							const bool power_on = digitalRead(FULL_POWER_ON) != LOW;

							if(front_door_position != 0) {
								if(power_on) {
									if(!door_timer_enabled)
										digitalWrite(FULL_POWER_ON, LOW);
								} else {
									digitalWrite(FULL_POWER_ON, HIGH);
									digitalWrite(BL_ON, HIGH);
									door_timer_enabled = true;
									door_timer = 0;
								}
							}
						}
					}
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
				} else if(msg.l >= 2 && msg.data[0] == 0x38) { //Open/close.
					const uint8_t command = msg.data[1];
					if(command == 0)
						open_handler.setClosed();
					else if(command == 2)
						open_handler.setOpen();
				} else if(msg.l >= 4 && msg.data[0] == 0x33) { //Forward.
					ack = false;
					ai_handler.sendAcknowledgement(ID_NAV_SCREEN, msg.sender);

					const uint8_t dest = msg.data[1], button = msg.data[2], state = msg.data[3];
					uint8_t button_data[] = {0x30, button, state};
					AIData button_msg(sizeof(button_data), ID_NAV_SCREEN, dest);
					button_msg.refreshAIData(button_data);

					ai_handler.writeAIData(&button_msg);
				} else if(msg.l >= 2 && msg.data[0] == 0x34) { //AidF logo on/off.
					const bool logo_on = msg.data[1] != 0;
					nav_mcp.digitalWriteIO(NAV_MCP_ILL_AIDF, logo_on);
				}

				if(ack)
					ai_handler.sendAcknowledgement(ID_NAV_SCREEN, msg.sender);
			}

			ai_timer = 0;
		}
	} while(ai_timer < 50);

	const uint8_t nav_states = nav_mcp.getInputStates();
	for(int i=0;i<TOGGLE_INDEX_SIZE;i+=1) {
		if(i<TOGGLE_INDEX_SIZE - 1)
			button_handler.toggle[i] = (nav_states&bit(i)) != 0;
		else
			button_handler.toggle[i] = (nav_states&0x80) != 0;
	}

	button_handler.loop();
	
	const bool last_vol_turned = vol_turned;
	vol_turned = digitalRead(VOL_CLK) != LOW;
	if(!vol_turned && last_vol_turned) {
		vol_timer = 0;
		
		const bool cw = digitalRead(VOL_UP) != LOW;
		if(cw)
			vol_steps += 1;
		else
			vol_steps -= 1;
	}
	
	const bool last_nav_turned = nav_turned;
	nav_turned = digitalRead(NAV_CLK) != LOW;
	if(!nav_turned && last_nav_turned) {
		nav_timer = 0;
		const bool cw = digitalRead(NAV_UP) != LOW;
		if(cw)
			nav_steps += 1;
		else
			nav_steps -= 1;
	}
	
	if((vol_timer > VOL_TIMER || abs(vol_steps) >= 0xF) && vol_steps != 0)
		setVolume(parameters.audio_dest);
	
	if((nav_timer > VOL_TIMER || abs(nav_steps) >= 0xF) && nav_steps != 0)
		setVolume(parameters.all_dest);

	if(door_timer_enabled && door_timer > DOOR_TIMER && parameters.key_position == 0)
		digitalWrite(FULL_POWER_ON, LOW);
}

//Respond to a button query.
void sendButtonsPresent(const uint8_t receiver) {
	uint8_t button_data[] = {0x31, 0x30, 0x6, 0x6, 0x7, 0x23, 0x26, 0x36, 0x38, 0x25, 0x24, 0x53, 0x52, 0x20, 0x51, 0x27, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x28, 0x29, 0x2A, 0x2B};
	AIData button_msg(sizeof(button_data), ID_NAV_SCREEN, receiver);
	button_msg.refreshAIData(button_data);

	ai_handler.writeAIData(&button_msg);
}

//Send a volume message.
void setVolume(const uint8_t receiver) {
	while(abs(vol_steps) > 0) {
		uint8_t vol_data[] = {0x32, 0x6, abs(vol_steps)&0xF};
		if(vol_steps > 0)
			vol_data[2] |= 0x10;

		AIData vol_msg(sizeof(vol_data), ID_NAV_SCREEN, receiver);
		vol_msg.refreshAIData(vol_data);

		bool ack = true;
		if(receiver == ID_NAV_COMPUTER && !parameters.computer_connected)
			ack = false;
		else if(receiver == ID_RADIO && !parameters.radio_connected)
			ack = false;
		else if(receiver == 0xFF || receiver == ID_NAV_SCREEN)
			ack = false;
		
		ai_handler.writeAIData(&vol_msg, ack);

		if(abs(vol_steps) <= 0xF)
			vol_steps = 0;
		else {
			if(vol_steps > 0)
				vol_steps -= 0xF;
			else
				vol_steps += 0xF;
		}

		ai_handler.cachePending(ID_NAV_SCREEN);
	}

	vol_steps = 0;
}

//Send a nav knob message.
void setNavigation(const uint8_t receiver) {
	while(abs(nav_steps) > 0) {
		uint8_t nav_data[] = {0x32, 0x7, abs(nav_steps)&0xF};
		if(nav_steps > 0)
			nav_data[2] |= 0x10;

		AIData nav_msg(sizeof(nav_data), ID_NAV_SCREEN, receiver);
		nav_msg.refreshAIData(nav_data);

		bool ack = true;
		if(receiver == ID_NAV_COMPUTER && !parameters.computer_connected)
			ack = false;
		else if(receiver == ID_RADIO && !parameters.radio_connected)
			ack = false;
		else if(receiver == 0xFF || receiver == ID_NAV_SCREEN)
			ack = false;

		ai_handler.writeAIData(&nav_msg, ack);

		if(abs(nav_steps) <= 0xF)
			nav_steps = 0;
		else {
			if(nav_steps > 0)
				nav_steps -= 0xF;
			else
				nav_steps += 0xF;
		}

		ai_handler.cachePending(ID_NAV_SCREEN);
	}

	nav_steps = 0;
}