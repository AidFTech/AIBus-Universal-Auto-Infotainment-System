#include <Arduino.h>
#include <stdint.h>
#include <elapsedMillis.h>

#include "AIBus.h"
#include "AIBus_Handler.h"
#include "Parameter_List.h"
#include "Locale.h"

#ifndef can_menu_handler_h
#define can_menu_handler_h

enum active_menu_t : uint8_t {
	ACTIVE_MENU_NONE,
	ACTIVE_MENU_SETTINGS_MAIN,
	ACTIVE_MENU_SETTINGS_COMFORT_CONVENIENCE,
	ACTIVE_MENU_SETTINGS_HEADLIGHT_INTEGRATION
};

class CANMenuHandler {
public:
	CANMenuHandler(AIBusHandler* ai_handler, ParameterList* parameter_list);

	active_menu_t getActiveMenu();
	void setActiveMenu(const active_menu_t active_menu);

	void createMainSettingsMenu();
	void createComfortConvenienceMenu(const bool light_sensor_connected, const bool rain_sensor_connected);
	void createHeadlightTempereatureMenu();
private:
	AIBusHandler* ai_handler;
	ParameterList* parameter_list;

	active_menu_t active_menu = ACTIVE_MENU_NONE;

	bool startMenu(const uint8_t count, const uint8_t rows, const bool loop, String title);
	void appendMenu(const uint8_t position, String text);
	void displayMenu(const uint8_t selected);
	bool clearMenu();
};

#endif