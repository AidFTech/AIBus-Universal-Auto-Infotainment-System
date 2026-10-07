#include "IEBus_Handler.h"

#ifdef HW_IEBUS
IEBusHandler::IEBusHandler(Stream* ie_serial, const int8_t active_pin) {
	this->ie_serial = ie_serial;
	this->active_pin = active_pin;
#else
IEBusHandler::IEBusHandler(const int8_t rx_pin, const int8_t tx_pin) {
	this->rx_pin = rx_pin;
	this->tx_pin = tx_pin;
	
	if(rx_pin < 0 || tx_pin < 0)
		return;
		
	pinMode(this->rx_pin, OUTPUT);
	pinMode(this->tx_pin, OUTPUT);

	digitalWrite(this->tx_pin, LOW);
	digitalWrite(this->rx_pin, LOW);
	
	pinMode(this->rx_pin, INPUT);
	
	tx_bitmask = digitalPinToBitMask(this->tx_pin);
	const uint8_t tx_port = digitalPinToPort(this->tx_pin);
	this->tx_portregister = portOutputRegister(tx_port);
	
	rx_bitmask = digitalPinToBitMask(this->rx_pin);
	const uint8_t rx_port = digitalPinToPort(this->rx_pin);
	this->rx_portregister = portInputRegister(rx_port);
	#endif

	rx_cache.setStorage(rx_cache_array, 0);
}

#ifndef HW_IEBUS
void IEBusHandler::sendBit(const bool data) {
	TIMER = 0;

	*tx_portregister |= this->tx_bitmask;
	if(data)
		while(TIMER < IE_BIT_1_HOLD_ON_LENGTH);
	else
		while(TIMER < IE_BIT_0_HOLD_ON_LENGTH);
	*tx_portregister &= ~this->tx_bitmask;

	if(data)
		while(TIMER < IE_NORMAL_BIT_1_LENGTH);
	else
		while(TIMER < IE_NORMAL_BIT_0_LENGTH);

}

void IEBusHandler::sendAckBit() {
	TIMER = 0;

	*tx_portregister |= this->tx_bitmask;
	while(TIMER < IE_BIT_A_HOLD_ON_LENGTH);
	*tx_portregister &= ~this->tx_bitmask;
	while(TIMER < IE_NORMAL_BIT_A_LENGTH);
}

void IEBusHandler::sendStartBit() {
	TIMER_SCALER = 3;
	TIMER = 0;
	*tx_portregister |= this->tx_bitmask;
	while(TIMER < START_LENGTH);

	*tx_portregister &= ~this->tx_bitmask;
	TIMER_SCALER = 2;
	TIMER = 0;
	while(TIMER < START_END_LENGTH);
}

void IEBusHandler::sendBits(const int16_t data, const uint8_t size) {
	this->sendBits(data, size, true, true);
}

void IEBusHandler::sendBits(const int16_t data, const uint8_t size, const bool send_parity, const bool ack) {
	bool parity = false;
	for(uint8_t i=0;i<size;i+=1) {
		//if((size-1)-i < 0)
		//	continue;

		const bool data_bit = data & bit((size-1) - i);
		this->sendBit(data_bit);
		if(data_bit)
			parity = !parity;
	}

	if(send_parity)
		this->sendBit(parity);

	if(ack) {
		this->sendBit(true); //TODO: Full acknowledgement check?
	}
}

//Get bits from a number.
void IEBusHandler::getBits(Vector<bool>* bits, const int16_t data, const uint8_t size, const bool send_parity, const bool ack) {
	bool parity = false;
	for(uint8_t i=0;i<size;i+=1) {
		if((size-1)-i < 0)
			continue;

		const bool data_bit = data & bit((size-1) - i);
		bits->push_back(data_bit);
		if(data_bit)
			parity = !parity;
	}

	if(send_parity)
		bits->push_back(parity);

	if(ack)
		bits->push_back(true); //TODO: Full acknowledgement check?
}

int8_t IEBusHandler::readBit() {
	TIMER_SCALER = 2;
	TIMER = 0;
	while((*rx_portregister&rx_bitmask) == 0) {
		if(TIMER > IE_NORMAL_BIT_0_LENGTH)
			return -1;
	}
	
	TIMER = 0;
	while((*rx_portregister&rx_bitmask) != 0) { //TODO: Check this.
		if(TIMER > IE_FAILSAFE_LENGTH)
			return -1;
	}
	const uint8_t timer_on = TIMER;
	
	/*TIMER = 0;
	while((*rx_portregister&rx_bitmask) == 0) {
		if(TIMER > IE_NORMAL_BIT_1_LENGTH)
			break;
	}
	const uint8_t timer_off = TIMER;*/
	

	//if(TIMER < IE_BIT_COMP_LENGTH)
	if(timer_on < IE_BIT_COMP_LENGTH)
		return 1;
	else
		return 0;
}

int IEBusHandler::readBits(const uint8_t length, const bool with_parity, const bool with_ack, bool send_ack) {
	int value = 0;
	bool parity = false;

	for(int i=0;i<int(length);i+=1) {
		value <<= 1;

		const int8_t data = readBit();
		if(data > 0) {
			value |= 1;
			parity = !parity;
		} else if(data < 0)
			return -1;
	}
	
	if(with_parity) {
		/*const int8_t bit_data = readBit();
		if(bit_data < 0)
			return -1;*/

		const bool data = readBit() > 0;
		if(data != parity && send_ack) {
			send_ack = false;
			value = -1;
		}
	}

	if(with_ack && send_ack) {
		//TIMER = 0;
		while((*rx_portregister&rx_bitmask) == 0);
		sendAckBit();
	} else if(with_ack && !send_ack)
		readBit();

	return value;
}
#endif

void IEBusHandler::sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum) {
	sendMessage(ie_d, ack_response, checksum, true);
}

