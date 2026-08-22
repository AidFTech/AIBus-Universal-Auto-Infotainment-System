#include <iostream>
#include <vector>

#include <pthread.h>
#include <unistd.h>

#include "Parameter_List.h"
#include "Text_Handler.h"
#include "BT_Handler.h"
#include "Ini_Context.h"

#include "AIBus/AIBus.h"
#include "AIBus/Client_AIBus_Handler.h"

#include "Audio/BT_Audio_Handler.h"

#include "Devices/BTA_Device.h"

#include "Locale/Locale.h"

#ifndef aidf_bta_h
#define aidf_bta_h

//#define PRINT_AIBUS

#define BTA_SOCKET_PATH "/tmp/abta"

#define RADIO_PING_TIMER 5000
#define SCREEN_PING_TIMER 5000

using namespace std;

struct ElapsedMillis {
	unsigned long time = 0;
	bool* run;
};

//Main AidF bluetooth handler.
class AidFBTA {
public:
	AidFBTA();
	~AidFBTA();

	void loop();

	bool running = true;

private:
	//General
	ClientAIBusHandler aibus_handler = ClientAIBusHandler(BTA_SOCKET_PATH, ID_PHONE);
	ClientHandlerParameters socket_parameters;

	ElapsedMillis elapsed_millis;
	ParameterList parameter_list;

	TextHandler text_handler = TextHandler(&aibus_handler, &parameter_list);

	//Bluetooth
	BTADevice* connected_device = nullptr;
	uint8_t pi_mac[MAC_LEN];

	BTHandler bt_handler = BTHandler(&parameter_list, &connected_device, &text_handler, pi_mac);

	BTAudioHandler audio_handler = BTAudioHandler(&aibus_handler, &bt_handler, &text_handler, &parameter_list);

	//Threads
	pthread_t aibus_thread, millis_thread;

	//Timers
	unsigned long ping_timer = 0, screen_ping_timer = 0, dimension_ping_timer = 0;

	void handleAIBusMessage(AIData* ai_msg);

	void createMenuNoPhone(const bool clear);
	void createMenuPhone(const bool clear);
};

void setup(AidFBTA* aidf_bta);
void loop(AidFBTA* aidf_bta);

int main(int argc, char* args[]);

void *millisThread(void* millis_v);

#endif
