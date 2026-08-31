#include "Honda_CD_XM_Trans.h"

HondaCDXMTrans honda_cd_xm_trans;

//Arduino setup.
void setup() {
	honda_cd_xm_trans.setup();
}

//Arduino loop.
void loop() {
	honda_cd_xm_trans.loop();
}

//Main class setup.
void HondaCDXMTrans::setup() {
	AISerial.begin(AI_BAUD);
	ai_handler.setCacheAck(true);

	pinMode(GA_ON, OUTPUT);
	digitalWrite(GA_ON, LOW);

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);

	parameters.audio_pin = AUDIO_ON;
	parameters.ie_cache_vec.setStorage(parameters.ie_cache, 0);

	#ifdef DDRA
	DDRA = 0;
	DDRC = 0;
	#endif

	pinMode(ILL_ANODE, OUTPUT);
	//pinMode(IEBUS_TX, OUTPUT);
	//pinMode(IEBUS_RX, INPUT);
	pinMode(ILL_CS, OUTPUT);
	pinMode(MAIN_POWER, OUTPUT);
	pinMode(REC_SET, INPUT);
	pinMode(REC_CLEAR, OUTPUT);
	pinMode(AUDIO_ON, OUTPUT);
	pinMode(TRUNK_OPEN, OUTPUT);

	pinMode(GAH_COUNT_0, INPUT);
	pinMode(GAH_COUNT_1, INPUT);
	pinMode(GAH_COUNT_2, INPUT);
	pinMode(GAH_COUNT_3, INPUT);

	pinMode(GAH_COUNT_ENABLE, OUTPUT);
	pinMode(GAH_COUNT_CLEAR, OUTPUT);
	pinMode(SPDIF_RESET, OUTPUT);

	digitalWrite(ILL_ANODE, LOW);
	//digitalWrite(IEBUS_TX, LOW);
	digitalWrite(ILL_CS, HIGH);
	digitalWrite(MAIN_POWER, LOW);
	digitalWrite(REC_CLEAR, HIGH);
	digitalWrite(AUDIO_ON, LOW);
	digitalWrite(TRUNK_OPEN, LOW);

	digitalWrite(GAH_COUNT_CLEAR, HIGH);
	digitalWrite(GAH_COUNT_CLEAR, LOW);

	digitalWrite(GAH_COUNT_ENABLE, HIGH);
	
	digitalWrite(SPDIF_RESET, HIGH);
	delay(100);
	digitalWrite(SPDIF_RESET, LOW);
	delay(100);
	digitalWrite(SPDIF_RESET, HIGH);

	EnIEBusParams ie_params;
	ie_params.count_enable = GAH_COUNT_ENABLE;
	ie_params.count_reset = GAH_COUNT_CLEAR;
	ie_params.rec_set = REC_SET;
	ie_params.rec_clear = REC_CLEAR;
	ie_params.ai_handler = &ai_handler;
	#ifdef PINA
	ie_params.high_byte_register = GAH_READ_H;
	ie_params.low_byte_register = GAH_READ_L;
	#endif

	ie_params.count[0] = GAH_COUNT_0;
	ie_params.count[1] = GAH_COUNT_1;
	ie_params.count[2] = GAH_COUNT_2;
	ie_params.count[3] = GAH_COUNT_3;

	ie_handler.init(&ie_params);

	//sendWideHandshake(&ie_handler);
	//sendPingHandshake(&ie_handler, IE_ID_IMID);
	//sendPingHandshake(&ie_handler, IE_ID_CDC);
	//sendPingHandshake(&ie_handler, IE_ID_TAPE);
	//sendPingHandshake(&ie_handler, IE_ID_SIRIUS);
	parameters.screen_request_timer = &screen_request_timer;

	#ifdef MEMORY_CHECK
	Serial.begin(115200);
	Serial.println("Ready!");
	#endif

	brightness_handler.init();
	
	uint8_t init_data[] = {0x4A, 0x1F};
	AIData init_msg(sizeof(init_data), ID_CDC, ID_CANSLATOR, init_data);

	digitalWrite(MAIN_POWER, HIGH);
	ai_handler.writeAIData(&init_msg, false);
	ai_handler.flush();
	digitalWrite(MAIN_POWER, LOW);
}

