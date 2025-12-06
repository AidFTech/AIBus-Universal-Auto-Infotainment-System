#include <stdint.h>
#include <Arduino.h>
#include <elapsedMillis.h>

#include "AIBus_Handler.h"

#ifndef nav_avr_controller_h
#define nav_avr_controller_h

#ifdef MEGACOREX
#define PI_POWER PIN_PA2
#define POWER_ON PIN_PA3
#define PI_RUNNING PIN_PA4
#define PI_BOOT PIN_PA5
#define PI_OFF_HARDWARE PIN_PA6
#define PI_OFF_SOFT PIN_PA7
#define AI_RX PIN_PC0
#else
#define PI_POWER 2
#define POWER_ON 3
#define PI_RUNNING 4
#define PI_BOOT 5
#define PI_OFF_HARDWARE 6
#define PI_OFF_SOFT 7
#define AI_RX 8
#endif

#define DOOR_TIMER 30000
#define PI_BOOT_TIMER 20

#define AISerial Serial

class NavAVRController {
public:
	void setup();
	void loop();
private:
	AIBusHandler ai_handler = AIBusHandler(&AISerial, AI_RX, ID_COMPUTER_PROXY);

	uint8_t key_position = 0, door_position = 0;
	bool pi_on = false, shutdown = false;
	bool boot = false, run = false;
	bool key_on = false; //True if the key has been in the on position at any time during this power cycle.

	elapsedMillis shutdown_timer = 0;
	bool use_shutdown_timer = false;

	elapsedMillis door_timer = 0;
	bool door_timer_enabled = false;

	elapsedMillis pi_boot_timer = 0;
	bool boot_timer_enabled = false;

	void powerOn();
	void powerOff();
};

void setup();
void loop();

#endif
