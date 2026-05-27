#include "BT_Audio_Handler.h"

BTAudioHandler::BTAudioHandler(ClientAIBusHandler* aibus_handler, BTHandler* bluetooth_handler, TextHandler* text_handler, ParameterList* parameter_list) {
	this->aibus_handler = aibus_handler;
	this->bluetooth_handler = bluetooth_handler;
	this->text_handler = text_handler;
	this->parameter_list = parameter_list;

	display_header = true;
	display_track = true;
	display_artist = true;
	display_album = true;
	refresh_imid = false;
}

//Initialize the radio communication.
void BTAudioHandler::radioInit() {
	uint8_t handshake_data[] = {0x1, 0x1, ID_PHONE};
	AIData handshake_msg(sizeof(handshake_data), ID_PHONE, ID_RADIO, handshake_data);
	aibus_handler->writeAIData(&handshake_msg, parameter_list->radio_connected);

	sendNameMessage();
}

//Audio handler loop function.
void BTAudioHandler::loop() {
	//Check for any property changes.
	if(bluetooth_handler->getChangedMediaProperties()->size() > 0) {
		map<string, Variant> changed_properties;
		map<string, Variant>* bt_properties = bluetooth_handler->getChangedMediaProperties();

		for(auto property : *bt_properties)
			changed_properties.emplace(property);

		bt_properties->clear();
		handleBTProperties(changed_properties);
	}

	//Scroll the IMID as required.
	if(timer != nullptr && timer != NULL && parameter_list->audio_selected && parameter_list->text_allowed) {
		unsigned long scroll_limit = 300;
		if(imid_scroll_wrap || imid_scroll_header)
			scroll_limit = 1000;
		else if(imid_split)
			scroll_limit = 3000;
		else if(imid_scroll_position == 0)
			scroll_limit = 750;

		if((imid_scroll >= 0 || imid_scroll_header) && parameter_list->imid_char > 0 && parameter_list->imid_lines > 0 && (*timer - scroll_timer > scroll_limit || refresh_imid)) {
			if(!refresh_imid) {
				scroll_timer = *timer;
				imid_scroll_position += 1;
			}
			refresh_imid = false;

			string scroll_param = "";

			if(imid_scroll_wrap || imid_scroll_header)
				imid_scroll_position = 0;

			if(imid_scroll_header)
				imid_scroll_header = false;

			if(imid_split) {
				if(imid_scroll_position >= SPLIT_TEXT_COUNT || split_text[imid_scroll_position].empty()) //Reset.
					imid_scroll_position = 0;

				scroll_param = split_text[imid_scroll_position];
			} else {
				scroll_param = split_text[0].substr(imid_scroll_position);
				if(scroll_param.length() >= parameter_list->imid_char) {
					scroll_param = scroll_param.substr(0, parameter_list->imid_char);
					imid_scroll_wrap = false;
				} else
					imid_scroll_wrap = true;
			}

			const uint8_t imid_y = parameter_list->imid_lines > 1 ? parameter_list->imid_lines/2 + 1 : 1;
				
			int imid_x = parameter_list->imid_char/2 - scroll_param.length()/2;
			if(imid_x < 0 || imid_x >= parameter_list->imid_char)
				imid_x = 0;

			uint8_t text_data[4 + scroll_param.length()];
			text_data[0] = 0x23;
			text_data[1] = 0x60;
			text_data[2] = uint8_t(imid_x&0xFF);
			text_data[3] = imid_y;
			for(int i=0;i<scroll_param.length();i+=1)
				text_data[i+4] = uint8_t(scroll_param[i]);

			AIData text_msg(sizeof(text_data), ID_PHONE, ID_IMID_SCR, text_data);
			aibus_handler->writeAIData(&text_msg);
		}
		
	}

	//Increment the timer as needed.
	if(timer != nullptr && timer != NULL &&
			bluetooth_handler->getConnectedDevice() != NULL &&
			bluetooth_handler->getConnectedDevice() != nullptr &&
			parameter_list->audio_selected &&
			*timer - last_position_change > 1000) {
		last_position_change = *timer;
		const uint32_t last_position = position;

		if(playback_status == PLAYBACK_STATUS_PLAYING)
			position += 1000;
		
		auto media_proxy = bluetooth_handler->getMediaProxy();

		if(media_proxy != NULL && media_proxy != nullptr) {
			try {
				auto properties = (*media_proxy)->getAllProperties().onInterface("org.bluez.MediaPlayer1");

				map<string, Variant> checked_properties;
				checked_properties.clear();

				for(auto property : properties)
					checked_properties.emplace(pair<string, Variant>(property.first, property.second));

				handleBTProperties(checked_properties);
			} catch(Error err) {
				//Device not connected. Continue.
			}
		}
		
		if(last_position != position)
			writePosition();
	}
}

