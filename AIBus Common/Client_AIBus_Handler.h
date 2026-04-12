#include <unistd.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <netinet/in.h>

#include <string>
#include <vector>
#include <iostream>

#include "AIBus.h"

#ifndef client_aibus_handler_h
#define client_aibus_handler_h

#define SOCKET_START "AidFSock"

#define OPCODE_AIBUS_SEND 0x18
#define OPCODE_AIBUS_RECEIVE 0x68

#define DEFAULT_READ_LENGTH 1024

using namespace std;

struct SocketMessage {
	uint8_t opcode;
	uint16_t l;

	uint8_t* data;

	SocketMessage();
	SocketMessage(const uint8_t opcode, const uint16_t l);
	~SocketMessage();
	SocketMessage(const SocketMessage& copy);

	SocketMessage& operator=(const SocketMessage& copy);

	void refreshSocketData(const uint8_t opcode, const uint16_t l);
	void refreshSocketData(uint8_t* data);
};

class ClientAIBusHandler {
public:
	ClientAIBusHandler(const char* socket_path, uint8_t id);
	~ClientAIBusHandler();

	void setTimer(unsigned long* timer);

	bool getCheckOK();

	void refreshSocket(const char* socket_path);
	void clearSocket();

	void writeSocketMessage(SocketMessage* msg);
	int readSocketMessage(SocketMessage* msg);

	void cacheAIData(uint8_t* data, const int l);

	bool readAIData(AIData* ai_d);
	bool writeAIData(AIData* ai_d);
	bool writeAIData(AIData* ai_d, const bool ack);

	void sendAcknowledgement(const uint8_t sender, const uint8_t receiver);

	int getClient();
private:
	vector<AIData> rx_cache = vector<AIData>(0);
	vector<AIData> multi_cache = vector<AIData>(0); //Cache for multi-block messages.
	vector<SocketMessage> tx_cache = vector<SocketMessage>(0);

	unsigned long* timer = nullptr;

	bool check_ok = true, cache_ok = true;

	int network_socket = -1;
	uint8_t my_id = 0;
};

struct ClientHandlerParameters {
	ClientAIBusHandler* ai_handler;
	bool* running;

	string socket_path;
};

bool getInitMessage(AIData* ai_d);
bool getPoweroffMessage(AIData* ai_d);

void *socketThread(void* parameters_v);

#endif