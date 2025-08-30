#include <stdint.h>
#include <vector>
#include <string>

#include "AidF_Color_Profile.h"
#include "Ini_Context.h"

#ifndef ini_color_preset_h
#define ini_color_preset_h

#define COLOR_FILE "./AidF_Color_Settings.ini"

#define ACTIVE_COLOR "Active_Color"

void saveIniColorProfile(AidFColorProfile day_profile, AidFColorProfile night_profile, std::string name);
bool getIniColorProfile(AidFColorProfile* day_profile, AidFColorProfile* night_profile, std::string name);
uint32_t getRGBA(const int rgb_in);

std::vector<std::string> getIniProfileList();

#endif
