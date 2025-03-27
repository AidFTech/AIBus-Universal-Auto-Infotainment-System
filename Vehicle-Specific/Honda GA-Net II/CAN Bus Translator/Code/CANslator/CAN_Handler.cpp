#include "CAN_Handler.h"

BCAN_Handler::BCAN_Handler(AIBusHandler* ai_handler, ParameterList* parameter_list, uint8_t cs_pin) {
	this->ai_handler = ai_handler;
	this->b_cs_pin = cs_pin;
	this->parameter_list = parameter_list;
	
	this->bcan_2515 = new MCP2515(b_cs_pin);
	
	auto_stop = false;
	econ_mode = false;
	e_brake = false;
	brightness_bar = false;
	lights_on = false;
	
	hybrid_battery_level = 6;
	coolant_temp = 0x35;
	eco_bar = 0x1C;
	doors_open = 0x00;
	brightness = 0x16;
	vehicle_speed = 0;
	
	outside_temp = 50;
	electric_ac_power = 0;
	eco_leaf_meter = 0;
	gear = 0x4002;
	
	odo_km = 0;

	key_pos = 0;
}

//Initialize the MCP2515.
void BCAN_Handler::init() {
	bcan_2515->reset();
	bcan_2515->setBitrate(CAN_125KBPS);
	bcan_2515->setNormalMode();
}

