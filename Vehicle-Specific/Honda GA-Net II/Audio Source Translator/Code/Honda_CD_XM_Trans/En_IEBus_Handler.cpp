#include "En_IEBus_Handler.h"

EnIEBusHandler::EnIEBusHandler(const int8_t rx_pin, const int8_t tx_pin, EnAIBusHandler* ai_handler, const int8_t ai_block_pin) : IEBusHandler(rx_pin, tx_pin) {
	this->ai_handler = ai_handler;
	this->ai_block_pin = ai_block_pin;

	if(this->ai_block_pin >= 0) {
		pinMode(this->ai_block_pin, OUTPUT);
		digitalWrite(this->ai_block_pin, LOW);
	}
}

bool EnIEBusHandler::cacheAIBus() {
	if(this->ai_handler != NULL)
		return this->ai_handler->cacheAllPending();
	else
		return false;
}

void EnIEBusHandler::sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum) {
	/*uint8_t block_data[] = {0x64};
	AIData block_msg(sizeof(block_data), ie_d->receiver&0xFF, 0xFF);
	block_msg.refreshAIData(block_data);

	this->ai_handler->writeAIData(&block_msg);*/

	ai_handler->cacheAllPending();

	if(ai_block_pin >= 0)
		digitalWrite(ai_block_pin, HIGH);

	IEBusHandler::sendMessage(ie_d, ack_response, checksum);

	if(ai_block_pin >= 0)
		digitalWrite(ai_block_pin, LOW);
}

void EnIEBusHandler::sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum, const bool wait) volatile {
	/*uint8_t block_data[] = {0x64};
	AIData block_msg(sizeof(block_data), ie_d->receiver&0xFF, 0xFF);
	block_msg.refreshAIData(block_data);

	this->ai_handler->writeAIData(&block_msg);*/

	ai_handler->cacheAllPending();

	if(ai_block_pin >= 0)
		digitalWrite(ai_block_pin, HIGH);

	IEBusHandler::sendMessage(ie_d, ack_response, checksum, wait);

	if(ai_block_pin >= 0)
		digitalWrite(ai_block_pin, LOW);
}

int EnIEBusHandler::readMessage(IE_Message* ie_d, bool ack_response, const uint16_t id) volatile  {
	ai_handler->cacheAllPending();

	if(ai_block_pin >= 0)
		digitalWrite(ai_block_pin, HIGH);
	
	const int result = IEBusHandler::readMessage(ie_d, ack_response, id);

	if(ai_block_pin >= 0)
		digitalWrite(ai_block_pin, LOW);

	return result;
}

void EnIEBusHandler::addID(const uint8_t id) {
	if(this->ai_handler != NULL)
		this->ai_handler->addID(id);
}
