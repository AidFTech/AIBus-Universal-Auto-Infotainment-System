#include "AidF_BTA.h"

AidFBTA::AidFBTA() {
	this->socket_parameters.ai_handler = &aibus_handler;
	this->socket_parameters.running = &running;
	this->socket_parameters.socket_path = BTA_SOCKET_PATH;

	this->elapsed_millis.run = &running;

	pthread_create(&aibus_thread, NULL, socketThread, (void*)&socket_parameters);
	pthread_create(&millis_thread, NULL, millisThread, (void*)&elapsed_millis);

	aibus_handler.setTimer(&elapsed_millis.time);

	for(int i=0;i<sizeof(pi_mac);i+=1)
		pi_mac[i] = 0;

	socket_parameters.process = true;

	bt_handler.init(&elapsed_millis.time);

	ping_timer = elapsed_millis.time + RADIO_PING_TIMER;
	screen_ping_timer = elapsed_millis.time;

	audio_handler.setTimer(&elapsed_millis.time);

	createMenuNoPhone(true);
	
	vector<IniList> file_settings = loadIniFile(BLUETOOTH_SETTING_FILE_PATH);
	bool split = true, autostart = true, flash = true;
	for(auto list: file_settings) {
		if(list.title.compare("AidF_BTA") != 0)
			continue;
			
		for(int s=0;s<list.l_n;s+=1) {
			if(list.num_vars[s].compare("Autostart") == 0)
				autostart = list.num_values[s] != 0;
			else if(list.num_vars[s].compare("Split") == 0)
				split = list.num_values[s] != 0;
			else if(list.num_vars[s].compare("Flash") == 0)
				flash = list.num_values[s] != 0;
		}
	}
	
	audio_handler.setSaveSettings(split, autostart, flash);
}

AidFBTA::~AidFBTA() {
	running = false;
	pthread_cancel(aibus_thread);
	pthread_cancel(millis_thread);

	if(connected_device != NULL)
		delete connected_device;
}

//Main object loop.
void AidFBTA::loop() {
	const bool last_selected = parameter_list.audio_selected;

	AIData ai_msg;
	//while(!aibus_handler.getCheckOK());
	while(aibus_handler.readAIData(&ai_msg)) {
		#ifdef PRINT_AIBUS
		cout<<"S: "<<hex<<int(ai_msg.sender)<<" R: "<<hex<<int(ai_msg.receiver)<<" D: ";
		for(int i=0;i<ai_msg.l;i+=1)
			cout<<hex<<int(ai_msg[i])<<' ';
		cout<<endl;
		#endif
		if(!parameter_list.radio_connected && ai_msg.sender == ID_RADIO && !getInitMessage(&ai_msg) && !getPoweroffMessage(&ai_msg)) {
			parameter_list.radio_connected = true;
			audio_handler.radioInit();
		}

		if(!parameter_list.screen_connected && ai_msg.sender == ID_NAV_SCREEN && !getInitMessage(&ai_msg) && !getPoweroffMessage(&ai_msg)) {
			parameter_list.screen_connected = true;
		}

		bool answered = audio_handler.handleAIBusMessage(&ai_msg);
		if(!answered)
			handleAIBusMessage(&ai_msg);
	}

	if(!last_selected && parameter_list.audio_selected)
		screen_ping_timer = elapsed_millis.time;

	bt_handler.loop();
	audio_handler.loop();

	if(parameter_list.connection_changed) {
		if(connected_device == nullptr || connected_device == NULL) {
			createMenuNoPhone(true);
		} else {
			createMenuPhone(true);
			bt_handler.stopDiscovery();
		}

		audio_handler.refreshDeviceConnection();
		parameter_list.connection_changed = false;
	}

	if(elapsed_millis.time - ping_timer > RADIO_PING_TIMER) {
		ping_timer = elapsed_millis.time;

		if(!parameter_list.radio_connected) {
			uint8_t ping_data[] = {0x1};
			AIData ping_msg(sizeof(ping_data), ID_PHONE, ID_RADIO, ping_data);
			aibus_handler.writeAIData(&ping_msg, false);
		}

		if(!parameter_list.screen_connected) {
			uint8_t ping_data[] = {0x1};
			AIData ping_msg(sizeof(ping_data), ID_PHONE, ID_NAV_SCREEN, ping_data);
			aibus_handler.writeAIData(&ping_msg, false);
		}

		if(!parameter_list.imid_native_phone && parameter_list.imid_char <= 0 && parameter_list.imid_lines <= 0) {
			uint8_t ping_data[] = {0x4, 0xE6, 0x3B};
			AIData ping_msg(sizeof(ping_data), ID_PHONE, ID_IMID_SCR, ping_data);
			aibus_handler.writeAIData(&ping_msg, false);
		}
	}

	//Ping the screen.
	if(parameter_list.audio_selected && elapsed_millis.time - screen_ping_timer > SCREEN_PING_TIMER) {
		screen_ping_timer = elapsed_millis.time;
		uint8_t screen_ping_data[] = {0x77, ID_PHONE, 0x80};
		AIData screen_ping_msg(sizeof(screen_ping_data), ID_PHONE, ID_NAV_SCREEN, screen_ping_data);
		aibus_handler.writeAIData(&screen_ping_msg, parameter_list.screen_connected);
	}

	if(!parameter_list.dimensions_set && elapsed_millis.time - dimension_ping_timer > SCREEN_PING_TIMER) {
		dimension_ping_timer = elapsed_millis.time;
		uint8_t dim_ping_data[] = {0x2C, 0xF0};
		AIData dim_ping_msg(sizeof(dim_ping_data), ID_PHONE, ID_NAV_COMPUTER, dim_ping_data);
		aibus_handler.writeAIData(&dim_ping_msg);
	}

	usleep(1000);
}

