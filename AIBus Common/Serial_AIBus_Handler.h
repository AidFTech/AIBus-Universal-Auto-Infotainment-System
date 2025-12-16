#if __has_include(<pigpio.h>)
#include <pigpio.h>
#define RPI_UART
#else
#include <iostream>
#endif

#include "AIBus/AIBus.h"
#include "AIBus/AIBus_Serial.h"

#if __has_include("Socket/AIBus_Socket.h")
#include "Socket/AIBus_Socket.h"
#define SOCKET_SERVER
#endif

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

#define AIDATA_LIMIT (0x30 - 4)

using namespace std;

class SerialAIBusHandler {
public:
	#ifdef RPI_UART
	#ifdef SOCKET_SERVER
	SerialAIBusHandler(std::string port, int** socket_list, const int socket_l, const uint8_t id, unsigned long* timer);
	#else
	SerialAIBusHandler(std::string port, const uint8_t id, unsigned long* timer);
	#endif
	#else
	#ifdef SOCKET_SERVER
	SerialAIBusHandler(int** socket_list, const int socket_l, const uint8_t id, unsigned long* timer);
	#else
	SerialAIBusHandler(const uint8_t id, unsigned long* timer);
	#endif
	#endif
	~SerialAIBusHandler();

	bool readAIData(AIData* ai_d);
	bool readAIData(AIData* ai_d, const bool cache, const bool multiple = true);
	
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
	bool getID(const uint8_t id);

	int ai_port;
	#ifdef RPI_UART
	const bool port_connected = true;
	#else
	bool port_connected = false;
	#endif

	vector<AIData> cached_vec;
	AIData cached_msg, cached_tx;

	#ifdef SOCKET_SERVER
	int** socket_list;
	int socket_l = 0;
	#endif

	unsigned long *timer;
	uint8_t id;
};

#ifndef RPI_UART
uint16_t stringToNumber(string str);
#endif

bool readAIByteData(AIData* ai_d, uint8_t* data, const uint8_t d_l);
bool getInitMessage(AIData* ai_d);
bool getPowerOffMessage(AIData* ai_d);

void printBytes(AIData* ai_d);
#endif
