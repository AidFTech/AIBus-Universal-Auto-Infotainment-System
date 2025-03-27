#include "Phone_Text_Handler.h"

PhoneTextHandler::PhoneTextHandler(AIBusHandler* ai_handler, ParameterList* parameters) {
	this->ai_handler = ai_handler;
	this->parameter_list = parameters;
}

//Write some text to the nav phone screen.
void PhoneTextHandler::writeNaviPhoneText(String text, const uint8_t group, const uint8_t area) {
	uint8_t text_data[text.length() + 3];

	text_data[0] = 0x21;
	text_data[1] = 0xA5;
	text_data[2] = ((group&0xF)<<4) | (area&0xF);

	for(int i=0;i<text.length();i+=1)
		text_data[i+3] = uint8_t(text.charAt(i));

	AIData text_msg(sizeof(text_data), ID_PHONE, ID_NAV_COMPUTER);
	text_msg.refreshAIData(text_data);

	ai_handler->writeAIData(&text_msg, parameter_list->navi_connected);
}

//Write some text to the nav audio screen.
void PhoneTextHandler::writeNaviAudioText(String text, const uint8_t group, const uint8_t area, const bool refresh) {
	uint8_t text_data[text.length() + 3];

	text_data[0] = 0x21;
	text_data[1] = 0x60 | (group&0xF);
	text_data[2] = area&0xF;

	if(refresh)
		text_data[1] |= 0x10;

	for(int i=0;i<text.length();i+=1)
		text_data[i+3] = uint8_t(text.charAt(i));

	AIData text_msg(sizeof(text_data), ID_PHONE, ID_NAV_COMPUTER);
	text_msg.refreshAIData(text_data);

	ai_handler->writeAIData(&text_msg, parameter_list->navi_connected);
}

//Create side buttons for in-call.
void PhoneTextHandler::createCallButtons() {
	for(int i=0;i<5;i+=1)
		this->writeNaviPhoneText("", 0xB, i);

	this->writeNaviPhoneText("End Call", 0xB, 0);
}

//Create side buttons for a connected phone.
void PhoneTextHandler::createConnectedButtons() {
	for(int i=0;i<5;i+=1)
		this->writeNaviPhoneText("", 0xB, i);

	this->writeNaviPhoneText("Dial Pad", 0xB, 0);
	this->writeNaviPhoneText("Contacts", 0xB, 1);
	this->writeNaviPhoneText("Recent Calls", 0xB, 2);

	this->writeNaviPhoneText("Disconnect", 0xB, 4);
}

//Create side buttons for when no phone is connected.
void PhoneTextHandler::createDisconnectedButtons(const bool pairing_on) {
	for(int i=0;i<5;i+=1)
		this->writeNaviPhoneText("", 0xB, i);

	if(pairing_on)
		this->writeNaviPhoneText("Disable Pairing", 0xB, 0);
	else
		this->writeNaviPhoneText("Connect", 0xB, 0);
	this->writeNaviPhoneText("Device List", 0xB, 1);
}

//Write the song time to the IMID and screen.
void PhoneTextHandler::writeTime(const long time) {
	if(time < 0) {
		writeNaviAudioText("-:--", 1, 0, true);
		//TODO: IMID time.
		return;
	}

	const int min = time/60, sec = time%60;

	String time_string = String(min) + ":";
	if(sec >= 10)
		time_string += String(sec);
	else
		time_string += "0" + String(sec);

	writeNaviAudioText(time_string, 1, 0, true);

	//TODO: IMID time.
}

//Write the song title to the screen and IMID.
void PhoneTextHandler::writeSongTitle(String title) {
	writeNaviAudioText(title, 0, 1, true);
	writeIMIDMetadata(title, FIELD_SONG_TITLE);
}

//Write the artist to the screen and IMID.
void PhoneTextHandler::writeArtist(String artist) {
	writeNaviAudioText(artist, 0, 2, true);
	writeIMIDMetadata(artist, FIELD_ARTIST);
}

//Write the album to the screen and IMID.
void PhoneTextHandler::writeAlbum(String album) {
	writeNaviAudioText(album, 0, 3, true);
	writeIMIDMetadata(album, FIELD_ALBUM);
}

//Write the phone name to the screen and IMID.
void PhoneTextHandler::writePhoneName(String phone_name) {
	writeNaviAudioText(phone_name, 1, 2, true);
	writeIMIDMetadata(phone_name, FIELD_PHONE_NAME);
}

//Write custom text to the IMID.
void PhoneTextHandler::writeIMIDText(String text, const uint8_t pos, const uint8_t line) {
	if(pos >= parameter_list->imid_char || line > parameter_list->imid_lines)
		return;

	uint8_t imid_data[4 + text.length()];
	imid_data[0] = 0x23;
	imid_data[1] = 0x60;
	imid_data[2] = pos;
	imid_data[3] = line;

	for(int i=0;i<text.length();i+=1)
		imid_data[i+4] = uint8_t(text.charAt(i));

	AIData imid_msg(sizeof(imid_data), ID_PHONE, ID_IMID_SCR);
	imid_msg.refreshAIData(imid_data);

	ai_handler->writeAIData(&imid_msg, parameter_list->imid_connected);
}

//Write an IMID metadata message. If no native BTA support, write as a line of text.
void PhoneTextHandler::writeIMIDMetadata(String text, const uint8_t field) {
	if(field < 1 || field > 4)
		return;

	if(parameter_list->imid_bta) {
		uint8_t imid_data[3 + text.length()];

		imid_data[0] = 0x23;
		imid_data[1] = 0x60 | field;
		imid_data[2] = 0x1;

		for(int i=0;i<text.length();i+=1)
			imid_data[i+3] = uint8_t(text.charAt(i));

		AIData imid_msg(sizeof(imid_data), ID_PHONE, ID_IMID_SCR);
		imid_msg.refreshAIData(imid_data);

		ai_handler->writeAIData(&imid_msg, parameter_list->imid_connected);
	} else {
		uint8_t line = 2;

		int imid_x = parameter_list->imid_char/2 - text.length()/2;
		if(imid_x < 0 || imid_x >= parameter_list->imid_char)
			imid_x = 0;

		switch(field) {
		case FIELD_SONG_TITLE:
			line = 2;
			break;
		case FIELD_ARTIST:
			line = 3;
			break;
		case FIELD_ALBUM:
			line = 4;
			break;
		default:
			return;
		}

		writeIMIDText(text, uint8_t(imid_x&0xFF), line);
	}
}