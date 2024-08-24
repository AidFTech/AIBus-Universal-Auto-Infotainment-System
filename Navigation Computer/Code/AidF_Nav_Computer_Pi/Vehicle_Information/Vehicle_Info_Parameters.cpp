#include "Vehicle_Info_Parameters.h"

void setLightState(AIData* light_msg, InfoParameters* info_parameters) {
    unsigned int start_byte = 0;
    if(light_msg->l >= 1 && light_msg->data[start_byte] == 0xA1)
        start_byte += 1;
    else if((light_msg->l >= 1 && light_msg->data[start_byte] != 0x11) || light_msg->l < 1)
        return;

    if(light_msg->l + start_byte < 3)
        return;

    info_parameters->light_state_a = light_msg->data[start_byte+1];
    info_parameters->light_state_b = light_msg->data[start_byte+2];
}