#include "Handshakes.h"

void sendWideHandshake(IEBusHandler* driver) {
	uint8_t handshake_data[] = {0x0, 0x30};
	IE_Message handshake(sizeof(handshake_data), IE_ID_RADIO, 0x1FF, 0xF, false);
	handshake.refreshIEData(handshake_data);

	driver->sendMessage(&handshake, false, false);
}

bool checkForMessages(IEBusHandler* driver, IE_Message* the_message) {
	if(driver->getInputOn()) {
		int16_t message_result = driver->readMessage(the_message, true, IE_ID_RADIO);
		return (message_result == 0);
	} else
		return false;
}

void sendFunctionMessage(IEBusHandler* driver, const bool change, const uint16_t recipient, uint8_t* data, uint16_t data_l) {
	IE_Message function_message(4+data_l, IE_ID_RADIO, recipient, 0xF, true);

	if(change)
		function_message.data[0] = 0x40;
	else
		function_message.data[0] = 0x70;
	function_message.data[1] = 0x0;
	function_message.data[2] = recipient&0xFF;
	function_message.data[3] = 0x2;
	for(uint8_t i=0;i<data_l;i+=1)
		function_message.data[4+i] = data[i];
	
	driver->sendMessage(&function_message, true, true);
}

void sendPingHandshake(IEBusHandler* driver, const uint16_t id) {
	uint8_t handshake_data[] = {0x1};
	IE_Message handshake(sizeof(handshake_data), IE_ID_RADIO, id&0xFFF, 0xF, true);
	handshake.refreshIEData(handshake_data);

	driver->sendMessage(&handshake, true, true);
}