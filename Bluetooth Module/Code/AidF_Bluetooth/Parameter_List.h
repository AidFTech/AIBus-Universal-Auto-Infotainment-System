#include <Arduino.h>
#include <stdint.h>

#ifndef parameter_list_h
#define parameter_list_h

struct ParameterList {
	bool radio_connected = false, navi_connected = false;
};

#endif