//Set the timer pointer.
void BTAudioHandler::setTimer(unsigned long* timer) {
	this->timer = timer;
	last_position_change = *timer;
	scroll_timer = *timer;
}

//Process an AIBus message. Return whether the message is applicable.
bool BTAudioHandler::handleAIBusMessage(AIData* ai_msg) {
	if(ai_msg->sender == ID_RADIO && ai_msg->l >= 3 && ai_msg->data[0] == 0x40 && ai_msg->data[1] == 0x10) { //Function change.
		const uint8_t source = ai_msg->data[2];
		if(source == ID_PHONE) { //Selected!
			parameter_list->audio_selected = true;
			bluetooth_handler->sendMediaControl(MEDIA_CONTROL_PLAY); //TODO: Only in autostart mode.

			uint8_t screen_ctl_data[] = {0x77, ID_PHONE, 0x80};
			AIData screen_ctl_msg(sizeof(screen_ctl_data), ID_PHONE, ID_NAV_SCREEN, screen_ctl_data);
			aibus_handler->writeAIData(&screen_ctl_msg, parameter_list->screen_connected);
		} else { //Deselected.
			parameter_list->audio_selected = false;
			parameter_list->text_allowed = false;
			bluetooth_handler->sendMediaControl(MEDIA_CONTROL_STOP);

			if(source != 0x0) {
				uint8_t screen_ctl_data[] = {0x77, source, 0x80};
				AIData screen_ctl_msg(sizeof(screen_ctl_data), ID_PHONE, ID_NAV_SCREEN, screen_ctl_data);
				aibus_handler->writeAIData(&screen_ctl_msg, parameter_list->screen_connected);
			}
		}
		return true;
	} else if(ai_msg->sender == ID_RADIO && ai_msg->l >= 3 && ai_msg->data[0] == 0x40 && ai_msg->data[1] == 0x1) { //Text control change.
		const uint8_t source = ai_msg->data[2];
		if(source == ID_PHONE) { //Selected!
			parameter_list->text_allowed = true;
			writeAllMetadata();
			writeFunctionButtons();
		}
		return true;
	} else if(ai_msg->sender == ID_NAV_SCREEN) {
		if(ai_msg->l >= 3 && ai_msg->data[0] == 0x30) { //Button press.
			const uint8_t button = ai_msg->data[1], state = (ai_msg->data[2]&0xC0)>>6;
			if(button == 0x25 && state == 2)  //Skip forward.
				bluetooth_handler->sendMediaControl(MEDIA_CONTROL_NEXT);
			else if(button == 0x24 && state == 2) //Skip back.
				bluetooth_handler->sendMediaControl(MEDIA_CONTROL_PREVIOUS);
			else if(button == 0x11 && state == 2) //Repeat.
				incRepeat();
			else if(button == 0x12 && state == 2) //Random.
				incRandom();
			else if(button == 0x13) { //FR.
				if(state == 0) {
					if(this->playback_status != PLAYBACK_STATUS_FR) {
						last_status = this->playback_status;
						bluetooth_handler->sendMediaControl(MEDIA_CONTROL_FR);
					}
				} else if(state == 2) {
					switch(last_status) {
					case PLAYBACK_STATUS_STOPPED:
						bluetooth_handler->sendMediaControl(MEDIA_CONTROL_STOP);
						break;
					case PLAYBACK_STATUS_PAUSED:
						bluetooth_handler->sendMediaControl(MEDIA_CONTROL_PAUSE);
						break;
					default:
						bluetooth_handler->sendMediaControl(MEDIA_CONTROL_PLAY);
						break;
					}
				}
			} else if(button == 0x14) { //FF.
				if(state == 0) {
					if(this->playback_status != PLAYBACK_STATUS_FR) {
						last_status = this->playback_status;
						bluetooth_handler->sendMediaControl(MEDIA_CONTROL_FF);
					}
				} else if(state == 2) {
					switch(last_status) {
					case PLAYBACK_STATUS_STOPPED:
						bluetooth_handler->sendMediaControl(MEDIA_CONTROL_STOP);
						break;
					case PLAYBACK_STATUS_PAUSED:
						bluetooth_handler->sendMediaControl(MEDIA_CONTROL_PAUSE);
						break;
					default:
						bluetooth_handler->sendMediaControl(MEDIA_CONTROL_PLAY);
						break;
					}
				}
			} else if(button == 0x53 && state == 2) { //Info.
				incrementInfo();
			}
			
			return true;
		}
		return false;
	} else return false;
}

