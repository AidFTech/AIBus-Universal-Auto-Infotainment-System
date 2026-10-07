#include <stdint.h>

#include "../AIBus/AIBus.h"

#ifndef vehicle_info_parameters_h
#define vehicle_info_parameters_h

//Exterior light status:
union aibus_ext_light_status {
	struct data {
		bool drl: 1;
		bool low_beam: 1;
		bool high_beam: 1;
		bool parking: 1;
		bool front_fog: 1;
		bool left_turn: 1;
		bool right_turn: 1;
		bool auto_light: 1;

		uint8_t spacer: 2;
		bool bed_light: 1;
		bool off_road: 1;
		bool license_plate: 1;
		bool taillight: 1;
		bool rear_fog: 1;
		bool reverse: 1;
	} data;
	uint8_t raw[sizeof(data)];
	
	aibus_ext_light_status() {
		raw[0] = 0;
		raw[1] = 0;
	}
};

//Time:
#define AIBUS_TIME_12H 0x80
#define AIBUS_TIME_AUTO 0x40

//Outside or coolant temperature:
union aibus_temperature_config {
	struct data {
		uint8_t byte_count: 3;
		bool negative: 1;
		uint8_t decimal_count: 3;
		bool fahrenheit: 1;
	} data;
	uint8_t raw[sizeof(data)];
};

//Speed or distance:
union aibus_speed_distance_config {
	struct data {
		uint8_t byte_count: 4;
		uint8_t decimal_count: 3;
		bool miles: 1;
	} data;
	uint8_t raw[sizeof(data)];
};

//Voltage:
union aibus_voltage_config {
	struct data {
		uint8_t byte_count: 3;
		bool negative: 1;
		uint8_t decimal_count: 4;
	} data;
	uint8_t raw[sizeof(data)];
};

//Fuel economy:
enum aibus_fuel_economy_units : uint8_t {
	AIBUS_FUEL_ECONOMY_L_100KM,
	AIBUS_FUEL_ECONOMY_KM_L,
	AIBUS_FUEL_ECONOMY_MPG_US,
	AIBUS_FUEL_ECONOMY_MPG_IMP,
};

union aibus_fuel_economy_config {
	struct data {
		uint8_t decimal_count: 5;
		bool undefined: 1;
		aibus_fuel_economy_units units: 2;
	} data;
	uint8_t raw[sizeof(data)];
};

//Trip time:
union aibus_trip_time_config {
	struct data {
		uint8_t byte_count: 7;
		bool minutes: 1;
	} data;
	uint8_t raw[sizeof(data)];
};

enum hybrid_system_type_t : uint8_t {
	INFO_HYBRID_TYPE_NONE,
	INFO_HYBRID_TYPE_SERIES = 1,
	INFO_HYBRID_TYPE_PARALLEL,
	INFO_HYBRID_TYPE_SERIES_PARALLEL,
	INFO_HYBRID_TYPE_POWER_SPLIT
};

union hybrid_params_t {
	struct data {
		hybrid_system_type_t system_type: 3;
		bool spacer: 1;
		bool charge_assist: 1;
		uint8_t spacer2: 3;

		bool hybrid_feature_plug: 1;
		bool hybrid_feature_rear_motor: 1;
		bool hybrid_feature_front_motor: 1;
		bool spacer3: 1;
		bool hybrid_feature_e_ac: 1;
		bool hybrid_feature_conv_ac: 1;
		bool hybrid_feature_e_heat: 1;
	} data;
	uint8_t raw[sizeof(data)];
};

union hybrid_mode_t {
	struct data {
		bool bat_to_motor: 1;
		bool motor_to_bat: 1;
		bool regen_to_bat: 1;
		bool motor_to_wheels: 1;
		bool wheels_to_motor: 1;
		bool engine_to_wheels: 1;
		bool engine_to_motor: 1;
		bool plug_to_bat: 1;
	} data;
	uint8_t raw[sizeof(data)];
};

#define PARAM_COUNT 4

