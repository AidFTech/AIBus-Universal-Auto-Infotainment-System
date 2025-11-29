#include <sdbus-c++/sdbus-c++.h>

#include <iostream>

#include "Parameter_List.h"
#include "Text_Handler.h"

#include "Devices/BTA_Device.h"

using namespace std;
using namespace sdbus;

#ifndef bt_handler_h
#define bt_handler_h

enum media_control_t {
	MEDIA_CONTROL_NULL,
	MEDIA_CONTROL_FF,
	MEDIA_CONTROL_NEXT,
	MEDIA_CONTROL_PAUSE,
	MEDIA_CONTROL_PLAY,
	MEDIA_CONTROL_PREVIOUS,
	MEDIA_CONTROL_FR,
	MEDIA_CONTROL_STOP,
	MEDIA_CONTROL_VOLUMEDOWN,
	MEDIA_CONTROL_VOLUMEUP
};

//IO proxy for all Bluetooth messages.
class BTHandler {
public:
	BTHandler(ParameterList* parameter_list, BTADevice** connected_device, TextHandler* text_handler, uint8_t* bt_mac);
	void loop();
	void init();

	//Discovery:
	void startDiscovery();
	void stopDiscovery();
	bool getDiscoverable();

	void getObjectList();
private:
	uint8_t* bt_mac;
	BTADevice** connected_device;
	ParameterList* parameter_list;

	TextHandler* text_handler;

	unique_ptr<IConnection> connection;
	unique_ptr<IProxy> adapter_proxy;

	void getPiMac();

	void onInterfaceAdd(Message msg);
	void onInterfaceRemove(Message msg);
};

#endif