#include "En_IEBus_Handler.h"
#include "AIBus_Handler.h"
#include "Handshakes.h"
#include "BCD_to_Dec.h"
#include "Parameter_List.h"
#include "Honda_Source_Handler.h"
#include "Honda_IMID_Handler.h"
#include "IE_CD_Status.h"

#include <stdint.h>
#include <Arduino.h>
#include <elapsedMillis.h>
#include <string.h>

#ifndef honda_cd_handler
#define honda_cd_handler

#define BUTTON_FF 0x90
#define BUTTON_FR 0x91
#define BUTTON_TRACK_NEXT 0x92
#define BUTTON_TRACK_PREV 0x93
#define BUTTON_TRACK_CHANGE 0x94
#define BUTTON_UP 0x95
#define BUTTON_DOWN 0x96
#define BUTTON_DISC_CHANGE 0x97
#define BUTTON_PAUSE_RESUME 0x98
#define BUTTON_SCAN 0x9B
#define BUTTON_REPEAT 0x9C
#define BUTTON_RANDOM 0x9D

#define FRFF_INTERVAL 500
#define DISC_COUNT 6

#define TEXT_SONG 1
#define TEXT_ARTIST 2
#define TEXT_ALBUM 0
#define TEXT_FOLDER 3
#define TEXT_FILE 4
#define TEXT_NONE -1
#define TEXT_INFO_MESSAGE -2

#define TEXT_MODE_BLANK 0 //Any CD text absent.
#define TEXT_MODE_WITH_TEXT 1 //CD text present.
#define TEXT_MODE_MP3 2 //MP3 ID3 text present.

#define TEXT_REFRESH_TIMER 200
#define DISPLAY_INFO_TIMER 750
#define SCROLL_TIMER 300
#define SCROLL_TIMER_0 600
#define FUNCTION_CHANGE_TIMER 2500
#define SHORT_CHANGE_TIMER 5
#define MP3_REQUEST_TIMER 1500

#define MENU_SELECT_DISC 0x16

class HondaCDHandler : public HondaSourceHandler {
public:
	HondaCDHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list, HondaIMIDHandler *imid_handler);
	void loop();

	void interpretCDMessage(IE_Message* the_message);
	void readAIBusMessage(AIData* the_message);
	
	void sendSourceNameMessage();

	void sendNextTrackMessage();
	void sendPrevTrackMessage();
	void sendPauseMessage();
	void sendButtonByte(const uint8_t byte_msg);

private:
	uint8_t disc = 0, track = 0, folder_num = 0;
	int16_t timer = -1;

	uint8_t ai_cd_status = 0;

	uint8_t text_mode = TEXT_MODE_BLANK; //Whether or not CD text is present.

	char song_title[32];
	char artist[32];
	char album[32];
	char filename[32];
	char folder[32];

	uint8_t song_title_stage = 0, artist_stage = 0, album_stage = 0, folder_stage = 0, filename_stage = 0;

	bool autostart = false;

	//Timer to send all CD-text info to the IMID if it is connected, to prevent hanging.
	bool text_timer_enabled = false, text_timer_song = false, text_timer_artist = false, text_timer_album = false, text_timer_folder = false, text_timer_file = false;
	elapsedMillis text_timer, text_request_timer;

	//Timer to change the IMID screen to single CD to ensure text function.
	bool function_timer_enabled = false, use_function_timer = true;
	elapsedMillis function_timer;

	//Timer to scroll the info text.
	bool display_timer_enabled = false;
	elapsedMillis display_timer, scroll_timer;
	int8_t display_parameter = TEXT_NONE, scroll_state = 0;

	//Timer to change the disc.
	bool disc_change_timer_enabled = false;
	elapsedMillis disc_change_timer;
	uint8_t disc_change_disc = 0;
	
	//Timer to display info.
	bool info_change_enabled = false, text_timer_info_change = false;
	elapsedMillis info_change_timer;

	HondaIMIDHandler* imid_handler;

	bool fr = false, ff = false; //True if actively fast-searching a disc.
	elapsedMillis ff_fr_timer;
	
	void sendButtonMessage(const uint8_t button);
	void sendButtonMessage(const uint8_t button, const uint8_t state);
	void sendAICDStatusMessage(const uint8_t recipient);
	void sendAICDTextMessage(const uint8_t recipient, const uint8_t field);

	void startTextTimer(const uint8_t field);
	void sendTextRequest();

	void sendSourceNameMessage(const uint8_t id);

	//Send info to the nav computer, i.e. ID 1.
	void sendCDTrackMessage();
	void sendCDTrackMessage(const bool track);
	void sendCDTimeMessage();
	void sendCDTextMessage(const uint8_t field, const bool refresh);
	void sendCDLoadWaitMessage(const uint8_t message_type);

	//Send info to the IMID, i.e. ID 11.
	void sendCDIMIDTrackandTimeMessage();
	void sendCDIMIDTextMessage(const uint8_t field, String meta_text);

	void incrementInfo();
	void sendIMIDInfoMessage();
	void sendIMIDInfoMessage(const bool resend);
	void sendIMIDInfoMessage(String text);

	void sendIMIDInfoHeader(String text);

	void clearCDText(const bool song_title, const bool artist, const bool album, const bool folder, const bool file);
	void clearAICDText(const bool song_title, const bool artist, const bool album, const bool folder, const bool file);
	void clearExternalCDIMID(const bool album);

	void sendFunctionTextMessage();

	//Menu stuff:
	void createCDMainMenu();
	void createCDMainMenuOption(const uint8_t option);
	void createCDChangeDiscMenu();
	void createCDChangeDiscMenuOption(const uint8_t option);
};

#endif
