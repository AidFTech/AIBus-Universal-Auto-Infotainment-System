#include "../AIBus/AIBus.h"
#include "../AIBus/Client_AIBus_Handler.h"

#include "../BT_Handler.h"
#include "../Parameter_List.h"

#ifndef bt_audio_handler_h
#define bt_audio_handler_h

//Bluetooth audio handler object.
class BTAudioHandler {
public:
	BTAudioHandler(ClientAIBusHandler* aibus_handler, BTHandler* bluetooth_handler, ParameterList* parameter_list);

	void radioInit();
	void loop();
private:
	ClientAIBusHandler* aibus_handler;

	BTHandler* bluetooth_handler;
	ParameterList* parameter_list;

	void sendNameMessage();
};

#endif