//Main class loop.
void HondaCDXMTrans::loop() {
	#ifdef MEMORY_CHECK
	Serial.println(freeMemory());
	Serial.flush();
	#endif

	if(memory_timer > 1200) {
		memory_timer = 0;
		int mem = freeMemory();

		if(mem > (8192 - 4848))
			mem = 0;

		const int cache_limit = mem/4/(sizeof(AIData) + AIDATA_LIMIT);

		ai_handler.setMultiCacheLimit(cache_limit);

		#ifdef MEMORY_CHECK
		const String mem_str = String(mem) + ": " + cache_limit;
		AIData mem_msg(mem_str.length() + 2, ID_IMID_SCR, 0xFF);
		mem_msg[0] = 0xA1;
		mem_msg[1] = 0xFF;
		for(int i=0;i<mem_str.length();i+=1)
			mem_msg[i+2] = uint8_t(mem_str[i]);

		ai_handler.writeAIData(&mem_msg, false);
		#endif
	}

	IE_Message ie_msg;

	elapsedMillis message_timer;
	bool first_ie = false;

	while(message_timer < 50) {
		const bool ie_ready = !(imid_handler.getEstablished() &&
								imid_handler.getIMIDChangeTimer() < IMID_CHANGE_TIMER &&
								!cd_handler.getSelected() &&
								!tape_handler.getSelected() &&
								!xm_handler.getSelected());

		if (parameters.power_on && ie_ready) { // Don't check IEBus if the car isn't on or the IMID needs to change.
			//if (ai_handler.dataAvailable(false) > 0)
			//	ai_handler.cacheAllPending();

			const bool was_first_ie = first_ie;
			if (ie_handler.getInputOn()) {
				if (!first_ie) {
					first_ie = true;
					//ie_timer = 0;
				}

				const int message_value = ie_handler.readMessage(&ie_msg, true, IE_ID_RADIO);
				if (message_value == 0) {
					IE_Message check_msg(ie_msg.l - 1, ie_msg.sender, ie_msg.receiver, ie_msg.control, ie_msg.direct);
					for (int i = 0; i < ie_msg.l - 1; i += 1)
						check_msg.data[i] = ie_msg.data[i];

					if (ie_msg.checkVaildity()) {
						parameters.last_iebus_msg = 0;
						if (ie_msg.receiver == IE_ID_RADIO && ie_msg.l >= 1 && ie_msg.data[0] != 0x80) {
							ie_handler.sendAcknowledgement(ie_msg.receiver, ie_msg.sender);

							/*{ 
								AIData ie_ai(ie_msg.l + 2, ie_msg.sender&0xFF, 0xFF);
								ie_ai[0] = 0xA1;
								ie_ai[1] = 0xFF;
								for(int i=0;i<ie_msg.l;i+=1)
									ie_ai[i+2] = ie_msg.data[i];

								ai_handler.writeAIData(&ie_ai, false);
							}*/

							interpretIEData(ie_msg);
						}
					}
				} else if(message_value > 0 || message_value < -1)
					message_timer = 0;

				digitalWrite(REC_CLEAR, LOW);
				digitalWrite(REC_CLEAR, HIGH);

				ai_handler.clearSerial();
			}

			if(!first_ie || (first_ie && !was_first_ie))
				ai_handler.cacheAllPending();

			ie_handler.clearRX();

			if(digitalRead(REC_SET) == HIGH) { //Possible miss.
				digitalWrite(REC_CLEAR, LOW);
				digitalWrite(REC_CLEAR, HIGH);

				if(cd_handler.getSelected())
					cd_handler.refreshSource();
				else if(tape_handler.getSelected())
					tape_handler.refreshSource();
				else if(xm_handler.getSelected())
					xm_handler.refreshSource();
			}
		}
	
		AIData ai_msg;
		bool ai_received = false;
		const unsigned int avail = ai_handler.dataAvailable();
		if (avail > 0) {
			const aibus_read_result_t ai_result = ai_handler.readAIDataErr(&ai_msg);
			if (getPositiveResult(ai_result) && (ai_handler.getValidMessage(&ai_msg) || 
				(!parameters.computer_connected && ai_msg.sender == ID_NAV_COMPUTER) ||
				(!parameters.screen_connected && ai_msg.sender == ID_NAV_SCREEN) ||
				(!parameters.mirror_connected && ai_msg.sender == ID_ANDROID_AUTO) ||
				(!parameters.radio_connected && ai_msg.sender == ID_RADIO)
			)) {
				ai_received = true;
			} else if(ai_result == AIBUS_READ_TIMEOUT ||
						ai_result == AIBUS_READ_INVALID_CHECKSUM) {
				//Set the timer to ping the CANslator just in case we missed something.
				power_ping_timer = POWER_PING_TIMER - 200;
			}
		}

		if (ai_received) {
			if(!cd_handler.getSelected() && !tape_handler.getSelected() && !xm_handler.getSelected())
				message_timer = 0;

			if ((ai_msg.sender == ID_IMID_SCR && imid_handler.getEstablished()) ||
				(ai_msg.sender == ID_TAPE && tape_handler.getEstablished()) ||
				(ai_msg.sender == ID_CDC && cd_handler.getEstablished()) ||
				(ai_msg.sender == ID_XM && xm_handler.getEstablished()))
				continue;

			if (!parameters.computer_connected && ai_msg.sender == ID_NAV_COMPUTER && !getInitMessage(&ai_msg) && !getPoweroffMessage(&ai_msg)) {
				parameters.computer_connected = true;
				dimension_request_timer = DIMENSION_REQUEST_TIMER - 10;
				ai_handler.setCacheAck(false);
			}

			if (!parameters.screen_connected && ai_msg.sender == ID_NAV_SCREEN && !getInitMessage(&ai_msg) && !getPoweroffMessage(&ai_msg))
				parameters.screen_connected = true;

			if (!parameters.mirror_connected && ai_msg.sender == ID_ANDROID_AUTO && !getInitMessage(&ai_msg) && !getPoweroffMessage(&ai_msg))
				parameters.mirror_connected = true;

			#ifndef AI_DEBUG
			if (!parameters.radio_connected && ai_msg.sender == ID_RADIO && !getInitMessage(&ai_msg) && !getPoweroffMessage(&ai_msg)) {
				parameters.radio_connected = true;

				if (cd_handler.getEstablished())
					cd_handler.sendSourceNameMessage();
				if (tape_handler.getEstablished())
					tape_handler.sendSourceNameMessage();
				if (xm_handler.getEstablished())
					xm_handler.sendSourceNameMessage();

				if (imid_handler.getEstablished()) {
					imid_handler.writeScreenLayoutMessage();
					imid_handler.writeVolumeLimitMessage();
				}
			}

			if(!parameters.canslator_connected && ai_msg.sender == ID_CANSLATOR && !getInitMessage(&ai_msg) && !getPoweroffMessage(&ai_msg)) {
				parameters.canslator_connected = true;
			}
		 
			#endif
			
			if(getInitMessage(&ai_msg) || getPoweroffMessage(&ai_msg)) {
				if(ai_msg.sender == ID_RADIO)
					parameters.radio_connected = false;
				else if(ai_msg.sender == ID_NAV_COMPUTER) {
					parameters.computer_connected = false;
					parameters.has_dimensions = false;
					ai_handler.setCacheAck(true);
				} else if(ai_msg.sender == ID_NAV_SCREEN)
					parameters.screen_connected = false;
				else if(ai_msg.sender == ID_ANDROID_AUTO)
					parameters.mirror_connected = false;
			}

			if(ai_msg.receiver == 0xFF && ai_msg.l == 1 && ai_msg[0] == 0x1) { //Ping.
				uint8_t ping_data[] = {0x1};
				if(cd_handler.getEstablished()) {
					AIData ping_msg(sizeof(ping_data), ID_CDC, ai_msg.sender, ping_data);
					ai_handler.writeAIData(&ping_msg);
				}
				if(imid_handler.getEstablished()) {
					AIData ping_msg(sizeof(ping_data), ID_IMID_SCR, ai_msg.sender, ping_data);
					ai_handler.writeAIData(&ping_msg);
				}
				if(tape_handler.getEstablished()) {
					AIData ping_msg(sizeof(ping_data), ID_TAPE, ai_msg.sender, ping_data);
					ai_handler.writeAIData(&ping_msg);
				}
				if(xm_handler.getEstablished()) {
					AIData ping_msg(sizeof(ping_data), ID_XM, ai_msg.sender, ping_data);
					ai_handler.writeAIData(&ping_msg);
				}
			} else if (ai_msg.sender == ID_IMID_SCR && !imid_handler.getEstablished()) { //An external IMID is available.
				if (ai_msg.l >= 2 && ai_msg.data[0] == 0x3B) { // External IMID is announcing itself.
					if (ai_msg.data[1] == 0x23 && ai_msg.l >= 4) { // Custom text field.
						parameters.external_imid_char = ai_msg.data[2];
						parameters.external_imid_lines = ai_msg.data[3];
					} else if (ai_msg.data[1] == 0x57) { //OEM text field.
						for (int i = 2; i < ai_msg.l; i += 1) {
							if (ai_msg.data[i] == ID_CDC || ai_msg.data[i] == ID_CD)
								parameters.external_imid_cd = true;
							else if (ai_msg.data[i] == ID_TAPE)
								parameters.external_imid_tape = true;
							else if (ai_msg.data[i] == ID_XM)
								parameters.external_imid_xm = true;
						}
					}
				}
			} else if(ai_msg.sender == ID_RADIO && ai_msg.l >= 3 && ai_msg[0] == 0x40 && ai_msg[1] == 0x10) {
				parameters.radio_ping_timer = 0;
			} else if(ai_msg.sender == ID_RADIO && ai_msg.l >= 3 && ai_msg[0] == 0x70 && ai_msg[1] == 0x10) {
				parameters.radio_ping_timer = 0;
				
				const uint8_t selected = ai_msg[2];
				uint8_t request_data[] = {0x60, 0x10};

				if((selected == ID_CDC || selected == ID_CD || selected == ID_TAPE || selected == ID_XM) &&
				ai_handler.getValidMessage(&ai_msg) && !cd_handler.getSelected() && !tape_handler.getSelected() && !xm_handler.getSelected()) {
					AIData request_msg(sizeof(request_data), selected, ID_RADIO, request_data);
					ai_handler.writeAIData(&request_msg);

					uint8_t text_request_data[] = {0x60, 0x11};
					AIData text_request_msg(sizeof(text_request_data), selected, ID_RADIO, text_request_data);
					ai_handler.writeAIData(&text_request_msg);
				} else if(selected != ID_CDC && selected != ID_CD && cd_handler.getSelected()) {
					AIData request_msg(sizeof(request_data), ID_CDC, ID_RADIO, request_data);
					ai_handler.writeAIData(&request_msg);
				} else if(selected != ID_TAPE && tape_handler.getSelected()) {
					AIData request_msg(sizeof(request_data), ID_TAPE, ID_RADIO, request_data);
					ai_handler.writeAIData(&request_msg);
				} else if(selected != ID_XM && xm_handler.getSelected()) {
					AIData request_msg(sizeof(request_data), ID_XM, ID_RADIO, request_data);
					ai_handler.writeAIData(&request_msg);
				}
			} else if(ai_msg.sender == ID_NAV_COMPUTER && ai_msg.l >= 5 && ai_msg[0] == 0x2C) { //Dimensions.
				parameters.screen_w = (ai_msg[1]<<8) | ai_msg[2];
				parameters.screen_h = (ai_msg[3]<<8) | ai_msg[4];
				parameters.has_dimensions = true;
			}

			if (ai_handler.getValidMessage(&ai_msg) && ai_msg.l >= 2 && ai_msg.data[0] == 0xA1) { // Broadcast message.
				if (ai_msg.l >= 3 && ai_msg.data[1] == 0x1F) {
					if (ai_msg.data[2] == 0x1 && ai_msg.l >= 5) { // Time.
						parameters.hour = ai_msg.data[3]&0x1F;
						parameters.minute = ai_msg.data[4];
						parameters.display_24h = (ai_msg.data[3]&0x80) == 0;
						imid_handler.writeTimeAndDayMessage(parameters.hour, parameters.minute, parameters.month, parameters.date, parameters.year, parameters.display_24h);
					} else if (ai_msg.data[2] == 0x2 && ai_msg.l >= 7) { // Date.
						parameters.year = (ai_msg.data[3] << 8) | ai_msg.data[4];
						parameters.month = ai_msg.data[5];
						parameters.date = ai_msg.data[6];
						imid_handler.writeTimeAndDayMessage(parameters.hour, parameters.minute, parameters.month, parameters.date, parameters.year, parameters.display_24h);
					}
				}
				else if (ai_msg.sender == ID_CANSLATOR && ai_msg.l >= 3 && ai_msg.data[1] == 0x2) { // Key position.
					const uint8_t last_key = parameters.key_position, key = ai_msg.data[2] & 0xF;

					parameters.key_position = key;
					if (key != last_key) {
						if (key != 0) {
							digitalWrite(GA_ON, HIGH);
							digitalWrite(MAIN_POWER, HIGH);
							parameters.power_on = true;
							door_timer_enabled = false;

							sendWideHandshake(&ie_handler);
							sendPingHandshake(&ie_handler, IE_ID_IMID);
							sendPingHandshake(&ie_handler, IE_ID_CDC);
							sendPingHandshake(&ie_handler, IE_ID_TAPE);
							sendPingHandshake(&ie_handler, IE_ID_SIRIUS);
						} else {
							if ((parameters.door_position & 0xF) != 0)
								powerOff();
							else {
								door_timer_enabled = true;
								door_timer = 0;
							}
						}
					}
				} else if (ai_msg.sender == ID_CANSLATOR && ai_msg.l >= 3 && ai_msg.data[1] == 0x43) { // Doors.
					const uint8_t last_door_state = parameters.door_position;
					parameters.door_position = ai_msg.data[2] & 0xF;

					if (last_door_state == 0x0 && parameters.door_position != 0x0 && parameters.key_position == 0) {
						powerOff();
					}

					const bool trunk_open = (ai_msg.data[2] & 0x10) != 0;

					if (trunk_open) {
						digitalWrite(TRUNK_OPEN, HIGH);
						digitalWrite(MAIN_POWER, HIGH);
					} else {
						digitalWrite(TRUNK_OPEN, LOW);
						if (!parameters.power_on)
							digitalWrite(MAIN_POWER, LOW);
					}
				} else if (ai_msg.sender == ID_CANSLATOR && ai_msg.l >= 4 && ai_msg.data[1] == 0x10) { // Lights.
					const bool illum = (ai_msg.data[3] & 0x1) != 0;
					const uint8_t brightness = ai_msg.data[2];

					brightness_handler.setBrightness(brightness, illum);
				}
			} else if (ai_msg.receiver == 0xFF && ai_msg.sender == ID_RADIO) {
				if (ai_msg.l >= 3 && ai_msg.data[0] == 0x4 && ai_msg.data[1] == 0xE6 && ai_msg.data[2] == 0x10) {
					elapsedMillis wait_timer;
					while (wait_timer < 20) {
						ie_handler.cacheAIBus();
					}

					if (cd_handler.getEstablished())
						cd_handler.sendSourceNameMessage();
					if (tape_handler.getEstablished())
						tape_handler.sendSourceNameMessage();
					if (xm_handler.getEstablished())
						xm_handler.sendSourceNameMessage();
					if (imid_handler.getEstablished())
						imid_handler.writeScreenLayoutMessage();
				}
			} else if (ai_msg.receiver == ID_CDC && cd_handler.getEstablished())
				cd_handler.readAIBusMessage(&ai_msg);
			else if (ai_msg.receiver == ID_TAPE && tape_handler.getEstablished())
				tape_handler.readAIBusMessage(&ai_msg);
			else if (ai_msg.receiver == ID_IMID_SCR && imid_handler.getEstablished())
				imid_handler.readAIBusMessage(&ai_msg);
			else if (ai_msg.receiver == ID_XM && xm_handler.getEstablished())
				xm_handler.readAIBusMessage(&ai_msg);
		}
	}

	if (parameters.power_on) {
		if (function_timer > FUNCTION_DELAY) {
			function_timer = 0;

			if (imid_handler.getEstablished()) {
				const uint16_t source = imid_handler.getMode();
				imid_handler.setIMIDSource(source & 0xFF, (source & 0xFF00) >> 8);
			}

			if (cd_handler.getSelected()) {
				uint8_t cdc_function[] = {0x6, 0x0, 0x1};
				sendFunctionMessage(&ie_handler, false, IE_ID_CDC, cdc_function, sizeof(cdc_function));
			} else if (tape_handler.getSelected()) {
				uint8_t tape_function[] = {0x13, 0x0, 0x1};
				sendFunctionMessage(&ie_handler, false, IE_ID_TAPE, tape_function, sizeof(tape_function));
				//tape_handler.refreshSource();
			} else if (xm_handler.getSelected()) {
				uint8_t xm_function[] = {0x19, 0x0, 0x1};
				if (xm_handler.getXM2())
					xm_function[2] = 0x2;
				sendFunctionMessage(&ie_handler, false, IE_ID_SIRIUS, xm_function, sizeof(xm_function));
			}
		}

		imid_handler.loop();
		tape_handler.loop();
		cd_handler.loop();
		xm_handler.loop();

		if (screen_request_timer > SOURCE_DELAY) {
			screen_request_timer = 0;
			if (cd_handler.getSelected())
				cd_handler.requestControl();
			else if (tape_handler.getSelected())
				tape_handler.requestControl();
			else if (xm_handler.getSelected())
				xm_handler.requestControl();
		}

		if (ping_timer > SOURCE_DELAY) {
			ping_timer = 0;

			uint8_t sender_id = 0;
			if (cd_handler.getEstablished())
				sender_id = ID_CDC;
			else if (tape_handler.getEstablished())
				sender_id = ID_TAPE;
			else if (xm_handler.getEstablished())
				sender_id = ID_XM;

			if (!parameters.imid_connected && !parameters.external_imid_cd && !parameters.external_imid_tape && !parameters.external_imid_xm && parameters.external_imid_lines <= 0 && parameters.external_imid_char <= 0) {
				if (sender_id != 0) {
					uint8_t ping_data[] = {0x4, 0xE6, 0x3B};
					AIData ping_msg(sizeof(ping_data), sender_id, ID_IMID_SCR);

					ping_msg.refreshAIData(ping_data);
					ai_handler.writeAIData(&ping_msg, false);
				}
			}

			if (!parameters.radio_connected) {
				if (sender_id != 0) {
					uint8_t ping_data[] = {0x1};
					AIData ping_msg(sizeof(ping_data), sender_id, ID_RADIO);

					ping_msg.refreshAIData(ping_data);
					ai_handler.writeAIData(&ping_msg, false);
				}
			}
			if(!parameters.computer_connected) {
				if (sender_id != 0) {
					uint8_t ping_data[] = {0x1};
					AIData ping_msg(sizeof(ping_data), sender_id, ID_NAV_COMPUTER, ping_data);

					ai_handler.writeAIData(&ping_msg, false);
				}
			}
		}

		if(power_ping_timer > POWER_PING_TIMER) {
			power_ping_timer = 0;

			uint8_t ping_src = ID_CDC;

			if(!cd_handler.getEstablished()) {
				if(tape_handler.getEstablished())
					ping_src = ID_TAPE;
				else if(xm_handler.getEstablished())
					ping_src = ID_XM;
				else if(imid_handler.getEstablished())
					ping_src = ID_IMID_SCR;
				else ping_src = 0;
			}

			if(ping_src > 0) {
				uint8_t power_ping_data[] = {0x4A, 0x1E};
				AIData power_ping_msg(sizeof(power_ping_data), ping_src, ID_CANSLATOR, power_ping_data);
				ai_handler.writeAIData(&power_ping_msg, false);
			}
		}

		if(!parameters.has_dimensions && dimension_request_timer > DIMENSION_REQUEST_TIMER) {
			dimension_request_timer = 0;
			uint8_t ping_src = ID_CDC;

			if(!cd_handler.getEstablished()) {
				if(tape_handler.getEstablished())
					ping_src = ID_TAPE;
				else if(xm_handler.getEstablished())
					ping_src = ID_XM;
				else if(imid_handler.getEstablished())
					ping_src = ID_IMID_SCR;
				else ping_src = 0;
			}

			if(ping_src > 0) {
				uint8_t dim_ping_data[] = {0x2C, 0xF0};
				AIData dim_ping_msg(sizeof(dim_ping_data), ping_src, ID_NAV_COMPUTER, dim_ping_data);
				ai_handler.writeAIData(&dim_ping_msg, parameters.computer_connected);
			}
		}
	}
	
	if(parameters.key_position == 0 && door_timer_enabled && door_timer > DOOR_TIMER) {
		door_timer_enabled = false;
		powerOff();
	}
}

