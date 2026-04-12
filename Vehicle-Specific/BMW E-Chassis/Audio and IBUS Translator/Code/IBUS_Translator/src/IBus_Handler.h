#include <Arduino.h>

#include "AIBus_Handler.h"

typedef AIData IBData;

class IBusHandler : public AIBusHandler {
public:
	IBusHandler(Stream* serial, const int8_t rx_pin, const uint8_t id);

	void writeIBData(IBData* ib_d);
	bool readIBData(IBData* ib_d);
	bool readIBData(IBData* ib_d, const bool cache);
};