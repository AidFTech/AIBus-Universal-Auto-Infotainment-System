#include "CAN_Handler.h"

BCAN_Handler::BCAN_Handler(AIBusHandler* ai_handler,
							ParameterList* parameter_list,
							const uint8_t b_cs_pin,
							const uint8_t imid_cs_pin,
							const uint8_t rls_cs_pin,
							const uint8_t f_cs_pin) :
	bcan_2515(b_cs_pin),
	imid_2515(imid_cs_pin),
	rls_2515(rls_cs_pin),
	fcan_2515(f_cs_pin),
	menu_handler(ai_handler, parameter_list) {
	this->ai_handler = ai_handler;
	this->parameter_list = parameter_list;
	
	auto_stop = false;
	econ_mode = false;
	e_brake = false;
	brightness_bar = false;
	lights_on = false;

	left_signal_on = false;
	right_signal_on = false;
	hazard_on = false;
	brake_light_on = false;
	left_signal_illum = false;
	right_signal_illum = false;
	high_beam_full = false;
	
	coolant_temp = 0x35;
	eco_bar = 0x1C;
	parameter_list->doors_open = 0x00;
	brightness = 0x16;
	vehicle_speed = 0;
	wiper_pos = 0;
	wiper_delay_pos = 0;
	
	outside_temp = 50;
	electric_ac_power = 0;
	eco_leaf_meter = 0;
	honda_gear = 0x4002;
	
	odo_km = 0;

	parameter_list->key_pos = 0;
	
	rec_device_vec.setStorage(rec_device_list, 0);
}

//Initialize the MCP2515.
void BCAN_Handler::init() {
	bcan_2515.reset();
	bcan_2515.setBitrate(CAN_125KBPS);
	bcan_2515.setNormalMode();

	imid_2515.reset();
	imid_2515.setBitrate(CAN_125KBPS);
	imid_2515.setNormalMode();

	rls_2515.reset();
	rls_2515.setBitrate(CAN_125KBPS);
	rls_2515.setNormalMode();

	fcan_2515.reset();
	fcan_2515.setBitrate(CAN_500KBPS);
	fcan_2515.setNormalMode();
}

//Interpret an AIBus message. Return whether the message is relevant.
bool BCAN_Handler::handleAIBus(AIData* ai_msg) {
	if(ai_msg->sender == ID_NAV_COMPUTER && ai_msg->l >= 2 && ai_msg->data[0] == 0x2B) { //Menu operation.
		const uint8_t command = ai_msg->data[1];
		if(command == 0x45) { //Settings menu request.
			if(menu_handler.getActiveMenu() != ACTIVE_MENU_SETTINGS_MAIN) 
				menu_handler.createMainSettingsMenu();
			return true;
		} else if(command == 0x60) { //Option selected.
			handleSelection((*ai_msg)[2]);
			return true;
		} else if(command == 0x55 || command == 0x56 || command == 0x57) { //Menu request by name.
			String request = "";
			for(int i=2;i<ai_msg->l;i+=1)
				request += char(ai_msg->data[i]);

			if(command == 0x55) {
				if(request.equalsIgnoreCase("SETTINGS"))
					menu_handler.createMainSettingsMenu();
				else if(request.equalsIgnoreCase("DEVSET"))
					menu_handler.createSecretMenu();
			} else {
				const bool open = command == 0x57;
				
			}
		}
	} else if(ai_msg->l >= 2 && ai_msg->data[0] == 0x4A && ai_msg->data[1] == 0x18) { //Add this device to the broadcast list.
		bool device_exists = false;
		for(int i=0;i<rec_device_vec.size();i+=1) {
			if(rec_device_vec[i] == ai_msg->sender) {
				device_exists = true;
				break;
			}
		}

		if(!device_exists)
			rec_device_vec.push_back(ai_msg->sender);

		return true;
	}

	return false;
}

//Set the auxiliary light controller.
void BCAN_Handler::setAuxLightController(AuxLightController* aux_light_controller) {
	this->aux_light_controller = aux_light_controller;
}

//Set the interior light controller MCP4251.
void BCAN_Handler::setIntLightController(MCP4251* int_light_controller) {
	this->int_light_controller = int_light_controller;
}

//Return whether the interior lights are on.
bool BCAN_Handler::getInteriorLightsOn() {
	return this->lights_on;
}

//Get the ambient light brightness.
uint8_t BCAN_Handler::getBrightness() {
	return brightness >= 0x16 ? 255 : brightness*11;
}

//Get whether the left signal is on.
bool BCAN_Handler::getLeftSignalOn() {
	return this->left_signal_on;
}

//Get whether the right signal is on.
bool BCAN_Handler::getRightSignalOn() {
	return this->right_signal_on;
}

