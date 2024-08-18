#include "IEBus_Handler.h"
#include "En_AIBus_Handler.h"

#ifndef en_iebus_handler_h
#define en_iebus_handler_h

class EnIEBusHandler : public IEBusHandler {
public:
	EnIEBusHandler(const int8_t rx_pin, const int8_t tx_pin, EnAIBusHandler* ai_handler, const int8_t ai_block_pin);
	bool cacheAIBus();

	void sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum);
	void sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum, const bool wait) volatile;
	int readMessage(IE_Message* ie_d, bool ack_response, const uint16_t id) volatile;

	void addID(const uint8_t id);
private:
	EnAIBusHandler* ai_handler;
	int8_t ai_block_pin = -1;
};

#endif