//Write metadata to the IMID.
void BTAudioHandler::refreshIMIDConnection() {
	if(!parameter_list->audio_selected || !parameter_list->text_allowed)
		return;

	if(parameter_list->imid_native_phone && imid_scroll < 0) {
		text_handler->writeMetadata(song_title, ID_IMID_SCR, 1);
		text_handler->writeMetadata(artist, ID_IMID_SCR, 2);
		text_handler->writeMetadata(album, ID_IMID_SCR, 3);
		
		BTADevice* device = bluetooth_handler->getConnectedDevice();
		if(device != nullptr && device != NULL) 
			text_handler->writeMetadata(device->getDeviceName(), ID_IMID_SCR, 4);
	}
}

//Refresh the device connection.
void BTAudioHandler::refreshDeviceConnection() {
	if(!parameter_list->audio_selected || !parameter_list->text_allowed)
		return;

	string device_name = "";
	BTADevice* device = bluetooth_handler->getConnectedDevice();
	if(device != nullptr && device != NULL)
		device_name = device->getDeviceName();
	else {
		song_title = "";
		artist = "";
		album = "";
		writeTitleMetadata();
		writeArtistMetadata();
		writeAlbumMetadata();

		position = 0;
		playback_status = PLAYBACK_STATUS_STOPPED;
		writePosition();
		writeStatus();
	}

	text_handler->writeAudioWindowText(device_name, 1, 2);
	text_handler->writeMetadata(device_name, ID_RADIO, 4);
	//TODO: Special IMID function for device name.
}

//Send the name message to the radio.
void BTAudioHandler::sendNameMessage() {
	const string name = "Bluetooth";

	uint8_t name_data[name.size() + 3];
	name_data[0] = 0x1;
	name_data[1] = 0x22;
	name_data[2] = 0x0;

	for(int i=0;i<name.size();i+=1)
		name_data[i+3] = uint8_t(name[i]);

	AIData name_msg(sizeof(name_data), ID_PHONE, ID_RADIO, name_data);
	aibus_handler->writeAIData(&name_msg, parameter_list->radio_connected);

	name_msg[1] = 0x23;
	aibus_handler->writeAIData(&name_msg, parameter_list->radio_connected);
}

//Write the song title metadata to the computer, radio, and IMID.
void BTAudioHandler::writeTitleMetadata() {
	if(!parameter_list->audio_selected)
		return;

	text_handler->writeMetadata(song_title, ID_RADIO, 1);

	if(!parameter_list->text_allowed)
		return;

	text_handler->writeAudioWindowText(song_title, 0, 1);

	writeIMIDTitle();
}

//Write the artist metadata to the computer, radio, and IMID.
void BTAudioHandler::writeArtistMetadata() {
	if(!parameter_list->audio_selected)
		return;

	text_handler->writeMetadata(artist, ID_RADIO, 2);

	if(!parameter_list->text_allowed)
		return;

	text_handler->writeAudioWindowText(artist, 0, 2);

	writeIMIDArtist();
}

//Write the album metadata to the computer, radio, and IMID.
void BTAudioHandler::writeAlbumMetadata() {
	if(!parameter_list->audio_selected)
		return;

	text_handler->writeMetadata(album, ID_RADIO, 3);

	if(!parameter_list->text_allowed)
		return;

	text_handler->writeAudioWindowText(album, 0, 3);

	writeIMIDAlbum();
}

//Write the phone name metadata to the computer, radio, and IMID.
void BTAudioHandler::writePhoneMetadata() {
	if(!parameter_list->audio_selected)
		return;

	if(bluetooth_handler->getConnectedDevice() == nullptr || bluetooth_handler->getConnectedDevice() == NULL)
		return;

	text_handler->writeMetadata(bluetooth_handler->getConnectedDevice()->getDeviceName(), ID_RADIO, 4);

	if(!parameter_list->text_allowed)
		return;

	text_handler->writeAudioWindowText(bluetooth_handler->getConnectedDevice()->getDeviceName(), 1, 2);

	if(parameter_list->imid_native_phone && imid_scroll < 0)
		text_handler->writeMetadata(bluetooth_handler->getConnectedDevice()->getDeviceName(), ID_IMID_SCR, 4);
}

