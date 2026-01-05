#include "AIBus_Handler.h"

AIBusHandler::AIBusHandler(Stream* serial, const int8_t rx_pin, const uint8_t id, const unsigned int ai_cache_size) {
	this->ai_serial = serial;
	this->rx_pin = rx_pin;

	if(this->rx_pin >= 0)
		pinMode(this->rx_pin, INPUT_PULLUP);

	this->id = id;

	cached_byte = new AIData[ai_cache_size];

	cached_vec.setStorage(cached_byte, ai_cache_size, 0);
	this->ai_serial->setTimeout(1);
}

AIBusHandler::~AIBusHandler() {
	delete[] cached_byte;
}

//The amount of AIBus data available.
int AIBusHandler::dataAvailable() {
	return dataAvailable(true);
}

//The amount of AIBus data available.
int AIBusHandler::dataAvailable(const bool cache) {
	if(!cache || cached_vec.size() <= 0)
		return ai_serial->available();
	else
		return cached_vec[0].l + 4;
}

//Read AIBus data from the main stream and cache.
bool AIBusHandler::readAIData(AIData* ai_d) {
	return readAIData(ai_d, true);
}

//Read AIBus data from the main stream.
bool AIBusHandler::readAIData(AIData* ai_d, const bool cache, const bool multiple) {
	const aibus_read_result_t result = readAIDataErr(ai_d, cache, multiple);
	return result == AIBUS_READ_OK_SERIAL || result == AIBUS_READ_OK_CACHED || result == AIBUS_READ_OK_MULTIPLE;
}

//Read AIBus data from the main stream and cache.
aibus_read_result_t AIBusHandler::readAIDataErr(AIData* ai_d) {
	return readAIDataErr(ai_d, true);
}

//Read AIBus data from the main stream.
aibus_read_result_t AIBusHandler::readAIDataErr(AIData* ai_d, const bool cache, const bool multiple) {
	if(cache && cached_vec.size() > 0) {
		ai_d->refreshAIData(cached_vec[0]);

		if(cached_vec.size() <= 1)
			cached_vec.clear();
		else
			cached_vec.remove(0);
		return AIBUS_READ_OK_CACHED;
	}

	if(ai_serial->available() < 2)
		return ai_serial->available() > 0 ? AIBUS_READ_INCOMPLETE : AIBUS_READ_NODATA;

	int avail = ai_serial->available();
	while(micros() >= UINT32_MAX - AI_DELAY_U*2);
	elapsedMicros wait_micros = 0;

	while(wait_micros < AI_DELAY_U) {
		if(ai_serial->available() != avail) {
			avail = ai_serial->available();
			wait_micros = 0;
		}
	}

	if(ai_serial->available() >= 2) {
		uint8_t init_stream[2];
		ai_serial->readBytes(init_stream, 2);
		const uint8_t s = init_stream[0], l = init_stream[1];
		
		elapsedMillis ai_timer=0;

		while(ai_serial->available() < l) {
			while(millis() >= UINT32_MAX - AI_WAIT*2);

			if(this->rx_pin >= 0) {
				int8_t rx = digitalRead(this->rx_pin);
				if(rx == LOW)
					ai_timer = 0;
			}

			if(ai_timer > AI_WAIT) {
				elapsedMicros clear_timer;

				int avail = ai_serial->available();
				while(clear_timer < AI_DELAY_U) {
					if(this->rx_pin >= 0) {
						int8_t rx = digitalRead(this->rx_pin);
						if(rx == LOW)
							clear_timer = 0;
					} else if(ai_serial->available() > avail) {
						avail = ai_serial->available();
						clear_timer = 0;
					}
				}

				uint8_t db[ai_serial->available()];
				ai_serial->readBytes(db, ai_serial->available());
				return AIBUS_READ_TIMEOUT;
			}
		}

		if(ai_serial->available() < l)
			return AIBUS_READ_TIMEOUT;

		ai_d->refreshAIData(0,0,0);

		uint8_t r_stream[1];
		ai_serial->readBytes(r_stream, 1);
		const uint8_t r = r_stream[0];
		
		uint8_t d[l-1];
		ai_serial->readBytes(d, l-1);

		{
			uint8_t chex[l+2];
			chex[0] = s;
			chex[1] = l;
			chex[2] = r;
			for(uint8_t i=0;i<l-1;i+=1)
				chex[i+3] = d[i];

			if(!checkValidity(chex,l+2)) {
				elapsedMicros clear_timer;

				int avail = ai_serial->available();
				while(clear_timer < AI_DELAY_U) {
					if(this->rx_pin >= 0) {
						int8_t rx = digitalRead(this->rx_pin);
						if(rx == LOW)
							clear_timer = 0;
					} else if(ai_serial->available() > avail) {
						avail = ai_serial->available();
						clear_timer = 0;
					}
				}

				uint8_t db[ai_serial->available()];
				ai_serial->readBytes(db, ai_serial->available());
				return AIBUS_READ_INVALID_CHECKSUM;
			}
		}

		ai_d->refreshAIData(l-2, s, r);

		for(uint8_t i=0;i<ai_d->l;i+=1)
			ai_d->data[i] = d[i];

		if(multiple && ai_d->l >= 3 && ai_d->data[0] == 0x91 && (getID(ai_d->receiver) || ai_d->receiver == 0xFF)) { //Multiple messages are on the way.
			if(getID(ai_d->receiver))
				sendAcknowledgement(ai_d->receiver, ai_d->sender);

			if(ai_d->data[2] != 0)
				return AIBUS_READ_INVALID_MULTIPLE;

			const int msg_count = ai_d->data[1];
			uint8_t full_data[msg_count*AIDATA_LIMIT];
			int full_length = ai_d->l - 3;

			if(msg_count <= 0)
				return AIBUS_READ_OK_SERIAL;

			for(int i=0;i<ai_d->l - 3;i+=1)
				full_data[i] = ai_d->data[i+3];
			int m = 1;
			elapsedMillis ai_timer;

			AIData test_msg;
			while(m < msg_count && ai_timer < 200) {
				if(readAIData(&test_msg, true, false)) {
					if(test_msg.l < 3 || test_msg.receiver != ai_d->receiver || test_msg.sender != ai_d->sender || test_msg.data[0] != 0x91 || test_msg.data[1] != msg_count) {
						if((test_msg.receiver == ai_d->receiver || test_msg.receiver == 0xFF) && test_msg.l > 0 && test_msg.data[0] != 0x80)
							cacheMessage(&test_msg); 
						continue;
					}

					if(getID(test_msg.receiver))
						sendAcknowledgement(test_msg.receiver, test_msg.sender);

					for(int i=0;i<test_msg.l-3;i+=1)
						full_data[(AIDATA_LIMIT)*m + i] = test_msg[i+3];

					full_length += test_msg.l - 3;

					m += 1;
					ai_timer = 0;
				}
				if(ai_timer > 200)
					return AIBUS_READ_INVALID_MULTIPLE;
			}

			ai_d->refreshAIData(full_length, ai_d->sender, ai_d->receiver);
			ai_d->refreshAIData(full_data);
			return AIBUS_READ_OK_MULTIPLE;
		}

		return AIBUS_READ_OK_SERIAL;
	} else {
		return AIBUS_READ_NODATA;
	}
}