//Read a CAN frame.
void BCAN_Handler::readCANMessage() {
	struct can_frame can_msg;
	elapsedMillis can_timer = 300;
	bool first_can = false;

	do {
		if(can_timer < 300)
			ai_handler->cachePending(ID_CANSLATOR);

		if(bcan_2515.readMessage(&can_msg) == MCP2515::ERROR_OK) {
			if(!first_can) {
				first_can = true;
				can_timer = 0;
			}

			last_can_msg = 0;

			if(can_msg.can_id == BCAN_ID_KEYPOS && can_msg.can_dlc == 5) { //Key position main, brake lights, signal lights.
				const uint8_t last_key = parameter_list->key_pos;

				if((can_msg.data[0]&0x28) != 0 && (can_msg.data[2]&0x80) != 0) { //ACC 2, not cranking.
					if(parameter_list->key_pos != 4)
						parameter_list->key_pos = 2;
				} else if((can_msg.data[0]&0x28) != 0 && (can_msg.data[2]&0x80) == 0) //Cranking.
					parameter_list->key_pos = 3;
				else if((can_msg.data[0]&0x20) != 0 && (can_msg.data[2]&0x80) != 0) //ACC 1.
					parameter_list->key_pos = 1;
				else //Key off.
					parameter_list->key_pos = 0;

				if(parameter_list->key_pos != last_key) {
					writeAIBusKeyMessage();
					if(parameter_list->key_pos == 0 && parameter_list->switched_on)
						writeAIBusDoorMessage();
				}

				parameter_list->power_on = parameter_list->key_pos != 0 || (parameter_list->doors_open&0xC) != 0 || last_can_msg < CAN_TIMER;
				
				if(parameter_list->key_pos != 0)
					parameter_list->switched_on = true;

				//Brake and signal lights.
				const bool last_left_signal_illum = left_signal_illum, last_right_signal_illum = right_signal_illum, last_brake_lights = brake_light_on, last_hazard = hazard_on;

				left_signal_illum = (can_msg.data[2]&0x10) != 0;
				right_signal_illum = (can_msg.data[2]&0x8) != 0;
				hazard_on = (can_msg.data[2]&0x20) != 0;
				brake_light_on = (can_msg.data[0]&0x10) != 0;

				if(last_left_signal_illum != left_signal_illum || last_right_signal_illum != right_signal_illum) {
					light_state.data.left_turn = left_signal_illum;
					light_state.data.right_turn = right_signal_illum;
					writeAIBusLightMessage(0xFF);

					aux_light_controller->setLeftTurn(left_signal_illum);
					aux_light_controller->setRightTurn(right_signal_illum);
				}

				if(last_brake_lights != brake_light_on || last_hazard != hazard_on)
					writeAIBusSignalMessage();
				
			} else if(can_msg.can_id == BCAN_ID_GEAR && can_msg.can_dlc == 5) { //Engine running, e-brake and gear.
				const uint16_t last_gear = honda_gear;
				const uint8_t last_key = parameter_list->key_pos;
				const bool last_ebrake = e_brake;

				honda_gear = ((can_msg.data[0]<<8) | can_msg.data[1])&0x5005;
				e_brake = (can_msg.data[1]&0x2) != 0;

				if(can_msg.data[2] == 0x0) //Engine on.
					parameter_list->key_pos = 4;

				if(parameter_list->key_pos != last_key)
					writeAIBusKeyMessage();

				if(e_brake != last_ebrake) {
					writeAIBusSignalMessage();

					if(!e_brake && !light_state.data.parking && parameter_list->drl_setting != DRL_SETTING_OFF) {
						ext_drl_on = true;
						setDRLs(ext_drl_on);

						light_state.data.drl = true;
						writeAIBusLightMessage(0xFF);
					}
				}

				parameter_list->power_on = parameter_list->key_pos != 0 || (parameter_list->doors_open&0xC) != 0 || last_can_msg < CAN_TIMER;

				if(parameter_list->key_pos != 0)
					parameter_list->switched_on = true;
			} else if(can_msg.can_id == BCAN_ID_LEFTDOORS && can_msg.can_dlc == 1) { //Left doors.
				const uint8_t last_door = parameter_list->doors_open;

				parameter_list->doors_open &= ~0xA;
				parameter_list->doors_open |= (can_msg.data[0]&0xA0)>>4;

				parameter_list->power_on = parameter_list->key_pos != 0 || (parameter_list->doors_open&0xC) != 0 || last_can_msg < CAN_TIMER;;

				if(parameter_list->doors_open != last_door)
					writeAIBusDoorMessage();
			} else if(can_msg.can_id == BCAN_ID_RIGHTDOORS && can_msg.can_dlc == 1) { //Right doors.
				const uint8_t last_door = parameter_list->doors_open;

				parameter_list->doors_open &= ~0x5;
				parameter_list->doors_open |= (can_msg.data[0]&0x50)>>4;

				parameter_list->power_on = parameter_list->key_pos != 0 || (parameter_list->doors_open&0xC) != 0 || last_can_msg < CAN_TIMER;

				if(parameter_list->doors_open != last_door)
					writeAIBusDoorMessage();
			} else if(can_msg.can_id == BCAN_ID_TRUNK && can_msg.can_dlc == 1) { //Trunk.
				const uint8_t last_door = parameter_list->doors_open;

				parameter_list->doors_open &= ~0x10;
				parameter_list->doors_open |= (can_msg.data[0]&0x80)>>3;

				parameter_list->power_on = parameter_list->key_pos != 0 || (parameter_list->doors_open&0xC) != 0 || last_can_msg < CAN_TIMER;

				if(parameter_list->doors_open != last_door)
					writeAIBusDoorMessage();
			} else if(can_msg.can_id == BCAN_ID_BRIGHTNESS && can_msg.can_dlc == 8) { //Brightness.
				const uint8_t last_brightness = brightness;
				const bool last_night = night_mode, last_light = lights_on;

				brightness = can_msg.data[0]&0x1F;
				lights_on = (can_msg.data[0]&0x40) != 0;
				night_mode = (can_msg.data[5]&0x40)	!= 0;

				if(last_brightness != brightness || lights_on != last_light || night_mode != last_night) {
					writeAIBusBrightnessMessage();
					
					if(int_light_controller != nullptr) {
						const uint8_t abs_brightness = brightness >= 0x16 ? 255 : brightness*11;
						int_light_controller->DigitalPotSetWiperPosition(0, abs_brightness);
						int_light_controller->DigitalPotSetWiperPosition(1, abs_brightness*parameter_list->ambient_light_brightness/255);
					}
				}

				if(lights_on != last_light) {
					if((parameter_list->doors_open&0xF) == 0 && parameter_list->ambient_light_enable_int)
						parameter_list->ambient_state = lights_on ? AMBIENT_LIGHTS_DIMMED : AMBIENT_LIGHTS_OFF;
					else if((parameter_list->doors_open&0xF) != 0 && parameter_list->ambient_light_enable_door)
						parameter_list->ambient_state = AMBIENT_LIGHTS_FULL;
					else
						parameter_list->ambient_state = AMBIENT_LIGHTS_OFF;
				}
			} else if(can_msg.can_id == BCAN_ID_LIGHTS && can_msg.can_dlc == 2) { //Light state and hood.
				const aibus_ext_light_status last_state = light_state;
				const bool last_hood = (this->parameter_list->doors_open&0x20) != 0, last_tail = light_state.data.taillight, last_low_beam = light_state.data.low_beam;

				const bool side = (can_msg.data[0]&0x40) != 0;
				const bool low_beam = (can_msg.data[0]&0x2) != 0;
				const bool front_fog = (can_msg.data[1]&0x80) != 0;
				const bool high_beam = (can_msg.data[0]&0x1) != 0;

				if(low_beam)
					ext_drl_on = false;
				else if(parameter_list->parking_light_drl && !side)
					ext_drl_on = false;
				else if((!e_brake && (parameter_list->key_pos == 0x2 || parameter_list->key_pos == 0x4)) || (parameter_list->parking_light_drl && side))
					ext_drl_on = true;

				const bool drl = (can_msg.data[0]&0x8) != 0 || (ext_drl_on && parameter_list->drl_setting != DRL_SETTING_OFF);

				const bool tail = (can_msg.data[1]&0x40) != 0;
				const bool license = (can_msg.data[1]&0x8) != 0; //TODO: Check this?

				const bool hood = (can_msg.data[0]&0x80) != 0;

				light_state.data.drl = drl;
				light_state.data.parking = side;
				light_state.data.low_beam = low_beam;
				light_state.data.front_fog = front_fog;
				light_state.data.left_turn = left_signal_illum;
				light_state.data.right_turn = right_signal_illum;
				light_state.data.high_beam = high_beam;

				if(last_state.raw[0] != light_state.raw[0] || last_state.raw[1] != light_state.raw[1])
					writeAIBusLightMessage(0xFF);

				if(last_tail != tail)
					aux_light_controller->setTail(tail);

				if(last_low_beam != low_beam) {
					if(low_beam)
						aux_light_controller->setProjector(high_beam_full);
					else
						aux_light_controller->setProjector(false);
				}

				if(last_hood != hood) {
					this->parameter_list->doors_open &= ~0x20;
					if(hood)
						this->parameter_list->doors_open |= 0x20;

					writeAIBusDoorMessage();
				}

				if(drl != last_state.data.drl)
					setDRLs(drl);

				if(parameter_list->parking_light_drl && side != last_state.data.parking)
					setDRLs(drl || side);
			} else if(can_msg.can_id == BCAN_ID_LIGHTSTALKPOS && can_msg.can_dlc == 2) { //Headlight stalk position.
				const bool last_left_signal_on = left_signal_on, last_right_signal_on = right_signal_on, last_highbeam_on = high_beam_full;

				left_signal_on = (can_msg.data[1]&0x80) != 0 && ((parameter_list->key_pos&0xF) == 0x2 || (parameter_list->key_pos&0xF) == 0x4);
				right_signal_on = (can_msg.data[1]&0x40) != 0 && ((parameter_list->key_pos&0xF) == 0x2 || (parameter_list->key_pos&0xF) == 0x4);
				high_beam_full = (can_msg.data[0]&0x40) != 0;

				if(last_left_signal_on != left_signal_on || last_right_signal_on != right_signal_on) {
					writeAIBusSignalMessage();

					if(parameter_list->drl_setting == DRL_SETTING_WINK && !(parameter_list->parking_light_drl ? (light_state.data.low_beam || light_state.data.parking) : light_state.data.low_beam)) {
						parameter_list->left_drl_on = !left_signal_on;
						parameter_list->right_drl_on = !right_signal_on;
					}
				}

				if(high_beam_full != last_highbeam_on) {
					if(light_state.data.low_beam)
						aux_light_controller->setProjector(high_beam_full);
					else
						aux_light_controller->setProjector(false);
				}
			} else if(can_msg.can_id == BCAN_ID_WIPERSTALKPOS && can_msg.can_dlc == 3) { //Wiper stalk position.
				const bool auto_wiper = parameter_list->auto_wiper, wiper_door_off = parameter_list->wiper_door_off;
				const uint8_t last_wiper_pos = wiper_pos, last_delay_pos = wiper_delay_pos;

				wiper_pos = can_msg.data[0];
				wiper_delay_pos = can_msg.data[1];

				if(rain_sensor_connected && !auto_wiper && ((wiper_pos != last_wiper_pos && (wiper_pos&0x40) != 0) || wiper_delay_pos < last_delay_pos)) {
					runWiper();
					if(wiper_timer != nullptr)
						*wiper_timer = 0;
				}

				if((can_msg.data[0]&0x40) != 0) {
					if(!auto_wiper || (wiper_door_off && (parameter_list->doors_open&0xC) != 0))
						can_msg.data[0] &= ~0x40;
				}

				parameter_list->wiper_time_limit = int32_t((WIPER_TIMER_L - WIPER_TIMER_H)*wiper_delay_pos)/255 + WIPER_TIMER_H;
			} else if(can_msg.can_id == BCAN_ID_SPEED && can_msg.can_dlc == 5) { //Speed and ECON.
				const uint8_t last_speed = vehicle_speed;
				vehicle_speed = can_msg.data[0];

				if(vehicle_speed != last_speed) {
					writeAIBusSpeedMessage(0xFF);

					if(parameter_list->special_trim == TRIM_HYBRID) {
						if(vehicle_speed > 0) {
							if(hybrid_status.data.engine_to_motor && !hybrid_status.data.motor_to_bat) { //Engine to motor but not battery.
								hybrid_status.raw[0] = 0;
								hybrid_status.data.engine_to_wheels = true;
								writeAIBusHybridStatusMessage();
							} else if(hybrid_status.data.engine_to_motor && hybrid_status.data.motor_to_bat) { //Engine to motor and battery.
								hybrid_status.data.engine_to_wheels = true;
								writeAIBusHybridStatusMessage();
							}
						} else {
							if(hybrid_status.data.engine_to_wheels && !hybrid_status.data.motor_to_bat) { //Engine to wheels but not battery.
								hybrid_status.raw[0] = 0;
								hybrid_status.data.engine_to_motor = true;
								writeAIBusHybridStatusMessage();
							} else if(hybrid_status.data.engine_to_wheels && hybrid_status.data.motor_to_bat) { //Engine to wheels and battery.
								hybrid_status.data.engine_to_wheels = false;
								hybrid_status.data.engine_to_motor = true;
								writeAIBusHybridStatusMessage();
							}
						}
					}
				}

				this->econ_mode = (can_msg.data[3]&0x20) != 0;
			} else if(can_msg.can_id == BCAN_ID_TEMP_RANGE && can_msg.can_dlc == 7) { //Temperature and range.
				const uint8_t last_honda_temp = honda_temp;
				const int16_t last_temp = outside_temp;
				const bool last_fahrenheit = honda_fahrenheit;

				const uint16_t last_range = range;
				const bool last_range_miles = range_miles;

				honda_fahrenheit = (can_msg.data[0]&0x2) != 0;
				honda_temp = can_msg.data[2];

				range_miles = (can_msg.data[0]&0x80) != 0;
				range = ((can_msg.data[0]&0x7) << 8) | can_msg.data[1];

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

				if(last_range_miles != range_miles || last_range != range)
					writeAIBusRangeMessage();

				parameter_list->display_miles = range_miles;
			} else if(can_msg.can_id == BCAN_ID_AC_AUTOSTOP && can_msg.can_dlc == 4) { //A/C operation and auto stop.
				const bool last_auto_stop = this->auto_stop;

				this->electric_ac_power = (can_msg.data[0] << 8) | can_msg.data[1];
				this->auto_stop = (can_msg.data[3]&0x80) != 0;

				if(this->auto_stop != last_auto_stop)
					writeAIBusAutoStopMessage();
			} else if(can_msg.can_id == BCAN_ID_COOLANT && can_msg.can_dlc == 4) { //Coolant temperature.
				const uint8_t last_temp = coolant_temp;
				coolant_temp = can_msg.data[0];

				if(last_temp != coolant_temp)
					writeAIBusCoolantTempMessage(0xFF);
			} else if(can_msg.can_id == BCAN_ID_HYBRID_SYSTEM && can_msg.can_dlc == 7) { //Hybrid status.
				if(parameter_list->special_trim == TRIM_HYBRID) { // @ TODO: This message may mean something different in other trims.
					const uint8_t last_hybrid_status = hybrid_status.raw[0], last_hybrid_battery = hybrid_battery, last_charge_assist = charge_assist;
					hybrid_status.raw[0] = 0;

					switch(can_msg.data[2]&0xF) {
					case 0x1: //Battery only.
						hybrid_status.data.bat_to_motor = true;
						hybrid_status.data.motor_to_wheels = true;
						break;
					case 0x2: //Regenerative charging.
						hybrid_status.data.wheels_to_motor = true;
						hybrid_status.data.regen_to_bat = true;
						break;
					case 0x3: //Engine and battery both providing power.
						hybrid_status.data.bat_to_motor = true;
						hybrid_status.data.motor_to_wheels = true;
						hybrid_status.data.engine_to_wheels = true;
						break;
					case 0x4: //Engine only.
						if(vehicle_speed > 0)
							hybrid_status.data.engine_to_motor = true;
						else
						 	hybrid_status.data.engine_to_wheels = true;
						break;
					case 0x5: //Engine charging battery.
						hybrid_status.data.engine_to_motor = true;
						hybrid_status.data.bat_to_motor = true;
						if(vehicle_speed > 0)
							hybrid_status.data.engine_to_wheels = true;
						break;
					default:
						break;
					}

					const uint8_t honda_battery = can_msg.data[3]&0xF;
					hybrid_battery = uint32_t(honda_battery*255)/8; // @TODO: Calculate from F-CAN if possible.

					switch(can_msg.data[2]&0xF) {
					case 0x1:
					case 0x3:
						charge_assist = 0x7F + (can_msg.data[1]&0xF);
						break;
					case 0x2:
					case 0x5:
						charge_assist = 0x7F + (0xF - can_msg.data[1]&0xF); // @TODO: Check.
						break;
					default:
						charge_assist = 0x7F;
						break;
					}

					if(hybrid_init && (hybrid_status.raw[0] != last_hybrid_status || hybrid_battery != last_hybrid_battery || charge_assist != last_charge_assist))
						writeAIBusHybridStatusMessage();
				}
			} else if(can_msg.can_id == BCAN_ID_AVG_ECONOMY && can_msg.can_dlc == 8) { //Average economy.
				const bool last_mpg = economy_mpg;
				economy_mpg = (can_msg.data[0]&0x40) != 0; //TODO: Keep experimenting with this. It doesn't seem solid.

				const int32_t last_current_economy = current_economy;

				if((can_msg.data[0]&0x3F) == 0x3F && (can_msg.data[1]&0xF8) == 0xF8) //Last economy is undefined.
					current_economy = -1;
				else
					current_economy = ((can_msg.data[0]&0x3F) << 5) | ((can_msg.data[1]&0xF8) >> 3);

				if(last_current_economy != current_economy || last_mpg != economy_mpg) {
					writeAIBusAverageEconomyMessage();
				}
			}

			if(can_msg.can_id == BCAN_ID_TEMP_RANGE && can_msg.can_dlc >= 3) { //Temperature message. Forward to IMID.
				if((honda_fahrenheit && parameter_list->display_celsius) || (!honda_fahrenheit && !parameter_list->display_celsius)) {
					const int16_t norm_temp = outside_temp/10 + 0x28;
					const uint8_t display_temp = norm_temp >= 0 ? (norm_temp <= 0xFF ? norm_temp&0xFF : 0xFF) : 0;

					can_msg.data[2] = display_temp;
					if(parameter_list->display_celsius)
						can_msg.data[0] &= ~0x20;
					else
						can_msg.data[0] |= 0x20;
				}
			} else if(can_msg.can_id == BCAN_ID_IMIDMSG1 && can_msg.can_dlc >= 1) { //Washer fluid low message.
				if(parameter_list->washer_fluid_low)
					can_msg.data[0] |= 0x1;
			}

			imid_2515.sendMessage(&can_msg);
			rls_2515.sendMessage(&can_msg);
		} else if(last_can_msg >= CAN_TIMER && parameter_list->key_pos == 0 && (parameter_list->doors_open&0xC) == 0)
			parameter_list->power_on = false;

		forwardIMIDMessage();
		forwardRLSMessage();
	} while(can_timer < 300);

	forwardIMIDMessage();
}

