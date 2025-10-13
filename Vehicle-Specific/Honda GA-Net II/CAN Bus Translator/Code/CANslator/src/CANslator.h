#include <Arduino.h>
#include <stdint.h>
#include <mcp2515.h>

#include "AIBus.h"
#include "AIBus_Handler.h"
#include "CAN_Handler.h"
#include "CAN_Menu_Handler.h"
#include "Parameter_List.h"

#ifndef canslator_h
#define canslator_h

#ifdef MEGACOREX
#define AI_RX PIN_PA7
#define BCAN_CS PIN_PC0
#define BCAN_IMID_CS PIN_PC1
#define BCAN_RLS_CS PIN_PC2
#define FCAN_CS PIN_PC3

#define CAN_RESET PIN_PD0
#define WASHER_IND PIN_PD1

#define WASHER_SENSOR PIN_PF5
#define POWER_ON PIN_PF6
#else
#define AI_RX 4
#define POWER_ON 5
#define WASHER_SENSOR 6
#define WASHER_IND 7

#define BCAN_CS 10
#define BCAN_IMID_CS 14
#define BCAN_RLS_CS 15
#define FCAN_CS 16

#define CAN_RESET 17
#endif

#define PARAM_TIMER 750

#define WIPER_TIMER_L 15000
#define WIPER_TIMER_H 6000

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
	AIBusHandler ai_handler = AIBusHandler(&AISerial, AI_RX);
	ParameterList parameters;

	BCAN_Handler bcan_handler = BCAN_Handler(&ai_handler, &parameters, BCAN_CS, BCAN_IMID_CS, BCAN_RLS_CS, FCAN_CS);

	bool param_timer_enabled = false;
	elapsedMillis param_timer;

	uint32_t wiper_time_limit = WIPER_TIMER_L; //The wiper time limit.
	elapsedMillis wiper_timer; //The wiper timer.

	void handleAIBus(AIData* ai_msg);
};

int main();

#endif