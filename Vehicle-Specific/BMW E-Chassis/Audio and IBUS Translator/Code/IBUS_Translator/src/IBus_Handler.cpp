#include "IBus_Handler.h"

IBusHandler::IBusHandler(Stream* serial, const int8_t rx_pin, const uint8_t id) : AIBusHandler(serial, rx_pin, id) {
	//Same start procedure as AIBus handler.
}

//Write an IBus message to the OE system.
void IBusHandler::writeIBData(IBData* ib_d) {
	if(ib_d->l + 4 > 32) {
		IBData trimmed = *ib_d;
		trimmed.refreshAIData(32 - 4, trimmed.sender, trimmed.receiver);

		for(int i=0;i<trimmed.l && i<ib_d->l;i+=1)
			trimmed[i] = ib_d->data[i];

		writeIBData(&trimmed);
		return;
	}

	uint8_t data[ib_d->l + 4];
	ib_d->getBytes(data);

	int byte_count = ai_serial->available();
	elapsedMicros timer;
	while(timer < AI_DELAY_U) {
		while(micros() >= UINT32_MAX - AI_DELAY_U*2);
		if(this->rx_pin >= 0) {
			int8_t rx = digitalRead(this->rx_pin);
			elapsedMillis rx_timer;
			if(rx == LOW)
				timer = 0;
			while(rx == LOW) {
				if(rx_timer > 10) { //Problem.
					uint8_t ping[] = {0xF, 0xF0};
					ai_serial->write(ping, sizeof(ping));
					break;
				}
			}
		} else {
			const int new_byte_count = ai_serial->available();
			if(new_byte_count > byte_count) {
				byte_count = new_byte_count;
				timer = 0;
			}
		}
	}

	while(ai_serial->available() > 0)
		cachePending(ib_d->sender);

	ai_serial->write(data, ib_d->l + 4);
	ai_serial->flush();
}

//Read an IBus message into ib_d, return whether successful.
bool IBusHandler::readIBData(IBData* ib_d) {
	return readAIData(ib_d, true, false);
}

//Read an IBus message into ib_d, return whether successful. If cache is true, read from the data cache before the standard stream.
bool IBusHandler::readIBData(IBData* ib_d, const bool cache) {
	return readAIData(ib_d, cache, false);
}