//Read an IEBus message.
void HondaCDXMTrans::interpretIEData(IE_Message ie_msg) {
	if (!parameters.power_on)
		return;

	if (ie_msg.sender == IE_ID_CDC && ie_msg.receiver == IE_ID_RADIO)
		cd_handler.interpretCDMessage(&ie_msg);
	else if (ie_msg.sender == IE_ID_TAPE && ie_msg.receiver == IE_ID_RADIO)
		tape_handler.interpretTapeMessage(&ie_msg);
	else if (ie_msg.sender == IE_ID_IMID && ie_msg.receiver == IE_ID_RADIO) {
		imid_handler.interpretIMIDMessage(&ie_msg);
		parameters.imid_connected = true;
	} else if (ie_msg.sender == IE_ID_SIRIUS && ie_msg.receiver == IE_ID_RADIO)
		xm_handler.interpretSiriusMessage(&ie_msg);

	#ifdef IE_DEBUG
	AIData ie_ai(ie_msg.l, ie_msg.sender, 0x10);
	ie_ai.refreshAIData(ie_msg.data);
	ai_handler.writeAIData(&ie_ai, false);
	#endif
}

//Send a request to the IMID for its full specs.
void HondaCDXMTrans::sendIMIDRequest() {
	if (imid_handler.getEstablished())
		return;

	uint8_t ack_id = ID_CDC;
	if (!cd_handler.getEstablished() && tape_handler.getEstablished())
		ack_id = ID_TAPE;
	else if (!cd_handler.getEstablished() && xm_handler.getEstablished())
		ack_id = ID_XM;

	uint8_t imid_request_data[] = {0x4, 0xE6, 0x3B};
	AIData imid_request_msg(sizeof(imid_request_data), ack_id, ID_IMID_SCR, imid_request_data);

	ai_handler.writeAIData(&imid_request_msg);

	elapsedMillis response_timer;
	while (response_timer < 100) {
		AIData reply;
		if (ai_handler.dataAvailable() > 0) {
			if (ai_handler.readAIData(&reply, false)) {
				if (reply.sender != ID_IMID_SCR || (reply.l >= 1 && reply.data[0] == 0x80))
					continue;

				response_timer = 0;
				if (reply.sender == ID_IMID_SCR && reply.l >= 2 && reply.data[0] == 0x3B) {
					if (reply.data[1] == 0x23 && reply.l >= 4) { // Custom text field.
						parameters.external_imid_char = reply.data[2];
						parameters.external_imid_lines = reply.data[3];
					} else if (reply.data[1] == 0x57) { // OEM text field.
						for (int i = 2; i < reply.l; i += 1) {
							if (reply.data[i] == ID_CDC || reply.data[i] == ID_CD)
								parameters.external_imid_cd = true;
							else if (reply.data[i] == ID_TAPE)
								parameters.external_imid_tape = true;
							else if (reply.data[i] == ID_XM)
								parameters.external_imid_xm = true;
						}
					}

					parameters.imid_connected = true;
				}
			}
		}
	}
}

