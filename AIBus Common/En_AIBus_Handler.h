#include <Arduino.h>
#include <stdint.h>
#include <Vector.h>
#include <elapsedMillis.h>

#include "AIBus_Handler.h"

#ifndef en_aibus_handler
#define en_aibus_handler

#define EN_AI_CACHE_SIZE AI_CACHE_SIZE*4
#define ACK_CACHE_SIZE 32

//An enhanced AIBus handler capable of caching messages from multiple IDs.
class EnAIBusHandler : public AIBusHandler { //Enhanced AIBus handler.
public:
	EnAIBusHandler(Stream* serial, const int8_t rx_pin, const unsigned int id_count, const unsigned int ai_cache_size = EN_AI_CACHE_SIZE);
	~EnAIBusHandler();

	void addID(const uint8_t id);

	void setCacheAck(const bool cache_ack);

	bool getValidMessage(AIData* ai_d);

	bool cachePending(const uint8_t id);
	bool cacheAllPending();

	bool getPending(uint8_t* ids, const int id_l, AIData* msg);

	void waitForAIBus();

private:
	Vector<uint8_t> id_vec;
	uint8_t* id_list;

	bool cache_ack = false;

	bool getID(const uint8_t id);
};

#endif