//Handle metadata sent over SD-Bus.
void BTAudioHandler::handleBTProperties(map<string, Variant> properties) {
	const string last_title = song_title;
	const string last_artist = artist;
	const string last_album = album;

	const playback_status_t last_status = playback_status;
	const repeat_random_status_t last_repeat = repeat_random_status;

	const uint32_t last_position = position, last_track = track_number;

	for(auto property : properties) {
		if(property.first.compare("Track") == 0 && property.second.containsValueOfType<map<string, Variant>>()) { //Track info.
			for(auto element : property.second.get<map<string, Variant>>()) {
				if(element.first.compare("Title") == 0 && element.second.containsValueOfType<string>())
					this->song_title = element.second.get<string>();
				else if(element.first.compare("Artist") == 0 && element.second.containsValueOfType<string>())
					this->artist = element.second.get<string>();
				else if(element.first.compare("Album") == 0 && element.second.containsValueOfType<string>())
					this->album = element.second.get<string>();
				else if(element.first.compare("TrackNumber") == 0 && element.second.containsValueOfType<uint32_t>())
					this->track_number = element.second.get<uint32_t>();
			}
		} else if(property.first.compare("Status") == 0 && property.second.containsValueOfType<string>()) { //Playback status.
			const string status = property.second.get<string>();
			if(status.compare("paused") == 0)
				this->playback_status = PLAYBACK_STATUS_PAUSED;
			else if(status.compare("playing") == 0)
				this->playback_status = PLAYBACK_STATUS_PLAYING;
			else if(status.compare("forward-seek") == 0)
				this->playback_status = PLAYBACK_STATUS_FF;
			else if(status.compare("reverse-seek") == 0)
				this->playback_status = PLAYBACK_STATUS_FR;
			else
				this->playback_status = PLAYBACK_STATUS_STOPPED;

			last_position_change = *timer;
		} else if(property.first.compare("Position") == 0 && property.second.containsValueOfType<uint32_t>()) { //Position.
			this->position = property.second.get<uint32_t>();
			if(timer != nullptr && timer != NULL)
				last_position_change = *timer + this->position%1000;
		} else if(property.first.compare("Repeat") == 0 && property.second.containsValueOfType<string>()) { //Repeat.
			const string status = property.second.get<string>();
			if(status.compare("singletrack") == 0)
				this->repeat_random_status = RPTRND_REPEAT_T;
			else if(status.compare("alltracks") == 0)
				this->repeat_random_status = RPTRND_REPEAT_A;
			else if(status.compare("group") == 0)
				this->repeat_random_status = RPTRND_REPEAT_G;
			else if(this->repeat_random_status != RPTRND_RANDOM_A && this->repeat_random_status != RPTRND_RANDOM_G)
				this->repeat_random_status = RPTRND_NORMAL;
		} else if(property.first.compare("Shuffle") == 0 && property.second.containsValueOfType<string>()) { //Random.
			const string status = property.second.get<string>();
			if(status.compare("alltracks") == 0)
				this->repeat_random_status = RPTRND_RANDOM_A;
			else if(status.compare("group") == 0)
				this->repeat_random_status = RPTRND_RANDOM_G;
			else if(this->repeat_random_status != RPTRND_REPEAT_A && this->repeat_random_status != RPTRND_REPEAT_G && this->repeat_random_status != RPTRND_REPEAT_T)
				this->repeat_random_status = RPTRND_NORMAL;
		}
	}

	if(last_title.compare(song_title) != 0)
		writeTitleMetadata();
	if(last_artist.compare(artist) != 0)
		writeArtistMetadata();
	if(last_album.compare(album) != 0)
		writeAlbumMetadata();

	if(last_status != this->playback_status || last_repeat != this->repeat_random_status)
		writeStatus();

	if(last_track != track_number)
		writeTrackNumber();

	if(last_position/1000 != this->position/1000)
		writePosition();
}

