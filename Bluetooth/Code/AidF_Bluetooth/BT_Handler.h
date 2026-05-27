#include <sdbus-c++/sdbus-c++.h>
#include <stdlib.h>

#include <iostream>
#include <vector>

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
	void init(unsigned long* timer);

	//Discovery:
	void startDiscovery();
	void stopDiscovery();
	bool getDiscoverable();

	//Connect/disconnect.
	void disconnectDevice();

	//Get the device.
	BTADevice* getConnectedDevice();

	//Getters.
	void getObjectList();
	unique_ptr<IProxy>* getMediaProxy();
	map<string, Variant>* getChangedMediaProperties();

	//Media control.
	void sendMediaControl(const media_control_t cmd);
private:
	uint8_t* bt_mac;
	BTADevice** connected_device;
	ParameterList* parameter_list;

	TextHandler* text_handler;

	unique_ptr<IConnection> connection = nullptr;
	unique_ptr<IProxy> adapter_proxy = nullptr, media_proxy = nullptr;

	map<string, Variant> media_properties;
	bool media_check_ok = true;

	unsigned long* timer;

	void connectKnownDevice();
	void connectDevice(ObjectPath device_path);

	void getPiMac();

	void onInterfaceAdd(Message msg);
	void onInterfaceRemove(Message msg);
	void onMediaPropertyChange(string interface, map<string, Variant> changed_properties, vector<string> invalidated_properties);

	void setMediaProxy(ObjectPath proxy_path);
};

#endif
