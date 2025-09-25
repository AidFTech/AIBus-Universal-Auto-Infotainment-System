#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

#include "Parameter_List.h"

#ifndef radio_eeprom_h
#define radio_eeprom_h

void getEEPROMPresets(ParameterList* parameter_list);
void setEEPROMPresets(ParameterList* parmaeter_list);

#endif
