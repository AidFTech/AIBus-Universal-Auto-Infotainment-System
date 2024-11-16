#include <Arduino.h>
#include <stdint.h>
#include <EEPROM.h>

#ifndef honda_trans_eeprom_h
#define honda_trans_eeprom_h

#define TAPE_SETTINGS_MAIN 0
#define CD_SETTINGS_MAIN 1
#define XM_SETTINGS_MAIN 2

#define TAPE_SETTINGS_AUTOSTART _BV(0)
#define TAPE_SETTINGS_FWDSTART _BV(1)

#define CD_SETTINGS_AUTOSTART _BV(0)
#define CD_SETTINGS_IMID _BV(1)

void setTapeSettings(const bool auto_start, const bool fwd_start);
void getTapeSettings(bool* auto_start, bool* fwd_start);

void setCDSettings(const bool auto_start, const bool imid_text);
void getCDSettings(bool* auto_start, bool* fwd_start);

#endif
