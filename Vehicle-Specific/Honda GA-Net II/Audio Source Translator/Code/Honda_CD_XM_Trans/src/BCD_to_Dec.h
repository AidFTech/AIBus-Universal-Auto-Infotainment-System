#include <stdint.h>

#ifndef bcd_to_dec_h
#define bcd_to_dec_h

#ifdef __cplusplus
extern "C" {
#endif
uint8_t getByteFromBCD(uint8_t b);
unsigned int getBCDFromByte(unsigned int b);
#ifdef __cplusplus
}
#endif

#endif