#include <stdint.h>
#include <Arduino.h>
#include <mcp2515.h>
#include <elapsedMillis.h>

#include "AIBus_Handler.h"
#include "AIBus.h"
#include "Parameter_List.h"
#include "CAN_Menu_Handler.h"
#include "Locale.h"

#ifndef can_handler_h
#define can_handler_h

#define BCAN_ID_KEYPOS 0x92F81010
#define BCAN_ID_GEAR 0x92F85150
#define BCAN_ID_LEFTDOORS 0x92F83010
#define BCAN_ID_RIGHTDOORS 0x92F84010
#define BCAN_ID_TRUNK 0x92F84310
#define BCAN_ID_BRIGHTNESS 0x92F85450
#define BCAN_ID_LIGHTS 0x8AF81110
#define BCAN_ID_SPEED 0x92F85050
#define BCAN_ID_TEMP_RANGE 0x92F96250
#define BCAN_ID_AC_AUTOSTOP 0x92F86150
#define BCAN_ID_COOLANT 0x92F85250

#define BCAN_ID_WIPERSTALKPOS 0x0AF87110
#define BCAN_ID_LIGHTSENSOR 0x8EF87372
#define BCAN_ID_RAINSENSOR 0x8AF87274

#define BCAN_ID_IMIDMSG1 0x92F96350

#define BCAN_ID_NAV_DATA_LEN 0x92F95B55
#define BCAN_ID_NAV_CURRENT_STREET 0x92F95E55
#define BCAN_ID_NAV_NEXT_TURN 0x92F95C55

#define HEADLIGHT_TEMP_BUFFER 20 //Buffer for headlight temp hysteresis.

enum AidFNavSpecial : uint8_t {
	AIDF_NAV_SPECIAL_NORMAL,
	AIDF_NAV_SPECIAL_DESTINATION,
	AIDF_NAV_SPECIAL_TOLL,
	AIDF_NAV_SPECIAL_FERRY,
	AIDF_NAV_SPECIAL_TRAIN,
	AIDF_NAV_SPECIAL_TRAFFIC_CIRCLE,
	AIDF_NAV_SPECIAL_TRAFFIC_CIRCLE_EXIT,
	AIDF_NAV_SPECIAL_TRAFFIC_CIRCLE_ENTER_EXIT,
	AIDF_NAV_SPECIAL_UTURN_LEFT,
	AIDF_NAV_SPECIAL_UTURN_RIGHT,
	AIDF_NAV_SPECIAL_MERGE_LEFT,
	AIDF_NAV_SPECIAL_MERGE_RIGHT,
	AIDF_NAV_SPECIAL_WAYPOINT
};

class BCAN_Handler {
public:
	BCAN_Handler(AIBusHandler* ai_handler, ParameterList* parameter_list, const uint8_t b_cs_pin, const uint8_t imid_cs_pin, const uint8_t rls_cs_pin, const uint8_t f_cs_pin);
	void init();
	void readCANMessage();

	bool handleAIBus(AIData* ai_msg);

	void setWiperTimer(elapsedMillis* wiper_timer);
	bool getWiperIntActive();
	bool runWiper();
	
	void sendAllParameters();

	void setNavNextTurn(const uint8_t entry_angle, const uint8_t exit_angle, const uint16_t roads_visible, const uint8_t step_num, const uint8_t special, String street_name);
private:
	MCP2515 bcan_2515, imid_2515, rls_2515, fcan_2515;

	AIBusHandler* ai_handler;
	ParameterList* parameter_list;

	CANMenuHandler menu_handler;
	
	//BCAN-derived variables:
	bool auto_stop :1, econ_mode :1, e_brake :1, brightness_bar :1, lights_on:1, night_mode :1;
	uint8_t hybrid_battery_level, coolant_temp, eco_bar, doors_open, brightness, vehicle_speed, wiper_pos, wiper_delay_pos;
	uint16_t electric_ac_power, eco_leaf_meter, gear;
	int16_t outside_temp; //Last digit is tenths place.
	uint32_t odo_km;

	uint8_t honda_temp = 0x28; //Temp byte sent by the cluster.
	bool honda_fahrenheit = false; //True if the temp byte above is in Fahrenheit.

	uint8_t key_pos = 0;

	uint8_t light_state_a = 0, light_state_b = 0;

	void broadcastBCAN(can_frame* can_msg);

	//Queries:
	bool light_sensor_connected = false; //True if a light sensor has been detected.
	bool rain_sensor_connected = false; //True if a rain sensor has been detected.
	bool headlight_on_temp = false; //True if the headlights have come on due to temperature.
	int16_t headlight_temp_limit = 100; //The temperature at which headlights need to come on.
	elapsedMillis* wiper_timer = nullptr; //External wiper timer.

	//AIBus:
	void writeAIBusKeyMessage(const uint8_t receiver);
	void writeAIBusDoorMessage(const uint8_t receiver);
	void writeAIBusBrightnessMessage(const uint8_t receiver);
	void writeAIBusTempMessage(const uint8_t receiver);
	void writeAIBusLightMessage(const uint8_t receiver);
	void writeAIBusSpeedMessage(const uint8_t receiver);
	
	void writeAIBusCoolantTempMessage(const uint8_t receiver);

	//CAN message handling:
	void forwardIMIDMessage();
	void forwardRLSMessage();

	//Misc:
	void calculateHeadlightTemperature();

	//Menus:
	void handleSelection(const uint8_t selection);
};

uint8_t getHondaNavChecksum(can_frame* can_msg);

#endif
