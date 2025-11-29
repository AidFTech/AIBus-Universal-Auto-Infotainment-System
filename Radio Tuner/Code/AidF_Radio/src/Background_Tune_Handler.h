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
#define CLOCK_START_TIME 2000
#define CLOCK_END_TIME 5000
#define FIVE_SEC_LIMIT 55000

class BackgroundTuneHandler {
	public:
		BackgroundTuneHandler(Si4735Controller* br_tuner, ParameterList* parameters);
		~BackgroundTuneHandler();

		void loop();
		void setSeekMode(const bool seek);

		int getStationNames(String* names);
		int getRawStationNames(String* names);
		uint16_t getStationFrequency(const int index);

		void setStations(const int l, String* names, uint16_t* freqs);
	private:
		Si4735Controller* br_tuner;
		ParameterList* parameter_list;
		
		uint16_t freq_list[MAXIMUM_FREQUENCY_COUNT];
		Vector<uint16_t> freq_list_vec;

		String station_name[MAXIMUM_FREQUENCY_COUNT];
		Vector<String> station_name_vec;

		bool time_set = false; //True if the system is in time set mode.
		unsigned long last_seek_timer = 0;
		uint16_t last_checked_freq = 0;

		uint8_t max_rssi = 0;

		bool station_seek = true; //True if stations should be seeked.

		int rssi_mean = 0, rssi_count = 0, rds_mean = 0;
		String rds = "";

		elapsedMillis seek_timer;
		unsigned long seek_timer_limit = SEEK_TIME;

		elapsedMillis clock_timer;
		
		void addFrequency(const uint16_t freq, String station_name);
		int getStationNames(String* names, const bool freq);
};

#endif
