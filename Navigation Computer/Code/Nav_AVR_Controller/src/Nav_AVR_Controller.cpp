#include "Nav_AVR_Controller.h"

NavAVRController nav_avr_controller;

//Arduino setup.
void setup() {
	nav_avr_controller.setup();
}

//Arduino loop.
void loop() {
	nav_avr_controller.loop();
}

//Main object setup.
void NavAVRController::setup() {
	pinMode(PI_POWER, OUTPUT); //Hard Pi power.
	pinMode(POWER_ON, OUTPUT); //System power force on.
	pinMode(PI_RUNNING, INPUT); //High if the Pi is running normally.
	pinMode(PI_BOOT, INPUT); //High if the Pi is starting to boot.
	pinMode(PI_OFF_HARDWARE, OUTPUT); //Drive low to power Pi off.
	pinMode(PI_OFF_SOFT, OUTPUT); //Drive high to power Pi off when it is on.

	pinMode(AI_RX, INPUT);

	digitalWrite(PI_POWER, LOW);
	digitalWrite(POWER_ON, LOW);
	digitalWrite(PI_OFF_HARDWARE, LOW);
	digitalWrite(PI_OFF_SOFT, LOW);

	AISerial.begin(AI_BAUD);

	uint8_t init_data[] = {0x4A, 0x1F};
	AIData init_msg(sizeof(init_data), ID_COMPUTER_PROXY, ID_CANSLATOR, init_data);

	digitalWrite(POWER_ON, HIGH);
	ai_handler.writeAIData(&init_msg, false);
	digitalWrite(POWER_ON, LOW);
}

//Main object loop.
void NavAVRController::loop() {
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
							if(key_position != 0) { //Key in on position.
								powerOn();
								key_on = true;
								door_timer_enabled = false;
							} else {
								if((door_position&0xC) != 0) //Front door open.
									powerOff();
								else {
									door_timer_enabled = true;
									door_timer = 0;
								}
							}
						}
					} else if(ai_msg.l >= 3 && ai_msg.data[0] == 0xA1 && ai_msg.data[1] == 0x43) { //Door position.
						const uint8_t last_door_position = door_position;
						door_position = ai_msg.data[2];
						const uint8_t front_door_position = door_position&0xC;

						if(key_position == 0 && front_door_position != (last_door_position&0xC)) { //Power is off, door state changed.
							if(front_door_position != 0) { //Front door open.
								if(key_on) {
									if(pi_on)
										powerOff();
									else {
										digitalWrite(PI_POWER, LOW);
										digitalWrite(PI_OFF_HARDWARE, LOW);
										digitalWrite(POWER_ON, LOW);
									}
								} else {
									powerOn();
									door_timer_enabled = true;
									door_timer = 0;
								}
							}
						}
					}
				} else if(ai_msg.receiver == ID_NAV_COMPUTER) {
					if(ai_msg.sender != ID_CANSLATOR)
						continue;

					const bool pi_running = digitalRead(PI_RUNNING) == HIGH;

					if(!pi_running) { //Pi should answer this if it is on.
						/*bool ack = true;

						if(ack)
							ai_handler.sendAcknowledgement(ID_NAV_COMPUTER, ai_msg.sender);*/
					}

					if(ai_msg.l >= 3 && ai_msg.data[0] == 0x2 && ai_msg.data[1] == 0x0 && ai_msg.data[2] == 0x0) {
						digitalWrite(PI_OFF_HARDWARE, LOW);
						powerOff();
					}
				}
			}
		}
	} while(ai_timer < 50);

	const bool last_boot = boot, last_run = run;

	boot = digitalRead(PI_BOOT) == HIGH;
	run = digitalRead(PI_RUNNING) == HIGH;

	if(last_boot != boot || last_run != run || use_shutdown_timer) {
		if((boot || run) && !shutdown)
			powerOn();
		else if(shutdown && shutdown_timer >= 10000 && !boot && !run) { //Pi has shut down.
			use_shutdown_timer = false;
			digitalWrite(PI_POWER, LOW);
			digitalWrite(PI_OFF_HARDWARE, LOW);
			digitalWrite(POWER_ON, LOW);
		}
	}

	if(door_timer_enabled && door_timer > DOOR_TIMER) {
		door_timer_enabled = false;
		if(key_position == 0) {
			if(pi_on)
				powerOff();
			else {
				digitalWrite(PI_POWER, LOW);
				digitalWrite(PI_OFF_HARDWARE, LOW);
				digitalWrite(POWER_ON, LOW);
			}
		}
	}
}

//Turn full power on.
void NavAVRController::powerOn() {
	digitalWrite(PI_POWER, HIGH);
	digitalWrite(PI_OFF_HARDWARE, HIGH);
	digitalWrite(POWER_ON, HIGH);
	pi_on = true;
	shutdown = false;
}

//Turn full power off.
void NavAVRController::powerOff() {
	shutdown_timer = 0;
	use_shutdown_timer = true;
	shutdown = true;

	//GPIO3 power cycle.
	if(pi_on) {
		digitalWrite(PI_OFF_SOFT, HIGH);
		digitalWrite(PI_OFF_SOFT, LOW);
	}
	pi_on = false;
}