//Write the song title to the IMID.
void BTAudioHandler::writeIMIDTitle() {
	if(!parameter_list->audio_selected || !parameter_list->text_allowed)
		return;

	if(parameter_list->imid_lines < 1 && !parameter_list->imid_native_phone)
		return;

	if(parameter_list->imid_native_phone && imid_scroll < 0)
		text_handler->writeMetadata(song_title, ID_IMID_SCR, 1);
	else if(parameter_list->imid_char > 0 && parameter_list->imid_lines > 0 && imid_scroll < 0 && display_track) {
		string imid_text = song_title;

		if(imid_text.length() > parameter_list->imid_char)
			imid_text = imid_text.substr(0, parameter_list->imid_char);

		int imid_x = parameter_list->imid_char/2 - imid_text.length()/2;
		if(imid_x < 0 || imid_x >= parameter_list->imid_char)
			imid_x = 0;

		int imid_y = 1;
		if(display_header)
			imid_y += 1;

		if(imid_y <= parameter_list->imid_lines) {
			uint8_t imid_data[imid_text.length() + 4];
			imid_data[0] = 0x23;
			imid_data[1] = 0x60;
			imid_data[2] = uint8_t(imid_x&0xFF);
			imid_data[3] = uint8_t(imid_y&0xFF);
			for(int i=0;i<imid_text.length();i+=1)
				imid_data[i+4] = uint8_t(imid_text[i]);

			AIData imid_msg(sizeof(imid_data), ID_PHONE, ID_IMID_SCR, imid_data);
			aibus_handler->writeAIData(&imid_msg);
		}
	} else if(parameter_list->imid_char > 0 && parameter_list->imid_lines > 0 && imid_scroll == BTA_IMID_SCROLL_TRACK) {
		for(int i=0;i<SPLIT_TEXT_COUNT;i+=1)
			split_text[i] = "";

		if(imid_split) {
			splitText(parameter_list->imid_char, song_title, split_text, SPLIT_TEXT_COUNT);
		} else {
			split_text[0] = song_title;
		}
		imid_scroll_position = 0;
		scroll_timer = *timer;

		if(!imid_scroll_header)
			refresh_imid = true;
	}
}

//Write the artist to the IMID.
void BTAudioHandler::writeIMIDArtist() {
	if(!parameter_list->audio_selected || !parameter_list->text_allowed)
		return;

	if(parameter_list->imid_lines < 1 && !parameter_list->imid_native_phone)
		return;

	if(parameter_list->imid_native_phone && imid_scroll < 0)
		text_handler->writeMetadata(artist, ID_IMID_SCR, 2);
	else if(parameter_list->imid_char > 0 && parameter_list->imid_lines > 0 && imid_scroll < 0 && display_artist) {
		string imid_text = artist;

		if(imid_text.length() > parameter_list->imid_char)
			imid_text = imid_text.substr(0, parameter_list->imid_char);

		int imid_x = parameter_list->imid_char/2 - imid_text.length()/2;
		if(imid_x < 0 || imid_x >= parameter_list->imid_char)
			imid_x = 0;

		int imid_y = 1;
		if(display_header)
			imid_y += 1;
		if(display_track)
			imid_y += 1;

		if(imid_y <= parameter_list->imid_lines) {
			uint8_t imid_data[imid_text.length() + 4];
			imid_data[0] = 0x23;
			imid_data[1] = 0x60;
			imid_data[2] = uint8_t(imid_x&0xFF);
			imid_data[3] = uint8_t(imid_y&0xFF);
			for(int i=0;i<imid_text.length();i+=1)
				imid_data[i+4] = uint8_t(imid_text[i]);

			AIData imid_msg(sizeof(imid_data), ID_PHONE, ID_IMID_SCR, imid_data);
			aibus_handler->writeAIData(&imid_msg);
		}
	} else if(parameter_list->imid_char > 0 && parameter_list->imid_lines > 0 && imid_scroll == BTA_IMID_SCROLL_ARTIST) {
		for(int i=0;i<SPLIT_TEXT_COUNT;i+=1)
			split_text[i] = "";

		if(imid_split) {
			splitText(parameter_list->imid_char, artist, split_text, SPLIT_TEXT_COUNT);
		} else {
			split_text[0] = artist;
		}
		imid_scroll_position = 0;
		scroll_timer = *timer;

		if(!imid_scroll_header)
			refresh_imid = true;
	}
}

