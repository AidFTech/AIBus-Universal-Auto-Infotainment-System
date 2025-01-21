#include "En_IEBus_Handler.h"
#include "AIBus_Handler.h"
#include "Handshakes.h"
#include "Parameter_List.h"
#include "Honda_Source_Handler.h"
#include "BCD_to_Dec.h"
#include "IE_CD_Status.h"

#include <stdint.h>
#include <Arduino.h>
#include <elapsedMillis.h>

#ifndef honda_imid_handler_h
#define honda_imid_handler_h

#define LINES 1
#define VOL_LIMIT 40

#define TEXT_MODE_BLANK 0 //Any CD text absent.
#define TEXT_MODE_WITH_TEXT 1 //CD text present.
#define TEXT_MODE_MP3 2 //MP3 ID3 text present.

class HondaIMIDHandler : public HondaSourceHandler {
public:
	HondaIMIDHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list);

	void loop();

	void interpretIMIDMessage(IE_Message* the_message);
	void readAIBusMessage(AIData* the_message);

	void writeScreenLayoutMessage();
	void writeVolumeLimitMessage();

	void writeTimeAndDayMessage(const uint8_t hour, const uint8_t minute, const uint8_t month, const uint8_t day, const uint16_t year);
	
	bool writeIMIDTextMessage(String text);
	bool setIMIDSource(const uint8_t source, const uint8_t subsource);

	void writeIMIDRadioMessage(const uint16_t frequency, const int8_t decimal, const uint8_t preset, const uint8_t stereo_mode);
	void writeIMIDRDSMessage(String msg);
	void writeIMIDCallsignMessage(String msg);
	void writeIMIDVolumeMessage(const uint8_t volume);

	void writeIMIDSiriusNumberMessage(const uint8_t preset, const uint16_t channel, const bool xm2);
	void writeIMIDSiriusTextMessage(const uint8_t position, String text);

	void writeIMIDCDCTrackMessage(const uint8_t disc, const uint8_t track, const uint8_t track_count, const uint16_t time, const uint8_t state1, const uint8_t state2);
	void writeIMIDCDCTextMessage(const uint8_t position, String text);

	uint16_t getMode();

private:
	uint16_t imid_mode = 0;
	uint8_t max_char = 12;

	uint8_t button_held = 0;
	
	//Radio paremeters:
	uint16_t frequency = 0;
	int8_t decimal = 0;
	uint8_t preset = 0, stereo_mode = 0;

	//CD parameters:
	uint8_t track = 0, disc = 0, track_count = 0;
	uint16_t timer = 0;
	uint8_t ai_cd_mode = 0, cd_text_mode = TEXT_MODE_BLANK;

	//XM parameters:
	bool xm2 = false;

	void sendSourceRequest(const uint8_t source);
	bool getTuningMessage(uint8_t* frequency_bytes, uint8_t* subsource_byte, uint8_t* stereo_byte, uint8_t* hd_byte);
};

#endif