void IEBusHandler::sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum, const bool wait) {
	#ifdef HW_IEBUS
	while(active_pin >= 0 && digitalRead(active_pin) == HIGH);

	uint8_t serial_proxy[ie_d->l + 5];
	serial_proxy[0] = (ie_d->sender&0xF)>>8;
	serial_proxy[1] = ie_d->sender&0xFF;
	serial_proxy[2] = (ie_d->receiver&0xF)>>8;
	serial_proxy[3] = ie_d->receiver&0xFF;
	serial_proxy[4] = ie_d->control;

	if(ie_d->direct)
		serial_proxy[0] |= 0x80;

	if(!checksum)
		serial_proxy[0] |= 0x40;

	for(int i=0;i<ie_d->l;i+=1)
		serial_proxy[i+5] = ie_d->data[5];

	AIData serial_proxy_msg(sizeof(serial_proxy), 0x10, 0x11, serial_proxy);
	uint8_t bytes[serial_proxy_msg.l + 4];
	serial_proxy_msg.getBytes(bytes);

	ie_serial->write(bytes, sizeof(bytes));
	#else
	const uint8_t cx = ie_d->getChecksum();
	const unsigned int bit_count = 1+13+14+6+10+10*(checksum ? ie_d->l + 1 : ie_d->l);

	bool bits[bit_count];
	Vector<bool> bit_vec;
	bit_vec.setStorage(bits, bit_count, 0);

	bit_vec.push_back(ie_d->direct);
	getBits(&bit_vec, ie_d->sender, 12, true, false);
	getBits(&bit_vec, ie_d->receiver, 12, true, true);
	getBits(&bit_vec, ie_d->control, 4, true, true);
	getBits(&bit_vec, checksum ? ie_d->l+1 : ie_d->l, 8, true, true);
	
	for(unsigned int i=0;i<ie_d->l;i+=1)
		getBits(&bit_vec, ie_d->data[i], 8, true, true);

	if(checksum)
		getBits(&bit_vec, cx, 8, true, true);
	
	noInterrupts();
	
	if(wait) {
		TIMER_SCALER = 3;
		TIMER = 0;
		bool reset = false;

		while(TIMER < IE_DELAY) {
			if((*rx_portregister&rx_bitmask) != 0 && !reset) {
				TIMER = 0;
				reset = true;
			}
		}
		TIMER_SCALER = 2;
	}
	

	this->sendStartBit();

	TIMER = IE_NORMAL_BIT_0_LENGTH;
	volatile int bit_timer = IE_NORMAL_BIT_0_LENGTH;
	volatile bool current_bit = false, last_bit = false;
	for(unsigned int i=0;i<bit_count;i+=1) {
		last_bit = current_bit;
		current_bit = bits[i];

		bit_timer = last_bit ? IE_NORMAL_BIT_1_LENGTH : IE_NORMAL_BIT_0_LENGTH;
		while(TIMER < bit_timer);

		TIMER = 0;

		*tx_portregister |= this->tx_bitmask;
		if(current_bit)
			while(TIMER < IE_BIT_1_HOLD_ON_LENGTH);
		else
			while(TIMER < IE_BIT_0_HOLD_ON_LENGTH);
		*tx_portregister &= ~this->tx_bitmask;
	}

	interrupts();
	#endif
}

//Cache a GA-NET message.
int IEBusHandler::cacheMessage(const bool ack_response, const uint16_t id) {
	IE_Message msg;
	const int res = readMessage(&msg, ack_response, id);

	if(res == 0) {
		IE_Message ack(1, id, msg.sender, 0xF, true);
		ack.data[0]= 0x80;
		sendMessage(&ack, true, true);

		rx_cache.push_back(msg);
	}
	
	return res;
}

//Get the RX cache.
Vector<IE_Message>* IEBusHandler::getRX() {
	return &this->rx_cache;
}

//Clear the RX cache.
void IEBusHandler::clearRX() {
	rx_cache.clear();
}

