#include "AIBus.h"

AIData::AIData() : AIData(0, 0, 0) {

}

AIData::AIData(uint16_t newl, const uint8_t sender, const uint8_t receiver) {
	this->l = newl;
	this->data = new uint8_t[newl];
	this->sender = sender;
	this->receiver = receiver;
}

AIData::AIData(const AIData &copy) {
	this->l = copy.l;
	this->data = new uint8_t[this->l];
	this->sender = copy.sender;
	this->receiver = copy.receiver;
	
	for(uint16_t i = 0;i<this->l;i+=1)
		this->data[i] = copy.data[i];
}

AIData::~AIData() {
	delete[] this->data;
}

void AIData::refreshAIData(uint16_t newl, const uint8_t sender, const uint8_t receiver) {
	this->l = newl;
	delete[] this->data;
	
	this->data = new uint8_t[newl];
	this->sender = sender;
	this->receiver = receiver;
}

void AIData::refreshAIData(AIData newd) {
	this->refreshAIData(newd.l, newd.sender, newd.receiver);
	
	for(uint16_t i=0;i<newd.l;i+=1)
		this->data[i] = newd.data[i];
}

void AIData::refreshAIData(uint8_t* data) {
	for(uint8_t i=0;i<this->l;i+=1)
		this->data[i] = data[i];
}

uint8_t AIData::getChecksum() {
	uint8_t checksum = 0;
	checksum ^= this->sender;
	checksum ^= this->l+2;
	checksum ^= this->receiver;

	for(uint8_t i=0;i<this->l;i+=1)
		checksum ^= this->data[i];
	
	return checksum;
}

void AIData::getBytes(uint8_t* b) {
	b[0] = this->sender;
	b[1] = this->l + 2;
	b[2] = this->receiver;
	for(uint8_t i=0;i<this->l;i+=1)
		b[i+3] = this->data[i];
	b[this->l+3] = this->getChecksum();
}

bool checkValidity(uint8_t* b, const uint8_t l) {
	if(l<4)
		return false;
	if(b[1] != l-2)
		return false;

	uint16_t checksum = 0;
	
	for(int i=0;i<l-1;i+=1) {
		checksum ^= b[i];
	}

	if(checksum == b[l-1])
		return true;
	else
		return false;
}

bool checkDestination(AIData* ai_b, uint8_t dest_id) {
	if(ai_b->receiver == dest_id || ai_b->receiver == 0xFF)
		return true;
	else
		return false;
}

uint8_t getChecksum(AIData*ai_b) {	
	return ai_b->getChecksum();
}

void bytesToAIData(AIData* ai_b, uint8_t* b) {
	ai_b->l = b[1] - 2;

	delete[] ai_b->data;
	ai_b->data = new uint8_t[ai_b->l];

	ai_b->sender = b[0];
	ai_b->receiver = b[2];
	
	for(uint16_t i=0;i<ai_b->l;i+=1)
		ai_b->data[i] = b[i+2];
}
