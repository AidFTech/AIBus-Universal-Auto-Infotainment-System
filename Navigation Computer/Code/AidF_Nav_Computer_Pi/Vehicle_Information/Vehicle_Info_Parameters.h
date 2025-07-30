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

#define INFO_PARAM_NONE 0
#define INFO_PARAM_BATTERY_VOLTAGE 1
#define INFO_PARAM_OUTSIDE_TEMP 2
#define INFO_PARAM_COOLANT_TEMP 3
#define INFO_PARAM_INST_ECONOMY 4
#define INFO_PARAM_TRIP_AVERAGE_ECONOMY 5
#define INFO_PARAM_TRIP_TIMER 6
#define INFO_PARAM_CRUISE_SPEED 7
#define INFO_PARAM_GEAR 8
#define INFO_PARAM_RANGE 9
#define INFO_PARAM_TRIP_DISTANCE 10
#define INFO_PARAM_REMAINING_TIME 11
#define INFO_PARAM_REMAINING_DIST 12

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
	uint8_t param_index[PARAM_COUNT];
};

void setLightState(AIData* light_msg, InfoParameters* info_parameters);

#endif
