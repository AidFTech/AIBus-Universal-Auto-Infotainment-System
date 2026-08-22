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

	if(ai_serial->available() < 4) {
		//if(ai_serial->available() > 0)
		//	clearSerial();

		return ai_serial->available() > 0 ? AIBUS_READ_INCOMPLETE : AIBUS_READ_NODATA;
	}

	int avail = ai_serial->available();
	elapsedMicros wait_micros = 0;

	while(wait_micros < AI_DELAY_U) {
		if(ai_serial->available() != avail) {
			avail = ai_serial->available();
			wait_micros = 0;
		}
	}

	if(ai_serial->available() >= 4) {
		uint8_t init_stream[2];
		ai_serial->readBytes(init_stream, sizeof(init_stream));
		const uint8_t s = init_stream[0];
		const int l = init_stream[1];
		
		elapsedMillis ai_timer;

		if(l > AIDATA_LIMIT + 5 || l < 2)
			return AIBUS_READ_INVALID_CHECKSUM;

		while(ai_serial->available() < l) {
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

				return AIBUS_READ_TIMEOUT;
			}
		}

		if(ai_serial->available() < l)
			return AIBUS_READ_TIMEOUT;

		ai_d->refreshAIData(0,0,0);

		uint8_t r_stream[1];
		ai_serial->readBytes(r_stream, sizeof(r_stream));
		const uint8_t r = r_stream[0];
		
		uint8_t d[l-1];
		ai_serial->readBytes(d, l-1);

		if(getID(s))
			return AIBUS_READ_NODATA;

		{
			uint8_t chex[l+2];
			chex[0] = s;
			chex[1] = uint8_t(l&0xFF);
			chex[2] = r;
			for(uint8_t i=0;i<sizeof(d) && i+3<sizeof(chex);i+=1)
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

				clearSerial();
				return AIBUS_READ_INVALID_CHECKSUM;
			}
		}

		if(sizeof(d) == 3 && d[0] == s && d[1] == l && d[2] == r)
			return AIBUS_READ_INCOMPLETE;

		ai_d->refreshAIData(l-2, s, r);
		for(uint8_t i=0;i<ai_d->l && i<sizeof(d);i+=1)
			ai_d->data[i] = d[i];

		if(getID(r) && ai_d->l > 0 && ai_d->data[0] != 0x80)
			sendAcknowledgement(r, s);

		if(multiple && ai_d->l >= 3 && ai_d->data[0] == 0x91 && (getID(ai_d->receiver) || ai_d->receiver == 0xFF)) { //Multiple messages are on the way.
			if(ai_d->data[2] != 0)
				return AIBUS_READ_INVALID_MULTIPLE;

			const int msg_count = ai_d->data[1];
			uint8_t full_data[msg_count*AIDATA_LIMIT];
			int full_length = ai_d->l - 3;

			if(msg_count <= 0)
				return AIBUS_READ_INVALID_MULTIPLE;

			for(int i=0;i<ai_d->l - 3 && i < sizeof(full_data);i+=1)
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

					const uint8_t index = test_msg[2];
					if(index < m)
						continue;

					for(int i=0;i<test_msg.l-3 && i + AIDATA_LIMIT*m < sizeof(full_data);i+=1)
						full_data[(AIDATA_LIMIT)*index + i] = test_msg[i+3];

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

		elapsedMillis timer, rx_timer;
		int byte_count = ai_serial->available();

		//Wait for bus to be clear...
		while(timer < AI_DELAY_M) {
			if(this->rx_pin >= 0) {
				int8_t rx = digitalRead(this->rx_pin);
				if(rx == LOW)
					timer = 0;
				while(rx == LOW) {
					rx = digitalRead(this->rx_pin);
					if(rx_timer > 10) { //Problem.
						uint8_t ping_data[] = {0x1};
						AIData ping(sizeof(ping_data), id, 0xFF, ping_data);
						uint8_t ping_bytes[ping.l + 4];
						ping.getBytes(ping_bytes);
						//ai_serial->write(ping_bytes, sizeof(ping_bytes));

						timer = 0;
						break;
					}
					timer = 0;
				}
			} else {
				const int new_byte_count = ai_serial->available();
				if(new_byte_count > byte_count) {
					byte_count = new_byte_count;
					timer = 0;
				}
			}
		}

		bool ack = false;
		for(int i=0;i<sizeof(ai_group)/sizeof(AIData);i+=1) {
			ack = writeAIData(&ai_group[i], acknowledge);
			if(!ack && acknowledge)
				return false;
			delay(5);
		}
		return ack;
	}

	uint8_t data[ai_d->l + 4];
	ai_d->getBytes(data);

	int byte_count = ai_serial->available();
	elapsedMicros timer;
	elapsedMillis rx_timer;
	while(timer < AI_DELAY_U) {
		if(this->rx_pin >= 0) {
			int8_t rx = digitalRead(this->rx_pin);
			if(rx == LOW)
				timer = 0;
			while(rx == LOW) {
				rx = digitalRead(this->rx_pin);
				if(rx_timer > 10) { //Problem.
					uint8_t ping_data[] = {0x1};
					AIData ping(sizeof(ping_data), id, 0xFF, ping_data);
					uint8_t ping_bytes[ping.l + 4];
					ping.getBytes(ping_bytes);
					//ai_serial->write(ping_bytes, sizeof(ping_bytes));

					timer = 0;
					break;
				}
				timer = 0;
			}
		} else {
			const int new_byte_count = ai_serial->available();
			if(new_byte_count > byte_count) {
				byte_count = new_byte_count;
				timer = 0;
			}
		}
	}

	elapsedMillis cache_timer;
	while(ai_serial->available() > 0 && cache_timer < 5 && cached_vec.size() < cached_vec.max_size())
		cachePending(ai_d->sender);

	if(ai_d->l == 2 && ai_d->data[0] == ai_d->sender && data[sizeof(data)-1] == ai_d->receiver) {
		AIData ext_msg(ai_d->l + 1, ai_d->sender, ai_d->receiver);
		for(int i=0;i<ai_d->l;i+=1)
			ext_msg[i] = ai_d->data[i];
		ext_msg[ext_msg.l-1] = 0;

		uint8_t ext_data[ext_msg.l + 4];
		ext_msg.getBytes(ext_data);

		ai_serial->write(ext_data, sizeof(ext_data));
	} else
		ai_serial->write(data, sizeof(data));

	bool ack_sent = false;

	if(acknowledge && (ai_d->l > 1 || (ai_d->l >= 1 && ai_d->data[0] != 0x80))) {
		AIData msg;
		while(ai_serial->available() > 0) {
			if(readAIData(&msg, false)) {
				if(msg.sender == ai_d->sender)
					continue;
				
				if(msg.sender == ai_d->receiver && msg.receiver == ai_d->sender && msg.data[0] == 0x80) {
					ack_sent = true;
				} else if(msg.sender != ai_d->sender) {
					if((getID(msg.receiver) || msg.receiver == ai_d->sender || msg.receiver == 0xFF) && msg.l >= 1 && msg.data[0] != 0x80) {
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

//Flush the serial port.
void AIBusHandler::flush() {
	ai_serial->flush();
}

//Wait for receiver acknowledgement.
bool AIBusHandler::awaitAcknowledgement(AIData* ai_d) {
	elapsedMillis repeat_time = 0;
	bool acknowledge = false;
	int tries = 0;

	while(!acknowledge && tries < MAX_REPEAT) {
		AIData new_msg;
		
		if(ai_serial->available() > 0) {
			if(readAIData(&new_msg, false)) {
				if(new_msg.sender == ai_d->sender)
					continue;

				if(new_msg.sender == ai_d->receiver && new_msg.receiver == ai_d->sender && new_msg.l > 0 && new_msg.data[0] == 0x80) {
					acknowledge = true;
					break;
				} else if(new_msg.sender != ai_d->sender) {
					if((getID(new_msg.receiver) || new_msg.receiver == ai_d->sender || new_msg.receiver == 0xFF) && new_msg.l >= 1 && new_msg.data[0] != 0x80) {
						cacheMessage(&new_msg);
					}
					repeat_time = 0;
				}
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
	//if(cached_vec.size() < cached_vec.max_size()) {	
		cached_vec.push_back(*ai_msg);
	//}
}

//Cache any pending messages sent to the specified ID.
bool AIBusHandler::cachePending(const uint8_t id) {
	AIData ai_msg;
	if(ai_serial->available() > 0 && cached_vec.size() < cached_vec.max_size()) {
		if(readAIData(&ai_msg, false)) {
			if(!getID(ai_msg.sender) && ai_msg.sender != id) {
				if(getID(ai_msg.receiver) || ai_msg.receiver == id || ai_msg.receiver == 0xFF) {
					if(ai_msg.l >= 1 && ai_msg.data[0] != 0x80) {
						cacheMessage(&ai_msg);
						return true;
					}
				}
			}
		}
	}
	return false;
}

//Clear any pending data in the serial port.
void AIBusHandler::clearSerial() {
	elapsedMillis clear_timer;
	while(ai_serial->available() > 0) {
		const int avail = ai_serial->available();
		uint8_t db[avail];
		ai_serial->readBytes(db, avail);

		/*if(clear_timer > 10) {
			ai_serial->clearWriteError();
			break;
		}*/
	}
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