//Return whether the specified ID is valid for this device.
bool AIBusHandler::getID(const uint8_t id) {
	return this->id == id;
}

//Read AIBus data from a byte array.
bool AIBusHandler::readAIData(AIData* ai_d, uint8_t* data, const uint8_t d_l) {
	if(d_l < 2)
		return false;
	
	const uint8_t l=data[1]-2;
	if(!checkValidity(data, l+4))
		return false;
	
	ai_d->refreshAIData(l, data[0], data[2]);
	
	for(uint8_t i=0;i<ai_d->l;i+=1)
		ai_d->data[i] = data[i+3];
	
	return true;
}

//Write an AIBus message.
bool AIBusHandler::writeAIData(AIData* ai_d) {
	return writeAIData(ai_d, ai_d->receiver != 0xFF && ai_d->data[0] != 0x80);
}

//Write an AIBus message.
bool AIBusHandler::writeAIData(AIData* ai_d, const bool acknowledge) {
	if(ai_d->l > AIDATA_LIMIT + 3) {
		const int count = ai_d->l/AIDATA_LIMIT, r = ai_d->l%AIDATA_LIMIT;
		AIData ai_group[count + (r == 0 ? 0 : 1)];

		for(int i=0;i<sizeof(ai_group)/sizeof(AIData);i+=1) {
			const int l = (i<sizeof(ai_group)/sizeof(AIData) - 1 || r == 0) ? AIDATA_LIMIT + 3 : r + 3;
			uint8_t ai_data[l];
			ai_data[0] = 0x91;
			ai_data[1] = sizeof(ai_group)/sizeof(AIData);
			ai_data[2] = i;
			for(int d=0;d<l-3;d+=1)
				ai_data[d+3] = ai_d->data[AIDATA_LIMIT*i + d];

			ai_group[i] = AIData(l, ai_d->sender, ai_d->receiver, ai_data);
		}

		bool ack = false;
		for(int i=0;i<sizeof(ai_group)/sizeof(AIData);i+=1) {
			ack = writeAIData(&ai_group[i], acknowledge);
			if(!ack && acknowledge)
				return false;
		}
		return ack;
	}

	uint8_t data[ai_d->l + 4];
	ai_d->getBytes(data);

	int byte_count = ai_serial->available();
	elapsedMicros timer;
	while(timer < AI_DELAY_U) {
		while(micros() >= UINT32_MAX - AI_DELAY_U*2);
		if(this->rx_pin >= 0) {
			int8_t rx = digitalRead(this->rx_pin);
			elapsedMillis rx_timer;
			if(rx == LOW)
				timer = 0;
			while(rx == LOW) {
				if(rx_timer > 10) { //Problem.
					uint8_t ping[] = {0xF, 0xF0};
					ai_serial->write(ping, sizeof(ping));
					break;
				}
			}
		} else {
			const int new_byte_count = ai_serial->available();
			if(new_byte_count > byte_count) {
				byte_count = new_byte_count;
				timer = 0;
			}
		}
	}

	while(ai_serial->available() > 0)
		cachePending(ai_d->sender);

	ai_serial->write(data, ai_d->l + 4);
	ai_serial->flush();

	bool ack_sent = false;

	if(ai_d->l > 1 || (ai_d->l >= 1 && ai_d->data[0] != 0x80)) {
		AIData msg;
		while(ai_serial->available() > 0) {
			if(readAIData(&msg, false)) {
				if(msg.sender == ai_d->sender)
					continue;
				
				if(msg.sender == ai_d->receiver && msg.receiver == ai_d->sender && msg.data[0] == 0x80) {
					ack_sent = true;
				} else if(msg.sender != ai_d->sender) {
					if(msg.receiver == ai_d->sender && msg.l >= 1 && msg.data[0] != 0x80)
						sendAcknowledgement(ai_d->sender, msg.sender);
					if((msg.receiver == ai_d->sender || msg.receiver == 0xFF) && msg.l >= 1 && msg.data[0] != 0x80) {
						cacheMessage(&msg);
					}
				}
			}
		}
	} else if(ai_d->l == 1 && ai_d->data[0] == 0x80) //Acknowledgement we don't need to reacknowledge.
		ack_sent = true;

	if(!ack_sent && acknowledge)
		ack_sent = awaitAcknowledgement(ai_d);
	
	if(acknowledge)
		return ack_sent;
	else
		return true;
}

