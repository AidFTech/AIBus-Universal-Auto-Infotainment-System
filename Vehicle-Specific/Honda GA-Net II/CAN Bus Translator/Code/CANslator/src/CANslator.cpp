#include "CANslator.h"

//Main object setup.
void CANslator::setup() {
	AISerial.begin(AI_BAUD);
	bcan_handler.init();
}

//Main object loop.
void CANslator::loop() {
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
