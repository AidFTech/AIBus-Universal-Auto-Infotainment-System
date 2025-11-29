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

	bt_handler.init();

	radio_ping_timer = elapsed_millis.time + RADIO_PING_TIMER;

	createMenuNoPhone(true);
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
	AIData ai_msg;
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

		handleAIBusMessage(&ai_msg);
	}

	bt_handler.loop();
	audio_handler.loop();

	if(parameter_list.connection_changed) {
		if(connected_device == nullptr || connected_device == NULL) {
			createMenuNoPhone(true);
		} else {
			createMenuPhone(true);
			bt_handler.stopDiscovery();
		}
		parameter_list.connection_changed = false;
	}

	if(!parameter_list.radio_connected && elapsed_millis.time - radio_ping_timer > RADIO_PING_TIMER) {
		radio_ping_timer = elapsed_millis.time;

		uint8_t ping_data[] = {0x1};
		AIData ping_msg(sizeof(ping_data), ID_PHONE, ID_RADIO, ping_data);
		aibus_handler.writeAIData(&ping_msg, false);
	}

	usleep(1000);
}

//Handle an AIBus message.
void AidFBTA::handleAIBusMessage(AIData* ai_msg) {
	if(ai_msg->sender == ID_NAV_COMPUTER && ai_msg->l >= 1 && ai_msg->data[0] == 0x2B) { //Menu operation.
		if(ai_msg->l >= 3 && ai_msg->data[1] == 0x6C) { //Side menu selection.
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
					bt_handler.getObjectList();
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