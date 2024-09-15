#include <Arduino.h>

#ifndef bm83_aidf_h
#define bm83_aidf_h

class BM83 {
public:
	BM83(Stream* serial);
	void loop();
private:
	Stream* serial;
	
	void sendPowerOn();
};

#endif
