#include <elapsedMillis.h>

#include "AIBus.h"
#include "En_IEBus_Handler.h"

#include "Handshakes.h"
#include "Honda_CD_Handler.h"
#include "Honda_Tape_Handler.h"
#include "Honda_XM_Handler.h"
#include "Honda_IMID_Handler.h"

#include "Parameter_List.h"

//#define AI_DEBUG

#ifndef AI_RX
#define AI_RX 2
#endif

#if defined(HAVE_HWSERIAL1) && !defined(AI_DEBUG)
#define AISerial Serial1
#else
#define AISerial Serial
#endif

#ifndef AI_DEBUG
//#define AI_DEBUG
#endif

#define GA_ON 4
#define ILL_ANODE 5

#define IEBUS_TX 6
#define IEBUS_RX 7

#define AIBUS_BLOCK 8

#define ILL_CATHODE 9
//#define MEMORY_CHECK

#define FUNCTION_DELAY 5000
#define SOURCE_DELAY 5000

#define CACHE_SIZE 32

//#define IE_DEBUG

ParameterList parameters;

EnAIBusHandler ai_handler(&AISerial, AI_RX, 8);
EnIEBusHandler ie_handler(IEBUS_RX, IEBUS_TX, &ai_handler, AIBUS_BLOCK);

HondaIMIDHandler imid_handler(&ie_handler, &ai_handler, &parameters);
HondaTapeHandler tape_handler(&ie_handler, &ai_handler, &parameters, &imid_handler);
HondaXMHandler xm_handler(&ie_handler, &ai_handler, &parameters, &imid_handler);
HondaCDHandler cd_handler(&ie_handler, &ai_handler, &parameters, &imid_handler);

elapsedMillis function_timer, ai_timer, screen_request_timer;

uint8_t door_state = 0;

void setup() {
	AISerial.begin(AI_BAUD);
	pinMode(GA_ON, OUTPUT);
	digitalWrite(GA_ON, LOW);

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);

	sendWideHandshake(&ie_handler);
	sendPingHandshake(&ie_handler, IE_ID_IMID);
	sendPingHandshake(&ie_handler, IE_ID_CDC);
	sendPingHandshake(&ie_handler, IE_ID_TAPE);
	sendPingHandshake(&ie_handler, IE_ID_SIRIUS);
	parameters.screen_request_timer = &screen_request_timer;

	#ifdef MEMORY_CHECK
	Serial.begin(115200);
	Serial.println("Ready!");
	#endif
}

