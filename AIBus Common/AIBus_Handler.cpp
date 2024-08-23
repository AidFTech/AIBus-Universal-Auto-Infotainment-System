#include "AIBus_Handler.h"

AIBusHandler::AIBusHandler(Stream* serial, const int8_t rx_pin) {
	this->ai_serial = serial;
	this->rx_pin = rx_pin;

	if(this->rx_pin >= 0)
		pinMode(this->rx_pin, INPUT_PULLUP);

	cached_vec.setStorage(cached_byte, 0);
	this->ai_serial->setTimeout(1);
}

AIBusHandler::~AIBusHandler() {
	
}

int AIBusHandler::dataAvailable() {
	return dataAvailable(true);
}

int AIBusHandler::dataAvailable(const bool cache) {
	if(!cache || cached_msg.l <= 0)
		return ai_serial->available();
	else
		return cached_msg.l;
}

bool AIBusHandler::readAIData(AIData* ai_d) {
	return readAIData(ai_d, true);
}

bool AIBusHandler::readAIData(AIData* ai_d, const bool cache) {
	if(cache && cached_msg.l > 0) {
		ai_d->refreshAIData(cached_msg);
		cached_msg.refreshAIData(0,0,0);

		if(cached_vec.size() >= 4) {
			const uint8_t l = cached_vec.at(1);
			uint8_t data[l+2];

			if(cached_vec.size() < l + 2)
				cached_vec.clear();
			else {
				for(int i=0;i<l+2;i+=1) {
					data[i] = cached_vec.at(0);
					cached_vec.remove(0);
				}

				readAIData(&cached_msg, data, l);
			}
		} else if(cached_vec.size() > 0)
			cached_vec.clear();
		
		return true;
	}

	if(ai_serial->available() < 2)
		return false;

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

			int8_t rx = digitalRead(this->rx_pin);
			if(rx == LOW)
				ai_timer = 0;

			if(ai_timer > AI_WAIT) {
				elapsedMicros clear_timer;

				int avail = ai_serial->available();
				while(clear_timer < AI_DELAY_U) {
					if(ai_serial->available() > avail) {
						avail = ai_serial->available();
						clear_timer = 0;
					}
				}

				uint8_t db[ai_serial->available()];
				ai_serial->readBytes(db, ai_serial->available());
				return false;
			}
		}

		if(ai_serial->available() < l)
			return false;

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
					if(ai_serial->available() > avail) {
						avail = ai_serial->available();
						clear_timer = 0;
					}
				}

				uint8_t db[ai_serial->available()];
				ai_serial->readBytes(db, ai_serial->available());
				return false;
			}
		}

		ai_d->refreshAIData(l-2, s, r);

		for(uint8_t i=0;i<ai_d->l;i+=1)
			ai_d->data[i] = d[i];

		return true;
	} else {
		return false;
	}
}

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

bool AIBusHandler::writeAIData(AIData* ai_d) {
	return writeAIData(ai_d, ai_d->receiver != 0xFF && ai_d->data[0] != 0x80);
}

bool AIBusHandler::writeAIData(AIData* ai_d, const bool acknowledge) {
	uint8_t data[ai_d->l + 4];
	const unsigned int full_length = ai_d->l + 4;
	ai_d->getBytes(data);

	if(this->rx_pin >= 0 && (ai_d->l > 1 || (ai_d->l >= 1 && ai_d->data[0] != 0x80))) {
		elapsedMicros timer;
		while(timer < AI_DELAY_U) {
			while(micros() >= UINT32_MAX - AI_DELAY_U*2);
			int8_t rx = digitalRead(this->rx_pin);
			if(rx == LOW)
				timer = 0;
		}
		
		while(ai_serial->available() > 0)
			cachePending(ai_d->sender);
	}

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
	}

	if(!ack_sent && acknowledge)
		ack_sent = awaitAcknowledgement(ai_d);
	
	if(acknowledge)
		return ack_sent;
	else
		return true;
}

void AIBusHandler::sendAcknowledgement(const uint8_t sender, const uint8_t receiver) {
	AIData ack_msg(1, sender, receiver);
	ack_msg.data[0] = 0x80;

	writeAIData(&ack_msg, false);
}

bool AIBusHandler::awaitAcknowledgement(AIData* ai_d) {
	elapsedMillis repeat_time = 0;
	bool acknowledge = false;
	uint8_t tries = 0;

	while(!acknowledge && tries < MAX_REPEAT) {
		AIData new_msg;
		if(readAIData(&new_msg, false)) {
			if(new_msg.sender == ai_d->sender)
				continue;

			if(new_msg.sender == ai_d->receiver && new_msg.receiver == ai_d->sender && new_msg.data[0] == 0x80) {
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
			writeAIData(ai_d, false);
			repeat_time = 0;
			tries += 1;
		}
	}

	return acknowledge;
}

void AIBusHandler::cacheMessage(AIData* ai_msg) {
	if(cached_msg.l <= 0)
		cached_msg.refreshAIData(*ai_msg);
	else if(ai_msg->l + 4 < (cached_vec.max_size() - cached_vec.size() - 1)) {
		uint8_t data[ai_msg->l + 4];
		ai_msg->getBytes(data);
		for(int i=0;i<ai_msg->l + 4;i+=1)
			cached_vec.push_back(data[i]);
	}
}

bool AIBusHandler::cachePending(const uint8_t id) {
	AIData ai_msg;
	if(ai_serial->available() > 0) {
		if(readAIData(&ai_msg, false)) {
			if(ai_msg.sender != id) {
				if(ai_msg.receiver == id || ai_msg.receiver == 0xFF) {
					if(ai_msg.receiver == id && ai_msg.l >= 1 && ai_msg.data[0] != 0x80)
						sendAcknowledgement(id, ai_msg.sender);
					if((ai_msg.receiver == id || ai_msg.receiver == 0xFF) && ai_msg.l >= 1 && ai_msg.data[0] != 0x80) {
						cacheMessage(&ai_msg);
					}
					return true;
				}
			}
		}
	}
	return false;
}
