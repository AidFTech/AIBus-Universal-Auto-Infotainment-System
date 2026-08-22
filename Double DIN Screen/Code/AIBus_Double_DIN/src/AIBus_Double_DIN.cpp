#include "AIBus_Double_DIN.h"

AIBusDoubleDIN aibus_double_din;

static const volatile uint8_t aibt_edid[] = {
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x05, 0x24, 0x00, 0x01,
	0x01, 0x00, 0x00, 0x00, 0x01, 0x11, 0x01, 0x03, 0x81, 0x0E, 0x08, 0x78,
	0x2E, 0x35, 0x85, 0xA6, 0x56, 0x48, 0x9A, 0x24, 0x12, 0x50, 0x54, 0xAF,
	0xEF, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x3E, 0x0D, 0x20, 0x00, 0x31, 0xE0,
	0x37, 0x10, 0x2C, 0x58, 0x36, 0x00, 0xDC, 0x0C, 0x11, 0x00, 0x00, 0x1E,
	0x00, 0x00, 0x00, 0xFC, 0x00, 0x41, 0x69, 0x64, 0x46, 0x20, 0x41, 0x49,
	0x42, 0x54, 0x0A, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBA
};


volatile int static_vol_steps = 0, static_nav_steps = 0;
elapsedMillis static_vol_timer, static_nav_timer;

//Arduino setup.
void setup() {
	aibus_double_din.setup();
}

//Arduino loop.
void loop() {
	aibus_double_din.loop();
}

//Main object setup.
void AIBusDoubleDIN::setup() {
	this->vol_timer = &static_vol_timer;
	this->vol_steps = &static_vol_steps;
	this->nav_timer = &static_nav_timer;
	this->nav_steps = &static_nav_steps;

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
	digitalWrite(FULL_POWER_ON, HIGH);
	digitalWrite(MCP_RESET, HIGH);
	digitalWrite(NAV_CS, HIGH);
	digitalWrite(OPEN_CLOSE_CS, HIGH);
	digitalWrite(BL_ON, LOW);
	
	AISerial.begin(AI_BAUD);
	
	SPI.begin();
	delay(100);
	
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

	attachInterrupt(VOL_CLK, incVolume, FALLING);
	attachInterrupt(NAV_CLK, incNavigation, FALLING);

	uint8_t init_data[] = {0x4A, 0x1F};
	AIData init_msg(sizeof(init_data), ID_NAV_SCREEN, ID_CANSLATOR, init_data);
	ai_handler.writeAIData(&init_msg, false);

	/*Wire.begin(0x50);

	Wire.onReceive(receiveI2C);
	Wire.onRequest(handleEDID);
	
	for(int i=0;i<sizeof(aibt_edid);i+=1)
		Wire.write(aibt_edid[i]);*/

	uint8_t poweroff_data[] = {0xA0};
	AIData poweroff_msg(sizeof(poweroff_data), ID_NAV_SCREEN, 0xFF, poweroff_data);
	ai_handler.writeAIData(&poweroff_msg, false);

	digitalWrite(FULL_POWER_ON, LOW);
}

