#include <stdint.h>
#include <Arduino.h>
#include <elapsedMillis.h>
#include <Vector.h>

#include "En_IEBus_Handler.h"
#include "AIBus_Handler.h"
#include "Parameter_List.h"
#include "Handshakes.h"

#ifndef honda_source_handler_h
#define honda_source_handler_h

#define TIMEOUT_DEL 2000
#define IE_WAIT 10

#define IE_TRIES 25

#define TEXT_PING_TIMER 500
#define HANDSHAKE_WAIT 500

#define MODE_FLASH_TIMER 1500

#define IE_CACHE_SIZE 8

class HondaSourceHandler {
public:
	HondaSourceHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list);

	bool getEstablished();
	bool getSelected();

	void clearEstablished();

	bool sourceSendIEMessage(IE_Message* msg, const bool ack = true);

	virtual void requestControl();
	void sendMirrorMessage(String text, const uint8_t index, const bool refresh);
protected:
	EnIEBusHandler* ie_driver;
	AIBusHandler* ai_driver;

	ParameterList* parameter_list;

	bool source_established = false, source_sel = false, text_control = false;

	bool text_ping_timer_enabled = false;
	elapsedMillis text_ping_timer;

	uint16_t device_ie_id = 0xFF;
	uint8_t device_ai_id = 0x00;
	
	uint8_t* active_menu;
	bool open_audio_menu = false; //If true, open the audio settings menu upon menu exit.

	IE_Message ie_cache[IE_CACHE_SIZE];
	Vector<IE_Message> ie_cache_vec;

	bool use_ai_cache = false;

	bool sendHandshakeAckMessage();
	void sendIEAckMessage(const uint16_t recipient);
	bool getIEAckMessage(const uint16_t sender);
	bool getIEAckMessage(IE_Message* msg, const uint16_t sender);
	bool getIEAckMessageStrict(const uint16_t sender);
	
	void requestRadioControl();
	
	void clearExternalIMID();
	
	void startSettingsMenu(const uint8_t count, const uint8_t rows, const bool loop, String title);
	void startAudioMenu(const uint8_t count, const uint8_t rows, const bool loop, String title);
	void appendMenu(const uint8_t position, String text);
	void displayMenu(const uint8_t selected);
	void setMenuTitle(String title);

	void requestAudioSettingsMenu();

	void setNavHeader(String header);

	virtual void requestControl(const uint8_t id);
	virtual void listenForIEBus(const unsigned long wait = 500, const bool single_msg = true);

	virtual void handleAIBus(AIData* msg);
	virtual void handleIEBus(IE_Message* msg, const bool listen = true);
	virtual void handleIMIDAIBus(AIData* msg);

private:
	void startMenu(const bool audio, const uint8_t count, const uint8_t rows, const bool loop, String title);

	bool allow_recursive_aibus = true;
};

AIData getTextMessage(const uint8_t sender, String text, const uint8_t group, const uint8_t area, const bool refresh);

#endif
