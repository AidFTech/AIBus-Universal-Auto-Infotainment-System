#include <stdint.h>
#include <Arduino.h>
#include <mcp2515.h>
#include <elapsedMillis.h>

#include "AIBus_Handler.h"
#include "AIBus.h"
#include "Parameter_List.h"

#ifndef CAN_Handler_h
#define CAN_Hanlder_h

class BCAN_Handler {
public:
	BCAN_Handler(AIBusHandler* ai_handler, ParameterList* parameter_list, uint8_t cs_pin);
	void init();
	void readCANMessage();
	
	void sendAllParameters();
private:
	uint8_t b_cs_pin;
	MCP2515* bcan_2515;

	AIBusHandler* ai_handler;
	ParameterList* parameter_list;
	
	//BCAN-derived variables:
	bool auto_stop, econ_mode, e_brake, brightness_bar, lights_on, night_mode;
	uint8_t hybrid_battery_level, coolant_temp, eco_bar, doors_open, brightness, vehicle_speed;
	uint16_t electric_ac_power, eco_leaf_meter, gear;
	int16_t outside_temp; //Last digit is tenths place.
	uint32_t odo_km;

	uint8_t honda_temp = 0x28; //Temp byte sent by the cluster.
	bool honda_fahrenheit = false; //True if the temp byte above is in Fahrenheit.

	uint8_t key_pos = 0;

	uint8_t light_state_a = 0, light_state_b = 0;

	void writeAIBusKeyMessage(const uint8_t receiver, const bool repeat);
	void writeAIBusDoorMessage(const uint8_t receiver, const bool repeat);
	void writeAIBusBrightnessMessage(const uint8_t receiver, const bool repeat);
	void writeAIBusTempMessage(const uint8_t receiver);
	void writeAIBusLightMessage(const uint8_t receiver);
	void writeAIBusSpeedMessage(const uint8_t receiver);
	
	void writeAIBusCoolantTempMessage(const uint8_t receiver);
};

#endif
