#include "Background_Tune_Handler.h"

BackgroundTuneHandler::BackgroundTuneHandler(Si4735Controller* br_tuner, ParameterList* parameters) {
	this->parameter_list = parameters;

	this->br_tuner = br_tuner;

	this->freq_list_vec.setStorage(freq_list, 0);
	this->station_name_vec.setStorage(station_name, 0);

	seek_timer = 0;
}

BackgroundTuneHandler::~BackgroundTuneHandler() {

}

//Loop function.
void BackgroundTuneHandler::loop() {
	br_tuner->loop();

	if(parameter_list->clock_freq >= 0 &&
		((!parameter_list->received_time_change_message && (parameter_list->minute_timer >= FIVE_SEC_LIMIT || parameter_list->minute_timer < 5000))
	 	|| (parameter_list->received_time_change_message && clock_timer > CLOCK_START_TIME))) { //Reset the clock as needed.
		if(!time_set) {
			time_set = true;

			last_checked_freq = br_tuner->getFrequency();
			last_seek_timer = seek_timer;

			br_tuner->setFrequency(parameter_list->clock_freq);
		}

		if(br_tuner->getRSSI() >= FM_STEREO_THRESH)
			br_tuner->getDateTime();
		else
			parameter_list->clock_freq = -1;

		if(clock_timer > CLOCK_END_TIME)
			clock_timer = 0;
	} else if(station_seek) {
		if(time_set) {
			time_set = false;

			seek_timer = last_seek_timer;
			br_tuner->setFrequency(last_checked_freq);
		}

		const uint16_t freq = br_tuner->getFrequency();

		const uint8_t rssi = br_tuner->getRSSI();
		const bool callsign_broadcast = br_tuner->getCallsign(&rds);

		//if(rssi >= FM_STEREO_THRESH && seek_timer_limit < 10000)
		//	seek_timer_limit = 10000;

		rssi_mean += rssi;
		rssi_count += 1;
		if(callsign_broadcast)
			rds_mean += 2;

		if(parameter_list->clock_freq < 0) {
			if(br_tuner->getDateTimePresent() && rssi >= FM_STEREO_THRESH)
				parameter_list->clock_freq = br_tuner->getFrequency();
		}
		
		if(seek_timer > seek_timer_limit) {
			if(rssi_count > 0) {
				if(rssi_mean/rssi_count >= FM_STEREO_THRESH && (rds.length() > 0 && rds_mean > 1)) { //List this station.
					addFrequency(freq, rds);
				} else { //Remove this station.
					int index = -1;
					for(int i=0;i<freq_list_vec.size();i+=1) {
						if(freq_list_vec.at(i) == freq) {
							index = i;
							break;
						}
					}

					if(index >= 0) {
						freq_list_vec.remove(index);
						station_name_vec.remove(index);
					}
				}
			}

			rssi_count = 0;
			rssi_mean = 0;
			rds_mean = 0;
			rds = "";

			seek_timer = 0;

			br_tuner->incrementFrequency();

			seek_timer_limit = SEEK_TIME;
		}
	}

}

//Add a frequency to the station list. If the fequency already exists, set the station name.
void BackgroundTuneHandler::addFrequency(const uint16_t freq, String station_name_str) {
	bool found = false;
	int freq_index = -1;
	for(int i=0;i<freq_list_vec.size();i+=1) {
		if(freq_list_vec.at(i) == freq) {
			found = true;
			freq_index = i;
		}
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

				freq_list_vec.at(index) = freq;
				station_name_vec.at(index) = station_name_str;
			}
		} else {
			freq_list_vec.push_back(freq);
			station_name_vec.push_back(station_name_str);
		}
	} else if(freq_index >= 0) {
		station_name_vec.at(freq_index) = station_name_str;
	}
}

//Set whether the tuner should seek or add/remove stations, i.e. if a station list is open.
void BackgroundTuneHandler::setSeekMode(const bool seek) {
	this->station_seek = seek;
	
	if(seek) {
		seek_timer = 0;
		seek_timer_limit = SEEK_TIME;
	}
}

//Get the list of station names. Return the total count.
int BackgroundTuneHandler::getStationNames(String* names) {
	return getStationNames(names, true);
}

//Get the list of station names without frequency prefixes. Return the total count.
int BackgroundTuneHandler::getRawStationNames(String* names) {
	return getStationNames(names, false);
}

//Get the list of station names. Return the total count.
int BackgroundTuneHandler::getStationNames(String* names, const bool freq) {
	for(int i=0;i<freq_list_vec.size();i+=1) {
		String new_station_name = "";
		
		if(freq) {
			new_station_name = String(freq_list_vec.at(i)/100) + "." + String(freq_list_vec.at(i)%100);
			new_station_name += " MHz: ";
		}
		
		if(station_name_vec.at(i).length() > 0)
			new_station_name += station_name_vec.at(i);

		names[i] = new_station_name;
	}

	return freq_list_vec.size();
}

//Get the frequency at index.
uint16_t BackgroundTuneHandler::getStationFrequency(const int index) {
	if(index < freq_list_vec.size())
		return freq_list_vec.at(index);
	else
		return 0;
}

//Set the station list.
void BackgroundTuneHandler::setStations(const int l, String* names, uint16_t* freqs) {
	if(l > MAXIMUM_FREQUENCY_COUNT)
		return;

	freq_list_vec.clear();
	station_name_vec.clear();

	for(int i=0;i<l;i+=1) {
		freq_list_vec.push_back(freqs[i]);
		station_name_vec.push_back(names[i]);
	}
}