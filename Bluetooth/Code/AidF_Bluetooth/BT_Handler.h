#include <sdbus-c++/sdbus-c++.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <iostream>
#include <vector>

#include "Parameter_List.h"
#include "Text_Handler.h"

#include "Devices/BTA_Device.h"

using namespace std;
using namespace sdbus;

#ifndef bt_handler_h
#define bt_handler_h

#define INIT_WAIT 5000

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

struct CommandThreadParameters;

//IO proxy for all Bluetooth messages.
class BTHandler {
public:
	BTHandler(ParameterList* parameter_list, BTADevice** connected_device, TextHandler* text_handler, uint8_t* bt_mac);
	~BTHandler();

	void loop();
	void init(unsigned long* timer);

	//Discovery:
	void startDiscovery();
	void stopDiscovery();
	bool getDiscoverable();

	//Connect/disconnect.
	void connectDevice(const int index);
	void disconnectDevice();
	bool isConnected();

	//Get the device.
	BTADevice* getConnectedDevice();

	//Getters.
	void getObjectList();
	unique_ptr<IProxy>* getMediaProxy();
	map<string, Variant>* getChangedMediaProperties();

	//BT control:
	void handleBTCTLMessage(const string msg);

	//Media control.
	void sendMediaControl(const media_control_t cmd);

	//Menus.
	void createDeviceListMenu();
private:
	uint8_t* bt_mac;
	BTADevice** connected_device;
	ParameterList* parameter_list;

	TextHandler* text_handler;

	CommandThreadParameters* command_thread_params;
	pthread_t command_thread;

	unique_ptr<IConnection> connection = nullptr;
	unique_ptr<IProxy> adapter_proxy = nullptr, dev_proxy = nullptr, media_proxy = nullptr;

	pid_t bluetoothctl_process;
	int pipe_stdin[2], pipe_stdout[2];

	bool media_set = false;

	vector<string> paired_address_list;

	map<string, Variant> media_properties;
	bool media_check_ok = true;

	unsigned long* timer;
	unsigned long start_time;

	bool init_connect = false;

	void connectKnownDevice();
	void connectDevice(ObjectPath device_path);

	void getPiMac();

	void onInterfaceAdd(Message msg);
	void onInterfaceRemove(Message msg);
	void onMediaPropertyChange(string interface, map<string, Variant> changed_properties, vector<string> invalidated_properties);

	void setMediaProxy(ObjectPath proxy_path);

	void btCommand(string command);
};

string getDevicePath(string path);

void* readCommandThread(void* parameters_v);

struct CommandThreadParameters {
	BTHandler* self;
	int stdout_read, stdin_write;
};

#endif