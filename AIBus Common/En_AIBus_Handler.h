#include <Arduino.h>
#include <stdint.h>
#include <Vector.h>
#include <elapsedMillis.h>

#include "AIBus_Handler.h"

#ifndef en_aibus_handler
#define en_aibus_handler

#define EN_AI_CACHE_SIZE AI_CACHE_SIZE*4

//An enhanced AIBus handler capable of caching messages from multiple IDs.
class EnAIBusHandler : public AIBusHandler { //Enhanced AIBus handler.
public:
	EnAIBusHandler(Stream* serial, const int8_t rx_pin, const unsigned int id_count, const unsigned int ai_cache_size = EN_AI_CACHE_SIZE);
	~EnAIBusHandler();

	void addID(const uint8_t id);

	bool cachePending(const uint8_t id);
	bool cacheAllPending();

	void waitForAIBus();

private:
	Vector<uint8_t> id_vec;
	uint8_t* id_list;

	bool getID(const uint8_t id);
};

#endif