//Send an acknowledgement message.
void AIBusHandler::sendAcknowledgement(const uint8_t sender, const uint8_t receiver) {
	AIData ack_msg(1, sender, receiver);
	ack_msg.data[0] = 0x80;

	writeAIData(&ack_msg, false);
}

//Wait for receiver acknowledgement.
bool AIBusHandler::awaitAcknowledgement(AIData* ai_d) {
	elapsedMillis repeat_time = 0;
	bool acknowledge = false;
	uint8_t tries = 0;

	while(!acknowledge && tries < MAX_REPEAT) {
		AIData new_msg;
		if(readAIData(&new_msg, false)) {
			if(new_msg.sender == ai_d->sender)
				continue;

			if(new_msg.sender == ai_d->receiver && new_msg.receiver == ai_d->sender && new_msg.l > 0 && new_msg.data[0] == 0x80) {
				acknowledge = true;
				break;
			} else if(new_msg.sender != ai_d->sender) {
				if(new_msg.receiver == ai_d->sender && new_msg.l >= 1 && new_msg.data[0] != 0x80)
					sendAcknowledgement(ai_d->sender, new_msg.sender);
				if((new_msg.receiver == ai_d->sender || new_msg.receiver == 0xFF) && new_msg.l >= 1 && new_msg.data[0] != 0x80) {
					cacheMessage(&new_msg);
				}
				repeat_time = 0;
			}
		}

		if(repeat_time > REPEAT_DELAY && !acknowledge) {
			if(ai_d->l == 2 && ai_d->data[0] != 0xA1) {
				AIData padded_msg(ai_d->l + 1, ai_d->sender, ai_d->receiver, ai_d->data);
				padded_msg[padded_msg.l-1] = 0x0;
				writeAIData(&padded_msg, false);
			}
			else
				writeAIData(ai_d, false);
			repeat_time = 0;
			tries += 1;
		}
	}

	return acknowledge;
}

//Cache a message.
void AIBusHandler::cacheMessage(AIData* ai_msg) {
	if(cached_vec.size() < cached_vec.max_size()) {	
		cached_vec.push_back(*ai_msg);
	}
}

//Cache any pending messages sent to the specified ID.
bool AIBusHandler::cachePending(const uint8_t id) {
	AIData ai_msg;
	if(ai_serial->available() > 0) {
		if(readAIData(&ai_msg, false)) {
			if(ai_msg.sender != id) {
				if(ai_msg.receiver == id || ai_msg.receiver == 0xFF) {
					if(ai_msg.l >= 1 && ai_msg.data[0] != 0x80) {
						if(ai_msg.receiver == id)
							sendAcknowledgement(id, ai_msg.sender);
						cacheMessage(&ai_msg);
						return true;
					}
				}
			}
		}
	}
	return false;
}

//Determine whether a message is the initialization message.
bool getInitMessage(AIData* ai_d) {
	if(ai_d->l < 2)
		return false;

	if(ai_d->data[0] == 0x4A && ai_d->data[1] == 0x1F)
		return true;
	else
		return false;
}

//Determine whether a message is the poweroff message.
bool getPoweroffMessage(AIData* ai_d) {
	if(ai_d->l < 1)
		return false;
	
	if(ai_d->data[0] == 0xA0)
		return true;
	else
		return false;
}

//Return whether an AIBus read result is positive.
bool getPositiveResult(const aibus_read_result_t result) {
	return result == AIBUS_READ_OK_SERIAL || result == AIBUS_READ_OK_CACHED || result == AIBUS_READ_OK_MULTIPLE;
}
