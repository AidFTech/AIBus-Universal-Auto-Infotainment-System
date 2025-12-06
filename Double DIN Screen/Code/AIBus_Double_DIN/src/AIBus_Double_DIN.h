#include <elapsedMillis.h>
#include <MCP23S08.h>
#include <MCP4251.h>
#include <SPI.h>
#define TWI_BUFFER_SIZE 128
#define BUFFER_LENGTH 128
#include <Wire.h>

//Must be compiled with an I2C cache size of at least 128.
#include "AIBus.h"
#include "AIBus_Handler.h"
#include "AIBT_Parameters.h"
#include "Button_Handler.h"
#include "Open_Close_Handler.h"

#ifndef aibus_double_din_h
#define aibus_double_din_h
#ifdef MEGACOREX
#define AI_RX PIN_PA7
#define ILL_CS PIN_PB0
#define ILL_ON PIN_PB1
#define FULL_POWER_ON PIN_PB2
#define MCP_RESET PIN_PB5

#define OPEN_CLOSE_CS PIN_PB3
#define BL_ON PIN_PB4

#define VOL_CLK PIN_PF2
#define VOL_UP PIN_PF3

#define NAV_CLK PIN_PE2
#define NAV_UP PIN_PE3

#define NAV_CS PIN_PF4

#else
#define AI_RX 4
#define ILL_CS 5
#define ILL_ON 6
#define FULL_POWER_ON 7
#define MCP_RESET 8

#define OPEN_CLOSE_CS 9
#define BL_ON 10

#define NAV_CLK 27
#define NAV_UP 28

#define VOL_CLK 29
#define VOL_UP 30

#define NAV_CS 31

#endif

#define NAV_MCP_NAV_PUSH 0
#define NAV_MCP_NAV_UP 1
#define NAV_MCP_NAV_DN 2
#define NAV_MCP_NAV_LEFT 3
#define NAV_MCP_NAV_RIGHT 4
#define NAV_MCP_ILL_AIDF 6
#define NAV_MCP_VOL_PUSH 7

#define AISerial Serial

#define VOL_TIMER 15
#define DOOR_TIMER 30000
#define CONTROL_TIMER 7000

class AIBusDoubleDin {
public:
	void setup();
	void loop();
private:
	AIBusHandler ai_handler = AIBusHandler(&AISerial, AI_RX, ID_NAV_SCREEN);
	MCP4251 ill_mcp4251 = MCP4251(ILL_CS, 100000, 0, 100000, 0);
	MCP23S08 nav_mcp = MCP23S08(NAV_CS);
	MCP23S08 open_close_mcp = MCP23S08(OPEN_CLOSE_CS);

	ParameterList parameters;
	OpenCloseHandler open_handler = OpenCloseHandler(&open_close_mcp, &parameters);
	ButtonHandler button_handler = ButtonHandler(&ai_handler, &open_handler, &parameters);

	elapsedMillis all_timer, radio_timer, source_timer;
	bool all_timer_enabled = false, radio_timer_enabled = false, source_timer_enabled = false;

	elapsedMillis* vol_timer;
	volatile int *vol_steps;
	bool vol_turned = false;

	elapsedMillis* nav_timer;
	volatile int *nav_steps;
	bool nav_turned = false;

	elapsedMillis door_timer;
	bool door_timer_enabled = false;

	bool key_on = false; //True if the key has been in the "on" position any time during this power cycle.
	bool audio_on = false; //True if the audio light is on.
	
	void sendButtonsPresent(const uint8_t receiver);
	void setVolume(const uint8_t receiver);
	void setNavigation(const uint8_t receiver);
};

void setup();
void loop();

void incVolume();
void incNavigation();
void receiveI2C(int byte_count);
void handleEDID();

#endif