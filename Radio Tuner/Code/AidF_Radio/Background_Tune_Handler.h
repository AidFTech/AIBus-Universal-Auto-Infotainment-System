#include <stdint.h>
#include <Arduino.h>
#include <Vector.h>
#include <elapsedMillis.h>

#include "Si4735_AidF.h"
#include "Parameter_List.h"

#ifndef background_tune_handler_h
#define background_tune_handler_h

#define MAXIMUM_FREQUENCY_COUNT 32

#define SEEK_TIME 1000

class BackgroundTuneHandler {
	public:
		BackgroundTuneHandler(Si4735Controller* br_tuner, ParameterList* parameters);
		~BackgroundTuneHandler();

		void loop();
		void setSeekMode(const bool seek);

		int getStationNames(String* names);
		uint16_t getStationFrequency(const int index);
	private:
		Si4735Controller* br_tuner;
		ParameterList* parameter_list;
		
		uint16_t freq_list[MAXIMUM_FREQUENCY_COUNT];
		Vector<uint16_t> freq_list_vec;

		String station_name[MAXIMUM_FREQUENCY_COUNT];
		Vector<String> station_name_vec;

		uint16_t time_frequency = 0, last_frequency = 0;
		bool time_station_set = false, time_set = false;

		uint8_t max_rssi = 0;

		bool station_seek = true; //True if stations should be seeked.

		int rssi_mean = 0, rssi_count = 0, rds_mean = 0;
		String rds = "";

		elapsedMillis seek_timer;
		unsigned long seek_timer_limit = SEEK_TIME;
		
		void addFrequency(const uint16_t freq, String station_name);
};

#endif
