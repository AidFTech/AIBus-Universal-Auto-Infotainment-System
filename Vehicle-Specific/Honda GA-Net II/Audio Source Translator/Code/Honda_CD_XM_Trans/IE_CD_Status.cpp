#include "IE_CD_Status.h"

uint8_t getAICDStatus(const uint8_t ie_status, const uint8_t ie_rpt_random) {
	uint8_t ai_status = 0;
	switch(ie_status) {
		case 0x3:
			ai_status = AI_CD_STATUS_PLAY;
			break;
		case 0x16:
			ai_status = AI_CD_STATUS_WAIT;
			break;
		case 0x17:
			ai_status = AI_CD_STATUS_READY;
			break;
		case 0x18:
			ai_status = AI_CD_STATUS_LOADING;
			break;
		case 0x10:
			ai_status = AI_CD_STATUS_READING;
			break;
		case 0x1A:
			ai_status = AI_CD_STATUS_EJECT;
			break;
		case 0x1B:
			ai_status = AI_CD_STATUS_TAKEDISC;
			break;
		case 0xF3:
			ai_status = AI_CD_STATUS_NODISC;
			break;
		default:
			ai_status = ie_status&0xF;
			break;
	}

	switch(ie_rpt_random) {
		case 0x0:
			ai_status |= AI_CD_REPEAT_T;
			break;
		case 0x10:
			ai_status |= AI_CD_REPEAT_D;
			break;
		case 0x11:
			ai_status |= AI_CD_RANDOM_D;
			break;
		case 0x21:
			ai_status |= AI_CD_RANDOM_A;
			break;
	}
	
	return ai_status;
}

uint8_t getIECDStatus(const uint8_t ai_status) {
	switch(ai_status&0xF) {
		case AI_CD_STATUS_PLAY:
			return 0x3;
		case AI_CD_STATUS_WAIT:
			return 0x16;
		case AI_CD_STATUS_READY:
			return 0x17;
		case AI_CD_STATUS_LOADING:
			return 0x18;
		case AI_CD_STATUS_READING:
			return 0x10;
		case AI_CD_STATUS_EJECT:
			return 0xA;
		case AI_CD_STATUS_TAKEDISC:
			return 0xB;
		case AI_CD_STATUS_NODISC:
			return 0xF3;
		default:
			return ai_status&0xF;
	}
}

uint8_t getIECDRepeat(const uint8_t ai_status) {
	switch(ai_status&0xF0) {
		case AI_CD_SCAN_A:
			return 0x22;
		case AI_CD_SCAN_D:
			return 0x12;
		case AI_CD_RANDOM_A:
			return 0x21;
		case AI_CD_RANDOM_D:
			return 0x11;
		case AI_CD_REPEAT_T:
			return 0x0;
		case AI_CD_REPEAT_D:
			return 0x10;
		default:
			return 0x20;
	}
}
