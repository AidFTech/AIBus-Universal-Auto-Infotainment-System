#include <stdint.h>
#include <Arduino.h>
#include <mcp2515.h>
#include <MCP4251.h>
#include <elapsedMillis.h>
#include <Vector.h>

#include "AIBus_Handler.h"
#include "AIBus.h"
#include "Parameter_List.h"
#include "CAN_Menu_Handler.h"
#include "Locale.h"
#include "CANslator_EEPROM.h"
#include "Aux_Light_Control.h"

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
#define BCAN_ID_HYBRID_SYSTEM 0x92F96050
#define BCAN_ID_TEMP_RANGE 0x92F96250
#define BCAN_ID_AC_AUTOSTOP 0x92F86150
#define BCAN_ID_COOLANT 0x92F85250
#define BCAN_ID_AVG_ECONOMY 0x92F96B50

#define BCAN_ID_LIGHTSTALKPOS 0x8AF87010
#define BCAN_ID_WIPERSTALKPOS 0x8AF87110
#define BCAN_ID_LIGHTSENSOR 0x8EF87372
#define BCAN_ID_RAINSENSOR 0x8AF87274

#define BCAN_ID_IMIDMSG1 0x92F96350

#define BCAN_ID_NAV_DATA_LEN 0x92F95B55
#define BCAN_ID_NAV_CURRENT_STREET 0x92F95E55
#define BCAN_ID_NAV_NEXT_TURN 0x92F95C55

#define HEADLIGHT_TEMP_BUFFER 20 //Buffer for headlight temp hysteresis.

#define REC_DEVICE_COUNT 16 //The number of devices that can query for status messages.
#define CAN_TIMER 5000 //The amount of time to wait for new CAN messages before powering off.

enum aidf_nav_special_t : uint8_t {
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

	void setAuxLightController(AuxLightController* aux_light_controller);
	void setIntLightController(MCP4251* int_light_controller);

	bool getLeftSignalOn();
	bool getRightSignalOn();

	bool getInteriorLightsOn();
	uint8_t getBrightness();

	void setWiperTimer(elapsedMillis* wiper_timer);
	bool getWiperIntActive();
	bool runWiper();
	
	void sendBatteryVoltage(const uint16_t voltage);
	void sendCommonParameters();
	void sendInfoParameters();

	void setNavNextTurn(const uint8_t entry_angle, const uint8_t exit_angle, const uint16_t roads_visible, const uint8_t step_num, const uint8_t special, String street_name);
private:
	MCP2515 bcan_2515, imid_2515, rls_2515, fcan_2515;

	AIBusHandler* ai_handler;
	ParameterList* parameter_list;

	AuxLightController* aux_light_controller = nullptr;
	MCP4251* int_light_controller = nullptr;

	CANMenuHandler menu_handler;

	uint8_t rec_device_list[REC_DEVICE_COUNT];
	Vector<uint8_t> rec_device_vec;
	
	//BCAN-derived variables:
	bool auto_stop :1, econ_mode :1, e_brake :1, brightness_bar :1, lights_on:1, night_mode :1;
	bool left_signal_on :1, right_signal_on :1, hazard_on :1, brake_light_on :1, left_signal_illum : 1, right_signal_illum: 1, high_beam_full: 1;
	uint8_t coolant_temp, eco_bar, brightness, vehicle_speed, wiper_pos, wiper_delay_pos;
	uint16_t electric_ac_power, eco_leaf_meter, honda_gear;
	int16_t outside_temp; //Last digit is tenths place.
	uint32_t odo_km;

	bool economy_mpg = false; //True if fuel economy is in MPG.
	int32_t current_economy = -1, last_economy = -1;

	uint8_t honda_temp = 0x28; //Temp byte sent by the cluster.
	bool honda_fahrenheit = false; //True if the temp byte above is in Fahrenheit.

	uint8_t light_state_a = 0, light_state_b = 0;
	bool ext_drl_on = false; //True if an external DRL is in use.

	elapsedMillis last_can_msg; //Duration since the last CAN message was sent.

	//Range:
	uint16_t range = 0;
	bool range_miles = false;

	//Hybrid-specific:
	uint8_t hybrid_status = 0x0, charge_assist = 0x7F, hybrid_battery = 0x0;
	bool hybrid_init = false; //True if the hybrid handshake has been sent.

	void broadcastBCAN(can_frame* can_msg);

	//Queries:
	bool light_sensor_connected = false; //True if a light sensor has been detected.
	bool rain_sensor_connected = false; //True if a rain sensor has been detected.
	bool headlight_on_temp = false; //True if the headlights have come on due to temperature.
	int16_t headlight_temp_limit = 100; //The temperature at which headlights need to come on.
	elapsedMillis* wiper_timer = nullptr; //External wiper timer.

	//Combined AIBus:
	void writeAIBusKeyMessage();
	void writeAIBusDoorMessage();
	void writeAIBusBrightnessMessage();

	//AIBus:
	void writeAIBusKeyMessage(const uint8_t receiver);
	void writeAIBusDoorMessage(const uint8_t receiver);
	void writeAIBusBrightnessMessage(const uint8_t receiver);
	void writeAIBusTempMessage(const uint8_t receiver);
	void writeAIBusLightMessage(const uint8_t receiver);
	void writeAIBusSignalMessage();
	void writeAIBusSpeedMessage(const uint8_t receiver);
	
	void writeAIBusCoolantTempMessage(const uint8_t receiver);
	void writeAIBusRangeMessage();
	void writeAIBusAverageEconomyMessage();

	void writeAIBusHybridHandshake();
	void writeAIBusHybridStatusMessage();

	//CAN message handling:
	void forwardIMIDMessage();
	void forwardRLSMessage();

	//Misc:
	void calculateHeadlightTemperature();
	void setDRLs(const bool drl);

	//Menus:
	void handleSelection(const uint8_t selection);
};

uint8_t getHondaNavChecksum(can_frame* can_msg);

#endif
