#include "IEBus.h"

IE_Message::IE_Message() : IE_Message(0, 0, 0, 0, false) {
}

IE_Message::IE_Message(const uint16_t newl, const uint16_t sender, const uint16_t receiver, const uint8_t control, const bool direct) {
	this->l = newl;
	this->sender = sender;
	this->receiver = receiver;
	this->control = control;
	this->direct = direct;
	
	this->data = new uint8_t[newl];
}

IE_Message::IE_Message(const IE_Message &copy) {
	this->l = copy.l;
	this->data = new uint8_t[this->l];
	this->sender = copy.sender;
	this->receiver = copy.receiver;
	this->control = copy.control;
	this->direct = copy.direct;
	
	for(uint16_t i = 0;i<this->l;i+=1)
		this->data[i] = copy.data[i];
}

IE_Message::~IE_Message() {
	delete[] this->data;
}

void IE_Message::refreshIEData(const uint16_t newl) volatile {
	this->l = newl;
	delete[] this->data;
	
	this->data = new uint8_t[newl];
}

void IE_Message::refreshIEData(const uint16_t newl, const uint16_t sender, const uint16_t receiver, const uint8_t control, const bool direct) volatile {
	this->refreshIEData(newl);
	this->sender = sender;
	this->receiver = receiver;
	this->control = control;
	this->direct = direct;
}

void IE_Message::refreshIEData(uint8_t* data) {
	for(uint16_t i=0;i<this->l;i+=1)
		this->data[i] = data[i];
}

void IE_Message::refreshIEData(IE_Message newd) volatile {
	const uint16_t old_len = this->l;
	this->refreshIEData(newd.l, newd.sender, newd.receiver, newd.control, newd.direct);
	
	uint16_t l = this->l;
	if(old_len < l)
		l = old_len;
	
	for(uint16_t i=0;i<l;i+=1)
		this->data[i] = newd.data[i];
}

uint8_t IE_Message::getChecksum() {
	unsigned int parity_byte = 0xA0;
	
	parity_byte |= this->control; //TODO: Double check?
	
	parity_byte += (this->sender & 0xF0) >> 4;
	parity_byte += (this->receiver & 0xF0) >> 4;
	
	parity_byte += (this->sender & 0xF) << 4;
	parity_byte += (this->receiver & 0xF) << 4;
	
	parity_byte |= (this->sender & 0xFF) & (this->receiver & 0xFF);
	
	parity_byte += this->l+1;
	
	for(int i=0;i<this->l;i+=1)
		parity_byte += this->data[i];

	parity_byte &= 0xFF;

	return (uint8_t)parity_byte;
}

bool IE_Message::checkVaildity() {
	unsigned int parity_byte = 0xA0;
	
	parity_byte |= this->control;
	
	parity_byte += (this->sender & 0xF0) >> 4;
	parity_byte += (this->receiver & 0xF0) >> 4;
	
	parity_byte += (this->sender & 0xF) << 4;
	parity_byte += (this->receiver & 0xF) << 4;
	
	parity_byte |= (this->sender & 0xFF) & (this->receiver & 0xFF);
	
	parity_byte += this->l;
	
	for(int i=0;i<this->l-1;i+=1)
		parity_byte += this->data[i];

	parity_byte &= 0xFF;
	
	return ((uint8_t)parity_byte) == this->data[this->l-1];
}

uint8_t getChecksum(IE_Message* ie_b) {
	return ie_b->getChecksum();
}
