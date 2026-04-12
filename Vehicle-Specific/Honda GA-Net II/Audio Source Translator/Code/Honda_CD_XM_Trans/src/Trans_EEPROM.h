#include <Arduino.h>
#include <stdint.h>
#include <EEPROM.h>

#ifndef honda_trans_eeprom_h
#define honda_trans_eeprom_h

enum eeprom_settings_index_t : uint8_t {
	TAPE_SETTINGS_MAIN,
	CD_SETTINGS_MAIN,
	XM_SETTINGS_MAIN,
	IMID_SETTINGS_MAIN
};

#define TAPE_SETTINGS_AUTOSTART _BV(0)
#define TAPE_SETTINGS_FWDSTART _BV(1)

#define CD_SETTINGS_AUTOSTART _BV(0)
#define CD_SETTINGS_IMID _BV(1)
#define CD_SETTINGS_SPLIT _BV(2)

#define IMID_SETTINGS_RDS _BV(0)
#define IMID_SETTINGS_VOL _BV(1)
#define IMID_SETTINGS_CHAR 2

void setTapeSettings(const bool auto_start, const bool fwd_start);
void getTapeSettings(bool* auto_start, bool* fwd_start);

void setCDSettings(const bool auto_start, const bool imid_text, const bool split);
void getCDSettings(bool* auto_start, bool* fwd_start, bool* split);

void setIMIDSettings(const bool rds, const bool volume, const uint8_t char_count);
void getIMIDSettings(bool* rds, bool* volume, uint8_t* char_count);

#endif
