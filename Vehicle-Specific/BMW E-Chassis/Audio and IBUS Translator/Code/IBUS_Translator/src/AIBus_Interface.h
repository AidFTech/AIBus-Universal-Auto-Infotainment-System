#include <stdint.h>

#include "AIBus_Handler.h"
#include "AIBus.h"

//Ignition status:
enum aibus_ignition_status : uint8_t {
	AIBUS_IGNITION_OFF,
	AIBUS_IGNITION_ACC1,
	AIBUS_IGNITION_ACC2,
	AIBUS_IGNITION_CRANK,
	AIBUS_IGNITION_ON
};

#define AIBUS_IGNITION_STATUS_KEY_IN 0x10

//Door status:
#define AIBUS_DOOR_STATUS_DRIVER_OPEN 0x8
#define AIBUS_DOOR_STATUS_PASSENGER_OPEN 0x4
#define AIBUS_DOOR_STATUS_LEFT_REAR_OPEN 0x2
#define AIBUS_DOOR_STATUS_RIGHT_REAR_OPEN 0x1
#define AIBUS_DOOR_STATUS_HOOD_OPEN 0x20
#define AIBUS_DOOR_STATUS_TRUNK_OPEN 0x10

void writeAIBusIgnitionStatus(AIBusHandler* aibus_handler, const aibus_ignition_status ignition_status);
void writeAIBusDoorStatus(AIBusHandler* aibus_handler, const uint8_t door_status);