//Perform power off procedures.
void HondaCDXMTrans::powerOff() {
	parameters.power_on = false;

	uint8_t poweroff_data[] = {0xA0};
	AIData poweroff_msg(sizeof(poweroff_data), ID_IMID_SCR, 0xFF, poweroff_data);

	if(parameters.imid_connected)
		ai_handler.writeAIData(&poweroff_msg, false);
	if(cd_handler.getEstablished()) {
		poweroff_msg.sender = ID_CD;
		ai_handler.writeAIData(&poweroff_msg, false);
		poweroff_msg.sender = ID_CDC;
		ai_handler.writeAIData(&poweroff_msg, false);
	}
	if(tape_handler.getEstablished()) {
		poweroff_msg.sender = ID_TAPE;
		ai_handler.writeAIData(&poweroff_msg, false);
	}
	if(xm_handler.getEstablished()) {
		poweroff_msg.sender = ID_XM;
		ai_handler.writeAIData(&poweroff_msg, false);
	}
	ai_handler.flush();

	digitalWrite(GA_ON, LOW);
	digitalWrite(MAIN_POWER, LOW);
	digitalWrite(AUDIO_ON, LOW);

	cd_handler.clearEstablished();
	tape_handler.clearEstablished();
	xm_handler.clearEstablished();
	imid_handler.clearEstablished();

	parameters.imid_connected = false;
	parameters.external_imid_cd = false;
	parameters.external_imid_tape = false;
	parameters.external_imid_xm = false;
	parameters.external_imid_char = 0;
	parameters.external_imid_lines = 0;

	parameters.first_cd = false;
	parameters.first_tape = false;
	parameters.first_xm = false;
	parameters.first_imid = false;

	parameters.radio_connected = false;
	parameters.computer_connected = false;
	parameters.screen_connected = false;
	parameters.mirror_connected = false;
	
	door_timer_enabled = false;
}

//Get the available memory.
int freeMemory() {
	int size = 8192;
	byte *buf;
	while ((buf = (byte *)malloc(--size)) == NULL);
	free(buf);
	return size;
}
