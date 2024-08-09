#include <stdint.h>
#include <Arduino.h>
#include <elapsedMillis.h>

#include "En_IEBus_Handler.h"
#include "AIBus_Handler.h"
#include "Parameter_List.h"
#include "Handshakes.h"

#ifndef honda_source_handler_h
#define honda_source_handler_h

#define TIMEOUT_DEL 2000
#define IE_WAIT 10

#define HANDSHAKE_WAIT 500

class HondaSourceHandler {
public:
	HondaSourceHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list);

	bool getEstablished();
	bool getSelected();
protected:
	EnIEBusHandler* ie_driver;
	AIBusHandler* ai_driver;

	ParameterList* parameter_list;

	bool source_established = false, source_sel = false, text_control = false;

	uint16_t device_ie_id = 0xFF;
	uint8_t device_ai_id = 0x00;
	
	uint8_t* active_menu;

	bool sendHandshakeAckMessage();
	void sendIEAckMessage(const uint16_t recipient);
	bool getIEAckMessage(const uint16_t sender);
	bool getIEAckMessageStrict(const uint16_t sender);

	void sendAIAckMessage(const uint8_t receiver);
	
	void requestRadioControl();
	
	void clearExternalIMID();
	
	void startAudioMenu(const uint8_t count, const uint8_t rows, const bool loop, String title);
	void appendAudioMenu(const uint8_t position, String text);
	void displayAudioMenu(const uint8_t selected);
	void setMenuTitle(String title);
};

AIData getTextMessage(const uint8_t sender, String text, const uint8_t group, const uint8_t area, const bool refresh);

#endif
