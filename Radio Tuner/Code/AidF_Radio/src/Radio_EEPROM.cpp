#include "Radio_EEPROM.h"

void getEEPROMPresets(ParameterList* parameter_list) {
	for(int i=0;i<PRESET_COUNT;i+=1) {
		if(2*i+1+4*PRESET_COUNT >= EEPROM.length())
			break;

		const uint8_t fm1_high = EEPROM.read(2*i), fm1_low = EEPROM.read(2*i + 1);
		const uint8_t fm2_high = EEPROM.read(2*i + 2*PRESET_COUNT), fm2_low = EEPROM.read(2*i + 1 + 2*PRESET_COUNT);
		const uint8_t am_high = EEPROM.read(2*i + 4*PRESET_COUNT), am_low = EEPROM.read(2*i + 1 + 4*PRESET_COUNT);

		parameter_list->fm1_presets[i] = (fm1_high << 8) | fm1_low;
		parameter_list->fm2_presets[i] = (fm2_high << 8) | fm2_low;
		parameter_list->am_presets[i] = (am_high << 8) | am_low;
	}
}

void setEEPROMPresets(ParameterList* parameter_list) {
	for(int i=0;i<PRESET_COUNT;i+=1) {
		if(2*i+1+4*PRESET_COUNT >= EEPROM.length())
			break;

		const uint8_t old_fm1_high = EEPROM.read(2*i), old_fm1_low = EEPROM.read(2*i + 1);
		const uint8_t old_fm2_high = EEPROM.read(2*i + 2*PRESET_COUNT), old_fm2_low = EEPROM.read(2*i + 1 + 2*PRESET_COUNT);
		const uint8_t old_am_high = EEPROM.read(2*i + 4*PRESET_COUNT), old_am_low = EEPROM.read(2*i + 1 + 4*PRESET_COUNT);

		const uint16_t old_fm1 = (old_fm1_high << 8) | old_fm1_low;
		const uint16_t old_fm2 = (old_fm2_high << 8) | old_fm2_low;
		const uint16_t old_am = (old_am_high << 8) | old_am_low;

		if(old_fm1 != parameter_list->fm1_presets[i]) {
			const uint8_t new_fm1_low = parameter_list->fm1_presets[i]&0xFF, new_fm1_high = parameter_list->fm1_presets[i]>>8;
			EEPROM.write(2*i, new_fm1_high);
			EEPROM.write(2*i + 1, new_fm1_low);
		}

		if(old_fm2 != parameter_list->fm2_presets[i]) {
			const uint8_t new_fm2_low = parameter_list->fm2_presets[i]&0xFF, new_fm2_high = parameter_list->fm2_presets[i]>>8;
			EEPROM.write(2*i + 2*PRESET_COUNT, new_fm2_high);
			EEPROM.write(2*i + 1 + 2*PRESET_COUNT, new_fm2_low);
		}

		if(old_am != parameter_list->am_presets[i]) {
			const uint8_t new_am_low = parameter_list->am_presets[i]&0xFF, new_am_high = parameter_list->am_presets[i]>>8;
			EEPROM.write(2*i + 4*PRESET_COUNT, new_am_high);
			EEPROM.write(2*i + 1 + 4*PRESET_COUNT, new_am_low);
		}
	}
}