void loop() {
	#ifdef MEMORY_CHECK
	Serial.println(freeMemory());
	Serial.flush();
	#endif

	IE_Message ie_msg;
	AIData ai_msg;

	const uint8_t last_source = parameters.active_source, last_subsource = parameters.active_subsource;

	if(ie_handler.getInputOn()) {
		if(ie_handler.readMessage(&ie_msg, true, IE_ID_RADIO) == 0) {
			IE_Message check_msg(ie_msg.l-1, ie_msg.sender, ie_msg.receiver, ie_msg.control, ie_msg.direct);
			for(int i=0;i<ie_msg.l-1;i+=1)
				check_msg.data[i] = ie_msg.data[i];

			if(ie_msg.checkVaildity()) {
				if(ie_msg.receiver == IE_ID_RADIO && ie_msg.l >= 1 && ie_msg.data[0] != 0x80) {
					ie_handler.sendAcknowledgement(ie_msg.receiver, ie_msg.sender);
					interpretIEData(ie_msg);
				}
			}
		}
	}

	do {
		bool ai_received = false;
		if(ai_handler.dataAvailable() > 0) {
			if(ai_handler.readAIData(&ai_msg)) {
				ai_received = true;
			}
		}

		if(ai_received) {
			ai_timer = 0;

			if((ai_msg.sender == ID_IMID_SCR && imid_handler.getEstablished()) ||
				(ai_msg.sender == ID_TAPE && tape_handler.getEstablished()) ||
				(ai_msg.sender == ID_CDC && cd_handler.getEstablished()) ||
				(ai_msg.sender == ID_XM && xm_handler.getEstablished()))
					continue;
			
			if(!parameters.computer_connected && ai_msg.sender == ID_NAV_COMPUTER)
				parameters.computer_connected = true;

			if(!parameters.screen_connected && ai_msg.sender == ID_NAV_SCREEN)
				parameters.screen_connected = true;
				
			#ifndef AI_DEBUG
			if(!parameters.radio_connected && ai_msg.sender == ID_RADIO) {
				parameters.radio_connected = true;
				
				if(cd_handler.getEstablished())
					cd_handler.sendSourceNameMessage();
				if(tape_handler.getEstablished())
					tape_handler.sendSourceNameMessage();
				if(xm_handler.getEstablished())
					xm_handler.sendSourceNameMessage();
			}
			#endif

			if(ai_msg.sender == ID_IMID_SCR && !imid_handler.getEstablished()) { //An external IMID is available.
				if(ai_msg.data[0] == 0x3B) { //External IMID is announcing itself.
					if(ai_msg.data[1] == 0x23 && ai_msg.l >= 4) { //Custom text field.
						parameters.external_imid_char = ai_msg.data[2];
						parameters.external_imid_lines = ai_msg.data[3];
					} else if(ai_msg.data[1] == 0x57) { //OEM text field.
						for(int i=2;i<ai_msg.l;i+=1) {
							if(ai_msg.data[i] == ID_CDC || ai_msg.data[i] == ID_CD)
								parameters.external_imid_cd = true;
							else if(ai_msg.data[i] == ID_TAPE)
								parameters.external_imid_tape = true;
							else if(ai_msg.data[i] == ID_XM)
								parameters.external_imid_xm = true;
						}
					}
				}
			}
			
			if(ai_msg.receiver == ID_CDC && cd_handler.getEstablished())
				cd_handler.readAIBusMessage(&ai_msg);
			else if(ai_msg.receiver == ID_TAPE && tape_handler.getEstablished())
				tape_handler.readAIBusMessage(&ai_msg);
			else if(ai_msg.receiver == ID_IMID_SCR && imid_handler.getEstablished())
				imid_handler.readAIBusMessage(&ai_msg);
			else if(ai_msg.receiver == ID_XM && xm_handler.getEstablished())
				xm_handler.readAIBusMessage(&ai_msg);
			else if(ai_msg.receiver == 0xFF && ai_msg.l >= 2 && ai_msg.data[0] == 0xA1) { //Broadcast message.
				if(ai_msg.data[1] == 0x1F) {
					if(ai_msg.data[2] == 0x1 && ai_msg.l >= 5) { //Time.
						parameters.hour = ai_msg.data[3];
						parameters.minute = ai_msg.data[4];
						imid_handler.writeTimeAndDayMessage(parameters.hour, parameters.minute, parameters.month, parameters.date, parameters.year);
					} else if(ai_msg.data[2] == 0x2 && ai_msg.l >= 7) { //Date.
						parameters.year = (ai_msg.data[3] << 8) | ai_msg.data[4];
						parameters.month = ai_msg.data[5];
						parameters.date = ai_msg.data[6];
						imid_handler.writeTimeAndDayMessage(parameters.hour, parameters.minute, parameters.month, parameters.date, parameters.year);
					}
				} else if(ai_msg.sender == ID_CANSLATOR && ai_msg.data[1] == 0x2) { //Key position.
					const uint8_t key = ai_msg.data[2]&0xF;
					if(key != 0) {
						digitalWrite(GA_ON, HIGH);
						parameters.power_on = true;

						sendWideHandshake(&ie_handler);
						sendPingHandshake(&ie_handler, IE_ID_IMID);
						sendPingHandshake(&ie_handler, IE_ID_CDC);
						sendPingHandshake(&ie_handler, IE_ID_TAPE);
						sendPingHandshake(&ie_handler, IE_ID_SIRIUS);
					} else {
						parameters.power_on = false;

						if((door_state&0xF) != 0)
							digitalWrite(GA_ON, LOW);
					}
				} else if(ai_msg.sender == ID_CANSLATOR && ai_msg.data[1] == 0x43 && ai_msg.l >= 3) { //Doors.
					const uint8_t last_door_state = door_state;
					door_state = ai_msg.data[2];

					if((last_door_state&0xF) == 0x0 && (door_state&0xF) != 0x0 && !parameters.power_on) {
						digitalWrite(GA_ON, LOW);
					}
				} else if(ai_msg.sender == ID_CANSLATOR && ai_msg.data[1] == 0x10) { //Lights.
				} 
			} else if(ai_msg.receiver == 0xFF && ai_msg.sender == ID_RADIO) {
				if(ai_msg.l >= 3 && ai_msg.data[0] == 0x4 && ai_msg.data[1] == 0xE6 && ai_msg.data[2] == 0x10) {
					elapsedMillis wait_timer;
					while(wait_timer < 20) {
						ie_handler.cacheAIBus();
					}

					if(cd_handler.getEstablished())
						cd_handler.sendSourceNameMessage();
					if(tape_handler.getEstablished())
						tape_handler.sendSourceNameMessage();
					if(xm_handler.getEstablished())
						xm_handler.sendSourceNameMessage();
					if(imid_handler.getEstablished())
						imid_handler.writeScreenLayoutMessage();
				}
			}
		}
	} while(ai_timer < 50);

	if(function_timer > FUNCTION_DELAY) {
		function_timer = 0;
		
		if(imid_handler.getEstablished()) {
			const uint16_t source = imid_handler.getMode();
			imid_handler.setIMIDSource(source&0xFF, (source&0xFF00)>>8);
		}
		
		if(cd_handler.getSelected()) {
			uint8_t cdc_function[] = {0x6, 0x0, 0x1};
			sendFunctionMessage(&ie_handler, false, IE_ID_CDC, cdc_function, sizeof(cdc_function));
		} else if(tape_handler.getSelected()) {
			uint8_t tape_function[] = {0x13, 0x0, 0x1};
			sendFunctionMessage(&ie_handler, false, IE_ID_TAPE, tape_function, sizeof(tape_function));
		} else if(xm_handler.getSelected()) {
			uint8_t xm_function[] = {0x19, 0x0, 0x1};
			if(xm_handler.getXM2())
				xm_function[2] = 0x2;
			sendFunctionMessage(&ie_handler, false, IE_ID_SIRIUS, xm_function, sizeof(xm_function));
		}
	}

	imid_handler.loop();
	cd_handler.loop();
	xm_handler.loop();

	if(screen_request_timer > SOURCE_DELAY) {
		screen_request_timer = 0;
		if(cd_handler.getSelected())
			cd_handler.requestControl();
		else if(tape_handler.getSelected())
			tape_handler.requestControl();
		else if(xm_handler.getSelected())
			xm_handler.requestControl();
	}
}

