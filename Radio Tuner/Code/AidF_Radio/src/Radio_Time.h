#include <stdint.h>

#ifndef radio_time_h
#define radio_time_h

#define CHECK_LIMIT 5000

bool getTimePlausible(int16_t h, int16_t m, const int16_t lh, const int16_t lm, const int tol);
bool getMinuteInRange(const unsigned long minute_timer);

#endif