#include "En_IEBus_Handler.h"

EnIEBusHandler::EnIEBusHandler(const int8_t rx_pin, const int8_t tx_pin) : IEBusHandler(rx_pin, tx_pin) {
	
}

//Initialize the handler.
void EnIEBusHandler::init(EnIEBusParams* ie_params) {
	this->ai_handler = ie_params->ai_handler;

	this->low_byte_register = ie_params->low_byte_register;
	this->high_byte_register = ie_params->high_byte_register;

	if(ie_params->rec_clear >= 0) {
		this->rec_clear_bitmask = digitalPinToBitMask(ie_params->rec_clear);
		const uint8_t port = digitalPinToPort(ie_params->rec_clear);
		this->rec_clear_register = portOutputRegister(port);
	}

	if(ie_params->rec_set >= 0) {
		this->rec_set_bitmask = digitalPinToBitMask(ie_params->rec_set);
		const uint8_t port = digitalPinToPort(ie_params->rec_set);
		this->rec_set_register = portInputRegister(port);
	}

	if(ie_params->count_enable >= 0) {
		this->count_enable_bitmask = digitalPinToBitMask(ie_params->count_enable);
		const uint8_t port = digitalPinToPort(ie_params->count_enable);
		this->count_enable_register = portOutputRegister(port);
	}

	if(ie_params->count_reset >= 0) {
		this->count_reset_bitmask = digitalPinToBitMask(ie_params->count_reset);
		const uint8_t port = digitalPinToPort(ie_params->count_reset);
		this->count_reset_register = portOutputRegister(port);
	}

	uint8_t count_ports[] = {digitalPinToPort(ie_params->count[0]),
							digitalPinToPort(ie_params->count[1]),
							digitalPinToPort(ie_params->count[2]),
							digitalPinToPort(ie_params->count[3])};

	/*for(int i=0;i<sizeof(count_ports);i+=1) {
		if(count_ports[i] != count_ports[0])
			return; //Count ports can't be used.
	}*/ //TODO: Try this.
	
	this->count_register = portInputRegister(count_ports[0]);
	this->count_bitmask = (digitalPinToBitMask(ie_params->count[0])) | 
							(digitalPinToBitMask(ie_params->count[1])) |
							(digitalPinToBitMask(ie_params->count[2])) |
							(digitalPinToBitMask(ie_params->count[3]));

	for(int i=0;i<8;i+=1) {
		if((digitalPinToBitMask(ie_params->count[0]) & (1<<i)) != 0) {
			this->count_shift = i;
			break;
		}
	}
}

bool EnIEBusHandler::cacheAIBus() {
	if(this->ai_handler != NULL)
		return this->ai_handler->cacheAllPending();
	else
		return false;
}

void EnIEBusHandler::sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum) {
	ai_handler->cacheAllPending();
	IEBusHandler::sendMessage(ie_d, ack_response, checksum);

	*rec_clear_register &= ~rec_clear_bitmask;
	*rec_clear_register |= rec_clear_bitmask;
}

void EnIEBusHandler::sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum, const bool wait) volatile {
	ai_handler->cacheAllPending();
	IEBusHandler::sendMessage(ie_d, ack_response, checksum, wait);

	*rec_clear_register &= ~rec_clear_bitmask;
	*rec_clear_register |= rec_clear_bitmask;
}

int EnIEBusHandler::readMessage(IE_Message* ie_d, bool ack_response, const uint16_t id)  {
	/*int avail = ai_handler->dataAvailable(false);
	elapsedMicros timer;

	while(timer < AI_DELAY_U) {
		if(ai_handler->dataAvailable(false) != avail || ai_handler->dataAvailable(false) > 0) {
			ai_handler->cacheAllPending();
			avail = ai_handler->dataAvailable(false);
			timer = 0;
		}
	}*/
	
	ai_handler->cacheAllPending();
	const int result = IEBusHandler::readMessage(ie_d, ack_response, id);
	
	*rec_clear_register &= ~rec_clear_bitmask;
	*rec_clear_register |= rec_clear_bitmask;
	/*ai_handler->waitForAIBus();
	ai_handler->cacheAllPending();*/

	return result;
}

int EnIEBusHandler::readMessageStrict(IE_Message* ie_d, bool ack_response, const uint16_t id) volatile {
	return IEBusHandler::readMessage(ie_d, ack_response, id);
}

//Add an AIBus ID.
void EnIEBusHandler::addID(const uint8_t id) {
	if(this->ai_handler != NULL)
		this->ai_handler->addID(id);
}

//Read bits with hardware.
inline int EnIEBusHandler::readBitsH(const int length, const bool with_parity, const bool with_ack, bool send_ack) {
	bool dummy_d;
	return readBitsH(length, with_parity, with_ack, send_ack, false, &dummy_d);
}

