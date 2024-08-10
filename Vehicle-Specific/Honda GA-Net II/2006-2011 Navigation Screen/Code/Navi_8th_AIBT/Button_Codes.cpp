#include "Button_Codes.h"

uint8_t getButtonCode(const int index) {
	switch(index) {
	case BUTTON_INDEX_AMFM:
		return 0x36;
	case BUTTON_INDEX_CDXM:
		return 0x38;
	case BUTTON_INDEX_AUDIO:
		return 0x26;
	case BUTTON_INDEX_SCAN:
		return 0x39;
	case BUTTON_INDEX_SKIPUP:
		return 0x25;
	case BUTTON_INDEX_SKIPDOWN:
		return 0x24;
	case BUTTON_INDEX_PRESET1:
		return 0x11;
	case BUTTON_INDEX_PRESET2:
		return 0x12;
	case BUTTON_INDEX_PRESET3:
		return 0x13;
	case BUTTON_INDEX_PRESET4:
		return 0x14;
	case BUTTON_INDEX_PRESET5:
		return 0x15;
	case BUTTON_INDEX_PRESET6:
		return 0x16;
	case BUTTON_INDEX_CANCEL:
		return 0x27;
	case BUTTON_INDEX_SETUP:
		return 0x51;
	case BUTTON_INDEX_MENU:
		return 0x20;
	case BUTTON_INDEX_MAP:
		return 0x55;
	case BUTTON_INDEX_INFO:
		return 0x53;
	case BUTTON_INDEX_JOGUP:
		return 0x28;
	case BUTTON_INDEX_JOGDOWN:
		return 0x29;
	case BUTTON_INDEX_JOGLEFT:
		return 0x2A;
	case BUTTON_INDEX_JOGRIGHT:
		return 0x2B;
	case BUTTON_INDEX_JOGENTER:
		return 0x7;
	default:
		return 0;
	}
}