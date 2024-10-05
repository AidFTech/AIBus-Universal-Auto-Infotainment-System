#include "En_IEBus_Handler.h"
#include "AIBus_Handler.h"
#include "Handshakes.h"
#include "Parameter_List.h"
#include "BCD_to_Dec.h"
#include "Honda_Source_Handler.h"
#include "Honda_IMID_Handler.h"

#include <stdint.h>
#include <Arduino.h>
#include <elapsedMillis.h>

#ifndef honda_xm_handler_h
#define honda_xm_handler_h

#define MENU_PRESET_LIST 0x1A
#define MENU_DIRECT_TUNE 0x1B

#define N_TEXT_NONE -1
#define N_CHANNEL_NAME 0
#define N_SONG_NAME 2
#define N_ARTIST_NAME 1
#define N_GENRE 3

#define XM_STATE_NORMAL 0x1
#define XM_STATE_ANTENNA 0xF0
#define XM_STATE_NOSIGNAL 0xFF

#define XM_TEXT_REFRESH_TIMER 100
#define XM_DISPLAY_INFO_TIMER 750
#define XM_SCROLL_TIMER 300
#define XM_SCROLL_TIMER_0 600

#define XM_FULL_TIMER 200

#define XM_STATION_TIMER 500
#define XM_QUERY_TIMER 1000
#define PRESET_QUERY_TIMER 120000

class HondaXMHandler : public HondaSourceHandler {
public:
	HondaXMHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list, HondaIMIDHandler* imid_handler);
	void interpretSiriusMessage(IE_Message* the_message);
	void readAIBusMessage(AIData* the_message);
	
	void loop();

	bool getXM2();
	
	void sendSourceNameMessage();
private:
	HondaIMIDHandler* imid_handler;

    String channel_name, song, artist, genre;
	uint16_t xm1_channel = 1, xm2_channel = 1;
	uint16_t* channel = &xm1_channel;

	uint16_t direct_num = 0;
	
	uint8_t xm_state = XM_STATE_NORMAL;
	
	uint16_t xm1_preset[6] = {1, 1, 1, 1, 1, 1};
	uint16_t xm2_preset[6] = {1, 1, 1, 1, 1, 1};

	String xm1_preset_name[6], xm2_preset_name[6];

	String tuner_id;
	
	uint8_t current_preset = 0;
	
	bool xm2 = false;
	bool full_xm = false; //True if the XM tuner does not split the presets into XM1/XM2.

	bool manual_tune = false;

	//Timer to send all XM info to the IMID if it is connected, to prevent hanging.
	bool text_timer_enabled = false, text_timer_song = false, text_timer_artist = false, text_timer_channel = false, text_timer_genre;
	elapsedMillis text_timer;

	//Timer to check whether the full XM tuner is connected.
	bool xm_timer_enabled = false;
	elapsedMillis xm_timer;

	//Timer to scroll the info text.
	bool display_timer_enabled = false;
	elapsedMillis display_timer, scroll_timer;
	int8_t display_parameter = N_TEXT_NONE, scroll_state = 0;

	//Timer to request text data.
	elapsedMillis request_timer = 0;

	//Timer to request preset data.
	elapsedMillis preset_request_timer = 0;
	uint8_t preset_request_increment = 1;
	unsigned int preset_request_wait = XM_QUERY_TIMER;
	bool preset_received = false;
	
	//Timer to request station data.
	elapsedMillis station_request_timer = 0;
	uint8_t station_request_increment = 0;
	unsigned int station_request_wait = XM_STATION_TIMER;
	bool station_received = false;

	void sendAINumberMessage(const uint8_t receiver);
	void sendAITextMessage(const uint8_t receiver, const uint8_t field);

	void setNumberDisplay();
	void setTextDisplay(const uint8_t field);

	void sendSourceNameMessage(const uint8_t id);

	void sendTextRequest();

	void setChannel(uint16_t channel);
	void changeToPreset(uint8_t preset);
	void savePreset(uint8_t preset);

	void sendIMIDNumberMessage();
	void setIMIDTextDisplay(const uint8_t field, String selected);
	void clearXMIMIDText();

	void sendIMIDInfoMessage();
	void sendIMIDInfoMessage(const bool resend);
	void sendIMIDInfoMessage(String text);

	void clearXMText(const bool song_title, const bool artist, const bool channel, const bool genre);
	void clearAIXMText(const bool song_title, const bool artist, const bool channel, const bool genre);
	
	void getPreset(const uint8_t preset);
	void getChannel(const uint16_t channel);

	//Menu stuff.
	void createXMMainMenu();
	void createXMMainMenuOption(const uint8_t option);

	void createXMPresetMenu();
	void createXMPresetMenuOption(const uint8_t option);

	void createXMDirectMenu();
	void createXMDirectMenuOption(const uint8_t option);
	void appendDirectNumber(const uint8_t num);

	void clearUpperField();
};

#endif
