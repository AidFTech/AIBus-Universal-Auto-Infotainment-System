#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "../AIBus/AIBus_Serial.h"

#ifndef amirror_socket_h
#define amirror_socket_h

#define SOCKET_PATH "/tmp/amirror"
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

class AMirrorSocket {
public:
	AMirrorSocket();

	void writeSocketMessage(SocketMessage* msg);
	int readSocketMessage(SocketMessage* msg);

	int getClient();
private:
	int client_socket = -1, server_socket = -1;
};

struct SocketHandlerParameters {
	int* ai_serial;
	int client_socket = -1;

	bool* running;
};

void writeSocketMessage(SocketMessage* msg, const int socket);

void *socketThread(void* parameters_v);

#endif