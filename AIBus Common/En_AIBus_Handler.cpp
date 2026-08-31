#include "En_AIBus_Handler.h"

EnAIBusHandler::EnAIBusHandler(Stream* serial, const int8_t rx_pin, const unsigned int id_count, const unsigned int ai_cache_size) :
EnAIBusHandler(serial, rx_pin, id_count, ai_cache_size, AI_CACHE_SIZE)
{

}

EnAIBusHandler::EnAIBusHandler(Stream* serial, const int8_t rx_pin, const unsigned int id_count, const unsigned int ai_cache_size, const unsigned int tx_cache_size) :
	AIBusHandler(serial, rx_pin, 0, ai_cache_size, tx_cache_size) {
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

//Set whether acknowledgment messages should be cached, e.g. if "pinging" another device.
void EnAIBusHandler::setCacheAck(const bool cache_ack) {
	this->cache_ack = cache_ack;
}

//Cache pending messages for all IDs and the provided ID.
bool EnAIBusHandler::cachePending(const uint8_t cache_id) {
	if(cached_vec.size() >= cached_vec.max_size())
		return false;

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
			else {
				for(unsigned int i=0;i<id_vec.size();i+=1) {
					if((ai_msg.sender == id_vec[i]) && id_vec[i] != 0x0) {
						id = false;
						break;
					}
				}
			}

			if(id) {
				if(cached_vec.size() < cached_vec.max_size() && ((ai_msg.l >= 1 && ai_msg.data[0] != 0x80) || cache_ack)) {
					cached_vec.push_back(ai_msg);
				}
				return true;
			}
		}
	}
	return false;
}

//Cache all pending AIBus messages for all IDs.
bool EnAIBusHandler::cacheAllPending() {
	if(cached_vec.size() >= cached_vec.max_size())
		return false;

	if(ai_serial->available() >= 2) {
		AIData ai_msg;
		const aibus_read_result_t ai_err = readAIDataErr(&ai_msg, false);
		if(ai_err == AIBUS_READ_OK_SERIAL || ai_err == AIBUS_READ_OK_MULTIPLE) {
			bool id = false;

			if(ai_msg.receiver == 0xFF || getID(ai_msg.receiver))
				id = true;

			if(getID(ai_msg.sender))
				id = false;

			if(id) {
				if((ai_msg.l >= 1 && ai_msg.data[0] != 0x80) || cache_ack) {			
					if(cached_vec.size() < cached_vec.max_size()) {
						cached_vec.push_back(ai_msg);
					}
				}
				
				return true;
			}
		} else if(ai_err != AIBUS_READ_NODATA) {
			delay(5);
		}
	}
	return false;
}

//Return any incoming messages whose receiver ID is on the list, cache otherwise.
bool EnAIBusHandler::getPending(uint8_t* ids, const int id_l, AIData* msg) {
	if(ai_serial->available() >= 2) {
		AIData ai_msg;
		const aibus_read_result_t ai_err = readAIDataErr(&ai_msg, false);
		if(ai_err == AIBUS_READ_OK_SERIAL || ai_err == AIBUS_READ_OK_MULTIPLE) {
			for(int i=0;i<id_l;i+=1) {
				if(ids[i] == ai_msg.receiver) {
					msg->refreshAIData(ai_msg);
					return true;
				}
			}

			if(cached_vec.size() >= cached_vec.max_size())
				return false;

			bool id = false;

			if(ai_msg.receiver == 0xFF || getID(ai_msg.receiver))
				id = true;

			if(getID(ai_msg.sender))
				id = false;

			if(id) {
				if((ai_msg.l >= 1 && ai_msg.data[0] != 0x80) || cache_ack) {			
					if(cached_vec.size() < cached_vec.max_size()) {
						cached_vec.push_back(ai_msg);
					}
				}
			}
		} else if(ai_err != AIBUS_READ_NODATA) {
			delay(5);
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

//Return whether a message is valid for this device.
bool EnAIBusHandler::getValidMessage(AIData* ai_d) {
	return ai_d->receiver == 0xFF || getID(ai_d->receiver);
}

//Get whether the specified ID is valid.
bool EnAIBusHandler::getID(const uint8_t id) {
	for(int i=0;i<this->id_vec.size();i+=1) {
		if(id_vec[i] == id)
			return true;
	}
	return false;
}
