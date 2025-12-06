#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <vector>
#include <string>

#if __has_include("AIBus_Serial.h")
#include "AIBus_Serial.h"
#else
#include "../AIBus/AIBus_Serial.h"
#endif

using namespace std;

#ifndef amirror_socket_h
#define amirror_socket_h

#define AMIRROR_SOCKET_PATH "/tmp/amirror"
#define BTA_SOCKET_PATH "/tmp/abta"

#define SOCKET_START "AidFSock"

#define OPCODE_AIBUS_SEND 0x68
#define OPCODE_AIBUS_RECEIVE 0x18

#define DEFAULT_READ_LENGTH 1024

struct SocketMessage {
	uint8_t opcode;
	uint16_t l;

	uint8_t* data;

	SocketMessage(const uint8_t opcode, const uint16_t l);
	~SocketMessage();

	void refreshSocketData(const uint8_t opcode, const uint16_t l);
	void refreshSocketData(uint8_t* data);
};

class AIBusSocket {
public:
	AIBusSocket(const char* socket_path);
	~AIBusSocket();

	void writeSocketMessage(SocketMessage* msg);
	int readSocketMessage(SocketMessage* msg);

	int getClient();
private:
	int client_socket = -1, server_socket = -1;
};

struct SocketHandlerParameters {
	int* ai_serial;
	int client_socket = -1;
	
	AIBusSocket* socket_ptr = nullptr;

	string socket_path;

	bool* running;
	unsigned long* timer = nullptr;
};

void writeSocketMessage(SocketMessage* msg, const int socket);

void *socketThread(void* parameters_v);

#endif