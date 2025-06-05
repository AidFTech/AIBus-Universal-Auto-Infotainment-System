#include "Button_Codes.h"

uint8_t getButtonCode(const int index) {
	switch(index) {
	case BT_EJECT:
		return 0;
	case BT_SOURCE:
		return 0x23;
	case BT_AUDIO:
		return 0x26;
	case BT_FMAM:
		return 0x36;
	case BT_AUX:
		return 0x38;
	case BT_SKIPUP:
		return 0x25;
	case BT_SKIPDN:
		return 0x24;
	case BT_INFO:
		return 0x53;
	case BT_TONE:
		return 0x52;
	case BT_HOME:
		return 0x20;
	case BT_SETUP:
		return 0x51;
	case BT_BACK:
		return 0x27;
	case BT_F1:
		return 0x11;
	case BT_F2:
		return 0x12;
	case BT_F3:
		return 0x13;
	case BT_F4:
		return 0x14;
	case BT_F5:
		return 0x15;
	case BT_F6:
		return 0x16;
	default:
		return 0;
	}
}