enum info_param : uint8_t {
	INFO_PARAM_NONE,
	INFO_PARAM_BATTERY_VOLTAGE,
	INFO_PARAM_OUTSIDE_TEMP,
	INFO_PARAM_COOLANT_TEMP,
	INFO_PARAM_INST_ECONOMY,
	INFO_PARAM_TRIP_AVERAGE_ECONOMY,
	INFO_PARAM_TRIP_TIMER,
	INFO_PARAM_CRUISE_SPEED,
	INFO_PARAM_GEAR,
	INFO_PARAM_RANGE,
	INFO_PARAM_TRIP_DISTANCE,
	INFO_PARAM_REMAINING_TIME,
	INFO_PARAM_REMAINING_DIST,
};

enum econ_unit : uint8_t {
	ECON_L_100KM,
	ECON_KM_L,
	ECON_MPG_US,
	ECON_MPG_IMP,
};

enum transmission_type_t : uint8_t {
	TRANSMISSION_MANUAL,
	TRANSMISSION_AUTOMATIC,
	TRANSMISSION_SEMI_AUTO,
	TRANSMISSION_DCT,
	TRANSMISSION_SMG,
	TRANSMISSION_CVT,
	TRANSMISSION_IVT,
	TRANSMISSION_ECVT,
	TRANSMISSION_EV_DD,
	TRANSMISSION_EV_DD_ENGINE,

	TRANSMISSION_OTHER = 0xFF,
};
#define TRANSMISSION_POS_PARK 0x4
#define TRANSMISSION_POS_REVERSE 0x2
#define TRANSMISSION_POS_NEUTRAL 0x0
#define TRANSMISSION_POS_DRIVE 0x1
#define TRANSMISSION_POS_LOW 0x8
#define TRANSMISSION_POS_MANUAL 0x9

enum auto_stop_t : uint8_t {
	AUTO_STOP_OFF,
	AUTO_STOP_DISABLED,
	AUTO_STOP_ON = 0xFF,
};

struct InfoParameters {
	//Supported common parameters.
	uint8_t supported_a = 0xF, supported_b = 0xFF;
		
	//Lights:
	aibus_ext_light_status light_state;

	//Battery voltage:
	uint16_t battery_voltage = 0;
	uint8_t battery_voltage_precision = 0;

	//Temperatures:
	int16_t outside_temp = 250, coolant_temp = 250;
	bool outside_temp_sent = false, coolant_temp_sent = false;

	bool outside_temp_fahrenheit = false, coolant_temp_fahrenheit = false;

	//Trip info:
	uint16_t range = 0;
	bool range_miles = false;
	
	float inst_mpg = -1, avg_mpg = -1;
	econ_unit inst_units = ECON_L_100KM, avg_units = ECON_L_100KM;

	uint32_t trip_distance = 0; //x10
	bool distance_miles = false;

	uint16_t trip_time = 0;
	bool trip_time_minutes = false;

	//Cruise control.
	int16_t cruise_speed = -1; //x10
	bool cruise_mph = false;

	//Transmission.
	transmission_type_t transmission_type = TRANSMISSION_OTHER;
	int8_t selected_pos = -1, gear = -1;
	auto_stop_t auto_stop = AUTO_STOP_OFF;

	//Display:
	bool display_cruise = true;

	//Hybrid:
	bool hybrid_system_present = false; //True if a hybrid system exists.
	bool draw_charge_assist = true; //True if we should draw the charge/assist meter.
	
	hybrid_params_t hybrid_params; //The hybrid system type and features present.
	hybrid_mode_t hybrid_status_main; //The hybrid system status.

	uint8_t hybrid_battery_state = 0; //The hybrid battery state of charge.
	uint8_t charge_assist_pos = 0x7F; //The position of the charge/assist meter, centered at 0x7F.

	//Displayed parameters:
	info_param param_index[PARAM_COUNT] = {INFO_PARAM_NONE, INFO_PARAM_NONE, INFO_PARAM_NONE, INFO_PARAM_NONE};

	InfoParameters() {
		hybrid_params.raw[0] = 0;
		hybrid_params.raw[1] = 0;
		hybrid_status_main.raw[0] = 0;

		light_state.raw[0] = 0;
		light_state.raw[1] = 0;
	}
};

void setLightState(AIData* light_msg, InfoParameters* info_parameters);

#endif
