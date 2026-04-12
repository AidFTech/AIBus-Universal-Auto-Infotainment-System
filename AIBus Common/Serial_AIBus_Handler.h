#if __has_include(<gpiod.h>)
#include <gpiod.h>
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

#include <unistd.h>
#include <pthread.h>

#define AIBUS_PRINT

#ifndef aibus_handler_h
#define aibus_handler_h

#define AI_RX 4
#define AI_WAIT 1

#define AIBUS_WAIT 5

#define REPEAT_DELAY 100
#define MAX_REPEAT 50

using namespace std;

#ifdef RPI_UART
enum pin_mode_t : uint8_t {
	PIN_MODE_INPUT,
	PIN_MODE_INPUT_PULLUP,
	PIN_MODE_OUTPUT
};
#endif

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
	void writeToCache(AIData* ai_d, const bool acknowledge);

	void sendAcknowledgement(const uint8_t sender, const uint8_t receiver);

	int getAvailableBytes();
	int getAvailableBytes(const bool cache);
	
	bool getConnected();

	int* getPortPointer();

	bool cachePending();
	void cacheMessage(AIData* ai_msg);
	
	bool flushCached();
	void readMultiple(const uint8_t sender, const uint8_t receiver, vector<uint8_t> data, const int message_count);

private:
	#ifndef RPI_UART
	int connectAIPort(string port);
	#endif

	bool awaitAcknowledgement(AIData* ai_d);

	void writeToSocket(AIData* ai_d);
	bool getID(const uint8_t id);

	bool writeAIBusToSerial(AIData* ai_d, const bool acknowledge);

	bool readAIData(AIData* ai_d, const bool cache, const bool multiple, const bool threaded);

	bool cachePending(const bool threaded);

	int ai_port;
	#ifdef RPI_UART
	const bool port_connected = true;

	gpiod_chip *chip;
	#else
	bool port_connected = false;
	#endif

	vector<AIData> cached_rx, cached_tx;
	vector<bool> cached_ack;
	pthread_t cache_thread;
	bool cache_thread_enabled = false;

	void* multi_thread_params;
	pthread_t multi_thread;
	bool multi_thread_enabled = false;

	#ifdef SOCKET_SERVER
	int** socket_list;
	int socket_l = 0;
	#endif

	unsigned long *timer;
	uint8_t id;
	
	bool thread_locked = false, main_locked = false; //If true, the serial port is locked by another object.
};

#ifndef RPI_UART
uint16_t stringToNumber(string str);
#endif

bool readAIByteData(AIData* ai_d, uint8_t* data, const uint8_t d_l);
bool getInitMessage(AIData* ai_d);
bool getPowerOffMessage(AIData* ai_d);

void printBytes(AIData* ai_d);

void* flushCacheThread(void* v_aibus_handler);
void* readMultiThread(void* multi_thread_params);

#ifdef RPI_UART
void setPinMode(gpiod_chip* chip, const int pin, const pin_mode_t mode);
bool readPin(gpiod_chip* chip, const int pin);
#endif

struct MultiMessageThreadParameters {
	SerialAIBusHandler* ai_handler;
	uint8_t sender, receiver;
	vector<uint8_t> loaded_data = vector<uint8_t>(0);
	int message_count = 0;
};
#endif
