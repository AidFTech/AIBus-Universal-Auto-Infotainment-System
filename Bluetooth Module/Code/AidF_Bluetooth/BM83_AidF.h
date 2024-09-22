#include <Arduino.h>

#include "AIBus_Handler.h"
#include "Bluetooth_Device.h"

#ifndef bm83_aidf_h
#define bm83_aidf_h

struct BM83Data {
	uint8_t* data;
	uint8_t opcode;
	uint16_t l;

	BM83Data(const uint16_t l, const uint8_t opcode);
	~BM83Data();
	BM83Data(const BM83Data &copy);

	void refreshBMData(uint8_t* data);
};

class BM83 {
public:
	BM83(Stream* serial, AIBusHandler* ai_handler);
	BM83(Stream* serial, AIBusHandler* ai_handler, const uint16_t pin_code);
	void loop();

	void init();

	bool handleAIBus(AIData* ai_msg);
private:
	Stream* serial;
	AIBusHandler* ai_handler;

	BluetoothDevice* active_device = NULL;

	uint16_t pin_code;
	
	void sendPowerOn();
	void sendDeviceName(String name);
	void sendDevicePIN();
	void sendDevicePIN(uint16_t pin);

	void sendConnect(BluetoothDevice* device);

	void sendBM83Message(BM83Data* msg);
};

#endif
