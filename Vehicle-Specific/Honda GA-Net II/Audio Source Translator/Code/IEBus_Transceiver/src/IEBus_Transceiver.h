#include <Arduino.h>
#include <stdint.h>

#include <elapsedMillis.h>
#include <Vector.h>

#include "AIBus.h"
#include "IEBus.h"

#ifndef IEBUSTRANSCEIVER_H
#define IEBUSTRANSCEIVER_H

#define IEBUS_ACTIVE PIN_PA2 //High when an AIBus operation is active and serial listening is off.
#define IEBUS_TX_BIT PIN_PA3 //High when a 1 is to be sent.
#define TX_ENABLE PIN_PA4 //High to enable the TX clock.
#define RX_ENABLE PIN_PA5 //High to enable the RX clock.
#define IEBUS_RX PIN_PA6 //Receiving input.
#define IEBUS_TX PIN_PA7 //Sending input.

#define IEBUS_CLK PIN_PC1 //Clock input.
#define IEBUS_START PIN_PC2 //Start bit output.

#define MY_ID 0x100

#define IE_CACHE_SIZE 16

class IEBusTransceiver {
public:
	IEBusTransceiver();

	void loop();

private:
	bool readBit();
	int16_t readBits(int count, bool use_parity, const bool use_ack, bool ack);

	inline bool readMessage(IE_Message* rec);

	void writeBit(const bool bit);
	void writeBits(const int count, const uint16_t data, const bool use_parity, const bool use_ack);
	void writeMessage(IE_Message* ie_data, const bool ack = true);
	void writeMessageChecksum(IE_Message* ie_data);

	IE_Message ie_cache[IE_CACHE_SIZE], ie_ack_cache[IE_CACHE_SIZE];
	Vector<IE_Message> ie_cache_vec, ie_ack_vec;

	elapsedMillis last_ie, last_aibus;
	int avail = 0;

	bool test_mode = false;
};

void setup();
void loop();

#endif