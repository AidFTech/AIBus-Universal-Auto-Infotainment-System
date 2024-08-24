#include <stdint.h>

#include "../AIBus/AIBus.h"

#ifndef vehicle_info_parameters_h
#define vehicle_info_parameters_h

#define INFO_LIGHTS_A_RAPID_FLASH 0x80
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

struct InfoParameters {
    uint8_t light_state_a = 0, light_state_b = 0; 
};

void setLightState(AIData* light_msg, InfoParameters* info_parameters);

#endif