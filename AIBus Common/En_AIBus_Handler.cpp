#include "En_AIBus_Handler.h"

EnAIBusHandler::EnAIBusHandler(Stream* serial, const int8_t rx_pin, const unsigned int id_count, const unsigned int ai_cache_size) : AIBusHandler(serial, rx_pin, ai_cache_size) {
	this->id_list = new uint8_t[id_count];

	this->id_vec.setStorage(id_list, id_count, 0);
}

EnAIBusHandler::~EnAIBusHandler() {
	delete[] this->id_list;
}

//Add an ID to the AIBus list.
void EnAIBusHandler::addID(const uint8_t id) {
	if(id_vec.full())
		return;
	
	for(int i=0;i<id_vec.size();i+=1) {
		if(id_vec[i] == id)
			return;
	}

	id_vec.push_back(id);
}

//Cache pending messages for all IDs and the provided ID.
bool EnAIBusHandler::cachePending(const uint8_t cache_id) {
	if(ai_serial->available() > 0) {
		AIData ai_msg;
		if(readAIData(&ai_msg, false)) {
			bool id = false;

			if(ai_msg.receiver == 0xFF || ai_msg.receiver == cache_id)
				id = true;
			else {
				for(unsigned int i=0;i<id_vec.size();i+=1) {
					if((ai_msg.receiver == id_vec[i]) && id_vec[i] != 0x0) {
						id = true;
						break;
					}
				}
			}
			
			if(ai_msg.sender == cache_id)
				id = false;

			for(unsigned int i=0;i<id_vec.size();i+=1) {
				if((ai_msg.sender == id_vec[i]) && id_vec[i] != 0x0) {
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
					else if(cached_vec.size() < cached_vec.max_size()) {
						cached_vec.push_back(ai_msg);
					}
				}
				return true;
			}
		}
	}
	return false;
}

//Cache all pending AIBus messages for all IDs.
bool EnAIBusHandler::cacheAllPending() {
	if(ai_serial->available() > 0) {
		AIData ai_msg;
		if(readAIData(&ai_msg, false)) {
			bool id = false;

			if(ai_msg.receiver == 0xFF)
				id = true;
			else {
				for(unsigned int i=0;i<id_vec.size();i+=1) {
					if((ai_msg.receiver == id_vec[i]) && id_vec[i] != 0x0) {
						id = true;
						break;
					}
				}
			}

			for(unsigned int i=0;i<id_vec.size();i+=1) {
				if((ai_msg.sender == id_vec[i]) && id_vec[i] != 0x0) {
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
						cached_vec.push_back(ai_msg);
					}
				}
			return true;
			}
		}
	}
	return false;
}

//Delay until the AIBus line is clear.
void EnAIBusHandler::waitForAIBus() {
	elapsedMicros ai_timer;
	while(ai_timer < AI_DELAY_U) {
		if(digitalRead(this->rx_pin) == LOW) {
			ai_timer = 0;
		}
	}
}