//Set the wiper timer.
void BCAN_Handler::setWiperTimer(elapsedMillis* wiper_timer) {
	this->wiper_timer = wiper_timer;
}

//Return whether the wiper stalk is in the INT/AUTO position.
bool BCAN_Handler::getWiperIntActive() {
	return (this->wiper_pos&0x40) != 0;
}

//Run the wipers if in manual mode or wipers on. Return whether the wipe was successful.
bool BCAN_Handler::runWiper() {
	if(parameter_list->auto_wiper || (parameter_list->wiper_door_off && (parameter_list->doors_open&0xC) != 0))
		return false;

	if(!rain_sensor_connected)
		return false;

	can_frame wiper_msg;
	wiper_msg.can_id = BCAN_ID_RAINSENSOR;
	wiper_msg.can_dlc = 1;
	wiper_msg.data[0] = 0x84;

	bcan_2515.sendMessage(&wiper_msg);
	imid_2515.sendMessage(&wiper_msg);

	return true;
}

//Send a next turn message to the IMID.
void BCAN_Handler::setNavNextTurn(const uint8_t entry_angle, const uint8_t exit_angle, const uint16_t roads_visible, const uint8_t step_num, const uint8_t special, String street_name) {
	street_name.toUpperCase();
	
	const int street_msg_count = 3 + ((street_name.length()-1)/6 + 1);

	can_frame count_msg;
	count_msg.can_id = BCAN_ID_NAV_DATA_LEN;
	count_msg.can_dlc = 4;
	count_msg.data[0] = 0x20;
	count_msg.data[1] = 0x0;
	count_msg.data[2] = 7*street_msg_count;
	count_msg.data[3] = street_msg_count;

	broadcastBCAN(&count_msg);

	const int normalized_angle = (exit_angle-entry_angle-8)%256;
	const unsigned int honda_angle = (((255-normalized_angle)*16)/256)&0xF;

	for(int m=0;m<street_msg_count;m+=1) {
		can_frame name_msg;
		name_msg.can_id = BCAN_ID_NAV_NEXT_TURN;
		name_msg.can_dlc = 8;

		for(int i=0;i<name_msg.can_dlc;i+=1)
			name_msg.data[i] = 0;

		name_msg.data[0] = m+1;
		if(m==0) { //Start bytes.
			name_msg.data[1] = step_num;
			uint8_t* special_option = &name_msg.data[2];
			switch(special) {
			case AIDF_NAV_SPECIAL_TRAFFIC_CIRCLE:
			case AIDF_NAV_SPECIAL_TRAFFIC_CIRCLE_EXIT:
			case AIDF_NAV_SPECIAL_TRAFFIC_CIRCLE_ENTER_EXIT:
				*special_option = 0x1;
				break;
			case AIDF_NAV_SPECIAL_UTURN_LEFT:
			case AIDF_NAV_SPECIAL_UTURN_RIGHT:
				*special_option = 0x6;
				break;
			case AIDF_NAV_SPECIAL_TOLL:
			case AIDF_NAV_SPECIAL_FERRY:
			case AIDF_NAV_SPECIAL_TRAIN:
				*special_option = 0x7;
				break;
			case AIDF_NAV_SPECIAL_WAYPOINT:
				*special_option = 0x8;
				break;
			case AIDF_NAV_SPECIAL_DESTINATION:
				*special_option = 0x9;
				break;
			default:
				*special_option = 0x0;
				break;
			}

			name_msg.data[3] = honda_angle&0xF;
			name_msg.data[4] = (roads_visible&0xFF00)>>8;
			name_msg.data[5] = roads_visible&0xFF;
			name_msg.data[6] = street_name.length();
		} else if(m>=3) { //Street name.
			const String substr = street_name.substring((m-3)*6, (m-2)*6);
			for(int i=0;i<6&&i<substr.length();i+=1)
				name_msg.data[i+1] = uint8_t(substr.charAt(i));
		}

		name_msg.data[7] = getHondaNavChecksum(&name_msg);
		broadcastBCAN(&name_msg);
	}
}