//Main object loop.
void AIBusDoubleDIN::loop() {
	AIData msg;
	elapsedMillis ai_timer = 0;
	bool first_msg = false;
	
	do {
		bool message_read = false;
		if(ai_handler.dataAvailable() > 0) {
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
								nav_mcp.digitalWriteIO(NAV_MCP_ILL_AIDF, audio_on);
								parameters.allow_open = true;
								key_on = true;
								door_timer_enabled = false;
							} else {
								if((parameters.door_position&0xC) != 0) {
									nav_mcp.digitalWriteIO(NAV_MCP_ILL_AIDF, false);

									parameters.allow_open = false;

									uint8_t poweroff_data[] = {0xA0};
									AIData poweroff_msg(sizeof(poweroff_data), ID_NAV_SCREEN, 0xFF, poweroff_data);
									ai_handler.writeAIData(&poweroff_msg, false);

									digitalWrite(FULL_POWER_ON, LOW);
									digitalWrite(BL_ON, LOW);
								} else {
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
								if(power_on || (parameters.door_position&0x80) != 0) {
									if(key_on || (parameters.door_position&0x80) != 0) {
										nav_mcp.digitalWriteIO(NAV_MCP_ILL_AIDF, false);

										uint8_t poweroff_data[] = {0xA0};
										AIData poweroff_msg(sizeof(poweroff_data), ID_NAV_SCREEN, 0xFF, poweroff_data);
										ai_handler.writeAIData(&poweroff_msg, false);

										parameters.allow_open = false;

										digitalWrite(FULL_POWER_ON, LOW);
										digitalWrite(BL_ON, LOW);
									}
								} else {
									digitalWrite(FULL_POWER_ON, HIGH);
									digitalWrite(BL_ON, HIGH);
									nav_mcp.digitalWriteIO(NAV_MCP_ILL_AIDF, audio_on);
									door_timer_enabled = true;
									door_timer = 0;
								}
							}
						}
					}
				}
			}

			if(message_read) {
				if(msg.sender == ID_NAV_SCREEN)
					continue;

				if(msg.l >= 2 && msg.data[0] == 0x31 && msg.data[1] == 0x30) { //Button request.
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
					const uint8_t dest = msg.data[1], button = msg.data[2], state = msg.data[3];
					uint8_t button_data[] = {0x30, button, state};
					AIData button_msg(sizeof(button_data), ID_NAV_SCREEN, dest, button_data);

					ai_handler.writeAIData(&button_msg);
				} else if(msg.l >= 2 && msg.data[0] == 0x34) { //AidF logo on/off.
					const bool logo_on = msg.data[1] != 0;
					nav_mcp.digitalWriteIO(NAV_MCP_ILL_AIDF, logo_on);
					audio_on = logo_on;
				} else if(msg.sender == ID_NAV_COMPUTER && msg.l >= 2 && msg[0] == 0x10) { //Screen on/off.
					const bool on = msg[1] != 0x0;
					digitalWrite(BL_ON, on ? HIGH : LOW);
				} else if(msg.l >= 2 && msg.data[0] == 0x3E && msg.data[1] == 0xF0) { //EDID request.
					AIData edid_msg(sizeof(aibt_edid) + 1, ID_NAV_SCREEN, msg.sender);
					edid_msg[0] = 0x3E;
					for(int i=0;i<sizeof(aibt_edid);i+=1)
						edid_msg[i+1] = aibt_edid[i];
					
					ai_handler.writeAIData(&edid_msg);
				} else if(msg.l >= 2 && msg.data[0] == 0x2C && msg.data[1] == 0xF0) { //Screen resolution request.
					const uint16_t w = 800, h = 480;
					uint8_t resolution_data[] = {0x2C, w>>8, w&0xFF, h>>8, h&0xFF};
					AIData resolution_msg(sizeof(resolution_data), ID_NAV_SCREEN, msg.sender, resolution_data);
					ai_handler.writeAIData(&resolution_msg);
				}
			}

			if(!first_msg) {
				ai_timer = 0;
				first_msg = true;
			}
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
	open_handler.loop();
	
	if((*vol_timer > VOL_TIMER || abs(*vol_steps) >= 0xF) && *vol_steps != 0) {
		setVolume(parameters.audio_dest);
		*vol_timer = 0;
		*vol_steps = 0;
	}
	
	if((*nav_timer > VOL_TIMER || abs(*nav_steps) >= 0xF) && *nav_steps != 0) {
		setNavigation(parameters.all_dest);
		*nav_timer = 0;
		*nav_steps = 0;
	}

	if(door_timer_enabled && door_timer > DOOR_TIMER && parameters.key_position == 0) {
		door_timer_enabled = false;
		nav_mcp.digitalWriteIO(NAV_MCP_ILL_AIDF, false);

		uint8_t poweroff_data[] = {0xA0};
		AIData poweroff_msg(sizeof(poweroff_data), ID_NAV_SCREEN, 0xFF, poweroff_data);
		ai_handler.writeAIData(&poweroff_msg, false);

		digitalWrite(FULL_POWER_ON, LOW);
		digitalWrite(BL_ON, LOW);
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
}

