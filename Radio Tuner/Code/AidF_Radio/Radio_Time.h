#include <stdint.h>

#ifndef radio_time_h
#define radio_time_h

bool getTimePlausible(int16_t h, int16_t m, const int16_t lh, const int16_t lm, const int tol);

#endif