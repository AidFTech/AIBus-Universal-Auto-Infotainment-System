#include "Nav_Parameters.h"

//Handle a navigation message.
void handleNavMessage(AIData* ai_msg, NavParameters* nav_parameters) {
	if(ai_msg->l < 14)
		return;

	//Latitude:
	const int16_t lat_degrees = (ai_msg->data[2] << 8) | ai_msg->data[3];
	const uint8_t lat_minutes = ai_msg->data[4]&0x7F;
	const uint8_t lat_seconds = ai_msg->data[5], lat_second_frac = ai_msg->data[6];
	const bool lat_negative = (ai_msg->data[4]&0x80) != 0;

	//Longitude:
	const int16_t long_degrees = (ai_msg->data[7] << 8) | ai_msg->data[8];
	const uint8_t long_minutes = ai_msg->data[9]&0x7F;
	const uint8_t long_seconds = ai_msg->data[10], long_second_frac = ai_msg->data[11];
	const bool long_negative = ((ai_msg->data[9]&0x80) != 0);

	nav_parameters->altitude = (ai_msg->data[12] << 8) | ai_msg->data[13];

	//Calculate latitude double:
	double latitude = lat_degrees;
	const int8_t lat_sign = lat_degrees >= 0 && !lat_negative ? 1 : -1;
	latitude += (lat_minutes/60.0)*lat_sign;
	latitude += (lat_seconds/3600.0)*lat_sign;
	latitude += (lat_second_frac/(3600.0*255))*lat_sign;

	//Calculate longitude double:
	double longitude = long_degrees;
	const int8_t long_sign = long_degrees >= 0 && !long_negative ? 1 : -1;
	longitude += (long_minutes/60.0)*long_sign;
	longitude += (long_seconds/3600.0)*long_sign;
	longitude += (long_second_frac/(3600.0*255))*long_sign;

	nav_parameters->longitude = longitude;
	nav_parameters->latitude = latitude;
}