//Respond to a button query.
void AIBusDoubleDIN::sendButtonsPresent(const uint8_t receiver) {
	uint8_t button_data[] = {0x31, 0x30, 0x16, 0x6, 0x7, 0x23, 0x26, 0x36, 0x38, 0x25, 0x24, 0x53, 0x52, 0x20, 0x51, 0x27, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x28, 0x29, 0x2A, 0x2B};
	AIData button_msg(sizeof(button_data), ID_NAV_SCREEN, receiver, button_data);

	ai_handler.writeAIData(&button_msg);
}

//Send a volume message.
void AIBusDoubleDIN::setVolume(const uint8_t receiver) {
	while(abs(*vol_steps) > 0) {
		uint8_t vol_data[] = {0x32, 0x6, abs(*vol_steps)&0xF};
		if(*vol_steps > 0)
			vol_data[2] |= 0x10;

		AIData vol_msg(sizeof(vol_data), ID_NAV_SCREEN, receiver, vol_data);

		bool ack = true;
		if(receiver == ID_NAV_COMPUTER && !parameters.computer_connected)
			ack = false;
		else if(receiver == ID_RADIO && !parameters.radio_connected)
			ack = false;
		else if(receiver == 0xFF || receiver == ID_NAV_SCREEN)
			ack = false;
		
		ai_handler.writeAIData(&vol_msg, ack);

		if(abs(*vol_steps) <= 0xF)
			*vol_steps = 0;
		else {
			if(*vol_steps > 0)
				*vol_steps -= 0xF;
			else
				*vol_steps += 0xF;
		}

		ai_handler.cachePending(ID_NAV_SCREEN);
	}

	*vol_steps = 0;
}

//Send a nav knob message.
void AIBusDoubleDIN::setNavigation(const uint8_t receiver) {
	while(abs(*nav_steps) > 0) {
		uint8_t nav_data[] = {0x32, 0x7, abs(*nav_steps)&0xF};
		if(*nav_steps > 0)
			nav_data[2] |= 0x10;

		AIData nav_msg(sizeof(nav_data), ID_NAV_SCREEN, receiver, nav_data);

		bool ack = true;
		if(receiver == ID_NAV_COMPUTER && !parameters.computer_connected)
			ack = false;
		else if(receiver == ID_RADIO && !parameters.radio_connected)
			ack = false;
		else if(receiver == 0xFF || receiver == ID_NAV_SCREEN)
			ack = false;

		ai_handler.writeAIData(&nav_msg, ack);

		if(abs(*nav_steps) <= 0xF)
			*nav_steps = 0;
		else {
			if(*nav_steps > 0)
				*nav_steps -= 0xF;
			else
				*nav_steps += 0xF;
		}

		ai_handler.cachePending(ID_NAV_SCREEN);
	}

	*nav_steps = 0;
}

//Increment the volume function.
void incVolume() {
	if(static_vol_steps == 0)
		static_vol_timer = 0;
		
	const bool cw = digitalRead(VOL_UP) != LOW;
	if(cw)
		static_vol_steps += 1;
	else
		static_vol_steps -= 1;
}

//Increment the navigation function.
void incNavigation() {
	if(static_nav_steps == 0)
		static_nav_timer = 0;

	const bool cw = digitalRead(NAV_UP) == LOW;
	if(cw)
		static_nav_steps += 1;
	else
		static_nav_steps -= 1;
}

//Called when an I2C message is received.
void receiveI2C(int byte_count) {
	while(Wire.available())
		Wire.read();
}

//Send EDID data.
void handleEDID() {
	for(int i=0;i<sizeof(aibt_edid);i+=1)
		Wire.write(aibt_edid[i]);
}
