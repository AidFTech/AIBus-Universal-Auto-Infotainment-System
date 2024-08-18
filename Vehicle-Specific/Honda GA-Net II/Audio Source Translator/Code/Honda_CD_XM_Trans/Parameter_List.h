#include <stdint.h>
#include <elapsedMillis.h>

#ifndef parameter_list_h
#define parameter_list_h

#define MENU_TAPE 0x13
#define MENU_CD 0x6
#define MENU_XM 0x19
#define MENU_RADIO 0x10

#define SCREEN_REQUEST_TIMER 5000

struct ParameterList {
	bool power_on = true;

	uint8_t hour = 0, minute = 0, date = 0, month = 0;
	uint16_t year = 2011;
	
	uint16_t vehicle_speed = 0;
	bool computer_connected = false, screen_connected = false, radio_connected = false;
	bool imid_connected = false;

	uint8_t external_imid_char = 0, external_imid_lines = 0;
	bool external_imid_tape = false, external_imid_cd = false, external_imid_xm = false;

	bool first_cd = false, first_tape = false, first_xm = false, first_imid = false;
	
	uint8_t active_source = 0x0, active_subsource = 0x0;
	uint8_t active_menu = 0;

	uint16_t screen_w = 800, screen_h = 480;
	elapsedMillis *screen_request_timer;
};

#endif
