#include <stdint.h>

#include <string>

#ifndef parameter_list_h
#define parameter_list_h

using namespace std;

enum bta_side_menu_t : uint8_t {
	SIDE_MENU_PHONE_NC,
	SIDE_MENU_PHONE
};

struct ParameterList {
	uint8_t locale = 0;

	bool bt_mac_set = false; //True if the BT MAC address has been received.
	string pi_name = "";

	bool connection_changed = false; //True if the connection status has changed.

	bool radio_connected = false, mirror_connected = false, screen_connected = false;

	bool audio_selected = false, text_allowed = false;

	bool imid_native_phone = false;
	uint8_t imid_char = 0, imid_lines = 0;

	bta_side_menu_t side_menu; //The current side menu.
};

#endif