#if __has_include(<pigpio.h>)
#include <pigpio.h>
#define RPI_UART
#else
#include <iostream>
#endif

#include "AIBus/AIBus.h"
#include "AIBus/AIBus_Serial.h"

#include "Socket/AIBus_Socket.h"

#include <string>
#include <stdint.h>
#include <vector>

#define AIBUS_PRINT

#ifndef aibus_handler_h
#define aibus_handler_h

#define AI_RX 4
#define AI_WAIT 1

#define REPEAT_DELAY 100
#define MAX_REPEAT 50

using namespace std;

class AIBusHandler {
public:
	#ifdef RPI_UART
	AIBusHandler(std::string port, int** socket_list, const int socket_l, unsigned long* timer);
	#else
	AIBusHandler(int** socket_list, const int socket_l, unsigned long* timer);
	#endif
	~AIBusHandler();

	bool readAIData(AIData* ai_d);
	bool readAIData(AIData* ai_d, const bool cache);
	
	bool writeAIData(AIData* ai_d);
	bool writeAIData(AIData* ai_d, const bool acknowledge);

	void sendAcknowledgement(const uint8_t sender, const uint8_t receiver);

	int getAvailableBytes();
	int getAvailableBytes(const bool cache);
	
	bool getConnected();

	int* getPortPointer();

	bool cachePending();
	void cacheMessage(AIData* ai_msg);
	void cacheTxMessage(AIData* ai_msg);

	bool flushCached();

private:
	#ifndef RPI_UART
	int connectAIPort(string port);
	#endif

	bool awaitAcknowledgement(AIData* ai_d);

	void writeToSocket(AIData* ai_d);

	int ai_port;
	#ifdef RPI_UART
	const bool port_connected = true;
	#else
	bool port_connected = false;
	#endif

	vector<AIData> cached_vec;
	AIData cached_msg, cached_tx;

	int** socket_list;
	int socket_l = 0;

	unsigned long *timer;
};

#ifndef RPI_UART
uint16_t stringToNumber(string str);
#endif

bool readAIByteData(AIData* ai_d, uint8_t* data, const uint8_t d_l);
bool getInitMessage(AIData* ai_d);
bool getPowerOffMessage(AIData* ai_d);

void printBytes(AIData* ai_d);
#endif
