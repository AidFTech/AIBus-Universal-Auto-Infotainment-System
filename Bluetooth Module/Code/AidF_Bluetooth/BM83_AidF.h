#include <Arduino.h>

#include "Bluetooth_Device.h"

#ifndef bm83_aidf_h
#define bm83_aidf_h

class BM83 {
public:
	BM83(Stream* serial);
	void loop();
private:
	Stream* serial;

	BluetoothDevice* active_device = NULL;
	
	void sendPowerOn();

	void sendConnect(BluetoothDevice* device);
};

#endif
