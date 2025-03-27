#include <Arduino.h>
#include <stdint.h>

#include "AIBus_Handler.h"
#include "Parameter_List.h"

#ifndef phone_text_handler_h
#define phone_text_handler_h

#define FIELD_SONG_TITLE 1
#define FIELD_ARTIST 2
#define FIELD_ALBUM 3
#define FIELD_PHONE_NAME 4

class PhoneTextHandler {
public: 
	PhoneTextHandler(AIBusHandler* ai_handler, ParameterList* parameters);

	void writeNaviPhoneText(String text, const uint8_t group, const uint8_t area);

	void createCallButtons();
	void createConnectedButtons();
	void createDisconnectedButtons(const bool pairing_on);

	void writeTime(const long time);
	void writeSongTitle(String title);
	void writeArtist(String artist);
	void writeAlbum(String album);
	void writePhoneName(String phone_name);
private:
	AIBusHandler* ai_handler;
	ParameterList* parameter_list;

	void writeNaviAudioText(String text, const uint8_t group, const uint8_t area, const bool refresh);

	void writeIMIDText(String text, const uint8_t pos, const uint8_t line);
	void writeIMIDMetadata(String text, const uint8_t field);
};

#endif