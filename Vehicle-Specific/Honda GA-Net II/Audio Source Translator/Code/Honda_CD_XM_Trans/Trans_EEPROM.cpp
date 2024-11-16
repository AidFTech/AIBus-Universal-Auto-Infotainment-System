#include "Trans_EEPROM.h"

//Save tape settings to EEPROM.
void setTapeSettings(const bool auto_start, const bool fwd_start) {
	uint8_t save_byte = 0;

	if(auto_start)
		save_byte |= TAPE_SETTINGS_AUTOSTART;

	if(fwd_start)
		save_byte |= TAPE_SETTINGS_FWDSTART;

	EEPROM.write(TAPE_SETTINGS_MAIN, save_byte);
}

//Load tape settings from EEPROM.
void getTapeSettings(bool* auto_start, bool* fwd_start) {
	const uint8_t load_byte = EEPROM.read(TAPE_SETTINGS_MAIN);

	*auto_start = (load_byte&TAPE_SETTINGS_AUTOSTART) != 0;
	*fwd_start = (load_byte&TAPE_SETTINGS_FWDSTART) != 0;
}

//Save CD settings to EEPROM.
void setCDSettings(const bool auto_start, const bool imid_text) {
	uint8_t save_byte = 0;

	if(auto_start)
		save_byte |= CD_SETTINGS_AUTOSTART;
	
	if(imid_text)
		save_byte |= CD_SETTINGS_IMID;

	EEPROM.write(CD_SETTINGS_MAIN, save_byte);
}

//Load CD settings from EEPROM.
void getCDSettings(bool* auto_start, bool* imid_text) {
	const uint8_t load_byte = EEPROM.read(CD_SETTINGS_MAIN);

	*auto_start = (load_byte&CD_SETTINGS_AUTOSTART) != 0;
	*imid_text = (load_byte&CD_SETTINGS_IMID) != 0;
}