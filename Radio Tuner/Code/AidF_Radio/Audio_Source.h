#include <stdint.h>
#include <Arduino.h>
#include <elapsedMillis.h>

#include "AIBus.h"
#include "AIBus_Handler.h"
#include "Parameter_List.h"
#include "Text_Handler.h"
#include "Si4735_AidF.h"
#include "Background_Tune_Handler.h"
#include "Volume_Handler.h"

#include "Radio_EEPROM.h"

#ifndef audio_source_h
#define audio_source_h

#define NO_MENU 0
#define SOURCE_MENU 1
#define PRESET_MENU 2
#define STATION_MENU 3
#define TONE_MENU 4

#define TONE_OPTION_BASS 0
#define TONE_OPTION_TREBLE 1
#define TONE_OPTION_BALANCE 2
#define TONE_OPTION_FADER 3
#define TONE_OPTION_SVC 4

struct AudioSource {
	uint8_t source_id, sub_id;
	String source_name;
};

class SourceHandler {
	public:
		AudioSource* source_list;
		uint16_t source_count;
	
		SourceHandler(AIBusHandler* ai_handler, Si4735Controller* tuner_main, BackgroundTuneHandler* tuner_background, ParameterList* parameter_list, VolumeHandler* volume_handler, uint16_t source_count);
		~SourceHandler();

		void sendRadioHandshake();
		bool handleAIBus(AIData* ai_d);
	
		uint8_t getCurrentSourceID();
		uint16_t getCurrentSource();
		void setSource(const uint16_t source);
		bool setSourceID(const uint8_t source);

		void checkSources();
		
		void setPower(const bool power);

		uint16_t getFilledSourceCount();
		uint16_t getFilledSources(AudioSource* source_list);
		uint16_t getSourceNames(String* source_list);
		uint16_t getSourceIDs(uint8_t* source_list);

		void incrementSource();
		void incrementSource(const bool direction);

	private:
		bool audio_on = false;
		uint8_t menu_open = NO_MENU;

		uint16_t current_source = 0;
		AIBusHandler* ai_handler;
		Si4735Controller* tuner_main;
		BackgroundTuneHandler *tuner_background;

		ParameterList* parameter_list;

		VolumeHandler* volume_handler;

		bool query = false; //True if the query function is active to prevent recursive loops.
		
		int getFirstOccurenceOf(const uint8_t source);
		int getFirstOccurenceOf(const uint8_t source, const uint16_t s);
		int getFirstAvailable();
		int getFirstAvailable(const uint16_t s);

		void createSubsource(const uint8_t id);
		void clearSubsources(const uint8_t id);

		bool sendSourceQuery(const uint8_t source);

		void clearMenu();
		bool createMenu(const String title, const int items);

		void createSourceMenu();
		void createPresetMenu(const uint8_t group);
		void createStationListMenu();

		void createToneMenu();
		void createToneMenuItem(const int item);

		void sendManualTuneMessage();
		
		void manualTuneIncrement(const bool up, const uint8_t steps);

		void setCurrentSource(const uint8_t id, const uint8_t sub_id);

		void handleSteeringControl(const uint8_t command, const uint8_t state);

		void savePreset(const uint16_t freq, const uint8_t preset, const uint8_t group);
};

#endif