//Handle an AIBus message.
void AidFBTA::handleAIBusMessage(AIData* ai_msg) {
	if(ai_msg->receiver == 0xFF && ai_msg->l == 1 && ai_msg->data[0] == 0x1) { //Ping.
		uint8_t ping_data[] = {0x1};
		AIData ping_msg(sizeof(ping_data), ID_PHONE, ai_msg->sender, ping_data);
		aibus_handler.writeAIData(&ping_msg);	
	} else if(ai_msg->sender == ID_NAV_COMPUTER && ai_msg->l >= 1 && ai_msg->data[0] == 0x2B) { //Menu operation.
		if(ai_msg->l >= 2 && ai_msg->data[1] == 0x40) { //Menu closed.
			parameter_list.current_menu = BTA_MENU_NONE;
		} else if(ai_msg->l >= 3 && ai_msg->data[1] == 0x60) { //Main menu selection.
			const int selection = ai_msg->data[2] - 1;

			if(parameter_list.current_menu == BTA_MENU_DEVICES) {
				bt_handler.connectDevice(selection);
				text_handler.clearMenu();
			}
		} else if(ai_msg->l >= 3 && ai_msg->data[1] == 0x6C) { //Side menu selection.
			const int selected = ai_msg->data[2] - 1;
			if(selected < 0)
				return;

			if(parameter_list.side_menu == SIDE_MENU_PHONE_NC) {
				const MenuList side_menu = getMenu(MENU_INDEX_MAIN_NC, parameter_list.locale);
				switch(side_menu.getGlobalIndex(selected)) {
				case MENU_INDEX_MAIN_NC_PAIR_ON:
					if(!bt_handler.getDiscoverable())
						bt_handler.startDiscovery();
					else
						bt_handler.stopDiscovery();
					createMenuNoPhone(false);
					break;
				case MENU_INDEX_MAIN_NC_DEVICE_LIST:
					bt_handler.createDeviceListMenu();
					break;
				default:
					break;
				}
			} else if(parameter_list.side_menu == SIDE_MENU_PHONE) {
				const MenuList side_menu = getMenu(MENU_INDEX_MAIN, parameter_list.locale);
				switch(side_menu.getGlobalIndex(selected)) {
				case MENU_INDEX_MAIN_DISCONNECT:
					bt_handler.disconnectDevice();
					break;
				default:
					break;
				}
			}
		}
	} else if(ai_msg->l >= 1 && ai_msg->data[0] == 0x12) { //MAC address request.
		if(parameter_list.bt_mac_set) {
			uint8_t mac_data[] = {0x12,
								pi_mac[0],
								pi_mac[1],
								pi_mac[2],
								pi_mac[3],
								pi_mac[4],
								pi_mac[5]};
			
			AIData mac_msg(sizeof(mac_data), ID_PHONE, ai_msg->sender, mac_data);
			aibus_handler.writeAIData(&mac_msg);
		}
	} else if(ai_msg->l >= 3 && ai_msg->data[0] == 0x4 && ai_msg->data[1] == 0xE6 && ai_msg->data[2] == 0x10) { //Radio request.
		audio_handler.radioInit();
	} else if(ai_msg->l >= 2 && ai_msg->data[0] == 0x3B && (ai_msg->data[1] == 0x57 || ai_msg->data[1] == 0x23)) {
		if(ai_msg->sender == ID_IMID_SCR && ai_msg->receiver != ID_PHONE) {
			uint8_t imid_request_data[] = {0x4, 0xE6, 0x3B};
			AIData imid_request_msg(sizeof(imid_request_data), ID_PHONE, ID_IMID_SCR, imid_request_data);
			aibus_handler.writeAIData(&imid_request_msg);
		}

		if(ai_msg->data[1] == 0x23 && ai_msg->l >= 4) { //Generic string length and height.
			const uint8_t last_char = parameter_list.imid_char, last_lines = parameter_list.imid_lines;

			parameter_list.imid_char = ai_msg->data[2];
			parameter_list.imid_lines = ai_msg->data[3];

			if(ai_msg->receiver == ID_PHONE && (parameter_list.imid_char != last_char || parameter_list.imid_lines != last_lines) && parameter_list.imid_char > 0 && parameter_list.imid_lines > 0)
				audio_handler.refreshIMIDConnection();
		} else if(ai_msg->data[1] == 0x57) {
			const bool last_native = parameter_list.imid_native_phone;

			parameter_list.imid_native_phone = false;
			for(int i=2;i<ai_msg->l;i+=1) {
				if(ai_msg->data[i] == ID_PHONE) {
					parameter_list.imid_native_phone = true;
					break;
				}
			}

			if(ai_msg->receiver == ID_PHONE && !last_native && parameter_list.imid_native_phone)
				audio_handler.refreshIMIDConnection();
		}
	} else if(ai_msg->sender == ID_NAV_COMPUTER && ai_msg->l >= 5 && ai_msg->data[0] == 0x2C) { //Dimensions.
		parameter_list.screen_w = (ai_msg->data[1]<<8) | ai_msg->data[2];
		parameter_list.screen_h = (ai_msg->data[3]<<8) | ai_msg->data[4];
		parameter_list.dimensions_set = true;
	}
}