//Broadcast a BCAN message to all units.
void BCAN_Handler::broadcastBCAN(can_frame* can_msg) {
	bcan_2515.sendMessage(can_msg);
	imid_2515.sendMessage(can_msg);
	rls_2515.sendMessage(can_msg);
}

//Write the battery voltage message.
void BCAN_Handler::sendBatteryVoltage(const uint16_t voltage, const uint8_t receiver) {
	aibus_voltage_config voltage_config;
	voltage_config.data.negative = false;
	voltage_config.data.decimal_count = 2;

	int bytes = 1;
	uint64_t data_measure = 0x100;

	while(voltage >= data_measure && bytes < 7) {
		data_measure <<= 8;
		bytes += 1;
	}

	voltage_config.data.byte_count = bytes;

	AIData bv_msg(4+bytes, ID_CANSLATOR, receiver);
	bv_msg[0] = 0xA1;
	bv_msg[1] = 0x1F;
	bv_msg[2] = 0x6;
	bv_msg[3] = voltage_config.raw[0];
	for(int i=0;i<bytes;i+=1)
		bv_msg[i+4] = (voltage>>(8*(bytes-i-1)))&0xFF;

	ai_handler->writeAIData(&bv_msg, receiver != 0xFF);
}

//Write all common CAN-derived parameters.
void BCAN_Handler::sendCommonParameters(const uint8_t receiver) {
	writeAIBusKeyMessage(receiver);
	writeAIBusDoorMessage(receiver);
	writeAIBusBrightnessMessage(receiver);	
	writeAIBusLightMessage(receiver);
	writeAIBusTempMessage(receiver);
	writeAIBusSpeedMessage(receiver);
}

