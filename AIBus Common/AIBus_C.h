#include <stdint.h>
#include <stdlib.h>

#ifndef aibus_h
#define aibus_h

#define AI_BAUD 115200

#define ID_NAV_COMPUTER 0x1
#define ID_NAV_SCREEN 0x7

#define ID_CD 0x4
#define ID_CDC 0x6
#define ID_RADIO 0x10
#define ID_IMID_SCR 0x11
#define ID_TAPE 0x13
#define ID_XM 0x19
#define ID_GPS_ANTENNA 0x3B
#define ID_CANSLATOR 0x57
#define ID_AMPLIFIER 0x6A
#define ID_STEERING_CTRL 0x6F
#define ID_ANDROID_AUTO 0x8E
#define ID_PHONE 0xC8

#define ID_BROADCAST 0xFF

typedef struct {
	uint8_t* data;
	uint8_t sender, receiver;
	uint16_t l;
} AIData;

AIData* createMessage(const uint16_t l, const uint8_t sender, const uint8_t receiver);
void clearMessage(AIData* ai_msg);

void refreshAIData(AIData* ai_msg, uint8_t* data);

uint8_t getChecksum(AIData* ai_msg);
void getBytes(AIData* ai_msg, uint8_t* b);

AIData* createAckMessage(const uint8_t sender, const uint8_t receiver);

#endif