//Write the album to the IMID.
void BTAudioHandler::writeIMIDAlbum() {
	if(!parameter_list->audio_selected || !parameter_list->text_allowed)
		return;

	if(parameter_list->imid_lines < 1 && !parameter_list->imid_native_phone)
		return;
	
	if(parameter_list->imid_native_phone && imid_scroll < 0)
		text_handler->writeMetadata(album, ID_IMID_SCR, 3);
	else if(parameter_list->imid_char > 0 && parameter_list->imid_lines > 0 && imid_scroll < 0 && display_album) {
		string imid_text = album;

		if(imid_text.length() > parameter_list->imid_char)
			imid_text = imid_text.substr(0, parameter_list->imid_char);

		int imid_x = parameter_list->imid_char/2 - imid_text.length()/2;
		if(imid_x < 0 || imid_x >= parameter_list->imid_char)
			imid_x = 0;

		int imid_y = 1;
		if(display_header)
			imid_y += 1;
		if(display_track)
			imid_y += 1;
		if(display_artist)
			imid_y += 1;

		if(imid_y <= parameter_list->imid_lines) {
			uint8_t imid_data[imid_text.length() + 4];
			imid_data[0] = 0x23;
			imid_data[1] = 0x60;
			imid_data[2] = uint8_t(imid_x&0xFF);
			imid_data[3] = uint8_t(imid_y&0xFF);
			for(int i=0;i<imid_text.length();i+=1)
				imid_data[i+4] = uint8_t(imid_text[i]);

			AIData imid_msg(sizeof(imid_data), ID_PHONE, ID_IMID_SCR, imid_data);
			aibus_handler->writeAIData(&imid_msg);
		}
	} else if(parameter_list->imid_char > 0 && parameter_list->imid_lines > 0 && imid_scroll == BTA_IMID_SCROLL_ALBUM) {
		for(int i=0;i<SPLIT_TEXT_COUNT;i+=1)
			split_text[i] = "";

		if(imid_split) {
			splitText(parameter_list->imid_char, album, split_text, SPLIT_TEXT_COUNT);
		} else {
			split_text[0] = album;
		}
		imid_scroll_position = 0;
		scroll_timer = *timer;

		if(!imid_scroll_header)
			refresh_imid = true;
	}
}

//Increment the info display.
void BTAudioHandler::incrementInfo() {
	if(!parameter_list->audio_selected || !parameter_list->text_allowed || parameter_list->imid_char <= 0 || parameter_list->imid_lines <= 0)
		return;
 
	for(int i=1;i<=parameter_list->imid_lines;i+=1) {
		if(i==parameter_list->imid_lines/2 || i == parameter_list->imid_lines/2+1)
			continue;

		uint8_t clear_data[] = {0x23, 0x60, 0x0, uint8_t(i&0xFF)};
		AIData clear_msg(sizeof(clear_data), ID_PHONE, ID_IMID_SCR, clear_data);
		aibus_handler->writeAIData(&clear_msg);
	}

	string info_param = "";

	switch(imid_scroll) {
	case BTA_IMID_SCROLL_TRACK:
		imid_scroll = BTA_IMID_SCROLL_ARTIST;
		info_param = getString(LOCALE_STRING_IMID_ARTIST, parameter_list->locale);
		text_handler->writeNavHeaderText("Artist: " + artist);
		break;
	case BTA_IMID_SCROLL_ARTIST:
		imid_scroll = BTA_IMID_SCROLL_ALBUM;
		info_param = getString(LOCALE_STRING_IMID_ALBUM, parameter_list->locale);
		text_handler->writeNavHeaderText("Album: " + album);
		break;
	case BTA_IMID_SCROLL_ALBUM:
		imid_scroll = BTA_IMID_SCROLL_NONE;
		writeStatus();
		writePosition();
		writeIMIDTitle();
		writeIMIDArtist();
		writeIMIDAlbum();
		break;
	default:
		imid_scroll = BTA_IMID_SCROLL_TRACK;
		info_param = getString(LOCALE_STRING_IMID_TRACK, parameter_list->locale);
		text_handler->writeNavHeaderText("Track: " + song_title);
		break;
	}

	scroll_timer = *timer;

	if(info_param.length() > 0) {
		uint8_t header_data[4+info_param.length()];
		header_data[0] = 0x23;
		header_data[1] = 0x60;

		int imid_x = parameter_list->imid_char/2 - info_param.length()/2;
		if(imid_x < 0 || imid_x > parameter_list->imid_char)
			imid_x = 0;

		header_data[2] = uint8_t(imid_x&0xFF);

		if(parameter_list->imid_lines > 1)
			header_data[3] = parameter_list->imid_lines/2;
		else {
			header_data[3] = 1;
			imid_scroll_header = true;
		}

		for(int i=0;i<info_param.length(); i+=1)
			header_data[i+4] = uint8_t(info_param[i]);

		AIData header_msg(sizeof(header_data), ID_PHONE, ID_IMID_SCR, header_data);
		aibus_handler->writeAIData(&header_msg);
	}

	switch(imid_scroll) {
	case BTA_IMID_SCROLL_TRACK:
		writeIMIDTitle();
		break;
	case BTA_IMID_SCROLL_ARTIST:
		writeIMIDArtist();
		break;
	case BTA_IMID_SCROLL_ALBUM:
		writeIMIDAlbum();
		break;
	default:
		imid_scroll_header = false;
		break;
	}
}

