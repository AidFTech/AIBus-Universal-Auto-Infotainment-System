#include <stdint.h>

#ifndef iebus_h
#define iebus_h

#define IE_ID_RADIO 0x100
#define IE_ID_CDC 0x106
#define IE_ID_CDC2 0x116
#define IE_ID_TAPE 0x113
#define IE_ID_SIRIUS 0x119
#define IE_ID_IMID 0x111

struct IE_Message {
	uint8_t* data;
	uint16_t sender, receiver, l;
	uint8_t control;
	bool direct;
	
	IE_Message();
	IE_Message(const uint16_t newl, const uint16_t sender, const uint16_t receiver, const uint8_t control, const bool direct);
	IE_Message(const IE_Message &copy);
	~IE_Message();
	
	void refreshIEData(const uint16_t newl) volatile;
	void refreshIEData(const uint16_t newl, const uint16_t sender, const uint16_t receiver, const uint8_t control, const bool direct) volatile;
	void refreshIEData(IE_Message newd) volatile;
	void refreshIEData(uint8_t* data);
	
	uint8_t getChecksum();
	bool checkVaildity();
};

uint8_t getChecksum(IE_Message* ie_b);

#endif
