#include "Ini_Color_Preset.h"

//Save a color profile to a file.
void saveIniColorProfile(AidFColorProfile day_profile, AidFColorProfile night_profile, std::string name) {
	std::vector<IniList> color_file = loadIniFile(COLOR_FILE);

	IniList color_profile(18, 0);

	color_profile.title = name;

	color_profile.num_vars[0] = "Background1";
	color_profile.num_values[0] = day_profile.background>>8;

	color_profile.num_vars[1] = "Background2";
	color_profile.num_values[1] = day_profile.background2>>8;

	color_profile.num_vars[2] = "Text";
	color_profile.num_values[2] = day_profile.text>>8;

	color_profile.num_vars[3] = "Button";
	color_profile.num_values[3] = day_profile.button>>8;

	color_profile.num_vars[4] = "Selection";
	color_profile.num_values[4] = day_profile.selection>>8;

	color_profile.num_vars[5] = "Headerbar";
	color_profile.num_values[5] = day_profile.headerbar>>8;

	color_profile.num_vars[6] = "Outline";
	color_profile.num_values[6] = day_profile.outline>>8;

	color_profile.num_vars[7] = "Square";
	color_profile.num_values[7] = day_profile.square;

	color_profile.num_vars[8] = "Vertical";
	color_profile.num_values[8] = day_profile.vertical ? 1 : 0;

	color_profile.num_vars[9] = "NightBackground1";
	color_profile.num_values[9] = night_profile.background>>8;

	color_profile.num_vars[10] = "NightBackground2";
	color_profile.num_values[10] = night_profile.background2>>8;

	color_profile.num_vars[11] = "NightText";
	color_profile.num_values[11] = night_profile.text>>8;

	color_profile.num_vars[12] = "NightButton";
	color_profile.num_values[12] = night_profile.button>>8;

	color_profile.num_vars[13] = "NightSelection";
	color_profile.num_values[13] = night_profile.selection>>8;

	color_profile.num_vars[14] = "NightHeaderbar";
	color_profile.num_values[14] = night_profile.headerbar>>8;

	color_profile.num_vars[15] = "NightOutline";
	color_profile.num_values[15] = night_profile.outline>>8;

	color_profile.num_vars[16] = "NightSquare";
	color_profile.num_values[16] = night_profile.square;

	color_profile.num_vars[17] = "NightVertical";
	color_profile.num_values[17] = night_profile.vertical ? 1 : 0;

	std::vector<IniList> new_color_file(0);

	int ini_index = -1;
	for(int i=0;i<color_file.size();i+=1) {
		if(color_file.at(i).title.compare(name) == 0) {
			ini_index = i;
		}
		if(ini_index >= 0)
			break;
	}

	for(int i=0;i<color_file.size();i+=1) {
		if(ini_index < 0 || i != ini_index)
			new_color_file.push_back(color_file.at(i));
		else if(ini_index >= 0 && i == ini_index)
			new_color_file.push_back(color_profile);
	}

	if(ini_index < 0)
		new_color_file.push_back(color_profile);

	saveIniFile(COLOR_FILE, new_color_file);
}

//Load a color profile from a file. Return whether successful.
bool getIniColorProfile(AidFColorProfile* day_profile, AidFColorProfile* night_profile, std::string name) {
	std::vector<IniList> color_file = loadIniFile(COLOR_FILE);
	
	bool profile_found = false;

	for(int i=0;i<color_file.size();i+=1) {
		if(color_file[i].title.compare(name) != 0) 
			continue;
			
		profile_found = true;

		IniList color_ini = color_file[i];

		for(int n=0;n<color_ini.l_n;n+=1) {
			if(color_ini.num_vars[n].compare("Background1") == 0)
				day_profile->background = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("Background2") == 0)
				day_profile->background2 = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("Text") == 0)
				day_profile->text = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("Button") == 0)
				day_profile->button = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("Selection") == 0)
				day_profile->selection = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("Headerbar") == 0)
				day_profile->headerbar = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("Outline") == 0)
				day_profile->outline = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("Square") == 0)
				day_profile->square = (uint16_t)color_ini.num_values[n];
			else if(color_ini.num_vars[n].compare("Vertical") == 0)
				day_profile->vertical = color_ini.num_values[n] != 0;

			else if(color_ini.num_vars[n].compare("NightBackground1") == 0)
				night_profile->background = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("NightBackground2") == 0)
				night_profile->background2 = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("NightText") == 0)
				night_profile->text = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("NightButton") == 0)
				night_profile->button = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("NightSelection") == 0)
				night_profile->selection = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("NightHeaderbar") == 0)
				night_profile->headerbar = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("NightOutline") == 0)
				night_profile->outline = getRGBA(color_ini.num_values[n]);
			else if(color_ini.num_vars[n].compare("NightSquare") == 0)
				night_profile->square = (uint16_t)color_ini.num_values[n];
			else if(color_ini.num_vars[n].compare("NightVertical") == 0)
				night_profile->vertical = color_ini.num_values[n] != 0;
		}
		break;
	}
	
	return profile_found;
}

//Get a list of color sets available.
std::vector<std::string> getIniProfileList() {
	std::vector<std::string> profile_list(0);

	std::vector<IniList> profile_ini_list = loadIniFile(COLOR_FILE);

	for(int i=0;i<profile_ini_list.size();i+=1) {
		if(profile_ini_list.at(i).title.compare(ACTIVE_COLOR) != 0)
			profile_list.push_back(profile_ini_list.at(i).title);
	}

	return profile_list;
}

//Get the RGBA value of an RGB color.
uint32_t getRGBA(const int rgb_in) {
	uint32_t rgba = rgb_in << 8;
	rgba |= 0xFF;

	return rgba;
}
