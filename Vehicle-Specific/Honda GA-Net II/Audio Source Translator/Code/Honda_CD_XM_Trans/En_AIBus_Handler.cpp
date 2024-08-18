#include "En_AIBus_Handler.h"

EnAIBusHandler::EnAIBusHandler(Stream* serial, const int8_t rx_pin, const unsigned int id_count) : AIBusHandler(serial, rx_pin) {
	this->id_count = id_count;
	this->id_list = new uint8_t[id_count];

	for(unsigned int i=0;i<id_count;i+=1)
		id_list[i] = 0;
}

EnAIBusHandler::~EnAIBusHandler() {
	delete[] this->id_list;
}

void EnAIBusHandler::addID(const uint8_t id) {
	if(current_id >= id_count)
		return;
	
	for(int i=0;i<current_id;i+=1) {
		if(id_list[i] == id)
			return;
	}

	id_list[current_id] = id;
	current_id += 1;
}

bool EnAIBusHandler::cacheAllPending() {
	if(ai_serial->available() > 0) {
		AIData ai_msg;
		if(readAIData(&ai_msg, false)) {
			bool id = false;

			if(ai_msg.receiver == 0xFF)
				id = true;
			else {
				for(unsigned int i=0;i<id_count;i+=1) {
					if((ai_msg.receiver == id_list[i]) && id_list[i] != 0x0) {
						id = true;
						break;
					}
				}
			}

			for(unsigned int i=0;i<id_count;i+=1) {
				if((ai_msg.sender == id_list[i]) && id_list[i] != 0x0) {
					id = false;
					break;
				}
			}

			if(id) {
				if(ai_msg.l >= 1 && ai_msg.data[0] != 0x80) {
					if(ai_msg.receiver != 0xFF)
						sendAcknowledgement(ai_msg.receiver, ai_msg.sender);
					
					if(cached_msg.l <= 0)
						cached_msg.refreshAIData(ai_msg);
					else {
						uint8_t data[ai_msg.l + 4];
						ai_msg.getBytes(data);
						for(int i=0;i<ai_msg.l + 4;i+=1)
						cached_vec.push_back(data[i]);
					}
				}
			return true;
			}
		}
	}
	return false;
}
