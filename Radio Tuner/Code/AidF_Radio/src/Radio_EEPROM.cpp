#include "Radio_EEPROM.h"

//Load presets from EEPROM.
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

	parameter_list->fm_band = (fm_band_setting_t)EEPROM.read(FM_BAND_ADDR);
	parameter_list->fm_inc_setting = (fm_inc_setting_t)EEPROM.read(FM_INC_ADDR);
	parameter_list->am_band = (am_band_setting_t)EEPROM.read(AM_BAND_ADDR);

	parameter_list->rds_callsign_source = (rds_callsign_t)EEPROM.read(CALLSIGN_STD_ADDR);

	if(parameter_list->fm_band < 0 || parameter_list->fm_band > FM_BAND_OIRT)
		parameter_list->fm_band = FM_BAND_ITU;
	if(parameter_list->fm_inc_setting < 0 || parameter_list->fm_inc_setting > FM_INC_200KHZ_EVEN)
		parameter_list->fm_inc_setting = FM_INC_100KHZ;
	if(parameter_list->am_band < 0 || parameter_list->am_band > AM_BAND_AUSTRALIA)
		parameter_list->am_band = AM_BAND_WEST;

	if(parameter_list->rds_callsign_source < 0 || parameter_list->rds_callsign_source == 0xFF)
		parameter_list->rds_callsign_source = RDS_CALLSIGN_PI_US_CANADA;

	switch(parameter_list->fm_band) {
	case FM_BAND_ITU:
		if(parameter_list->fm_inc_setting == FM_INC_200KHZ_ODD) {
			parameter_list->fm_lower_limit = 8710;
			parameter_list->fm_upper_limit = 10790;
		} else {
			parameter_list->fm_lower_limit = 8700;
			parameter_list->fm_upper_limit = 10800;
		}
		break;
	case FM_BAND_JAPAN:
		if(parameter_list->fm_inc_setting == FM_INC_200KHZ_ODD) {
			parameter_list->fm_lower_limit = 7610;
			parameter_list->fm_upper_limit = 9490;
		} else {
			parameter_list->fm_lower_limit = 7600;
			parameter_list->fm_upper_limit = 9500;
		}
		break;
	case FM_BAND_BRAZIL:
		if(parameter_list->fm_inc_setting == FM_INC_200KHZ_ODD) {
			parameter_list->fm_lower_limit = 7610;
			parameter_list->fm_upper_limit = 10790;
		} else {
			parameter_list->fm_lower_limit = 7600;
			parameter_list->fm_upper_limit = 10800;
		}
	case FM_BAND_OIRT:
		if(parameter_list->fm_inc_setting == FM_INC_200KHZ_ODD) {
			parameter_list->fm_lower_limit = 6590;
			parameter_list->fm_upper_limit = 7390;
		} else {
			parameter_list->fm_lower_limit = 6580;
			parameter_list->fm_upper_limit = 7400;
		}
		break;
	default:
		break;
	}

	switch(parameter_list->fm_inc_setting) {
	case FM_INC_50KHZ:
		parameter_list->fm_inc = 5;
		break;
	case FM_INC_100KHZ:
		parameter_list->fm_inc = 10;
		break;
	case FM_INC_200KHZ_ODD:
	case FM_INC_200KHZ_EVEN:
		parameter_list->fm_inc = 20;
		break;
	default:
		break;
	}

	switch(parameter_list->am_band) {
	case AM_BAND_WEST:
		parameter_list->am_lower_limit = 530;
		parameter_list->am_upper_limit = 1700;
		parameter_list->am_inc = 10;
		break;
	case AM_BAND_EAST:
		parameter_list->am_lower_limit = 531;
		parameter_list->am_upper_limit = 1602;
		parameter_list->am_inc = 9;
		break;
	case AM_BAND_AUSTRALIA:
		parameter_list->am_lower_limit = 531;
		parameter_list->am_upper_limit = 1701;
		parameter_list->am_inc = 9;
		break;
	}
}

//Save the radio presets in EEPROM.
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

	const fm_band_setting_t old_fm_band = (fm_band_setting_t)EEPROM.read(FM_BAND_ADDR);
	const fm_inc_setting_t old_fm_inc = (fm_inc_setting_t)EEPROM.read(FM_INC_ADDR);
	const am_band_setting_t old_am_band = (am_band_setting_t)EEPROM.read(AM_BAND_ADDR);
	const rds_callsign_t old_callsign = (rds_callsign_t)EEPROM.read(CALLSIGN_STD_ADDR);

	if(old_fm_band != parameter_list->fm_band)
		EEPROM.write(FM_BAND_ADDR, parameter_list->fm_band);
	if(old_fm_inc != parameter_list->fm_inc_setting)
		EEPROM.write(FM_INC_ADDR, parameter_list->fm_inc_setting);
	if(old_am_band != parameter_list->am_band)
		EEPROM.write(AM_BAND_ADDR, parameter_list->am_band);
	if(old_callsign != parameter_list->rds_callsign_source)
		EEPROM.write(CALLSIGN_STD_ADDR, parameter_list->rds_callsign_source);
}
