#include <stdint.h>
#include <mcp2515.h>

#include "AIBus.h"
#include "AIBus_Handler.h"
#include "CAN_Handler.h"
#include "Parameter_List.h"

#ifndef canslator_h
#define canslator_h

#ifdef MEGACOREX
#define AI_RX PIN_PA4
#define BCAN_CS PIN_PC2
#else
#define AI_RX 4

#define BCAN_CS 10
#endif

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

	BCAN_Handler bcan_handler = BCAN_Handler(&ai_handler, &parameters, BCAN_CS);
};

#endif