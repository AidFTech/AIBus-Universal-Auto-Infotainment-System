#include "En_IEBus_Handler.h"

EnIEBusHandler::EnIEBusHandler(const int8_t rx_pin, const int8_t tx_pin, EnAIBusHandler* ai_handler) : IEBusHandler(rx_pin, tx_pin) {
	this->ai_handler = ai_handler;
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
	IEBusHandler::sendMessage(ie_d, ack_response, checksum);
}

void EnIEBusHandler::sendMessage(IE_Message* ie_d, const bool ack_response, const bool checksum, const bool wait) volatile {
	/*uint8_t block_data[] = {0x64};
	AIData block_msg(sizeof(block_data), ie_d->receiver&0xFF, 0xFF);
	block_msg.refreshAIData(block_data);

	this->ai_handler->writeAIData(&block_msg);*/

	ai_handler->cacheAllPending();
	IEBusHandler::sendMessage(ie_d, ack_response, checksum, wait);
}

void EnIEBusHandler::addID(const uint8_t id) {
	if(this->ai_handler != NULL)
		this->ai_handler->addID(id);
}
