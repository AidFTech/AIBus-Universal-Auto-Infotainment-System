#include <stdint.h>
#include <mcp2515.h>

#include "AIBus.h"
#include "AIBus_Handler.h"
#include "CAN_Handler.h"
#include "Parameter_List.h"

#ifdef MEGACOREX
#else
#define AI_RX 4

#define BCAN_CS 10
#endif

#if !defined(HAVE_HWSERIAL1)
#define AISerial Serial
#else
#define AISerial Serial1
#endif

AIBusHandler ai_handler(&AISerial, AI_RX);
ParameterList parameters;

BCAN_Handler bcan_handler(&ai_handler, &parameters, BCAN_CS);

void setup() {
	AISerial.begin(AI_BAUD);
	bcan_handler.init();
}

void loop() {
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

			if(ai_msg.sender == ID_CANSLATOR)
				continue;

			if(ai_msg.receiver == ID_CANSLATOR && !(ai_msg.l >= 1 && ai_msg.data[0] == 0x80))
				ai_handler.sendAcknowledgement(ID_CANSLATOR, ai_msg.sender);

			if(ai_msg.receiver == ID_CANSLATOR) {
				if(ai_msg.l >= 2 && ai_msg.data[0] == 0x4A && ai_msg.data[1] == 0x1F) { //Request for common states.
					bcan_handler.sendAllParameters();
				}
			}
		}
	} while (ai_timer < 5);

	bcan_handler.readCANMessage();
}
