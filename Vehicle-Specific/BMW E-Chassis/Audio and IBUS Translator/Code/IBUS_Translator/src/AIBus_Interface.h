#include <stdint.h>

#include "AIBus_Handler.h"
#include "AIBus.h"

//Ignition status:
enum aibus_ignition_status : uint8_t {
	AIBUS_IGNITION_OFF,
	AIBUS_IGNITION_ACC1,
	AIBUS_IGNITION_ACC2,
	AIBUS_IGNITION_CRANK,
	AIBUS_IGNITION_ON
};

#define AIBUS_IGNITION_STATUS_KEY_IN 0x10

//Illumination status:
#define AIBUS_ILL_STATUS_ILL_ON 0x1 //Interior backlighting on.
#define AIBUS_ILL_STATUS_NIGHT 0x80 //Screen in night mode.

//Exterior light status:
union aibus_ext_light_status {
	struct data {
		bool drl: 1;
		bool low_beam: 1;
		bool high_beam: 1;
		bool halo: 1;
		bool front_fog: 1;
		bool left_turn: 1;
		bool right_turn: 1;
		bool auto_light: 1;

		uint8_t spacer: 4;
		bool license_plate: 1;
		bool taillight: 1;
		bool rear_fog: 1;
		bool reverse: 1;
	} data;
	uint8_t raw[sizeof(data)];
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
		bool spacer: 1;
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

//Door status:
#define AIBUS_DOOR_STATUS_DRIVER_OPEN 0x8
#define AIBUS_DOOR_STATUS_PASSENGER_OPEN 0x4
#define AIBUS_DOOR_STATUS_LEFT_REAR_OPEN 0x2
#define AIBUS_DOOR_STATUS_RIGHT_REAR_OPEN 0x1
#define AIBUS_DOOR_STATUS_HOOD_OPEN 0x20
#define AIBUS_DOOR_STATUS_TRUNK_OPEN 0x10

void writeAIBusIgnitionStatus(AIBusHandler* aibus_handler, const aibus_ignition_status ignition_status);
void writeAIBusIlluminationStatus(AIBusHandler* aibus_handler, const uint8_t brightness, const bool ill_on, const bool night_mode);
void writeAIBusExtLightStatus(AIBusHandler* aibus_handler, const aibus_ext_light_status status);
void writeAIBusDoorStatus(AIBusHandler* aibus_handler, const uint8_t door_status);

void writeAIBusTime(AIBusHandler* aibus_handler, const uint8_t hour, const uint8_t minute, const uint8_t second, const bool use_12h, const bool auto_clock);
void writeAIBusOutsideTemp(AIBusHandler* aibus_handler, double temp, const bool fahrenheit, const int decimal_count = 0);
void writeAIBusCoolantTemp(AIBusHandler* aibus_handler, double temp, const bool fahrenheit, const int decimal_count = 0);
