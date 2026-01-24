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
	pinMode(PERIPHERAL_CS, OUTPUT);

	pinMode(AUX_LIGHT_CS, OUTPUT);
	pinMode(INT_LIGHT_CS, OUTPUT);
	pinMode(MCP_RESET, OUTPUT);

	pinMode(GPS_1SEC, INPUT);

	pinMode(AMBIENT_ENABLE, OUTPUT);
	pinMode(HFT_CS, OUTPUT);

	pinMode(POWER_ON, OUTPUT);

	digitalWrite(BCAN_CS, HIGH);
	digitalWrite(BCAN_IMID_CS, HIGH);
	digitalWrite(BCAN_RLS_CS, HIGH);
	digitalWrite(FCAN_CS, HIGH);

	digitalWrite(CAN_RESET, HIGH);
	digitalWrite(PERIPHERAL_CS, HIGH);
	digitalWrite(MCP_RESET, HIGH);

	digitalWrite(AMBIENT_ENABLE, LOW);
	digitalWrite(HFT_CS, HIGH);

	digitalWrite(POWER_ON, LOW);

	AISerial.begin(AI_BAUD);
	GPSSerial.swap(1);

	ai_handler.addID(ID_CANSLATOR);
	ai_handler.addID(ID_GPS_ANTENNA);

	bcan_handler.init();
	bcan_handler.setWiperTimer(&wiper_timer);
	bcan_handler.setAuxLightController(&aux_light_controller);
	bcan_handler.setIntLightController(&int_light_controller);

	delay(100);
	aux_light_controller.init();
	int_light_controller.begin();
	int_light_controller.DigitalPotSetWiperPosition(0, 0);
	int_light_controller.DigitalPotSetWiperPosition(1, 0);

	peripheral_mcp.begin();

	peripheral_mcp.pinModeIO(PERIPHERAL_WASHER_IND, OUTPUT);
	peripheral_mcp.pinModeIO(PERIPHERAL_WASHER_SENSOR, INPUT_PULLUP);
	peripheral_mcp.pinModeIO(PERIPHERAL_REAR_FOG_IND, OUTPUT);
	peripheral_mcp.pinModeIO(PERIPHERAL_REAR_FOG_SW, INPUT_PULLUP);
	peripheral_mcp.pinModeIO(PERIPHERAL_INTERIOR_LIGHTS, OUTPUT);
	peripheral_mcp.pinModeIO(PERIPHERAL_VIDEO_SELECT, OUTPUT);
	peripheral_mcp.pinModeIO(PERIPHERAL_VIDEO_POWER, OUTPUT);
	peripheral_mcp.pinModeIO(PERIPHERAL_IGNITION, OUTPUT);

	peripheral_mcp.digitalWriteIO(PERIPHERAL_WASHER_IND, false);
	peripheral_mcp.digitalWriteIO(PERIPHERAL_REAR_FOG_IND, false);
	peripheral_mcp.digitalWriteIO(PERIPHERAL_INTERIOR_LIGHTS, false);
	peripheral_mcp.digitalWriteIO(PERIPHERAL_VIDEO_POWER, false);
	peripheral_mcp.digitalWriteIO(PERIPHERAL_VIDEO_SELECT, false);
	peripheral_mcp.digitalWriteIO(PERIPHERAL_IGNITION, false);

	peripheral_mcp2.begin();

	peripheral_mcp2.pinModeIO(PERIPHERAL_REVERSE_SW, INPUT_PULLUP);
	peripheral_mcp2.pinModeIO(PERIPHERAL_CAMERA_CTL, OUTPUT);
	peripheral_mcp2.pinModeIO(PERIPHERAL_GPS_EN, OUTPUT);
	peripheral_mcp2.pinModeIO(PERIPHERAL_GPS_FIX, INPUT);
	peripheral_mcp2.pinModeIO(PERIPHERAL_LANEWATCH_SW, INPUT_PULLUP);

	peripheral_mcp2.digitalWriteIO(PERIPHERAL_CAMERA_CTL, false);
	peripheral_mcp2.digitalWriteIO(PERIPHERAL_GPS_EN, true);

	getCanslatorSettings(&parameters);
}

