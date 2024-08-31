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

struct InfoParameters {
	uint8_t light_state_a = 0, light_state_b = 0; 
	bool hybrid_system_present = false; //True if a hybrid system exists.
	
	uint8_t hybrid_system_type = 0; //The type of hybrid system.
	bool charge_assist_meter = false; //True if a charge/assist meter is provided.
	uint8_t hybrid_features = 0; //The hybrid features present.
	uint8_t hybrid_status_main = 0; //The hybrid system status.
};

void setLightState(AIData* light_msg, InfoParameters* info_parameters);

#endif
