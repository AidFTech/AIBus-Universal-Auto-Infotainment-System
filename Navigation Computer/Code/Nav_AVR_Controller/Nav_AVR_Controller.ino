#include <stdint.h>
#include <Arduino.h>
#include <elapsedMillis.h>

#include "AIBus_Handler.h"

#ifdef MEGACOREX
#define PI_POWER PIN_PA2
#define POWER_ON PIN_PA3
#define PI_RUNNING PIN_PA4
#define PI_BOOT PIN_PA5
#define PI_OFF PIN_PA6
#else
#define PI_POWER 2
#define POWER_ON 3
#define PI_RUNNING 4
#define PI_BOOT 5
#define PI_OFF 6
#endif

#define AISerial Serial

AIBusHandler ai_handler(&AISerial, -1);

uint8_t key_position = 0, door_position = 0;
bool pi_on = false, shutdown = false;
bool boot = false, run = false;

//Arduino setup.
void setup() {
	pinMode(PI_POWER, OUTPUT); //Hard Pi power.
	pinMode(POWER_ON, OUTPUT); //System power force on.
	pinMode(PI_RUNNING, INPUT); //High if the Pi is running normally.
	pinMode(PI_BOOT, INPUT); //High if the Pi is starting to boot.
	pinMode(PI_OFF, OUTPUT); //Drive low to power Pi off.

	digitalWrite(PI_POWER, LOW);
	digitalWrite(POWER_ON, LOW);
	digitalWrite(PI_OFF, LOW);

	AISerial.begin(AI_BAUD);
}

//Arduino loop.
void loop() {
	AIData ai_msg;
	elapsedMillis ai_timer;

	ai_handler.readAIData(&ai_msg);

	do {
		if(ai_handler.dataAvailable() > 0) {
			if(ai_handler.readAIData(&ai_msg)) {
				if(ai_msg.sender == ID_NAV_COMPUTER)
					continue;
				
				ai_timer = 0;
				
				if(ai_msg.receiver == 0xFF) {
					if(ai_msg.l >= 3 && ai_msg.data[0] == 0xA1 && ai_msg.data[1] == 0x2) { //Key position.
						const uint8_t last_key_position = key_position;
						key_position = ai_msg.data[2]&0xF;
						
						if(key_position != last_key_position) {
							if(key_position != 0) //Key in on position.
								powerOn();
							else if((door_position&0xC) != 0) //Front door open.
								powerOff();
						}
					} else if(ai_msg.l >= 3 && ai_msg.data[0] == 0xA1 && ai_msg.data[1] == 0x43) { //Door position.
						const uint8_t last_door_position = door_position;
						door_position = ai_msg.data[2];
						const uint8_t front_door_position = door_position&0xC;

						if(key_position == 0 && front_door_position != (last_door_position&0xC)) { //Power is off, door state changed.
							if(front_door_position != 0) { //Front door open.
								if(pi_on)
									powerOff();
								else
									powerOn();
							}
						}
					}
				} else if(ai_msg.receiver == ID_NAV_COMPUTER) {
					if(ai_msg.sender != ID_CANSLATOR)
						continue;

					const bool pi_running = digitalRead(PI_RUNNING) == HIGH;

					if(!pi_running) { //Pi should answer this if it is on.
						bool ack = true;

						if(ack) {
							uint8_t ack_data[] = {0x80};
							AIData ack_msg(sizeof(ack_data), ID_NAV_COMPUTER, ai_msg.sender);

							ack_msg.refreshAIData(ack_data);
							ai_handler.writeAIData(&ack_msg, false);
						}
					}
				}
			}
		}
	} while(ai_timer < 5);

	const bool last_boot = boot, last_run = run;

	boot = digitalRead(PI_BOOT) == HIGH;
	run = digitalRead(PI_RUNNING) == HIGH;

	if(last_boot != boot || last_run != run) {
		if((boot || run) && !shutdown)
			powerOn();
		else if(shutdown && !run & !pi_on) { //Pi has shut down.
			digitalWrite(PI_POWER, LOW);
			digitalWrite(PI_OFF, LOW);
			digitalWrite(POWER_ON, LOW);
		}
	}
}

//Turn full power on.
void powerOn() {
	digitalWrite(PI_POWER, HIGH);
	digitalWrite(PI_OFF, HIGH);
	digitalWrite(POWER_ON, HIGH);
	pi_on = true;
	shutdown = false;
}

//Turn full power off.
void powerOff() {
	digitalWrite(PI_OFF, LOW);
	pi_on = false;
	shutdown = true;
}