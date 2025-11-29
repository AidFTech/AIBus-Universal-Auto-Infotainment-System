#include "Saved_Settings.h"

//Get a resolution from a saved file.
void getResolution(int* w, int* h) {
	std::vector<IniList> dimension_file = loadIniFile(RESOLUTION_FILE);

	bool dimension_loaded = false;

	for(int i=0;i<dimension_file.size();i+=1) {
		if(dimension_file[i].title.compare("AidF_Navigation_Screen_Dimensions") == 0) {
			int new_w = *w, new_h = *h;

			for(int n=0;n<dimension_file[i].l_n;n+=1) {
				if(dimension_file[i].num_vars[n].compare("w") == 0)
					new_w = dimension_file[i].num_values[n];
				else if(dimension_file[i].num_vars[n].compare("h") == 0)
					new_h = dimension_file[i].num_values[n];
			}

			*w = new_w;
			*h = new_h;
			dimension_loaded = true;
			break;
		}
	}

	if(!dimension_loaded) //Dimension file not found.
		saveResolution(*w, *h);
}

//Save a resolution to a file.
void saveResolution(const int w, const int h) {
	IniList dimension_file(2,0);
	
	dimension_file.title = "AidF_Navigation_Screen_Dimensions";
	dimension_file.num_vars[0] = "w";
	dimension_file.num_vars[1] = "h";

	dimension_file.num_values[0] = w;
	dimension_file.num_values[1] = h;

	std::vector<IniList> file_list(0);
	file_list.push_back(dimension_file);

	saveIniFile(RESOLUTION_FILE, file_list);
}

//Get timekeeping parameters from a saved file.
void getTimekeepingParams(bool* display_12h, bool* auto_clock_set, uint8_t* timekeeper_id) {
	std::vector<IniList> timekeeping_file = loadIniFile(TIMEKEEPER_FILE);

	bool timekeeper_loaded = false;

	for(int i=0;i<timekeeping_file.size();i+=1) {
		if(timekeeping_file[i].title.compare("AidF_Nav_Timekeepers") == 0) {
			uint8_t new_timekeeper = *timekeeper_id;
			bool new_12h = *display_12h, new_auto_clock = *auto_clock_set;

			for(int n=0;n<timekeeping_file[i].l_n;n+=1) {
				if(timekeeping_file[i].num_vars[n].compare("ID") == 0)
					new_timekeeper = (uint8_t)(timekeeping_file[i].num_values[n]&0xFF);
				else if(timekeeping_file[i].num_vars[n].compare("Auto") == 0)
					new_auto_clock = timekeeping_file[i].num_values[n] != 0;
				else if(timekeeping_file[i].num_vars[n].compare("Display12h") == 0)
					new_12h = timekeeping_file[i].num_values[n] != 0;
			}

			*display_12h = new_12h;
			*auto_clock_set = new_auto_clock;
			*timekeeper_id = new_timekeeper;

			timekeeper_loaded = true;
			break;
		}
	}

	if(!timekeeper_loaded)
		saveTimekeepingParams(*display_12h, *auto_clock_set, *timekeeper_id);
}

//Save the timekeeping parameters to a file.
void saveTimekeepingParams(const bool display_12h, const bool auto_clock_set, const uint8_t timekeeper_id) {
	IniList timekeeping_file(3, 0);

	timekeeping_file.title = "AidF_Nav_Timekeepers";

	timekeeping_file.num_vars[0] = "ID";
	timekeeping_file.num_values[0] = timekeeper_id;

	timekeeping_file.num_vars[1] = "Auto";
	timekeeping_file.num_values[1] = auto_clock_set ? 1 : 0;

	timekeeping_file.num_vars[2] = "Display12h";
	timekeeping_file.num_values[2] = display_12h ? 1 : 0;

	std::vector<IniList> file_list(0);
	file_list.push_back(timekeeping_file);

	saveIniFile(TIMEKEEPER_FILE, file_list);
}

//Get the map path.
std::string getMapPath() {
	std::vector<IniList> map_file = loadIniFile(MAP_FILE);
	std::string map_path = "";

	for(int i=0;i<map_file.size();i+=1) {
		if(map_file[i].title.compare("AidF_Nav_Map") == 0) {
			for(int n=0;n<map_file[i].l_s;n+=1) {
				if(map_file[i].str_vars[n].compare("Path") == 0) {
					map_path = map_file[i].str_values[n];
					break;
				}
			}
			if(!map_path.empty())
				break;
		}
	}

	return map_path;
}

//Get vehicle info parameters from a saved file.
void getVehicleInfoParams(bool* display_cruise, uint8_t* displayed_params, const int param_count) {
	std::vector<IniList> info_file = loadIniFile(INFO_FILE);

	for(int i=0;i<info_file.size();i+=1) {
		if(info_file[i].title.compare("VehicleInfo") == 0) {
			for(int n=0;n<info_file[i].l_n;n+=1) {
				if(info_file[i].num_vars[n].compare("DisplayCruise") == 0)
					*display_cruise = info_file[i].num_values[n] != 0;
				else if(info_file[i].num_vars[n].find("DisplayParam") == 0) {
					const std::string param_var = info_file[i].num_vars[n].substr(sizeof("DisplayParam") - 1);

					try {
						const int param = std::stoi(param_var);
						if(param > param_count)
							continue;

						displayed_params[param] = info_file[i].num_values[n];
					} catch(const std::invalid_argument &e) {

					} catch(const std::out_of_range &e) {

					}
				}
			}
		}
	}
}

//Save vehicle info parameters to a file.
void saveVehicleInfoParams(const bool display_cruise, uint8_t* displayed_params, const int param_count) {
	IniList info_file(param_count + 1, 0);

	info_file.title = "VehicleInfo";

	info_file.num_vars[0] = "DisplayCruise";
	info_file.num_values[0] = display_cruise ? 1 : 0;

	for(int i=0;i<param_count;i+=1) {
		info_file.num_vars[i+1] = "DisplayParam" + std::to_string(i);
		info_file.num_values[i+1] = displayed_params[i];
	}

	std::vector<IniList> file_list(0);
	file_list.push_back(info_file);

	saveIniFile(INFO_FILE, file_list);
}