int IEBusHandler::readMessage(IE_Message* ie_d, bool ack_response, const uint16_t id) {
	#ifdef HW_IEBUS
	while(active_pin >= 0 && digitalRead(active_pin) == HIGH);
	if(ie_serial->available() >= 4) {
		uint8_t sl[3];
		ie_serial->readBytes(sl, sizeof(sl));

		const uint8_t s = sl[0], r = sl[2];
		const int len = sl[1];

		uint8_t main_data[len - 1];
		ie_serial->readBytes(main_data, sizeof(main_data));

		uint8_t full_data[sizeof(sl) + sizeof(main_data)];
		for(int i=0;i<sizeof(sl);i+=1)
			full_data[i] = sl[i];

		for(int i=0;i<sizeof(main_data);i+=1)
			full_data[i+sizeof(sl)] = main_data[i];

		if(!checkValidity(full_data, sizeof(full_data)))
			return 1;

		if(s != 0x11 && r != 0x10)
			return -1;

		AIData rx;
		bytesToAIData(&rx, full_data);

		if(rx.l < 5)
			return -1;

		const uint16_t sender = ((rx[0]&0xF)<<8) | rx[1], receiver = ((rx[2]&0xF)<<8) | rx[3];
		const uint8_t ctl = rx[4];

		const bool direct = (rx[0]&0x80) != 0;

		ie_d->refreshIEData(rx.l - 5);

		ie_d->sender = sender;
		ie_d->receiver = receiver;
		ie_d->control = ctl;
		ie_d->direct = direct;

		for(int i=0;i<ie_d->l;i+=1)
			ie_d->data[i] = rx[i+5];

		return 0;
	}

	return -1;
	#else
	noInterrupts();

	if((*rx_portregister&rx_bitmask) == 0) {
		interrupts();
		return -1;
	}

	TIMER_SCALER = 3;
	TIMER = 0;
	while((*rx_portregister&rx_bitmask) != 0) {
		if(TIMER > 200) {
			TIMER_SCALER = 2;
			interrupts();
			return -2;
		}
	}

	if(TIMER < START_COMP_LENGTH) {
		TIMER_SCALER = 2;
		interrupts();
		return -1;
	}

	TIMER_SCALER = 2;
	TIMER = 0;

	//Read the "direct" bit.
	const int8_t direct_int = readBits(1, false, false, ack_response);
	
	bool direct = false;
	if(direct_int == 1)
		direct = true;
	else if(direct_int < 0) {
		interrupts();
		return -2;
	}

	//Ensure sender bits are sent.
	TIMER = 0;
	while((*rx_portregister&rx_bitmask) == 0) {
		if(TIMER >= IE_FAILSAFE_LENGTH) {
			interrupts();
			return 1;
		}
	}

	//Read sender bits.
	const int sender = readBits(12, true, false, ack_response);

	if(sender < 0) {
		interrupts();
		return 1;
	}

	//Ensure receiver bits are sent.
	TIMER = 0;
	while((*rx_portregister&rx_bitmask) == 0) {
		if(TIMER >= IE_FAILSAFE_LENGTH) {
			interrupts();
			return 2;
		}
	}

	//Read receiver bits.
	const int receiver = readBits(12, true, false, ack_response);

	if(receiver >= 0) {
		if(receiver == id && ack_response) {
			TIMER = 0;
			while((*rx_portregister&rx_bitmask) == 0 && TIMER < IE_NORMAL_BIT_0_LENGTH);
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
	while((*rx_portregister&rx_bitmask) == 0) {
		if(TIMER >= IE_FAILSAFE_LENGTH) {
			interrupts();
			return 3;
		}
	}

	//Read control bits.
	const int control = readBits(4, true, true, ack_response);

	if(control < 0) {
		interrupts();
		return 3;
	}

	//Ensure length bits are sent.
	TIMER = 0;
	while((*rx_portregister&rx_bitmask) == 0) {
		if(TIMER >= IE_FAILSAFE_LENGTH) {
			interrupts();
			return 4;
		}
	}

	//Read length bits.
	const int l = readBits(8, true, true, ack_response);

	if(l < 0) {
		interrupts();
		return 4;
	}

	int16_t data[l];

	for(int i=0;i<l;i+=1) {
		//Ensure data bits are sent.
		TIMER = 0;
		while((*rx_portregister&rx_bitmask) == 0) {
			if(TIMER >= IE_FAILSAFE_LENGTH) {
				interrupts();
				return 5+i;
			}
		}

		//Read data bits.
		data[i] = readBits(8, true, true, ack_response);
		if(data[i] < 0) {
			interrupts();
			return 5+i;
		}
	}
	interrupts();

	uint8_t new_data[l];
	for(int i=0;i<sizeof(new_data);i+=1)
		new_data[i] = uint8_t(data[i]);

	ie_d->refreshIEData(l, sender, receiver, control, direct);
	ie_d->refreshIEData(new_data);

	return 0;
	#endif
}

void IEBusHandler::sendAcknowledgement(const uint16_t sender, const uint16_t receiver) {
	uint8_t ack_data[] = {0x80};
	IE_Message ack_msg(sizeof(ack_data), sender, receiver, 0xF, true);
	ack_msg.refreshIEData(ack_data);

	this->sendMessage(&ack_msg, true, true, false);
}