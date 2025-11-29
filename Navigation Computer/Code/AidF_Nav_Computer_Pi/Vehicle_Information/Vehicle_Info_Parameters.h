#include <stdint.h>

#include "../AIBus/AIBus.h"

#ifndef vehicle_info_parameters_h
#define vehicle_info_parameters_h

#define INFO_LIGHTS_A_AUTO 0x80
#define INFO_LIGHTS_A_RIGHT_SIGNAL 0x40
#define INFO_LIGHTS_A_LEFT_SIGNAL 0x20
#define INFO_LIGHTS_A_FRONT_FOG 0x10
#define INFO_LIGHTS_A_PARKING 0x8
#define INFO_LIGHTS_A_HIGH_BEAM 0x4
#define INFO_LIGHTS_A_LOW_BEAM 0x2
#define INFO_LIGHTS_A_DRL 0x1

#define INFO_LIGHTS_B_REVERSE 0x80
#define INFO_LIGHTS_B_REAR_FOG 0x40
#define INFO_LIGHTS_B_TAIL 0x20
#define INFO_LIGHTS_B_LICENSE 0x10
#define INFO_LIGHTS_B_OFFROAD 0x8
#define INFO_LIGHTS_B_BED 0x4

#define INFO_HYBRID_TYPE_SERIES 1
#define INFO_HYBRID_TYPE_PARALLEL 2
#define INFO_HYBRID_TYPE_SERIES_PARALLEL 3
#define INFO_HYBRID_TYPE_POWER_SPLIT 4

#define INFO_HYBRID_FEATURE_CHARGE_ASSIST 0x10
#define INFO_HYBRID_FEATURE_PLUG 0x1
#define INFO_HYBRID_FEATURE_REAR_MOTOR 0x2
#define INFO_HYBRID_FEATURE_FRONT_MOTOR 0x40
#define INFO_HYBRID_FEATURE_E_AC 0x10
#define INFO_HYBRID_FEATURE_CONV_AC 0x20
#define INFO_HYBRID_FEATURE_E_HEAT 0x40

#define INFO_HYBRID_MODE_BAT_TO_MOTOR 0x1
#define INFO_HYBRID_MODE_MOTOR_TO_BAT 0x2
#define INFO_HYBRID_MODE_REGEN_TO_BAT 0x4
#define INFO_HYBRID_MODE_MOTOR_TO_WHEELS 0x8
#define INFO_HYBRID_MODE_WHEELS_TO_MOTOR 0x10
#define INFO_HYBRID_MODE_ENGINE_TO_WHEELS 0x20
#define INFO_HYBRID_MODE_ENGINE_TO_MOTOR 0x40
#define INFO_HYBRID_MODE_PLUG_TO_BAT 0x80

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

struct InfoParameters {
	//Supported common parameters.
	uint8_t supported_a = 0xF, supported_b = 0xFF;
		
	//Lights:
	uint8_t light_state_a = 0, light_state_b = 0; 

	//Battery voltage:
	uint16_t battery_voltage = 0;

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

	//Display:
	bool display_cruise = true;

	//Hybrid:
	bool hybrid_system_present = false; //True if a hybrid system exists.
	
	uint8_t hybrid_system_type = 0; //The type of hybrid system.
	bool charge_assist_meter = false; //True if a charge/assist meter is provided.
	uint8_t hybrid_features = 0; //The hybrid features present.
	uint8_t hybrid_status_main = 0; //The hybrid system status.

	uint8_t hybrid_battery_state = 0; //The hybrid battery state of charge.
	uint8_t charge_assist_pos = 0x7F; //The position of the charge/assist meter, centered at 0x7F.

	//Displayed parameters:
	info_param param_index[PARAM_COUNT];
};

void setLightState(AIData* light_msg, InfoParameters* info_parameters);

#endif