//Read bits with hardware. If the "direct" bit is present, read that into direct_value.
inline int EnIEBusHandler::readBitsH(const int length, const bool with_parity, const bool with_ack, bool send_ack, const bool direct, bool* direct_value) {
	int value = -1;
	bool parity = false;

	int count_value = length;
	if(with_parity)
		count_value += 1;
	if(direct)
		count_value += 1;

	*this->count_reset_register |= this->count_reset_bitmask;
	*this->count_reset_register &= ~this->count_reset_bitmask;
	*this->count_enable_register |= this->count_enable_bitmask;

	elapsedMicros message_time;
	while((*count_register&(count_bitmask >> count_shift)) < count_value) {
		if(message_time > 40*(count_value + 2)) {
			*this->count_enable_register &= ~this->count_enable_bitmask;
			return -1;
		}
	}

	bool rec_parity = false;
	if(with_parity) {
		value = (*this->high_byte_register << 8 | *this->low_byte_register) >> 1;
		rec_parity = (*this->low_byte_register&0x1) != 0;
	} else 
		value = *this->high_byte_register << 8 | *this->low_byte_register;

	if(direct) {
		*direct_value = (value&(bit(length+1))) != 0;
		value &= ~(bit(length+1));
	}

	if(with_parity) {
		for(int i=0;i<length;i+=1) {
			if((bit(i) & value) != 0)
				parity = !parity;
		}

		if(rec_parity != parity) {
			*this->count_enable_register &= ~this->count_enable_bitmask;
			return -1;
		}
	}

	if(with_ack && send_ack) {
		TIMER = 0;
		while((*rx_portregister&rx_bitmask) == 0) {
			if(TIMER > IE_NORMAL_BIT_0_LENGTH) {
				*this->count_enable_register &= ~this->count_enable_bitmask;
				return -1;
			}
		}
		sendAckBit();
	} else if(with_ack && !send_ack)
		readBit();

	*this->count_enable_register &= ~this->count_enable_bitmask;
	return value;
}

//Read a message with hardware.
int EnIEBusHandler::readMessageH(IE_Message* ie_d, bool ack_response, const uint16_t id) {
	noInterrupts();

	if((*rx_portregister&rx_bitmask) == 0) {
		interrupts();
		return -1;
	}

	TIMER_SCALER = 3;
	TIMER = 0;
	while((*rx_portregister&rx_bitmask) != 0) {
		if(TIMER > 200) {
			interrupts();
			return -1;
		}
	}

	if(TIMER < START_COMP_LENGTH) {
		interrupts();
		return -1;
	}

	TIMER_SCALER = 2;
	TIMER = 0;

	bool direct = false;
	//Ensure sender bits are sent.
	TIMER = 0;
	/*while((*rx_portregister&rx_bitmask) == 0 && TIMER < IE_FAILSAFE_LENGTH) {
		if(TIMER >= START_COMP_LENGTH) {
			interrupts();
			return 1;
		}
	}*/

	//Read sender bits.
	const int sender = readBitsH(12, true, false, ack_response, true, &direct);

	if(sender < 0) {
		interrupts();
		return 1;
	}

	//Ensure receiver bits are sent.
	TIMER = 0;
	/*while((*rx_portregister&rx_bitmask) == 0 && TIMER < IE_FAILSAFE_LENGTH) {
		if(TIMER >= START_COMP_LENGTH) {
			interrupts();
			return 2;
		}
	}*/

	//Read receiver bits.
	const int receiver = readBitsH(12, true, false, ack_response);

	if(receiver >= 0) {
		if(receiver == id && ack_response) {
			while((*rx_portregister&rx_bitmask) == 0);
			this->sendBit(false);
		} else
			readBit();
	} else {
		interrupts();
		return 2;
	}

	if(receiver != id)
		ack_response = false;

	//Ensure control bits are sent.
	TIMER = 0;
	/*while((*rx_portregister&rx_bitmask) == 0 && TIMER < IE_FAILSAFE_LENGTH) {
		if(TIMER >= START_COMP_LENGTH) {
			interrupts();
			return 3;
		}
	}*/

	//Read control bits.
	const int control = readBitsH(4, true, true, ack_response);

	if(control < 0) {
		interrupts();
		return 3;
	}

	//Ensure length bits are sent.
	TIMER = 0;
	/*while((*rx_portregister&rx_bitmask) == 0 && TIMER < IE_FAILSAFE_LENGTH) {
		if(TIMER >= START_COMP_LENGTH) {
			interrupts();
			return 4;
		}
	}*/

	//Read length bits.
	const int l = readBitsH(8, true, true, ack_response);

	if(l < 0) {
		interrupts();
		return 4;
	}

	uint8_t data[l];

	for(unsigned int i=0;i<l;i+=1) {
		//Ensure data bits are sent.
		TIMER = 0;
		/*while((*rx_portregister&rx_bitmask) == 0 && TIMER < IE_FAILSAFE_LENGTH) {
			if(TIMER >= START_COMP_LENGTH) {
				interrupts();
				return 5+i;
			}
		}*/

		//Read data bits.
		data[i] = readBitsH(8, true, true, ack_response);
		if(data[i] < 0) {
			interrupts();
			return 5+i;
		}
	}
	interrupts();

	ie_d->refreshIEData(l, sender, receiver, control, direct);
	ie_d->refreshIEData(data);

	return 0;
}