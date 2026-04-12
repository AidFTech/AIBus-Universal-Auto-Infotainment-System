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
	#ifdef PI_CM
	pinMode(PI_BOOT, INPUT); //High if the Pi is starting to boot.
	pinMode(PI_OFF_HARDWARE, OUTPUT); //Drive low to power Pi off.
	pinMode(PI_OFF_SOFT, OUTPUT); //Drive high to power Pi off when it is on.
	pinMode(PROG, INPUT);
	#endif
	pinMode( UART_DTR, INPUT);

	pinMode(AI_RX, INPUT_PULLUP);

	digitalWrite(PI_POWER, LOW);
	digitalWrite(POWER_ON, LOW);
	#ifdef PI_CM
	digitalWrite(PI_OFF_HARDWARE, HIGH);
	digitalWrite(PI_OFF_SOFT, LOW);
	#endif

	AISerial.begin(AI_BAUD);
	delay(1000);

	uint8_t init_data[] = {0x4A, 0x1F};
	AIData init_msg(sizeof(init_data), ID_COMPUTER_PROXY, ID_CANSLATOR, init_data);

	digitalWrite(POWER_ON, HIGH);
	ai_handler.writeAIData(&init_msg, false);
	ai_handler.flush();
	digitalWrite(POWER_ON, LOW);
}

//Main object loop.
void NavAVRController::loop() {
	AIData ai_msg;
	elapsedMillis ai_timer;

	do {
		if(ai_handler.dataAvailable() > 0) {
			if(ai_handler.readAIData(&ai_msg)) {	
				ai_timer = 0;

				if(ai_msg.sender == ID_NAV_COMPUTER && !getPoweroffMessage(&ai_msg) && !getInitMessage(&ai_msg)) {
					pi_boot_timer = PI_BOOT_TIMER + 1;
					computer_connected = true;
				}
				
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
								if(key_on || (door_position&0x80) != 0) {
									if(pi_on)
										powerOff();
									else {
										digitalWrite(PI_POWER, LOW);
										#ifdef PI_CM
										digitalWrite(PI_OFF_HARDWARE, LOW);
										#endif
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
				} else if(ai_msg.receiver == ID_COMPUTER_PROXY) {
					if(ai_msg.sender != ID_CANSLATOR)
						continue;

					if(ai_msg.l >= 3 && ai_msg.data[0] == 0x2 && ai_msg.data[1] == 0x0 && ai_msg.data[2] == 0x0) {
						#ifdef PI_CM
						digitalWrite(PI_OFF_HARDWARE, LOW);
						#endif
						powerOff();
					}
				}
			}
		}
	} while(ai_timer < 50);

	const bool last_boot = boot, last_run = run;

	#ifdef PI_CM
	boot = digitalRead(PI_BOOT) == HIGH;
	run = digitalRead(PI_RUNNING) == HIGH;
	#else
	run = pi_on;//digitalRead(PI_RUNNING) == HIGH;
	boot = run;
	#endif

	if(last_boot != boot || last_run != run || use_shutdown_timer) {
		if((boot || run) && !shutdown)
			powerOn();
		#ifdef PI_CM
		else if(shutdown && shutdown_timer >= 1000 && !boot && !run) { //Pi has shut down.
		#else
		else if(shutdown && shutdown_timer >= 5000 && !boot && !run) { //Pi has shut down.
		#endif
			use_shutdown_timer = false;
			digitalWrite(PI_POWER, LOW);
			#ifdef PI_CM
			digitalWrite(PI_OFF_HARDWARE, LOW);
			#endif
			digitalWrite(POWER_ON, LOW);
		}

		if(!boot && !run)
			pi_on = false;
	}

	if(door_timer_enabled && door_timer > DOOR_TIMER) {
		door_timer_enabled = false;
		if(key_position == 0) {
			if(pi_on)
				powerOff();
			else {
				digitalWrite(PI_POWER, LOW);
				#ifdef PI_CM
				digitalWrite(PI_OFF_HARDWARE, LOW);
				#endif
				digitalWrite(POWER_ON, LOW);
			}
		}
	}

	if(boot_timer_enabled && pi_boot_timer > PI_BOOT_TIMER) {
		boot_timer_enabled = false;
		digitalWrite(PI_POWER, HIGH);
	}

	if(pi_ping_timer > PI_PING_TIMER && pi_on && key_position != 0) {
		pi_ping_timer = 0;

		uint8_t ping_data[] = {0x1};
		AIData ping_msg(sizeof(ping_data), ID_COMPUTER_PROXY, ID_NAV_COMPUTER, ping_data);
		const bool ack = ai_handler.writeAIData(&ping_msg, computer_connected);

		if(ack&&computer_connected)
			pi_boot_timer = PI_BOOT_TIMER + 1;
		else
			computer_connected = false;
	}

	if((!boot && !run) && pi_boot_timer > 30000 && pi_on && key_position != 0) { //Pi crashed? Power cycle it.
		uint8_t poweroff_data[] = {0xA0};
		AIData poweroff_msg(sizeof(poweroff_data), ID_NAV_COMPUTER, 0xFF, poweroff_data);
		ai_handler.writeAIData(&poweroff_msg, false);

		digitalWrite(PI_POWER, LOW);
		delay(100);
		powerOn();
	}
}

//Turn full power on.
void NavAVRController::powerOn() {
	#ifdef PI_CM
	digitalWrite(PI_OFF_HARDWARE, HIGH);
	#endif
	digitalWrite(POWER_ON, HIGH);
	boot_timer_enabled = true;
	pi_boot_timer = 0;
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
		#ifdef PI_CM
		digitalWrite(PI_OFF_SOFT, HIGH);
		delay(100);
		digitalWrite(PI_OFF_SOFT, LOW);
		#else
		pi_on = false;
		#endif
	}
}
