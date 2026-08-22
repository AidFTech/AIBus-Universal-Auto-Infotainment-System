#include "En_IEBus_Handler.h"
#include "AIBus_Handler.h"
#include "Handshakes.h"
#include "Parameter_List.h"
#include "Honda_Source_Handler.h"
#include "BCD_to_Dec.h"
#include "IE_CD_Status.h"
#include "Locale.h"

#include "Trans_EEPROM.h"

#include <stdint.h>
#include <Arduino.h>
#include <elapsedMillis.h>
#include <Vector.h>

#ifndef honda_imid_handler_h
#define honda_imid_handler_h

#define EXT_CHAR_LIMIT 10
#define INT_CHAR_LIMIT 12
#define LINES 1
#define VOL_LIMIT 40

#define IMID_CHANGE_TIMER 300
#define IMID_CHANGE_TIMER_LOCAL 200
#define FREQUENCY_CHANGE_TIMER 400

#define TEXT_MODE_BLANK 0 //No CD text.
#define TEXT_MODE_WITH_TEXT 1 //CD text present.
#define TEXT_MODE_MP3 2 //MP3 ID3 text present.

enum char_count_t : uint8_t {
	CHAR_COUNT_8,
	CHAR_COUNT_10,
	CHAR_COUNT_12
};

class HondaIMIDHandler : public HondaSourceHandler {
public:
	HondaIMIDHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list);

	void loop();

	void interpretIMIDMessage(IE_Message* the_message);
	void readAIBusMessage(AIData* the_message);

	unsigned long getIMIDChangeTimer();

	void writeScreenLayoutMessage();
	void writeVolumeLimitMessage();

	void writeTimeAndDayMessage(uint8_t hour, const uint8_t minute, const uint8_t month, const uint8_t day, const uint16_t year, const bool display_24h);
	
	bool writeIMIDTextMessage(String text);
	bool setIMIDSource(const uint8_t source, const uint8_t subsource);

	bool writeIMIDRadioMessage(const uint16_t frequency, const int8_t decimal, const uint8_t preset, const uint8_t stereo_mode, const bool acknowledge = true);
	bool writeIMIDRDSMessage(String msg);
	bool writeIMIDCallsignMessage(String msg);
	bool writeIMIDVolumeMessage(const uint8_t volume);

	bool writeIMIDSiriusNumberMessage(const uint8_t preset, const uint16_t channel, const bool xm2);
	bool writeIMIDSiriusTextMessage(const uint8_t position, String text);

	bool writeIMIDCDCTrackMessage(const uint8_t disc, const uint8_t track, const uint8_t track_count, const uint16_t time, const uint8_t state1, const uint8_t state2);
	bool writeIMIDCDCTextMessage(const uint8_t position, String text);
	bool clearIMIDCDText();

	uint16_t getMode();

private:
	uint16_t imid_mode = 0;
	uint8_t max_char = EXT_CHAR_LIMIT;

	elapsedMillis imid_change_timer = IMID_CHANGE_TIMER;

	uint8_t button_held = 0;

	uint8_t requestor_list[16];
	Vector<uint8_t> requestor_vec;

	AIData ai_cache[16];
	Vector<AIData> ai_cache_vec;
	int16_t imid_next_source = 0, imid_next_subsource = 0;

	elapsedMillis last_change;
	
	//Options:
	bool display_rds = true, display_volume = true, setting_changed = false;
	char_count_t char_count = CHAR_COUNT_10;

	//Radio parameters:
	uint16_t frequency = 0;
	int8_t decimal = 0;
	uint8_t preset = 0, stereo_mode = 0;
	bool rds = false;
	bool frequency_change_timer_enabled = false;
	elapsedMillis frequency_change_timer;

	//CD parameters:
	uint8_t track = 0, disc = 0, track_count = 0;
	uint16_t timer = 0;
	uint8_t ai_cd_mode = 0, cd_text_mode = TEXT_MODE_BLANK;

	//XM parameters:
	bool xm2 = false;

	void sendSourceRequest(const uint8_t source);
	bool setIMIDSource(const uint8_t source, const uint8_t subsource, const bool force_set);
	bool getTuningMessage(uint8_t* frequency_bytes, uint8_t* subsource_byte, uint8_t* stereo_byte, uint8_t* hd_byte);

	void setUSBMode();
	void clearUSBText(const uint8_t field);
	void setUSBText(const uint8_t field, String text);

	bool setBTMode();
	bool setBTModeNotConnected();
	bool setBTTimer(const long time);
	bool setBTText(const uint8_t field, String text);

	void writeScreenLayoutMessage(const uint8_t receiver);

	//Menu stuff:
	void createIMIDSettingsMenu();
	void createIMIDSettingsMenuOption(const unsigned int index);
	void createIMIDCharacterMenu();
	void createIMIDCharacterMenuOption(const unsigned int index);
};

#endif
