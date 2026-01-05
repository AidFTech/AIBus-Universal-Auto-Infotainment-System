#include "IEBus_Handler.h"
#include "En_AIBus_Handler.h"

#ifndef en_iebus_handler_h
#define en_iebus_handler_h

struct EnIEBusParams {
	EnAIBusHandler* ai_handler;
	volatile uint8_t* high_byte_register, *low_byte_register;

	int8_t rec_set = -1, rec_clear = -1, count_enable = -1, count_reset = -1;
	int8_t count[4] = {-1, -1, -1, -1};
};

class EnIEBusHandler : public IEBusHandler {
public:
	EnIEBusHandler(const int8_t rx_pin, const int8_t tx_pin);

	void init(EnIEBusParams* ie_params);

	bool cacheAIBus();

	void sendMessageStrict(IE_Message* ie_d, const bool ack_response, const bool checksum);
	void sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum);
	void sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum, const bool wait);
	int readMessage(IE_Message* ie_d, bool ack_response, const uint16_t id);
	int readMessageStrict(IE_Message* ie_d, bool ack_response, const uint16_t id);

	void addID(const uint8_t id);
private:
	EnAIBusHandler* ai_handler;

	volatile uint8_t *high_byte_register, *low_byte_register;

	volatile uint8_t *rec_set_register = NULL, *rec_clear_register = NULL, *count_enable_register = NULL, *count_reset_register = NULL, *count_register = NULL;
	uint8_t rec_set_bitmask, rec_clear_bitmask, count_enable_bitmask, count_reset_bitmask, count_bitmask;

	int count_shift = 0;

	inline int readBitsH(const int length, const bool with_parity, const bool with_ack, bool send_ack);
	inline int readBitsH(const int length, const bool with_parity, const bool with_ack, bool send_ack, const bool direct, bool* direct_value);

	int readMessageH(IE_Message* ie_d, bool ack_response, const uint16_t id);
};

#endif
