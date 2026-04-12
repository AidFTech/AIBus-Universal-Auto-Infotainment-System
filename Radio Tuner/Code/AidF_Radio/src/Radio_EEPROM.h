#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

#include "Parameter_List.h"

#ifndef radio_eeprom_h
#define radio_eeprom_h

enum EEPROM_ADDR {
	FM_BAND_ADDR = PRESET_COUNT*3*sizeof(uint16_t),
	FM_INC_ADDR,
	AM_BAND_ADDR,
	CALLSIGN_STD_ADDR,
};

void getEEPROMPresets(ParameterList* parameter_list);
void setEEPROMPresets(ParameterList* parmaeter_list);

#endif