//Write info parameters.
void BCAN_Handler::sendInfoParameters(const uint8_t receiver) {
	if(parameter_list->special_trim == TRIM_HYBRID) {
		writeAIBusHybridHandshake(receiver);
		writeAIBusHybridStatusMessage(receiver);
	}

	writeAIBusTempMessage(receiver);
	writeAIBusCoolantTempMessage(receiver);
	writeAIBusRangeMessage(receiver);
	writeAIBusLightMessage(receiver);
	writeAIBusAutoStopMessage(receiver);
}

//Write the key state message to all requesting devices.
void BCAN_Handler::writeAIBusKeyMessage() {
	writeAIBusKeyMessage(0xFF);
	for(int i=0;i<rec_device_vec.size();i+=1)
		writeAIBusKeyMessage(rec_device_vec[i]);
}

//Write the door state message to all requesting devices.
void BCAN_Handler::writeAIBusDoorMessage() {
	writeAIBusDoorMessage(0xFF);
	for(int i=0;i<rec_device_vec.size();i+=1)
		writeAIBusDoorMessage(rec_device_vec[i]);
}

//Write the brightness message to all requesting devices.
void BCAN_Handler::writeAIBusBrightnessMessage() {
	writeAIBusBrightnessMessage(0xFF);
	for(int i=0;i<rec_device_vec.size();i+=1)
		writeAIBusBrightnessMessage(rec_device_vec[i]);
}