//Read a CAN frame.
void BCAN_Handler::readCANMessage() {
	struct can_frame can_msg;
	elapsedMillis can_timer = 300;
	bool first_can = false;

	do {
		if(can_timer < 300)
			ai_handler->cachePending(ID_CANSLATOR);

		if(bcan_2515->readMessage(&can_msg) == MCP2515::ERROR_OK) {
			if(!first_can) {
				first_can = true;
				can_timer = 0;
			}

			if(can_msg.can_id == 0x92F85150 && can_msg.can_dlc == 5) { //Key position, e-brake and gear.
				const uint16_t last_gear = gear;
				const uint8_t last_key = key_pos;
				const bool last_ebrake = e_brake;

				gear = ((can_msg.data[0]<<8) | can_msg.data[1])&0x5005;
				e_brake = (can_msg.data[1]&0x2) != 0;

				switch(can_msg.data[2]) {
				case 0x2: //Key off or ACC 1.
					key_pos = 1; //TODO: Distinguish this.
					break;
				case 0x60: //ACC 2.
				case 0x40:
					key_pos = 2;
					break;
				//case 0x40: //Cranking. //TODO: Figure out the actual cranking byte.
				//	key_pos = 3;
				//	break;
				case 0x0: //Engine on.
					key_pos = 4;
					break;
				}

				if(key_pos != last_key)
					writeAIBusKeyMessage(0xFF, (last_key&0xF) == 0);
			} else if(can_msg.can_id == 0x92F83010 && can_msg.can_dlc == 1) { //Left doors.
				const uint8_t last_door = doors_open;

				doors_open &= 0xF5;
				doors_open |= (can_msg.data[0]&0xA0)>>4;

				if(doors_open != last_door)
					writeAIBusDoorMessage(0xFF, (key_pos&0xF) == 0);
			} else if(can_msg.can_id == 0x92F84010 && can_msg.can_dlc == 1) { //Right doors.
				const uint8_t last_door = doors_open;

				doors_open &= 0xFA;
				doors_open |= (can_msg.data[0]&0x50)>>4;

				if(doors_open != last_door)
					writeAIBusDoorMessage(0xFF, (key_pos&0xF) == 0);
			} else if(can_msg.can_id == 0x92F84310 && can_msg.can_dlc == 1) { //Trunk.
				const uint8_t last_door = doors_open;

				doors_open &= 0xEF;
				doors_open |= (can_msg.data[0]&0x80)>>3;

				if(doors_open != last_door)
					writeAIBusDoorMessage(0xFF, (key_pos&0xF) == 0);
			} else if(can_msg.can_id == 0x92F85450 && can_msg.can_dlc == 8) { //Brightness.
				const uint8_t last_brightness = brightness;
				const bool last_night = night_mode, last_light = lights_on;

				brightness = can_msg.data[0]&0x1F;
				lights_on = (can_msg.data[0]&0x40) != 0;
				night_mode = (can_msg.data[5]&0x40)	!= 0;

				if(last_brightness != brightness || lights_on != last_light || night_mode != last_night)
					writeAIBusBrightnessMessage(0xFF, (key_pos&0xF) == 0);
			} else if(can_msg.can_id == 0x8AF81110 && can_msg.can_dlc == 2) { //Light state and hood.
				const uint8_t last_state_a = light_state_a, last_state_b = light_state_b;
				const bool last_hood = (this->doors_open&0x20) != 0;

				const bool drl = (can_msg.data[0]&0x8) != 0;
				const bool side = (can_msg.data[0]&0x40) != 0;
				const bool low_beam = (can_msg.data[0]&0x2) != 0;
				const bool front_fog = (can_msg.data[1]&0x80) != 0;
				const bool high_beam = (can_msg.data[0]&0x1) != 0;

				const bool tail = (can_msg.data[1]&0x40) != 0;
				const bool license = (can_msg.data[1]&0x8) != 0; //TODO: Check this?

				const bool hood = (can_msg.data[0]&0x80) != 0;

				light_state_a = (drl ? 0x1 : 0x0) |
								(side ? 0x8 : 0x0) |
								(low_beam ? 0x2 : 0x0) |
								(front_fog ? 0x10 : 0x0) |
								(high_beam ? 0x4 : 0x0);

				light_state_b = (tail ? 0x20 : 0x0) |
								(license ? 0x10 : 0x0);

				if(light_state_a != last_state_a || light_state_b != last_state_b)
					writeAIBusLightMessage(0xFF);

				if(last_hood != hood) {
					this->doors_open &= 0xDF;
					if(hood)
						this->doors_open |= 0x20;

					writeAIBusDoorMessage(0xFF, key_pos == 0);
				}
			} else if(can_msg.can_id == 0x92F85050 && can_msg.can_dlc == 5) { //Speed and ECON.
				const uint8_t last_speed = vehicle_speed;
				vehicle_speed = can_msg.data[0];

				if(vehicle_speed != last_speed)
					writeAIBusSpeedMessage(0xFF);

				this->econ_mode = (can_msg.data[3]&0x20) != 0;
			} else if(can_msg.can_id == 0x92F96250 && can_msg.can_dlc == 7) { //Temperature and range.
				const uint8_t last_honda_temp = honda_temp;
				const int16_t last_temp = outside_temp;
				const bool last_fahrenheit = honda_fahrenheit;

				honda_fahrenheit = (can_msg.data[0]&0x2) != 0;
				honda_temp = can_msg.data[2];

				if((parameter_list->display_celsius && !honda_fahrenheit) || (!parameter_list->display_celsius && honda_fahrenheit)) {
					outside_temp = (honda_temp - 0x28)*10;
				} else if(parameter_list->display_celsius && honda_fahrenheit) { //Temp is being broadcast in Fahrenheit, but show Celsius.
					const int16_t norm_temp = (honda_temp - 0x28)*10;
					outside_temp = (norm_temp - 320)*5/9;

					if(outside_temp%10 >= 5)
						outside_temp = (outside_temp/10)*10 + 5;
					else
						outside_temp = (outside_temp/10)*10;
				} else if(!parameter_list->display_celsius && !honda_fahrenheit) { //Temp is being broadcast in Celsius, but show Fahrenheit.
					const int16_t norm_temp = (honda_temp - 0x28)*10;
					outside_temp = norm_temp*9/5 + 320;

					if(outside_temp%10 >= 5)
						outside_temp = (outside_temp/10)*10 + 5;
					else
						outside_temp = (outside_temp/10)*10;
				}
				
				if(last_honda_temp != honda_temp || last_fahrenheit != honda_fahrenheit || last_temp != outside_temp)
					writeAIBusTempMessage(0xFF);
			} else if(can_msg.can_id == 0x92F86150 && can_msg.can_dlc == 4) { //A/C operation and auto stop.
				this->electric_ac_power = (can_msg.data[0] << 8) | can_msg.data[1];
				this->auto_stop = (can_msg.data[3]&0x80) != 0;
			} else if(can_msg.can_id == 0x92F85250 && can_msg.can_dlc == 4) { //Coolant temperature.
				const uint8_t last_temp = coolant_temp;
				coolant_temp = can_msg.data[0];

				if(last_temp != coolant_temp)
					writeAIBusCoolantTempMessage(0xFF);
			}
		}
	} while(can_timer < 300);
}

//Write all CAN-derived parameters.
void BCAN_Handler::sendAllParameters() {
	
}

