#include <stdint.h>

#include <vector>
#include <string>

#include "Ini_Context.h"

#ifndef saved_settings_h
#define saved_settings_h

#define RESOLUTION_FILE "./AidF_Nav_Resolution.ini"
#define TIMEKEEPER_FILE "./AidF_Timekeeper.ini"
#define MAP_FILE "./AidF_Map.ini"
#define INFO_FILE "./AidF_Info.ini"

void getResolution(int* w, int* h);
void saveResolution(const int w, const int h);

void getTimekeepingParams(bool* display_12h, bool* auto_clock_set, uint8_t* timekeeper_id);
void saveTimekeepingParams(const bool display_12h, const bool auto_clock_set, const uint8_t timekeeper_id);

void getVehicleInfoParams(bool* display_cruise, bool* display_charge_assist, uint8_t* displayed_params, const int param_count);
void saveVehicleInfoParams(const bool display_cruise, const bool display_charge_assist, uint8_t* displayed_params, const int param_count);

std::string getMapPath();

#endif