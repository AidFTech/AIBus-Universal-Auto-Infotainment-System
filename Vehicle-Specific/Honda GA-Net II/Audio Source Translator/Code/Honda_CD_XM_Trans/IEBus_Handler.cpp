#include "IEBus_Handler.h"

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
}

void IEBusHandler::sendBit(const bool data) volatile {
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

void IEBusHandler::sendAckBit() volatile {
	TIMER = 0;

	*tx_portregister |= this->tx_bitmask;
	while(TIMER < IE_BIT_0_HOLD_ON_LENGTH);
	*tx_portregister &= ~this->tx_bitmask;
	while(TIMER < IE_NORMAL_BIT_1_LENGTH);
}

void IEBusHandler::sendStartBit() volatile {
	TIMER_SCALER = 3;
	TIMER = 0;
	*tx_portregister |= this->tx_bitmask;
	while(TIMER < START_LENGTH);

	*tx_portregister &= ~this->tx_bitmask;
	TIMER_SCALER = 2;
	TIMER = 0;
	while(TIMER < START_END_LENGTH);
}

void IEBusHandler::sendBits(const uint16_t data, const uint8_t size) volatile {
	this->sendBits(data, size, true, true);
}

void IEBusHandler::sendBits(const uint16_t data, const uint8_t size, const bool send_parity, const bool ack) volatile {
	bool parity = false;
	for(uint8_t i=0;i<size;i+=1) {
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

int8_t IEBusHandler::readBit() volatile {
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

int IEBusHandler::readBits(const uint8_t length, const bool with_parity, const bool with_ack, bool send_ack) volatile {
	int value = 0;
	bool parity = false;

	for(uint8_t i=0;i<length;i+=1) {
		value <<= 1;

		const int8_t data = readBit();
		if(data > 0) {
			value |= 1;
			parity = !parity;
		} else if(data < 0)
			return -1;
	}
	
	if(with_parity) {
		const bool data = readBit() > 0;
		if(data != parity && send_ack) {
			send_ack = false;
			value = -1;
		}
	}

	if(with_ack && send_ack) {
		TIMER = 0;
		while((*rx_portregister&rx_bitmask) == 0) { //TODO: Check.
			if(TIMER > IE_NORMAL_BIT_0_LENGTH)
				return -1;
		}
		sendAckBit();
	} else if(with_ack && !send_ack)
		readBit();

	return value;
}

void IEBusHandler::sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum) {
	sendMessage(ie_d, ack_response, checksum, true);
}

void IEBusHandler::sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum, const bool wait) volatile {
	noInterrupts();
	
	if(wait) {
		TIMER_SCALER = 3;
		TIMER = 0;
		while(TIMER < IE_DELAY) {
			if((*rx_portregister&rx_bitmask) != 0)
				TIMER = 0;
		}
		TIMER_SCALER = 2;
	}
	
	uint8_t direct = 0;
	if(ie_d->direct)
		direct = 1;
		
	const uint8_t cx = ie_d->getChecksum();

	this->sendStartBit();
	this->sendBit(ie_d->direct);
	this->sendBits(ie_d->sender, 12, true, false);
	this->sendBits(ie_d->receiver, 12);
	this->sendBits(ie_d->control, 4);

	if(checksum)
		this->sendBits(ie_d->l+1, 8);
	else
		this->sendBits(ie_d->l, 8);
	
	for(uint8_t i=0;i<ie_d->l;i+=1)
		this->sendBits(ie_d->data[i], 8);

	if(checksum)
		this->sendBits(cx, 8);

	interrupts();
}

int IEBusHandler::readMessage(IE_Message* ie_d, bool ack_response, const uint16_t id) volatile {
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

	//Read the "direct" bit.
	const uint8_t direct_int = readBits(1, false, false, ack_response);
	
	bool direct = false;
	if(direct_int == 1)
		direct = true;

	//Ensure sender bits are sent.
	TIMER = 0;
	while((*rx_portregister&rx_bitmask) == 0 && TIMER < START_COMP_LENGTH);
	if(TIMER >= START_COMP_LENGTH) {
		interrupts();
		return 1;
	}

	//Read sender bits.
	const int sender = readBits(12, true, false, ack_response);

	if(sender < 0) {
		interrupts();
		return 1;
	}

	//Ensure receiver bits are sent.
	TIMER = 0;
	while((*rx_portregister&rx_bitmask) == 0 && TIMER < START_COMP_LENGTH);
	if(TIMER >= START_COMP_LENGTH) {
		interrupts();
		return 2;
	}

	//Read receiver bits.
	const int receiver = readBits(12, true, false, ack_response);

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
	while((*rx_portregister&rx_bitmask) == 0 && TIMER < START_COMP_LENGTH);
	if(TIMER >= START_COMP_LENGTH) {
		interrupts();
		return 3;
	}

	//Read control bits.
	const int control = readBits(4, true, true, ack_response);

	if(control < 0) {
		interrupts();
		return 3;
	}

	//Ensure length bits are sent.
	TIMER = 0;
	while((*rx_portregister&rx_bitmask) == 0 && TIMER < START_COMP_LENGTH);
	if(TIMER >= START_COMP_LENGTH) {
		interrupts();
		return 4;
	}

	//Read length bits.
	const int l = readBits(8, true, true, ack_response);

	if(l < 0) {
		interrupts();
		return 4;
	}

	uint8_t data[l];

	for(unsigned int i=0;i<l;i+=1) {
		//Ensure data bits are sent.
		TIMER = 0;
		while((*rx_portregister&rx_bitmask) == 0 && TIMER < START_COMP_LENGTH);
		if(TIMER >= START_COMP_LENGTH) {
			interrupts();
			return 5+i;
		}

		//Read data bits.
		data[i] = readBits(8, true, true, ack_response);
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

void IEBusHandler::sendAcknowledgement(const uint16_t sender, const uint16_t receiver) volatile {
	uint8_t ack_data[] = {0x80};
	IE_Message ack_msg(sizeof(ack_data), sender, receiver, 0xF, true);
	ack_msg.refreshIEData(ack_data);

	this->sendMessage(&ack_msg, true, true, false);
}

bool IEBusHandler::getInputOn() {
	return((*rx_portregister&rx_bitmask) != 0);
}