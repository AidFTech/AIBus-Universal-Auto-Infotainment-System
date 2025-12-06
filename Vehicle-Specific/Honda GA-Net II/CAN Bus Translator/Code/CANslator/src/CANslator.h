#include <Arduino.h>
#include <stdint.h>
#include <mcp2515.h>
#include <Vector.h>
#include <MCP23S08.h>

#include "AIBus.h"
#include "En_AIBus_Handler.h"
#include "CAN_Handler.h"
#include "CAN_Menu_Handler.h"
#include "Parameter_List.h"
#include "CANslator_EEPROM.h"

#ifndef canslator_h
#define canslator_h

#ifdef MEGACOREX
#define MCP_RESET PIN_PA3
#define AI_RX PIN_PA7
#define BCAN_CS PIN_PC0
#define BCAN_IMID_CS PIN_PC1
#define BCAN_RLS_CS PIN_PC2
#define FCAN_CS PIN_PC3

#define CAN_RESET PIN_PD0
#define WASHER_IND PIN_PD1

#define AUX_LIGHT_CS PIN_PD4

#define WASHER_SENSOR PIN_PF5
#define POWER_ON PIN_PF6
#else
#define MCP_RESET 3
#define AI_RX 4
#define POWER_ON 5
#define WASHER_SENSOR 6
#define WASHER_IND 7

#define BCAN_CS 10
#define BCAN_IMID_CS 14
#define BCAN_RLS_CS 15
#define FCAN_CS 16

#define AUX_LIGHT_CS 18

#define CAN_RESET 17
#endif

#define AUX_LIGHT_DRL_L 0
#define AUX_LIGHT_DRL_R 1
#define AUX_LIGHT_TURN_L 2
#define AUX_LIGHT_TURN_R 3

#define PARAM_TIMER 750

#if !defined(HAVE_HWSERIAL1)
#define AISerial Serial
#else
#define AISerial Serial1
#endif

class CANslator {
public:
	void setup();
	void loop();
private:
	EnAIBusHandler ai_handler = EnAIBusHandler(&AISerial, AI_RX, 4, 64);
	ParameterList parameters;

	BCAN_Handler bcan_handler = BCAN_Handler(&ai_handler, &parameters, BCAN_CS, BCAN_IMID_CS, BCAN_RLS_CS, FCAN_CS);

	MCP23S08 aux_light_controller = MCP23S08(AUX_LIGHT_CS, 0);

	bool param_timer_enabled = false;
	elapsedMillis param_timer;

	elapsedMillis wiper_timer; //The wiper timer.

	elapsedMillis minute_timer = 0;
	uint32_t minute_count = 0;

	uint32_t trip_distance = 0;

	void handleAIBus(AIData* ai_msg);
	void writeAIBusTimerMessage();
	void writeAIBusTripDistanceMessage();
};

int main();

#endif