//Main object loop.
void CANslator::loop() {
	const bool last_left_drl = parameters.left_drl_on, last_right_drl = parameters.right_drl_on;

	const ambient_state_t last_ambient = parameters.ambient_state;

	elapsedMillis ai_timer;
	AIData ai_msg;

	do {
		bool ai_received = false;
		if(ai_handler.dataAvailable() > 0) {
			if(ai_handler.readAIData(&ai_msg) && ai_handler.getValidMessage(&ai_msg)) {
				ai_received = true;
			}
		}

		if(ai_received) {
			ai_timer = 0;

			if(ai_msg.sender == ID_CANSLATOR)
				continue;

			if(ai_msg.receiver == ID_CANSLATOR && !(ai_msg.l >= 1 && ai_msg[0] == 0x80))
				ai_handler.sendAcknowledgement(ID_CANSLATOR, ai_msg.sender);

			if(ai_msg.sender == ID_NAV_COMPUTER && !parameters.computer_connected && !getInitMessage(&ai_msg) && !getPoweroffMessage(&ai_msg)) {
				parameters.computer_connected = true;
				bcan_handler.sendInfoParameters();
				writeAIBusTimerMessage();
			}

			if(ai_msg.receiver == ID_CANSLATOR) {
				if(ai_msg.l >= 2 && ai_msg[0] == 0x4A && (ai_msg[1]&0x1E) == 0x1E) { //Request for common params.
					param_timer = 0;
					param_timer_enabled = true;
				}

				if(!bcan_handler.handleAIBus(&ai_msg))
					this->handleAIBus(&ai_msg);
			}

			if(getInitMessage(&ai_msg) || getPoweroffMessage(&ai_msg)) {
				if(ai_msg.sender == ID_NAV_COMPUTER)
					parameters.computer_connected = false;
			}
		}
	} while (ai_timer < 5);

	if(param_timer_enabled && param_timer > PARAM_TIMER) {
		param_timer_enabled = false;
		bcan_handler.sendCommonParameters();
	}

	const uint8_t last_door = parameters.doors_open;

	const bool last_power = parameters.power_on,
				last_left_signal_on = bcan_handler.getLeftSignalOn(),
				last_right_signal_on = bcan_handler.getRightSignalOn(),
				last_interior_lights = bcan_handler.getInteriorLightsOn();

	bcan_handler.readCANMessage();

	if(parameters.power_on != last_power) {
		peripheral_mcp.digitalWriteIO(PERIPHERAL_IGNITION, parameters.power_on);
		digitalWrite(POWER_ON, parameters.power_on ? HIGH : LOW);
	}

	if(parameters.doors_open != last_door) {
		if((parameters.doors_open&0xF) != 0 && parameters.ambient_light_enable_door)
			parameters.ambient_state = AMBIENT_LIGHTS_FULL;
		else if((parameters.doors_open&0xF) == 0) {
			if(bcan_handler.getInteriorLightsOn() && parameters.ambient_light_enable_int)
				parameters.ambient_state = AMBIENT_LIGHTS_DIMMED;
			else
				parameters.ambient_state = AMBIENT_LIGHTS_OFF;
		}
	}

	if(minute_timer/1000 >= 60) {
		minute_timer = 0;
		minute_count += 1;
		this->writeAIBusTimerMessage();
	}

	//DRLs.
	if(last_left_drl != parameters.left_drl_on)
		aux_light_controller.setLeftDRL(parameters.left_drl_on);

	if(last_right_drl != parameters.right_drl_on)
		aux_light_controller.setRightDRL(parameters.right_drl_on);

	//Interior lighting.
	if(last_interior_lights != bcan_handler.getInteriorLightsOn()) {
		peripheral_mcp.digitalWriteIO(PERIPHERAL_INTERIOR_LIGHTS, bcan_handler.getInteriorLightsOn());
	}

	if(last_ambient != parameters.ambient_state) {
		if(parameters.ambient_state == AMBIENT_LIGHTS_OFF)
			digitalWrite(AMBIENT_ENABLE, LOW);
		else if(parameters.ambient_state == AMBIENT_LIGHTS_DIMMED) {
			digitalWrite(AMBIENT_ENABLE, HIGH);
			int_light_controller.DigitalPotSetWiperPosition(1, bcan_handler.getBrightness()*parameters.ambient_light_brightness/255);
		} else if(parameters.ambient_state == AMBIENT_LIGHTS_FULL) {
			digitalWrite(AMBIENT_ENABLE, HIGH);
			int_light_controller.DigitalPotSetWiperPosition(1, bcan_handler.getBrightness());
		}
	}

	//LaneWatch. @TODO: Only do this if the car has the LaneWatch option and the backup camera is not in use.
	if(last_right_signal_on != bcan_handler.getRightSignalOn()) {
		if(bcan_handler.getRightSignalOn()) {
			peripheral_mcp.digitalWriteIO(PERIPHERAL_VIDEO_SELECT, true);
			peripheral_mcp.digitalWriteIO(PERIPHERAL_VIDEO_POWER, true);
		} else {
			peripheral_mcp.digitalWriteIO(PERIPHERAL_VIDEO_POWER, false);
			peripheral_mcp.digitalWriteIO(PERIPHERAL_VIDEO_SELECT, false);
		}
	}

	//Check battery voltage.
	if(battery_timer > BATTERY_TIMER) {
		battery_timer = 0;

		const uint8_t last_bv = battery_voltage;
		const float measured_voltage = bat_adc.readVin(15);

		battery_voltage = uint16_t(measured_voltage*100);
		
		if(battery_voltage/10 != last_bv/10)
			bcan_handler.sendBatteryVoltage(battery_voltage);
	}

	if(washer_fluid_timer > WASHER_FLUID_TIMER) {
		const bool last_washer_fluid = parameters.washer_fluid_low;
		parameters.washer_fluid_low = !peripheral_mcp.digitalReadIO(PERIPHERAL_WASHER_SENSOR);

		if(parameters.washer_fluid_low != last_washer_fluid) {
			washer_fluid_timer = 0;
			peripheral_mcp.digitalWriteIO(PERIPHERAL_WASHER_IND, parameters.washer_fluid_low);
		}
	}

	if(bcan_handler.getWiperIntActive() && wiper_timer > parameters.wiper_time_limit) {
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

//Write the trip timer message.
void CANslator::writeAIBusTimerMessage() {
	uint8_t timer_bytes[sizeof(uint32_t)];
	Vector<uint8_t> timer_vec;
	timer_vec.setStorage(timer_bytes);

	unsigned long trip_timer = minute_count;
	do {
		timer_vec.push_back(trip_timer%0x100);
		trip_timer >>= 8;
	} while(trip_timer > 0 && timer_vec.size() < timer_vec.max_size());

	const uint8_t byte_count = timer_vec.size();
	uint8_t timer_data[byte_count+4];
	timer_data[0] = 0xA1;
	timer_data[1] = 0x1F;
	timer_data[2] = 0xA;
	timer_data[3] = 0x80 | (byte_count&0x7F);
	for(int i=0;i<byte_count;i+=1)
		timer_data[i+4] = timer_vec[timer_vec.size()-1-i];

	AIData timer_msg(sizeof(timer_data), ID_CANSLATOR, 0xFF, timer_data);
	ai_handler.writeAIData(&timer_msg, false);
}

//Write the trip distance message.
void CANslator::writeAIBusTripDistanceMessage() {
	uint8_t distance_bytes[sizeof(uint32_t)];
	Vector<uint8_t> distance_vec;
	distance_vec.setStorage(distance_bytes);

	unsigned long trip_distance = this->trip_distance;
	if(parameters.display_miles)
		trip_distance *= 10/16;

	do {
		distance_vec.push_back(trip_distance%0x100);
		trip_distance >>= 8;
	} while(trip_distance > 0 && distance_vec.size() < distance_vec.max_size());

	const uint8_t byte_count = distance_vec.size();
	uint8_t dist_data[byte_count+4];
	dist_data[0] = 0xA1;
	dist_data[1] = 0x1F;
	dist_data[2] = 0xA;
	dist_data[3] = 0x10 | (byte_count&0xF);

	if(parameters.display_miles)
		dist_data[3] |= 0x80;

	for(int i=0;i<byte_count;i+=1)
		dist_data[i+4] = distance_vec[distance_vec.size()-1-i];

	AIData distance_msg(sizeof(dist_data), ID_CANSLATOR, 0xFF, dist_data);
	ai_handler.writeAIData(&distance_msg, false);
}
