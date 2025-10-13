#include "CANslator.h"

CANslator canslator;

//Arduino setup.
void setup() {
	canslator.setup();
}

//Arduino loop.
void loop() {
	canslator.loop();
}

//Main object setup.
void CANslator::setup() {
	pinMode(BCAN_CS, OUTPUT);
	pinMode(BCAN_IMID_CS, OUTPUT);
	pinMode(BCAN_RLS_CS, OUTPUT);
	pinMode(FCAN_CS, OUTPUT);

	pinMode(CAN_RESET, OUTPUT);
	pinMode(WASHER_SENSOR, INPUT_PULLUP);
	pinMode(WASHER_IND, OUTPUT);

	pinMode(POWER_ON, OUTPUT);

	digitalWrite(BCAN_CS, HIGH);
	digitalWrite(BCAN_IMID_CS, HIGH);
	digitalWrite(BCAN_RLS_CS, HIGH);
	digitalWrite(FCAN_CS, HIGH);

	digitalWrite(CAN_RESET, HIGH);
	digitalWrite(WASHER_IND, LOW);

	digitalWrite(POWER_ON, LOW);

	AISerial.begin(AI_BAUD);
	bcan_handler.init();
	bcan_handler.setWiperTimer(&wiper_timer);
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
					param_timer = 0;
					param_timer_enabled = true;
				}

				if(!bcan_handler.handleAIBus(&ai_msg))
					this->handleAIBus(&ai_msg);
			}
		}
	} while (ai_timer < 5);

	if(param_timer_enabled && param_timer > PARAM_TIMER) {
		param_timer_enabled = false;
		bcan_handler.sendAllParameters();
	}

	const bool last_power = parameters.power_on;
	bcan_handler.readCANMessage();

	if(parameters.power_on != last_power)
		digitalWrite(POWER_ON, parameters.power_on ? HIGH : LOW);

	//TODO: Buffer this.
	const bool last_washer_fluid = parameters.washer_fluid_low;
	parameters.washer_fluid_low = digitalRead(WASHER_SENSOR) != LOW;

	if(parameters.washer_fluid_low != last_washer_fluid)
		digitalWrite(WASHER_IND, parameters.washer_fluid_low ? HIGH : LOW);

	if(bcan_handler.getWiperIntActive() && wiper_timer > wiper_time_limit) {
		if(bcan_handler.runWiper())
			wiper_timer = 0;
	}
}

//Handle an AIBus message.
void CANslator::handleAIBus(AIData* ai_msg) {
	if(ai_msg->l >= 2 && ai_msg->data[0] == 0x49 && ai_msg->data[0] == 0x1F) { //Supported parameter request.
		uint8_t supported_data[] = {0x49,
									0x2 | 0x1,
									0x80 | 0x20 | 0x10 | 0x8 | 0x4 | 0x2 | 0x1};

		AIData supported_msg(sizeof(supported_data), ID_CANSLATOR, ai_msg->sender, supported_data);
		ai_handler.writeAIData(&supported_msg);
	} else if(ai_msg->l >= 2 && ai_msg->data[0] == 0x45) { //Consumption info request.

	} else if((ai_msg->sender == ID_NAV_COMPUTER || ai_msg->sender == ID_ANDROID_AUTO) && (ai_msg->l >= 2 && ai_msg->data[0] == 0x7F)) { //Navigation message.
		if(ai_msg->l >= 8 && ai_msg->data[1] == 0x4) { //Next turn.
			String street_name = "";
			for(int i=8;i<ai_msg->l;i+=1)
				street_name += char((*ai_msg)[i]);

			bcan_handler.setNavNextTurn((*ai_msg)[2], (*ai_msg)[3], ((*ai_msg)[6] << 8) | (*ai_msg)[7], (*ai_msg)[4], (*ai_msg)[5], street_name);
		}
	}
}