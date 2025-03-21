#include <Arduino.h>
#include <stdint.h>

#ifndef parameter_list_h
#define parameter_list_h

struct ParameterList {
	bool radio_connected = false, navi_connected = false;

	uint8_t key_position = 0, door_position = 0;
	bool power_on;

	uint8_t imid_char = 0, imid_lines = 0;
	bool imid_bta = false, imid_connected = false;
};

#endif