#include "../AIBus/AIBus.h"
#include "../AIBus/Client_AIBus_Handler.h"

#include "../Locale/Locale.h"

#include "../BT_Handler.h"
#include "../Parameter_List.h"
#include "../Text_Handler.h"

#include "../Text_Split.h"

#include <sdbus-c++/sdbus-c++.h>

#include <stdint.h>

#include <string>

#ifndef bt_audio_handler_h
#define bt_audio_handler_h

using namespace std;

#define SPLIT_TEXT_COUNT 12

enum playback_status_t : uint8_t {
	PLAYBACK_STATUS_STOPPED,
	PLAYBACK_STATUS_PAUSED,
	PLAYBACK_STATUS_PLAYING,
	PLAYBACK_STATUS_FR,
	PLAYBACK_STATUS_FF
};

enum repeat_random_status_t : uint8_t {
	RPTRND_NORMAL,
	RPTRND_REPEAT_T,
	RPTRND_REPEAT_A,
	RPTRND_REPEAT_G,
	RPTRND_RANDOM_A,
	RPTRND_RANDOM_G
};

enum bta_imid_scroll_t: int8_t {
	BTA_IMID_SCROLL_NONE = -1,
	BTA_IMID_SCROLL_TRACK,
	BTA_IMID_SCROLL_ARTIST,
	BTA_IMID_SCROLL_ALBUM,
};

//Bluetooth audio handler object.
class BTAudioHandler {
public:
	BTAudioHandler(ClientAIBusHandler* aibus_handler, BTHandler* bluetooth_handler, TextHandler* text_handler, ParameterList* parameter_list);

	void radioInit();
	void loop();

	void setTimer(unsigned long* timer);

	bool handleAIBusMessage(AIData* ai_msg);
	void refreshDeviceConnection();

	void refreshIMIDConnection();
private:
	ClientAIBusHandler* aibus_handler;

	BTHandler* bluetooth_handler;
	ParameterList* parameter_list;
	
	TextHandler* text_handler;

	string song_title = "", artist = "", album = "";
	playback_status_t playback_status = PLAYBACK_STATUS_STOPPED, last_status = PLAYBACK_STATUS_STOPPED;
	repeat_random_status_t repeat_random_status = RPTRND_NORMAL;
	uint32_t position = 0, track_number = 0;

	string split_text[SPLIT_TEXT_COUNT];

	unsigned long last_position_change = 0;
	unsigned long* timer = nullptr;

	bta_imid_scroll_t imid_scroll = BTA_IMID_SCROLL_NONE;
	uint8_t imid_scroll_position = 0;
	bool imid_split = true, imid_scroll_header = false, imid_scroll_wrap = false;
	unsigned long scroll_timer;

	bool display_header: 1, display_track: 1, display_artist: 1, display_album: 1, refresh_imid: 1;

	void sendNameMessage();

	void writeTitleMetadata();
	void writeArtistMetadata();
	void writeAlbumMetadata();
	void writePhoneMetadata();
	void writeAllMetadata();

	void handleBTProperties(map<string, Variant> properties);

	void writeIMIDTitle();
	void writeIMIDArtist();
	void writeIMIDAlbum();

	void writeStatus();
	void writeTrackNumber();
	void writePosition();

	void writeIMIDStatusandPosition();
	
	void incrementInfo();

	void writeFunctionButtons();

	void incRepeat();
	void incRandom();
	void setRepeat(const repeat_random_status_t status);
	void setRandom(const repeat_random_status_t status);
};

#endif