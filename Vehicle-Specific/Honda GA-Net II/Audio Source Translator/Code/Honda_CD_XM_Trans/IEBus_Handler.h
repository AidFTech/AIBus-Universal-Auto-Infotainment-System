#include <stdint.h>
#include <Arduino.h>
#include <elapsedMillis.h>

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
#define IE_NORMAL_BIT_0_LENGTH		int(33e-6*F_CPU/SCALE)
#define IE_NORMAL_BIT_1_LENGTH		int(33e-6*F_CPU/SCALE)
#define IE_BIT_1_HOLD_ON_LENGTH		int(20e-6*F_CPU/SCALE)
#define IE_BIT_0_HOLD_ON_LENGTH		int(31e-6*F_CPU/SCALE)
#define IE_BIT_COMP_LENGTH			int(28e-6*F_CPU/SCALE)

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
	virtual void sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum, const bool wait) volatile;
	virtual int readMessage(IE_Message* ie_d, bool ack_response, const uint16_t id) volatile;

	virtual void sendAcknowledgement(const uint16_t sender, const uint16_t receiver) volatile;
	
	virtual bool getInputOn();

protected:
	void sendBit(const bool data) volatile;
	void sendAckBit() volatile;
	void sendStartBit() volatile;
	void sendBits(const uint16_t data, const uint8_t size) volatile;
	void sendBits(const uint16_t data, const uint8_t size, const bool send_parity, const bool ack) volatile;

	int8_t readBit() volatile;
	int readBits(const uint8_t length, const bool with_parity, const bool with_ack, bool send_ack) volatile;

	int8_t rx_pin = -1, tx_pin = -1;
	
	uint8_t tx_bitmask, rx_bitmask;
	volatile uint8_t* tx_portregister, *rx_portregister;
};

#endif
