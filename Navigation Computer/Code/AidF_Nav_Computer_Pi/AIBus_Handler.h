#if __has_include(<pigpio.h>)
#include <pigpio.h>
#define RPI_UART
#else
#include <iostream>
#endif

#include "AIBus/AIBus.h"
#include "AIBus/AIBus_Serial.h"
#include <string>
#include <stdint.h>
#include <time.h>
#include <vector>

#define AIBUS_PRINT

#ifndef aibus_handler_h
#define aibus_handler_h

#define AI_RX 4
#define AI_WAIT 1

#define REPEAT_DELAY 100
#define MAX_REPEAT 50

class AIBusHandler {
public:
	#ifdef RPI_UART
	AIBusHandler(std::string port);
	#else
	AIBusHandler();
	#endif
	~AIBusHandler();

	bool readAIData(AIData* ai_d);
	bool readAIData(AIData* ai_d, const bool cache);
	
	void writeAIData(AIData* ai_d);
	void writeAIData(AIData* ai_d, const bool acknowledge);

	void sendAcknowledgement(const uint8_t sender, const uint8_t receiver);

	int getAvailableBytes();
	int getAvailableBytes(const bool cache);
	
	bool getConnected();

	int* getPortPointer();

	bool cachePending();
	void cacheMessage(AIData* ai_msg);

private:
	#ifndef RPI_UART
	int connectAIPort(std::string port);
	#endif


	bool awaitAcknowledgement(AIData* ai_d);

	int ai_port;
	#ifdef RPI_UART
	const bool port_connected = true;
	#else
	bool port_connected = false;
	#endif

	std::vector<uint8_t> cached_bytes;
	AIData cached_msg;
};

#ifndef RPI_UART
uint16_t stringToNumber(std::string str);
#endif

bool readAIByteData(AIData* ai_d, uint8_t* data, const uint8_t d_l);

void printBytes(AIData* ai_d);
#endif
