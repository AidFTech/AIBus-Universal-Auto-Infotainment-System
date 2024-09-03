#include <stdint.h>

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

#define WAIT_TIME 500

struct AIData {
	uint8_t* data;
	uint8_t sender, receiver;
	uint16_t l;
	
	AIData();
	AIData(uint16_t newl, const uint8_t sender, const uint8_t receiver);
	AIData(const AIData &copy);
	~AIData();
	
	void refreshAIData(uint16_t newl, const uint8_t sender, const uint8_t receiver);
	void refreshAIData(AIData newd);
	void refreshAIData(uint8_t* data);
	
	uint8_t getChecksum();
	void getBytes(uint8_t* b);
};

bool checkValidity(uint8_t* b, const uint8_t l);
bool checkDestination(AIData* ai_b, uint8_t dest_id);

uint8_t getChecksum(AIData*ai_b);

void bytesToAIData(AIData* ai_b, uint8_t* b);

#endif