void interpretIEData(IE_Message ie_msg) {
	if(ie_msg.sender == IE_ID_CDC && ie_msg.receiver == IE_ID_RADIO) 
		cd_handler.interpretCDMessage(&ie_msg);
	else if(ie_msg.sender == IE_ID_TAPE && ie_msg.receiver == IE_ID_RADIO)
		tape_handler.interpretTapeMessage(&ie_msg);
	else if(ie_msg.sender == IE_ID_IMID && ie_msg.receiver == IE_ID_RADIO) {
		imid_handler.interpretIMIDMessage(&ie_msg);
		parameters.imid_connected = true;
	} else if(ie_msg.sender == IE_ID_SIRIUS && ie_msg.receiver == IE_ID_RADIO)
		xm_handler.interpretSiriusMessage(&ie_msg);
	
	#ifdef IE_DEBUG
	AIData ie_ai(ie_msg.l, ie_msg.sender, 0x10);
	ie_ai.refreshAIData(ie_msg.data);
	ai_handler.writeAIData(&ie_ai, false);
	#endif
}

//Send a request to the IMID for its full specs.
void sendIMIDRequest() {
	if(imid_handler.getEstablished())
		return;
		
	uint8_t ack_id =  ID_CDC;
	if(!cd_handler.getEstablished() && tape_handler.getEstablished())
		ack_id = ID_TAPE;
	else if(!cd_handler.getEstablished() && xm_handler.getEstablished())
		ack_id = ID_XM;
	
	uint8_t imid_request_data[] = {0x4, 0xE6, 0x3B};
	AIData imid_request_msg(sizeof(imid_request_data), ack_id, ID_IMID_SCR);
	imid_request_msg.refreshAIData(imid_request_data);

	ai_handler.writeAIData(&imid_request_msg);
	

	elapsedMillis response_timer;
	while(response_timer < 100) {
		AIData reply;
		if(ai_handler.dataAvailable() > 0) {
			if(ai_handler.readAIData(&reply)) {
				if(reply.sender != ID_IMID_SCR || (reply.l >=1 && reply.data[0] == 0x80))
					continue;

				response_timer = 0;
				if(reply.sender == ID_IMID_SCR && reply.l >= 2 && reply.data[0] == 0x3B) {
					ai_handler.sendAcknowledgement(ack_id, reply.sender);
					if(reply.data[1] == 0x23 && reply.l >= 4) { //Custom text field.
						parameters.external_imid_char = reply.data[2];
						parameters.external_imid_lines = reply.data[3];
					} else if(reply.data[1] == 0x57) { //OEM text field.
						for(int i=2;i<reply.l;i+=1) {
							if(reply.data[i] == ID_CDC || reply.data[i] == ID_CD)
								parameters.external_imid_cd = true;
							else if(reply.data[i] == ID_TAPE)
								parameters.external_imid_tape = true;
							else if(reply.data[i] == ID_XM)
								parameters.external_imid_xm = true;
						}
					}

					parameters.imid_connected = true;
				}
			}
		}
	}
}

#ifdef MEMORY_CHECK
int freeMemory() {
	int size = 8192;
	byte *buf;
	while ((buf = (byte *) malloc(--size)) == NULL);
		free(buf);
	return size;
}
#endif
