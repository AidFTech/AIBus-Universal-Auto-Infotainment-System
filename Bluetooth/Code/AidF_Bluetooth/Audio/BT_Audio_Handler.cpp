#include "BT_Audio_Handler.h"

BTAudioHandler::BTAudioHandler(ClientAIBusHandler* aibus_handler, BTHandler* bluetooth_handler, ParameterList* parameter_list) {
	this->aibus_handler = aibus_handler;
	this->bluetooth_handler = bluetooth_handler;
	this->parameter_list = parameter_list;
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