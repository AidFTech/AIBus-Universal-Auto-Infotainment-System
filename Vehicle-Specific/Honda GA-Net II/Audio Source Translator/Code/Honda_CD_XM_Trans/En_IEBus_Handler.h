#include "IEBus_Handler.h"
#include "En_AIBus_Handler.h"

#ifndef en_iebus_handler_h
#define en_iebus_handler_h

class EnIEBusHandler : public IEBusHandler {
public:
	EnIEBusHandler(const int8_t rx_pin, const int8_t tx_pin, EnAIBusHandler* ai_handler);
	bool cacheAIBus();

	void sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum);
	void sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum, const bool wait) volatile;

	void addID(const uint8_t id);
private:
	EnAIBusHandler* ai_handler;
};

#endif
