#include <stdint.h>
#include <Arduino.h>

#include "AIBus_Handler.h"
#include "Parameter_List.h"

#ifndef __AVR__
#define __AVR__
#endif

#include "Text_Split.h"

#ifndef text_handler_h
#define text_handler_h

#define SUB_FM1 0
#define SUB_FM2 1
#define SUB_AM 2

class TextHandler {
public:
	TextHandler(AIBusHandler* ai_handler, ParameterList* parameter_list);

	void clearAllText();
	void clearAllText(const bool refresh);
	
	void setBlankHeader(String header);
	void sendSourceTextControl(const uint8_t recipient, const uint8_t source);
	
	void sendTunedFrequencyMessage(const uint16_t frequency, const bool mhz, const bool sub);
	void sendTunedFrequencyMessage(const uint8_t preset, const uint16_t frequency, const bool mhz, const bool sub);
	void sendStereoMessage(const bool stereo);
	
	void sendShortRDSMessage(String text);
	void sendLongRDSMessage(String text);

	void createRadioMenu(const uint8_t sub);
	void createPhoneWindow();

	void sendIMIDSourceMessage(const uint8_t source, const uint8_t subsource);
	void sendIMIDRDSMessage(String text);
	void sendIMIDRDSMessage(const uint16_t frequency, String text);
	void sendIMIDInfoMessage(String text);
	void sendIMIDCallsignMessage(String text);

	void sendTime();

private: 
	AIBusHandler* ai_handler;
	ParameterList* parameter_list;

	uint16_t last_frequency = 0;

	void sendIMIDFrequencyMessage(const uint16_t frequency, const uint8_t subsrc, const uint8_t preset);
};

AIData getTextMessage(String text, const uint8_t group, const uint8_t area);

#endif
