#include "BM83_AidF.h"

BM83::BM83(Stream* serial) {
	this->serial = serial;
}

void BM83::sendPowerOn() {
	uint8_t power_press[] = {0x2, 0x0, 0x51};
	uint8_t power_release[] = {0x2, 0x0, 0x52};
	
	serial->write(power_press, sizeof(power_press));
	serial->flush();
	serial->write(power_release, sizeof(power_release));
	serial->flush();
}