//Write the key state message.
void BCAN_Handler::writeAIBusKeyMessage(const uint8_t receiver) {
	uint8_t key_data[receiver != ID_NAV_COMPUTER ? 3 : 2];
	if(receiver != ID_NAV_COMPUTER) {
		key_data[0] = 0xA1;
		key_data[1] = 0x2;
		key_data[2] = parameter_list->key_pos;
	} else {
		key_data[0] = 0x2;
		key_data[1] = parameter_list->key_pos;
	}

	AIData key_msg(sizeof(key_data), ID_CANSLATOR, receiver, key_data);

	ai_handler->writeAIData(&key_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the door state message.
void BCAN_Handler::writeAIBusDoorMessage(const uint8_t receiver) {
	uint8_t door_data[receiver != ID_NAV_COMPUTER ? 3: 2];
	if(receiver != ID_NAV_COMPUTER) {
		door_data[0] = 0xA1;
		door_data[1] = 0x43;
		door_data[2] = parameter_list->doors_open | (parameter_list->key_pos == 0 && parameter_list->switched_on ? 0x80 : 0);
	} else {
		door_data[0] = 0x43;
		door_data[1] = parameter_list->doors_open | (parameter_list->key_pos == 0 && parameter_list->switched_on ? 0x80 : 0);
	}

	AIData door_msg(sizeof(door_data), ID_CANSLATOR, receiver, door_data);

	ai_handler->writeAIData(&door_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the brightness state message.
void BCAN_Handler::writeAIBusBrightnessMessage(const uint8_t receiver) {
	const uint8_t brightness_norm = brightness >= 0x16 ? 255 : brightness*11;
	const uint8_t light_byte = (night_mode ? 0x80 : 0x0) | (lights_on ? 0x1 : 0x0);

	uint8_t light_data[] = {0xA1, 0x10, brightness_norm, light_byte};
	AIData light_msg(sizeof(light_data), ID_CANSLATOR, receiver, light_data);

	ai_handler->writeAIData(&light_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the light state message.
void BCAN_Handler::writeAIBusLightMessage(const uint8_t receiver) {
	uint8_t light_data[receiver == 0xFF ? 4 : 3];
	if(receiver == 0xFF) {
		light_data[0] = 0xA1;
		light_data[1] = 0x11;
		light_data[2] = light_state.raw[0];
		light_data[3] = light_state.raw[1];
	} else {
		light_data[0] = 0x11;
		light_data[1] = light_state.raw[0];
		light_data[2] = light_state.raw[1];
	}
	AIData light_msg(sizeof(light_data), ID_CANSLATOR, receiver, light_data);

	ai_handler->writeAIData(&light_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the light signal message.
void BCAN_Handler::writeAIBusSignalMessage(const uint8_t receiver) {
	aibus_signal_status signal_status;
	signal_status.data.left_signal = left_signal_on;
	signal_status.data.right_signal = right_signal_on;
	signal_status.data.hazard = hazard_on;
	signal_status.data.brake_light = brake_light_on;
	signal_status.data.e_brake = e_brake;

	uint8_t signal_data[] = {0xA1, 0x13, signal_status.raw[0]};
	AIData signal_msg(sizeof(signal_data), ID_CANSLATOR, receiver, signal_data);
	ai_handler->writeAIData(&signal_msg, receiver != 0xFF);
}

//Write the speed message.
void BCAN_Handler::writeAIBusSpeedMessage(const uint8_t receiver) {
	uint8_t speed_data[] = {0xA1, 0x1F, 0x4, 0x1, vehicle_speed};
	AIData speed_msg(sizeof(speed_data), ID_CANSLATOR, receiver, speed_data);

	ai_handler->writeAIData(&speed_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the temperature message.
void BCAN_Handler::writeAIBusTempMessage(const uint8_t receiver) {
	aibus_temperature_config temp_config;
	temp_config.data.decimal_count = 1;
	temp_config.data.fahrenheit = !parameter_list->display_celsius;
	temp_config.data.negative = outside_temp < 0;

	int bytes = 1;
	uint64_t data_measure = 0x100;

	const uint16_t norm_temp = abs(outside_temp);
	while(norm_temp >= data_measure && bytes < 7) {
		data_measure <<= 8;
		bytes += 1;
	}

	temp_config.data.byte_count = bytes;

	AIData temp_msg(4 + bytes, ID_CANSLATOR, receiver);
	temp_msg[0] = 0xA1;
	temp_msg[1] = 0x1F;
	temp_msg[2] = 0x3;
	temp_msg[3] = temp_config.raw[0];
	for(int i=0;i<bytes;i+=1)
		temp_msg[i+4] = (norm_temp>>(8*(bytes-i-1)))&0xFF;

	ai_handler->writeAIData(&temp_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the coolant temp message.
void BCAN_Handler::writeAIBusCoolantTempMessage(const uint8_t receiver) {
	const int norm_temp = coolant_temp - 0x28;

	aibus_temperature_config temp_config;
	temp_config.data.decimal_count = 0;
	temp_config.data.fahrenheit = false;
	temp_config.data.negative = norm_temp < 0;
	temp_config.data.byte_count = 1;

	uint8_t temp_data[] = {0xA1, 0x1F, 0x5, temp_config.raw[0], 0x0};
	
	const uint16_t abs_temp = abs(norm_temp);
	temp_data[4] = uint8_t(abs_temp&0xFF);
	
	AIData temp_msg(sizeof(temp_data), ID_CANSLATOR, receiver, temp_data);

	ai_handler->writeAIData(&temp_msg, receiver != 0xFF && receiver != ID_CANSLATOR);
}

//Write the range message.
void BCAN_Handler::writeAIBusRangeMessage(const uint8_t receiver) {
	aibus_speed_distance_config range_config;
	range_config.data.decimal_count = 0;
	range_config.data.miles = range_miles;

	int bytes = 1;
	uint64_t data_measure = 0x100;
	while(range >= data_measure && bytes < 7) {
		data_measure <<= 8;
		bytes += 1;
	}

	range_config.data.byte_count = bytes;

	AIData range_msg(4 + bytes, ID_CANSLATOR, receiver);

	range_msg[0] = 0xA1;
	range_msg[1] = 0x1F;
	range_msg[2] = 0x7;
	range_msg[3] = range_config.raw[0];
	for(int i=0;i<bytes;i+=1)
		range_msg[i+4] = (range<<(8*(bytes-i-1)))&0xFF;

	ai_handler->writeAIData(&range_msg, receiver != 0xFF);
}

//Write the average fuel economy.
void BCAN_Handler::writeAIBusAverageEconomyMessage(const uint8_t receiver) {
	aibus_fuel_economy_config economy_config;
	economy_config.data.decimal_count = 1;
	economy_config.data.units = economy_mpg ? AIBUS_FUEL_ECONOMY_MPG_US : AIBUS_FUEL_ECONOMY_L_100KM;
	economy_config.data.undefined = current_economy < 0;

	int bytes = 1;
	if(current_economy >= 0) {
		uint64_t data_measure = 0x100;
		while(abs(current_economy) >= data_measure && bytes < 7) {
			data_measure <<= 8;
			bytes += 1;
		}
	}

	AIData econ_msg(4+bytes, ID_CANSLATOR, receiver);
	econ_msg[0] = 0xA1;
	econ_msg[1] = 0x1F;
	econ_msg[2] = 0x9;
	econ_msg[3] = economy_config.raw[0];
	if(current_economy >= 0) {
		for(int i=0;i<bytes;i+=1)
			econ_msg[i+4] = (current_economy>>(8*(bytes-i-1)))&0xFF;
	} else
		econ_msg[4] = 0xFF;

	ai_handler->writeAIData(&econ_msg, receiver != 0xFF);
}

//Write auto stop status.
void BCAN_Handler::writeAIBusAutoStopMessage(const uint8_t receiver) {
	uint8_t auto_stop_data[] = {0xA1, 0x14, auto_stop ? 0xFF : 0x0};

	AIData auto_stop_msg(sizeof(auto_stop_data), ID_CANSLATOR, receiver, auto_stop_data);
	ai_handler->writeAIData(&auto_stop_msg, receiver != 0xFF);
}

//Write the hybrid system handshake.
void BCAN_Handler::writeAIBusHybridHandshake(const uint8_t receiver) {
	uint8_t hybrid_data[] = {0xA1, 0x33, 0x1, 0x12, 0x30};
	AIData hybrid_msg(sizeof(hybrid_data), ID_CANSLATOR, receiver, hybrid_data);
	ai_handler->writeAIData(&hybrid_msg, receiver != 0xFF);

	hybrid_init = true;
}

//Write the hybrid system status.
void BCAN_Handler::writeAIBusHybridStatusMessage(const uint8_t receiver) {
	if(!hybrid_init)
		return;

	uint8_t hybrid_data[] = {0xA1, 0x33, 0x2, hybrid_status.raw[0], hybrid_battery, charge_assist};
	AIData hybrid_msg(sizeof(hybrid_data), ID_CANSLATOR, receiver, hybrid_data);
	ai_handler->writeAIData(&hybrid_msg, receiver != 0xFF);
}

//Forward a message from the IMID to the rest of the system.
void BCAN_Handler::forwardIMIDMessage() {
	struct can_frame can_msg;
	if(imid_2515.readMessage(&can_msg) == MCP2515::ERROR_OK) {
		bcan_2515.sendMessage(&can_msg);
		rls_2515.sendMessage(&can_msg);
		
		last_can_msg = 0;
	}
}

//Forward a message from the RLS to the rest of the system.
void BCAN_Handler::forwardRLSMessage() {
	struct can_frame can_msg;
	if(rls_2515.readMessage(&can_msg) != MCP2515::ERROR_OK)
		return;

	light_sensor_connected = true;

	if((can_msg.can_id&0xFF) == 0x74)
		rain_sensor_connected = true;

	if((can_msg.can_id&0xFFFFFF00) == (BCAN_ID_RAINSENSOR&0xFFFFFF00)) { //Rain sensor function.
		if(can_msg.can_dlc >= 1 && (can_msg.data[0]&0xE0) != 0) {
			if((parameter_list->doors_open&0xC) != 0 && parameter_list->wiper_door_off) //Doors are open, do not wipe.
				can_msg.data[0] &= ~0xE0;
			else if(!parameter_list->auto_wiper) //Intermittent mode.
				can_msg.data[0] &= ~0xE0;
		}
	} else if((can_msg.can_id&0xFFFFFF00) == (BCAN_ID_LIGHTSENSOR&0xFFFFFF00) && can_msg.can_dlc >= 1) { //Light sensor.
		if(parameter_list->headlight_temp_setting != HEADLIGHT_TEMP_OFF) {
			const int16_t outside_temp = parameter_list->display_celsius ? this->outside_temp : (this->outside_temp - 320)*5/9;

			const bool last_headlight_temp = headlight_on_temp;
			if(outside_temp <= headlight_temp_limit) {
				headlight_on_temp = true;
				if((can_msg.data[0]&0xC) == 0)
					can_msg.data[0] |= 0xDC;
			} else
				headlight_on_temp = false;

			if(last_headlight_temp != headlight_on_temp)
				calculateHeadlightTemperature();
		}

		//TODO: Manual transmission messages.
		if(parameter_list->parking_lights && (honda_gear == 0x4001 || (honda_gear == 0x401 && e_brake))) {
			if((can_msg.data[0]&0xC) == 0)
				can_msg.data[0] |= 0x14;
		}
	}

	bcan_2515.sendMessage(&can_msg);
	imid_2515.sendMessage(&can_msg);

	last_can_msg = 0;
}

//Get the headlight-temperature integration limit temp.
void BCAN_Handler::calculateHeadlightTemperature() {
	switch(parameter_list->headlight_temp_setting) {
	case HEADLIGHT_TEMP_5:
		headlight_temp_limit = 50;
		break;
	case HEADLIGHT_TEMP_10:
		headlight_temp_limit = 100;
		break;
	case HEADLIGHT_TEMP_15:
		headlight_temp_limit = 150;
		break;
	case HEADLIGHT_TEMP_OFF:
	default:
		return;
	}

	if(headlight_on_temp)
		headlight_temp_limit += HEADLIGHT_TEMP_BUFFER;
}

//Set the DRLs.
void BCAN_Handler::setDRLs(const bool drl) {
	const drl_setting_t drl_setting = parameter_list->drl_setting;
	if(drl_setting == DRL_SETTING_OFF) {
		parameter_list->left_drl_on = false;
		parameter_list->right_drl_on = false;
	} else if(drl_setting == DRL_SETTING_WINK) {
		parameter_list->left_drl_on = (drl & !left_signal_on) || (parameter_list->parking_light_drl && light_state.data.parking);
		parameter_list->right_drl_on = (drl & !right_signal_on) || (parameter_list->parking_light_drl && light_state.data.parking);
	} else if(drl_setting == DRL_SETTING_FULL) {
		parameter_list->left_drl_on = drl;
		parameter_list->right_drl_on = drl;
	}
}

//Get the Honda CAN bus checksum.
uint8_t getHondaNavChecksum(can_frame* can_msg) {
	unsigned int chx = 0;

	for(int i=0;i<can_msg->can_dlc - 1; i+=1) {
		const uint8_t msn = (can_msg->data[i]&0xF0)>>8, lsn = can_msg->data[i]&0xF;
		chx += msn + lsn;
	}

	return chx&0xF;
}

//Handle a menu selection.
void BCAN_Handler::handleSelection(const uint8_t selection) {
	const int selected = selection - 1;
	
	if(selected < 0)
		return;

	if(menu_handler.getActiveMenu() == ACTIVE_MENU_SETTINGS_MAIN) {
		const MenuList menu = getMenu(MENU_INDEX_SETTINGS_MAIN, parameter_list->locale);
		switch(menu.getGlobalIndex(selected)) {
		case MENU_INDEX_SETTINGS_MAIN_COMFORT_CONVENIENCE:
			menu_handler.createComfortConvenienceMenu(light_sensor_connected, rain_sensor_connected);
			break;
		case MENU_INDEX_SETTINGS_MAIN_AMBIENT_LIGHTING:
			if(parameter_list->has_ambient_lighting)
				menu_handler.createAmbientLightingMenu();
		default:
			break;
		}
	} else if(menu_handler.getActiveMenu() == ACTIVE_MENU_SETTINGS_COMFORT_CONVENIENCE) {
		const MenuList menu = getMenu(MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE, parameter_list->locale);
		switch(menu.getGlobalIndex(selected)) {
		case MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE_PARKING_LIGHTS:
			parameter_list->parking_lights = !parameter_list->parking_lights;
			menu_handler.createComfortConvenienceMenu(light_sensor_connected, rain_sensor_connected);
			setCanslatorSettings(parameter_list);
			break;
		case MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE_RAINSENSOR:
			parameter_list->auto_wiper = !parameter_list->auto_wiper;
			menu_handler.createComfortConvenienceMenu(light_sensor_connected, rain_sensor_connected);
			setCanslatorSettings(parameter_list);
			break;
		case MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE_WIPER_DOOR_OFF:
			parameter_list->wiper_door_off = !parameter_list->wiper_door_off;
			menu_handler.createComfortConvenienceMenu(light_sensor_connected, rain_sensor_connected);
			setCanslatorSettings(parameter_list);
			break;
		default:
			break;
		}
	}
}
