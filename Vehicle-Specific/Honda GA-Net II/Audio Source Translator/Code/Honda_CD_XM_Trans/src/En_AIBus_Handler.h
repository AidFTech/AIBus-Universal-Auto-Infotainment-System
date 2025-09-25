#include <Arduino.h>
#include <stdint.h>
#include <elapsedMillis.h>

#include "AIBus_Handler.h"
#include "IEBus_Handler.h"

#ifndef en_aibus_handler
#define en_aibus_handler

#define IE_CACHE_SIZE 32

class EnAIBusHandler : public AIBusHandler { //Enhanced AIBus handler.
public:
	EnAIBusHandler(Stream* serial, const int8_t rx_pin, const unsigned int id_count);
	~EnAIBusHandler();

	void addID(const uint8_t id);
	bool cacheAllPending();

	void waitForAIBus();

private:
	unsigned int id_count = 0, current_id = 0;

	uint8_t* id_list;
};

#endif