//Write status information.
void BTAudioHandler::writeStatus() {
	if(!parameter_list->audio_selected || !parameter_list->text_allowed)
		return;

	string window_text = "";
	switch(playback_status) {
	case PLAYBACK_STATUS_PLAYING:
		switch(repeat_random_status) {
		case RPTRND_REPEAT_T:
			window_text = "Repeat T";
			break;
		case RPTRND_REPEAT_A:
			window_text = "Repeat A";
			break;
		case RPTRND_REPEAT_G:
			window_text = "Repeat G";
			break;
		case RPTRND_RANDOM_A:
			window_text = "Random A";
			break;
		case RPTRND_RANDOM_G:
			window_text = "Random G";
			break;
		default:
			window_text = "#FWD";
			break;
		}
		break;
	case PLAYBACK_STATUS_PAUSED:
		window_text = "||";
		break;
	case PLAYBACK_STATUS_FF:
		window_text = "#FF ";
		break;
	case PLAYBACK_STATUS_FR:
		window_text = "#REW";
		break;
	default:
		break;
	}

	text_handler->writeAudioWindowText(window_text, 1, 1);

	if(imid_scroll < 0) { 
		if(parameter_list->imid_native_phone) {
			//TODO: Something.
		} else if(parameter_list->imid_char > 0 && parameter_list->imid_lines > 0)
			writeIMIDStatusandPosition();
	}
}

//Write the track number.
void BTAudioHandler::writeTrackNumber() {
	if(imid_scroll < 0) { 
		if(parameter_list->imid_native_phone) {
			//TODO: Something.
		} else if(parameter_list->imid_char > 0 && parameter_list->imid_lines > 0)
			writeIMIDStatusandPosition();
	}
}

//Write time code.
void BTAudioHandler::writePosition() {
	if(!parameter_list->audio_selected)
		return;

	const uint32_t position = this->position/1000;

	uint8_t meta_pos_data[] = {0x3B, 0x0, 0x0, uint8_t((position>>8)&0xFF), uint8_t(position&0xFF)};
	AIData meta_pos_msg(sizeof(meta_pos_data), ID_PHONE, ID_RADIO, meta_pos_data);
	aibus_handler->writeAIData(&meta_pos_msg, parameter_list->radio_connected);

	if(!parameter_list->text_allowed)
		return;

	const string tc_str = to_string(position/60) + ":" + (position%60 >= 10 ? to_string(position%60) : "0" + to_string(position%60));
	text_handler->writeAudioWindowText(tc_str, 1, 0);

	if(imid_scroll < 0) {
		if(parameter_list->imid_native_phone) {
			meta_pos_msg.receiver = ID_IMID_SCR;
			aibus_handler->writeAIData(&meta_pos_msg);
		} else if(parameter_list->imid_char > 0 && parameter_list->imid_lines > 0)
			writeIMIDStatusandPosition();
	}
}

//Write the status and position to a non-native IMID.
void BTAudioHandler::writeIMIDStatusandPosition() {
	if(!parameter_list->audio_selected || !parameter_list->text_allowed)
		return;

	if(!display_header || parameter_list->imid_lines < 1)
		return;

	string imid_text = "";
	if(parameter_list->imid_char > 8)
		imid_text = "BTA ";
	else if(track_number < 10)
		imid_text = "0";

	imid_text += to_string(track_number);

	const uint32_t position = this->position/1000;
	for(int p = imid_text.length();p<parameter_list->imid_char - (position/60 >= 10 ? 5 : 4) && p < 16;p+=1)
		imid_text += ' ';

	imid_text += to_string(position/60) + ':' + (position%60 >= 10 ? to_string(position%60) : '0' + to_string(position%60));

	int imid_x = parameter_list->imid_char/2 - imid_text.length()/2;
	if(imid_x < 0 || imid_x >= parameter_list->imid_char)
		imid_x = 0;

	uint8_t imid_data[imid_text.length() + 4];
	imid_data[0] = 0x23;
	imid_data[1] = 0x60;
	imid_data[2] = uint8_t(imid_x&0xFF);
	imid_data[3] = 0x1;
	for(int i=0;i<imid_text.length();i+=1)
		imid_data[i+4] = uint8_t(imid_text[i]);

	AIData imid_msg(sizeof(imid_data), ID_PHONE, ID_IMID_SCR, imid_data);
	aibus_handler->writeAIData(&imid_msg);
}