//Create the phone window menu with no phone connected.
void AidFBTA::createMenuNoPhone(const bool clear) {
	if(clear)
		text_handler.clearPhoneWindow();

	const MenuList side_menu = getMenu(MENU_INDEX_MAIN_NC, parameter_list.locale);

	text_handler.writePhoneWindowText(getString(LOCALE_STRING_PHONE_NOT_CONNECTED, parameter_list.locale), 0, 0);
	for(int i=0;i<side_menu.size();i+=1) {
		switch(side_menu.getGlobalIndex(i)) {
			case MENU_INDEX_MAIN_NC_PAIR_ON:
				if(bt_handler.getDiscoverable())
					text_handler.writeSideMenuText("#RON  " + string(side_menu[i]), i);
				else
					text_handler.writeSideMenuText("#ROF  " + string(side_menu[i]), i);
				break;
			default:
				text_handler.writeSideMenuText(side_menu[i], i);
		}
	}

	parameter_list.side_menu = SIDE_MENU_PHONE_NC;
}

//Create the phone window with a phone connected.
void AidFBTA::createMenuPhone(const bool clear) {
	if(clear)
		text_handler.clearPhoneWindow();

	const MenuList side_menu = getMenu(MENU_INDEX_MAIN, parameter_list.locale);

	text_handler.writePhoneWindowText(getString(LOCALE_STRING_PHONE_CONNECTED, parameter_list.locale), 0, 0);

	if(connected_device != nullptr && connected_device != NULL)
		text_handler.writePhoneWindowText(connected_device->getDeviceName(), 1, 2);

	for(int i=0;i<side_menu.size();i+=1) {
		text_handler.writeSideMenuText(side_menu[i], i);
	}

	parameter_list.side_menu = SIDE_MENU_PHONE;
}

//Global setup.
void setup(AidFBTA* aidf_bta) {
	
}

//Global loop.
void loop(AidFBTA* aidf_bta) {
	aidf_bta->loop();
}

int main(int argc, char* args[]) {
	AidFBTA aidf_bta;
	setup(&aidf_bta);

	while(aidf_bta.running)
		loop(&aidf_bta);
}

//Timer thread function.
void *millisThread(void* millis_v) {
	ElapsedMillis* elapsed_millis = (ElapsedMillis*)millis_v;

	while(*elapsed_millis->run) {
		usleep(1000);

		elapsed_millis->time += 1;

		if(!*elapsed_millis->run)
			break;
	}

	void* result;
	return result;
}
