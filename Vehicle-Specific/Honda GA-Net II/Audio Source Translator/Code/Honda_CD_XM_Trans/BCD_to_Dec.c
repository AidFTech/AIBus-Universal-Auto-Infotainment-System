#include "BCD_to_Dec.h"

uint8_t getByteFromBCD(uint8_t b) {
	if((b&0xF) > 0x9)
		b&=0xF0;

	if((b&0xF0) > 0x90)
		b&=0xF;

	const uint8_t b_r = b%0x10 + 10*((b/0x10)%0xA0);
	
	return b_r;
}

unsigned int getBCDFromByte(unsigned int b) {
	unsigned int the_return = 0, shift = 0;
	while(b > 0) {
		the_return |= (b%10) << shift;
		b /= 10;
		shift += 4;
	}
	return the_return;
}
