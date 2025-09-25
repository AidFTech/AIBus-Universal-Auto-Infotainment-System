#include <stdint.h>

#ifndef ie_cd_status_h
#define ie_cd_status_h

#define AI_CD_STATUS_PAUSE 0x2
#define AI_CD_STATUS_PLAY 0x3
#define AI_CD_STATUS_WAIT 0x6
#define AI_CD_STATUS_READY 0x7
#define AI_CD_STATUS_LOADING 0x8
#define AI_CD_STATUS_READING 0x9
#define AI_CD_STATUS_EJECT 0xA
#define AI_CD_STATUS_TAKEDISC 0xB
#define AI_CD_STATUS_NODISC 0x0

#define AI_CD_REPEAT_T 0x10
#define AI_CD_REPEAT_D 0x20
#define AI_CD_RANDOM_D 0x40
#define AI_CD_RANDOM_A 0x80
#define AI_CD_SCAN_D 0x60
#define AI_CD_SCAN_A 0xA0

uint8_t getAICDStatus(const uint8_t ie_status, const uint8_t ie_rpt_random);
uint8_t getIECDStatus(const uint8_t ai_status);
uint8_t getIECDRepeat(const uint8_t ai_status);

#endif
