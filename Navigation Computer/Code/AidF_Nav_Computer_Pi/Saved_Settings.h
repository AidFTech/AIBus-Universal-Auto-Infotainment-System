#include <stdint.h>
#include <vector>

#include "Ini_Context.h"

#ifndef saved_settings_h
#define saved_settings_h

#define RESOLUTION_FILE "./AidF_Nav_Resolution.ini"
#define TIMEKEEPER_FILE "./AidF_Timekeeper.ini"

void getResolution(int* w, int* h);
void saveResolution(const int w, const int h);

void getTimekeepingParams(bool* display_12h, bool* auto_clock_set, uint8_t* timekeeper_id);
void saveTimekeepingParams(const bool display_12h, const bool auto_clock_set, const uint8_t timekeeper_id);

#endif