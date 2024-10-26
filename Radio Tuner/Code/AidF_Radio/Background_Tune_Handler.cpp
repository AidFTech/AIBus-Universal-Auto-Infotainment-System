#include "Background_Tune_Handler.h"

BackgroundTuneHandler::BackgroundTuneHandler(Si4735Controller* br_tuner, ParameterList* parameter_list) {
	this->br_tuner = br_tuner;
	this->parameter_list = parameter_list;

	this->br_parameter_list = new ParameterList();

	this->freq_list_vec.setStorage(freq_list, 0);
	this->station_name_vec.setStorage(station_name, 0);

	seek_timer = 0;
}

BackgroundTuneHandler::~BackgroundTuneHandler() {
	delete this->br_parameter_list;
}

//Basic loop function.
void BackgroundTuneHandler::loop() {
	br_parameter_list->hour = parameter_list->hour;
	br_parameter_list->min = parameter_list->min;
	br_parameter_list->offset = parameter_list->offset;

	if(parameter_list->minute_timer < 50000 || time_frequency <= 0) {
		if(time_station_set) {
			time_station_set = false;
			br_tuner->setFrequency(last_frequency);

			if(!time_set)
				time_frequency = 0;
			
			time_set = false;
		}
		
		br_tuner->getParameters(br_parameter_list, 0);

		if(station_seek) {
			const uint8_t rssi = br_tuner->getRSSI();
			if(rssi > max_rssi) {
				max_rssi = rssi;
				freq_list_vec.clear();
				station_name_vec.clear();
			}

			if(rssi >= max_rssi/2 && br_parameter_list->rds_station_name.length() > 0) { //List this station.
				const uint16_t freq = br_tuner->getFrequency();
				addFrequency(freq, br_parameter_list->rds_station_name);
			} else { //Remove this station.
				const uint16_t freq = br_tuner->getFrequency();
				int index = -1;
				for(int i=0;i<freq_list_vec.size();i+=1) {
					if(freq_list_vec.at(i) == freq)
						index = i;
					if(index >= 0)
						break;
				}

				if(index >= 0) {
					freq_list_vec.remove(index);
					station_name_vec.remove(index);
				}
			}

			if(seek_timer > SEEK_TIME) {
				seek_timer = 0;
				br_tuner->incrementFrequency();
			}
		}

		if(br_parameter_list->hour != parameter_list->hour || br_parameter_list->min != parameter_list->min || br_parameter_list->offset != parameter_list->offset) {
			parameter_list->hour = br_parameter_list->hour;
			parameter_list->min = br_parameter_list->min;
			parameter_list->offset = br_parameter_list->offset;

			if(this->time_frequency <= 0)
				this->time_frequency = br_tuner->getFrequency();
		}
	} else {
		if(!time_station_set) {
			this->last_frequency = br_tuner->getFrequency();
			br_tuner->setFrequency(time_frequency);
			time_station_set = true;
		}

		br_tuner->getParameters(br_parameter_list, 0);
		if(br_parameter_list->hour != parameter_list->hour || br_parameter_list->min != parameter_list->min || br_parameter_list->offset != parameter_list->offset) {
			parameter_list->hour = br_parameter_list->hour;
			parameter_list->min = br_parameter_list->min;
			parameter_list->offset = br_parameter_list->offset;

			time_set = true;
		}
	}
}

void BackgroundTuneHandler::addFrequency(const uint16_t freq, String station_name_str) {
	bool found = false;
	for(int i=0;i<freq_list_vec.size();i+=1) {
		if(freq_list_vec.at(i) == freq)
			found = true;
		if(found)
			break;
	}

	if(!found) {
		int index = -1;
		for(int i=0;i<freq_list_vec.size();i+=1) {
			if(freq_list_vec.at(i) > freq)
				index = i;
			if(index >= 0)
				break;
		}

		if(index >= 0) {
			if(freq_list_vec.size() < freq_list_vec.max_size()) {
				if(freq_list_vec.size() > 0) {
					freq_list_vec.push_back(freq_list_vec.at(freq_list_vec.size() - 1));
					station_name_vec.push_back(station_name_vec.at(station_name_vec.size() - 1));
				}

				for(int i=freq_list_vec.size() - 1;i > index;i-=1) {
					freq_list_vec.at(i) = freq_list_vec.at(i-1);
					station_name_vec.at(i) = station_name_vec.at(i-1);
				}

				station_name_vec.at(index) = station_name_str;
				freq_list_vec.at(index) = freq;
			}
		} else {
			freq_list_vec.push_back(freq);
			station_name_vec.push_back(station_name_str);
		}
	}
}

//Set whether the tuner should seek or add/remove stations, i.e. if a station list is open.
void BackgroundTuneHandler::setSeekMode(const bool seek) {
	this->station_seek = seek;
}

//Get the list of station names. Return the total count.
int BackgroundTuneHandler::getStationNames(String* names) {
	for(int i=0;i<station_name_vec.size();i+=1) {
		String new_station_name = String(freq_list_vec.at(i)/100) + "." + String(freq_list_vec.at(i)%100);
		new_station_name += "MHz: ";
		new_station_name += station_name_vec.at(i);

		names[i] = new_station_name;
	}

	return station_name_vec.size();
}