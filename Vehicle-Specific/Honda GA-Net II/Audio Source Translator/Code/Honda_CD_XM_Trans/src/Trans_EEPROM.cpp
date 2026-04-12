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
void setCDSettings(const bool auto_start, const bool imid_text, const bool split) {
	uint8_t save_byte = 0;

	if(auto_start)
		save_byte |= CD_SETTINGS_AUTOSTART;
	
	if(imid_text)
		save_byte |= CD_SETTINGS_IMID;

	if(split)
		save_byte |= CD_SETTINGS_SPLIT;

	EEPROM.write(CD_SETTINGS_MAIN, save_byte);
}

//Load CD settings from EEPROM.
void getCDSettings(bool* auto_start, bool* imid_text, bool* split) {
	const uint8_t load_byte = EEPROM.read(CD_SETTINGS_MAIN);

	*auto_start = (load_byte&CD_SETTINGS_AUTOSTART) != 0;
	*imid_text = (load_byte&CD_SETTINGS_IMID) != 0;
	*split = (load_byte&CD_SETTINGS_SPLIT) != 0;
}

//Save IMID settings to EEPROM.
void setIMIDSettings(const bool rds, const bool volume, const uint8_t char_count) {
	uint8_t save_byte = 0;

	if(rds)
		save_byte |= IMID_SETTINGS_RDS;

	if(volume)
		save_byte |= IMID_SETTINGS_VOL;

	save_byte |= (char_count&0b11) << IMID_SETTINGS_CHAR;

	EEPROM.write(IMID_SETTINGS_MAIN, save_byte);
}

//Load IMID settings from EEPROM.
void getIMIDSettings(bool* rds, bool* volume, uint8_t* char_count) {
	const uint8_t load_byte = EEPROM.read(IMID_SETTINGS_MAIN);

	*rds = (load_byte&IMID_SETTINGS_RDS) != 0;
	*volume = (load_byte&IMID_SETTINGS_VOL) != 0;
	*char_count = (load_byte >> IMID_SETTINGS_CHAR)&0b11;
}