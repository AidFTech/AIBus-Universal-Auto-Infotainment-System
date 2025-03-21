#include <stdint.h>
#include <elapsedMillis.h>
#include <MCP4251.h>

#include "AIBus.h"
#include "AIBus_Handler.h"
#include "BM83_AidF.h"
#include "Phone_Text_Handler.h"

#include "Parameter_List.h"

#ifdef MEGACOREX
#define AI_RX PIN_PA7
#define POWER_ON PIN_PC2
#define DIGITAL_OUT PIN_PC3

#define BM83_MFB PIN_PD0
#define BM83_MODE PIN_PD1
#define BM83_RESET PIN_PD2

#define SPDIF_RESET PIN_PD3
#define DAC_MUTE PIN_PD4
#define DAC_FILTER PIN_PD5
#define BIAS_CS PIN_PD6
#define EEPROM_CS PIN_PD7

#else
#define AI_RX 4
#define POWER_ON 5
#define DIGITAL_OUT 6

#define BM83_MFB 7
#define BM83_MODE 8
#define BM83_RESET 9

#define SPDIF_RESET 14
#define DAC_MUTE 15
#define DAC_FILTER 16
#define BIAS_CS 17
#define EEPROM_CS 18
#endif

#define PING_TIMER 5000

#define AISerial Serial

#if !defined(HAVE_HWSERIAL1)
#include <SoftwareSerial.h>
SoftwareSerial BM83Serial(2, 3);
#else
#define BM83Serial Serial1
#endif

ParameterList parameters;

AIBusHandler ai_handler(&AISerial, AI_RX);

PhoneTextHandler text_handler(&ai_handler, &parameters);
BM83 bm83_handler(&BM83Serial, &ai_handler, &text_handler, &parameters);

elapsedMillis ping_timer;

//Arduino setup.
void setup() {
	pinMode(AI_RX, INPUT);

	randomSeed(analogRead(0));

	AISerial.begin(AI_BAUD);
	BM83Serial.begin(115200);

	pinMode(POWER_ON, OUTPUT);
	pinMode(DIGITAL_OUT, OUTPUT);
	pinMode(SPDIF_RESET, OUTPUT);
	pinMode(DAC_MUTE, OUTPUT);
	pinMode(DAC_FILTER, OUTPUT);
	pinMode(BIAS_CS, OUTPUT);
	pinMode(EEPROM_CS, OUTPUT);

	pinMode(BM83_MFB, OUTPUT);
	pinMode(BM83_MODE, INPUT_PULLUP);
	pinMode(BM83_RESET, OUTPUT);

	digitalWrite(POWER_ON, HIGH);
	digitalWrite(DIGITAL_OUT, LOW);
	digitalWrite(SPDIF_RESET, LOW);
	digitalWrite(DAC_MUTE, HIGH);
	digitalWrite(DAC_FILTER, LOW);
	digitalWrite(BIAS_CS, HIGH);
	digitalWrite(EEPROM_CS, HIGH);

	digitalWrite(BM83_MFB, LOW);
	digitalWrite(BM83_RESET, LOW);
	//digitalWrite(BM83_MODE, HIGH);

	delay(200);
	digitalWrite(BM83_MFB, HIGH);
	delay(20);
	digitalWrite(BM83_RESET, HIGH);
	delay(500);
	digitalWrite(BM83_MFB, LOW);

	bm83_handler.init();
}

//Arduino loop.
void loop() {
	bm83_handler.loop();

	elapsedMillis ai_timer;
	AIData ai_msg;

	do {
		bool ai_received = false;
		if(ai_handler.dataAvailable() > 0) {
			if(ai_handler.readAIData(&ai_msg)) {
				ai_received = true;
			}
		}

		if(ai_received) {
			ai_timer = 0;

			if(ai_msg.sender == ID_PHONE)
				continue;

			if(!parameters.navi_connected && ai_msg.sender == ID_NAV_COMPUTER) {
				parameters.navi_connected = true;
				text_handler.writeNaviText("Phone", 0, 0);
				text_handler.createDisconnectedButtons(false);
			}

			if(!parameters.radio_connected && ai_msg.sender == ID_RADIO) {
				parameters.radio_connected = true;

				//TODO: Send source handshake.
			}

			if(!parameters.imid_connected && ai_msg.sender == ID_IMID_SCR) {
				parameters.imid_connected = true;

				uint8_t req_data[] = {0x4, 0xE6, 0x3B};
				AIData req_msg(sizeof(req_data), ID_PHONE, ID_IMID_SCR);

				req_msg.refreshAIData(req_data);
				ai_handler.writeAIData(&req_msg, false);
			}

			if(ai_msg.receiver == ID_PHONE) {
				bool answered = false;
				answered = bm83_handler.handleAIBus(&ai_msg);
				if(!answered) {
					if(ai_msg.l >= 1 && ai_msg.data[0] != 0x80)
						ai_handler.sendAcknowledgement(ID_PHONE, ai_msg.sender);

					if(ai_msg.sender == ID_IMID_SCR) {
						if(ai_msg.l >= 4 && ai_msg.data[0] == 0x3B && ai_msg.data[1] == 0x23) {
							parameters.imid_char = ai_msg.data[2];
							parameters.imid_lines = ai_msg.data[3];
						} else if(ai_msg.l >= 2 && ai_msg.data[0] == 0x3B && ai_msg.data[1] == 0x57) {
							parameters.imid_bta = false;
							for(int i=2;i<ai_msg.l;i+=1) {
								if(ai_msg.data[i] == ID_PHONE)
									parameters.imid_bta = true;
							}
						}
					}
				}
			} else if(ai_msg.receiver == 0xFF && ai_msg.l >= 2 && ai_msg.data[0] == 0xA1) { //Broadcast message.
				if(ai_msg.sender == ID_CANSLATOR && ai_msg.data[1] == 0x2 && ai_msg.l >= 3) { //Key position.
					const uint8_t last_key = parameters.key_position, key = ai_msg.data[2]&0xF;
					parameters.key_position = key;

					if(key != last_key) {
						if(key != 0) {
							parameters.power_on = true;
							digitalWrite(POWER_ON, HIGH);
						} else {
							parameters.power_on = false;
							if((parameters.door_position&0xF) != 0)
								digitalWrite(POWER_ON, LOW);
						}
					}
					//TODO: Anything special to do with the BM83 on poweroff?
				} else if(ai_msg.sender == ID_CANSLATOR && ai_msg.data[1] == 0x43 && ai_msg.l >= 3) { //Door position.
					const uint8_t last_door = parameters.door_position;
					parameters.door_position = ai_msg.data[2]&0xF;

					if(last_door == 0x0 && parameters.door_position != 0x0 && !parameters.power_on)
						digitalWrite(POWER_ON, LOW);
				}
			}
		}
	} while(ai_timer < 50);

	if(ping_timer > PING_TIMER) {
		ping_timer = 0;

		uint8_t ping_data[] = {0x1};
		AIData ping_msg(sizeof(ping_data), ID_PHONE, 0xFF);
		ping_msg.refreshAIData(ping_data);

		if(!parameters.radio_connected) {
			ping_msg.receiver = ID_RADIO;
			ai_handler.writeAIData(&ping_msg, false);
		}
		if(!parameters.navi_connected) {
			ping_msg.receiver = ID_NAV_COMPUTER;
			ai_handler.writeAIData(&ping_msg, false);
		}

		if(!parameters.imid_connected) {
			ping_msg.receiver = ID_IMID_SCR;
			ai_handler.writeAIData(&ping_msg, false);
		}
	}
}

