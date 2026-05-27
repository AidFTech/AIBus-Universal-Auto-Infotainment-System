#include <stdint.h>
#include <Arduino.h>
#include <elapsedMillis.h>
#include <Vector.h>

#include "IEBus.h"	

#ifndef iebus_handler_h
#define iebus_handler_h

#ifndef __AVR__
#define TCNT2 _SFR_MEM8(0xB2)
#define TCCR2B _SFR_MEM8(0xB1)
#endif

#if defined(TCNT2)
#define TIMER TCNT2
#elif defined(TCNT2L)
#define TIMER TCNT2L
#else
#error Timer Not Defined
#endif

#if defined(TCCR2B)
#define TIMER_SCALER TCCR2B
#else
#error Scale Not Defined
#endif

//Thanks to Greg Nutt for these numbers, adjusted for the 16MHz Arduino Uno.
//Calculation for Uno if TCCR2B = 2: Byte = Time in Microseconds * (Clock Frequency/8), replace 8 with prescale value if TCCR2B is changed.

//Timing:
#define SCALE 8
#define SCALE_3 64

#define IE_FAILSAFE_LENGTH			int(40e-6*F_CPU/SCALE)
#define IE_NORMAL_BIT_0_LENGTH		int(40e-6*F_CPU/SCALE)
#define IE_NORMAL_BIT_1_LENGTH		int(40e-6*F_CPU/SCALE)
#define IE_NORMAL_BIT_A_LENGTH		int(33e-6*F_CPU/SCALE)
#define IE_BIT_1_HOLD_ON_LENGTH		int(20e-6*F_CPU/SCALE)
#define IE_BIT_0_HOLD_ON_LENGTH		int(32e-6*F_CPU/SCALE)
#define IE_BIT_A_HOLD_ON_LENGTH		int(31e-6*F_CPU/SCALE)
#define IE_BIT_COMP_LENGTH			int(26e-6*F_CPU/SCALE)

#define START_LENGTH				int(360e-6*F_CPU/SCALE_3)
#define START_COMP_LENGTH			int(170e-6*F_CPU/SCALE_3)
#define START_END_LENGTH			int(20.5e-6*F_CPU/SCALE)

#define IE_DELAY 					int(800e-6*F_CPU/SCALE_3)
#define REPEAT_DELAY 2
#define MAX_REPEAT 50

class IEBusHandler {
public:
	IEBusHandler(const int8_t rx_pin, const int8_t tx_pin);

	virtual void sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum);
	virtual void sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum, const bool wait);
	virtual int readMessage(IE_Message* ie_d, bool ack_response, const uint16_t id);
	virtual int cacheMessage(const bool ack_response, const uint16_t id);

	virtual void sendAcknowledgement(const uint16_t sender, const uint16_t receiver);

	Vector<IE_Message>* getRX();
	void clearRX();
	
	inline bool getInputOn() {
		return((*rx_portregister&rx_bitmask) != 0);
	}

protected:
	inline void sendBit(const bool data);
	inline void sendAckBit();
	inline void sendStartBit();
	inline void sendBits(const int16_t data, const uint8_t size);
	inline void sendBits(const int16_t data, const uint8_t size, const bool send_parity, const bool ack);

	inline void getBits(Vector<bool>* bits, const int16_t data, const uint8_t size, const bool send_parity, const bool ack);

	inline int8_t readBit();
	inline int readBits(const uint8_t length, const bool with_parity, const bool with_ack, bool send_ack);

	int8_t rx_pin = -1, tx_pin = -1;
	
	uint8_t tx_bitmask, rx_bitmask;
	volatile uint8_t* tx_portregister, *rx_portregister;

	IE_Message rx_cache_array[10];
	Vector<IE_Message> rx_cache;
};

#endif
