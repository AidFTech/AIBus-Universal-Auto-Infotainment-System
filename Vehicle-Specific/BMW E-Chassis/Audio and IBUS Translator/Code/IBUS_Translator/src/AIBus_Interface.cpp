#include "AIBus_Interface.h"

//Write an ignition status AIBus message.
void writeAIBusIgnitionStatus(AIBusHandler* aibus_handler, const aibus_ignition_status ignition_status) {
	uint8_t msg[] = {
		0xA1,			//Indicates a broadcast body-control message for general vehicle events (e.g. doors opening, lights turning on/off, etc.)
		0x2,			//Within the above, indicates a key position message.
		ignition_status	//The ignition status as defined in the enumeration.
	};

	AIData ai_msg(sizeof(msg), ID_CANSLATOR, 0xFF, msg);
	aibus_handler->writeAIData(&ai_msg, false); //Broadcast messages to 0xFF do not require acknowledgement.
}

//Write a door status AIBus message.
void writeAIBusDoorStatus(AIBusHandler *aibus_handler, const uint8_t door_status) {
	uint8_t msg[] = {
		0xA1,			//Indicates a broadcast body-control message for general vehicle events (e.g. doors opening, lights turning on/off, etc.)
		0x43,			//Within the above, indicates a door status message.
		door_status		//The door open/closed status as defined in the header file.
	};

	AIData ai_msg(sizeof(msg), ID_CANSLATOR, 0xFF, msg);
	aibus_handler->writeAIData(&ai_msg, false); //Broadcast messages to 0xFF do not require acknowledgement.
}