#include <Arduino.h>
#include <stdint.h>
#include <mcp2515.h>
#include <MCP4251.h>
#include <Vector.h>
#include <MCP23S08.h>

#include <mcp3201.h>

#include "AIBus.h"
#include "En_AIBus_Handler.h"
#include "CAN_Handler.h"
#include "CAN_Menu_Handler.h"
#include "Parameter_List.h"
#include "CANslator_EEPROM.h"
#include "Aux_Light_Control.h"

#ifndef canslator_h
#define canslator_h

#ifdef MEGACOREX
#define GPS_1SEC PIN_PA2
#define MCP_RESET PIN_PA3
#define AI_RX PIN_PA7
#define BCAN_CS PIN_PC0
#define BCAN_IMID_CS PIN_PC1
#define BCAN_RLS_CS PIN_PD0
#define FCAN_CS PIN_PD1

#define CAN_RESET PIN_PD2
#define PERIPHERAL_CS PIN_PD3

#define AUX_LIGHT_CS PIN_PD4
#define INT_LIGHT_CS PIN_PD5
#define BV_ADC_CS PIN_PD7

#define AMBIENT_ENABLE PIN_PF2
#define HFT_CS PIN_PF3

#define POWER_ON PIN_PF6
#else
#define GPS_1SEC 2
#define MCP_RESET 3
#define AI_RX 4
#define POWER_ON 5
#define AMBIENT_ENABLE 6
#define PERIPHERAL_CS 7

#define BCAN_CS 10
#define BCAN_IMID_CS 14
#define BCAN_RLS_CS 15
#define FCAN_CS 16

#define AUX_LIGHT_CS 18
#define INT_LIGHT_CS 19

#define BV_ADC_CS 21
#define HFT_CS 22

#define CAN_RESET 17
#endif

#define PARAM_TIMER 750
#define BATTERY_TIMER 1200

#define WASHER_FLUID_TIMER 5000

#define PERIPHERAL_WASHER_SENSOR 0
#define PERIPHERAL_WASHER_IND 1
#define PERIPHERAL_REAR_FOG_SW 2
#define PERIPHERAL_REAR_FOG_IND 3
#define PERIPHERAL_INTERIOR_LIGHTS 4
#define PERIPHERAL_VIDEO_POWER 5
#define PERIPHERAL_VIDEO_SELECT 6
#define PERIPHERAL_IGNITION 7

#define PERIPHERAL_REVERSE_SW 0
#define PERIPHERAL_CAMERA_CTL 1
#define PERIPHERAL_GPS_EN 2
#define PERIPHERAL_GPS_FIX 3
#define PERIPHERAL_LANEWATCH_SW 4

#define AISerial Serial
#define LWCSerial Serial1
#define GPSSerial Serial2

class CANslator {
public:
	void setup();
	void loop();
private:
	EnAIBusHandler ai_handler = EnAIBusHandler(&AISerial, AI_RX, 4, 64);
	ParameterList parameters;

	BCAN_Handler bcan_handler = BCAN_Handler(&ai_handler, &parameters, BCAN_CS, BCAN_IMID_CS, BCAN_RLS_CS, FCAN_CS);

	AuxLightController aux_light_controller = AuxLightController(AUX_LIGHT_CS, 0);
	MCP4251 int_light_controller = MCP4251(INT_LIGHT_CS, 100000, 0, 100000, 0);

	MCP3201 bat_adc = MCP3201(BV_ADC_CS);

	MCP23S08 peripheral_mcp = MCP23S08(PERIPHERAL_CS, 0), peripheral_mcp2 = MCP23S08(PERIPHERAL_CS, 1);

	bool param_timer_enabled = false;
	elapsedMillis param_timer;

	elapsedMillis wiper_timer; //The wiper timer.
	elapsedMillis washer_fluid_timer; //Buffer for "Washer Fluid Low."

	elapsedMillis minute_timer = 0;
	uint32_t minute_count = 0;

	elapsedMillis battery_timer;
	uint16_t battery_voltage;

	uint32_t trip_distance = 0;

	void handleAIBus(AIData* ai_msg);
	void writeAIBusTimerMessage();
	void writeAIBusTripDistanceMessage();
};

void setup();
void loop();

#endif