//Write the key state message.
void BCAN_Handler::writeAIBusKeyMessage(const uint8_t receiver, const bool repeat) {
	uint8_t key_data[receiver == 0xFF ? 3 : 2];
	if(receiver == 0xFF) {
		key_data[0] = 0xA1;
		key_data[1] = 0x2;
		key_data[2] = key_pos;
	} else {
		key_data[0] = 0x2;
		key_data[1] = key_pos;
	}

	AIData key_msg(sizeof(key_data), ID_CANSLATOR, receiver);
	key_msg.refreshAIData(key_data);

	ai_handler->writeAIData(&key_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
	if(repeat) //Send again to ensure everything wakes up and gets the message.
		ai_handler->writeAIData(&key_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the door state message.
void BCAN_Handler::writeAIBusDoorMessage(const uint8_t receiver, const bool repeat) {
	uint8_t door_data[receiver == 0xFF ? 3: 2];
	if(receiver == 0xFF) {
		door_data[0] = 0xA1;
		door_data[1] = 0x43;
		door_data[2] = doors_open;
	} else {
		door_data[0] = 0x43;
		door_data[1] = doors_open;
	}

	AIData door_msg(sizeof(door_data), ID_CANSLATOR, receiver);
	door_msg.refreshAIData(door_data);

	ai_handler->writeAIData(&door_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
	if(repeat) //Send again to ensure everything wakes up and gets the message.
		ai_handler->writeAIData(&door_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the brightness state message.
void BCAN_Handler::writeAIBusBrightnessMessage(const uint8_t receiver, const bool repeat) {
	const uint8_t brightness_norm = brightness >= 0x16 ? 255 : brightness*11;
	const uint8_t light_byte = (night_mode ? 0x80 : 0x0) | (lights_on ? 0x1 : 0x0);

	uint8_t light_data[] = {0xA1, 0x10, brightness_norm, light_byte};
	AIData light_msg(sizeof(light_data), ID_CANSLATOR, receiver);

	light_msg.refreshAIData(light_data);
	ai_handler->writeAIData(&light_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
	if(repeat)
		ai_handler->writeAIData(&light_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the light state message.
void BCAN_Handler::writeAIBusLightMessage(const uint8_t receiver) {
	uint8_t light_data[receiver == 0xFF ? 4 : 3];
	if(receiver == 0xFF) {
		light_data[0] = 0xA1;
		light_data[1] = 0x11;
		light_data[2] = light_state_a;
		light_data[3] = light_state_b;
	} else {
		light_data[0] = 0x11;
		light_data[1] = light_state_a;
		light_data[2] = light_state_b;
	}
	AIData light_msg(sizeof(light_data), ID_CANSLATOR, receiver);
	light_msg.refreshAIData(light_data);

	ai_handler->writeAIData(&light_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the speed message.
void BCAN_Handler::writeAIBusSpeedMessage(const uint8_t receiver) {
	uint8_t speed_data[] = {0xA1, 0x1F, 0x4, 0x1, vehicle_speed};
	AIData speed_msg(sizeof(speed_data), ID_CANSLATOR, receiver);
	speed_msg.refreshAIData(speed_data);

	ai_handler->writeAIData(&speed_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the temperature message.
void BCAN_Handler::writeAIBusTempMessage(const uint8_t receiver) {
	uint8_t temp_data[] = {0xA1, 0x1F, 0x3, 0x12, 0x0, 0x0};

	if(outside_temp < 0)
		temp_data[3] |= 0x8;

	if(!parameter_list->display_celsius)
		temp_data[3] |= 0x80;

	const uint16_t norm_temp = abs(outside_temp);
	temp_data[4] = uint8_t(norm_temp>>8);
	temp_data[5] = uint8_t(norm_temp&0xFF);

	AIData temp_msg(sizeof(temp_data), ID_CANSLATOR, receiver);
	temp_msg.refreshAIData(temp_data);

	ai_handler->writeAIData(&temp_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the coolant temp message.
void BCAN_Handler::writeAIBusCoolantTempMessage(const uint8_t receiver) {
	uint8_t temp_data[] = {0xA1, 0x1F, 0x5, 0x81, 0x0};
	
	const int norm_temp = coolant_temp - 0x28;
	
	if(norm_temp < 0)
		temp_data[3] |= 0x8;
	
	const uint16_t abs_temp = abs(norm_temp);
	temp_data[4] = uint8_t(abs_temp&0xFF);
	
	AIData temp_msg(sizeof(temp_data), ID_CANSLATOR, receiver);
	temp_msg.refreshAIData(temp_data);

	ai_handler->writeAIData(&temp_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}