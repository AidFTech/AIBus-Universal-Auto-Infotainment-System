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
	if(ai_block_pin >= 0)
		digitalWrite(ai_block_pin, LOW);

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

int EnIEBusHandler::readMessage(IE_Message* ie_d, bool ack_response, const uint16_t id) volatile  {
	int avail = ai_handler->dataAvailable(false);
	elapsedMicros timer;

	while(timer < AI_DELAY_U) {
		if(ai_handler->dataAvailable(false) != avail || ai_handler->dataAvailable(false) > 0) {
			ai_handler->cacheAllPending();
			avail = ai_handler->dataAvailable(false);
			timer = 0;
		}
	}

	if(ai_block_pin >= 0)
		digitalWrite(ai_block_pin, HIGH);
	
	const int result = IEBusHandler::readMessage(ie_d, ack_response, id);

	ai_handler->waitForAIBus();
	if(ai_block_pin >= 0)
		digitalWrite(ai_block_pin, LOW);

	ai_handler->waitForAIBus();
	ai_handler->cacheAllPending();

	return result;
}

int EnIEBusHandler::readMessageStrict(IE_Message* ie_d, bool ack_response, const uint16_t id) volatile {
	return IEBusHandler::readMessage(ie_d, ack_response, id);
}

void EnIEBusHandler::addID(const uint8_t id) {
	if(this->ai_handler != NULL)
		this->ai_handler->addID(id);
}
