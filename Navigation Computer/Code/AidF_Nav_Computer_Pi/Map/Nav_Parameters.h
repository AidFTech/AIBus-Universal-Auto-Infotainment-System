#include <stdint.h>

#include <string>

#include "../AIBus/AIBus.h"

using namespace std;

#ifndef nav_parameters_h
#define nav_parameters_h

//Parameter list specific for navigational information.
struct NavParameters {
	double latitude = 0, longitude = 0;
	int16_t altitude = 0;

	uint8_t zoom = 14;

	bool update_map = true; //True if the map should be updated on the next draw cycle.

	string map_path = "";
};

void handleNavMessage(AIData* ai_msg, NavParameters* nav_parameters);

#endif