//Write all metadata to the computer, radio, and IMID.
void BTAudioHandler::writeAllMetadata() {
	string device_name = "";
	BTADevice* device = bluetooth_handler->getConnectedDevice();
	if(device != nullptr && device != NULL)
		device_name = device->getDeviceName();

	text_handler->writeAudioWindowText("Bluetooth", 0, 0, false);
	text_handler->writeAudioWindowText(song_title, 0, 1, false);
	text_handler->writeAudioWindowText(artist, 0, 2, false);
	text_handler->writeAudioWindowText(album, 0, 3, false);
	text_handler->writeAudioWindowText(device_name, 1, 2);

	writeStatus();
	writePosition();

	text_handler->writeMetadata(song_title, ID_RADIO, 1);
	text_handler->writeMetadata(artist, ID_RADIO, 2);
	text_handler->writeMetadata(album, ID_RADIO, 3);
	text_handler->writeMetadata(device_name, ID_RADIO, 4);

	writeIMIDTitle();
	writeIMIDArtist();
	writeIMIDAlbum();

	if(parameter_list->imid_native_phone && imid_scroll < 0)
		text_handler->writeMetadata(device_name, ID_IMID_SCR, 4);
}

//Write the function buttons.
void BTAudioHandler::writeFunctionButtons() {
	text_handler->writeAudioWindowText("Repeat", 2, 0, false);
	text_handler->writeAudioWindowText("Random", 2, 1, false);
	text_handler->writeAudioWindowText("#REW", 2, 2, false);
	text_handler->writeAudioWindowText("#FF ", 2, 3);
}

//Increment the repeat status.
void BTAudioHandler::incRepeat() {
	switch(repeat_random_status) {
	case RPTRND_REPEAT_T:
		setRepeat(RPTRND_REPEAT_A);
		break;
	case RPTRND_REPEAT_A:
		setRepeat(RPTRND_REPEAT_G);
		break;
	case RPTRND_REPEAT_G:
		setRepeat(RPTRND_NORMAL);
		break;
	default:
		setRepeat(RPTRND_REPEAT_T);
		break;
	}
}

//Increment the random status.
void BTAudioHandler::incRandom() {
	switch(repeat_random_status) {
	case RPTRND_RANDOM_A:
		setRandom(RPTRND_RANDOM_G);
		break;
	case RPTRND_RANDOM_G:
		setRandom(RPTRND_NORMAL);
		break;
	default:
		setRandom(RPTRND_RANDOM_A);
		break;
	}
}

//Set the repeat status.
void BTAudioHandler::setRepeat(const repeat_random_status_t status) {
	string repeat_status;
	switch(status) {
	case RPTRND_REPEAT_T:
		repeat_status = "singletrack";
		break;
	case RPTRND_REPEAT_A:
		repeat_status = "alltracks";
		break;
	case RPTRND_REPEAT_G:
		repeat_status = "group";
		break;
	default:
		repeat_status = "off";
	}

	auto media_proxy = bluetooth_handler->getMediaProxy();
	
	try {
		(*media_proxy)->setProperty("Repeat").onInterface("org.bluez.MediaPlayer1").toValue(repeat_status);
	} catch(Error e) {
		try {
			(*media_proxy)->setProperty("Repeat").onInterface("org.bluez.MediaPlayer1").toValue("off");
		} catch(Error e) {
			cout<<"Error setting repeat."<<endl;
		}
	}

	writeStatus();
}

//Set the random/shuffle status.
void BTAudioHandler::setRandom(const repeat_random_status_t status) {
	string random_status;
	switch(status) {
	case RPTRND_RANDOM_A:
		random_status = "alltracks";
		break;
	case RPTRND_RANDOM_G:
		random_status = "group";
		break;
	default:
		random_status = "off";
	}

	auto media_proxy = bluetooth_handler->getMediaProxy();

	try {
		(*media_proxy)->setProperty("Shuffle").onInterface("org.bluez.MediaPlayer1").toValue(random_status);
	} catch(Error e) {
		try {
			(*media_proxy)->setProperty("Shuffle").onInterface("org.bluez.MediaPlayer1").toValue("off");
		} catch(Error e) {
			cout<<"Error setting random."<<endl;
		}
	}

	writeStatus();
}
