#include "AMirror_Socket.h"
#include <iostream>

SocketMessage::SocketMessage(const uint8_t opcode, const uint16_t l) {
	this->opcode = opcode;
	this->l = l;

	this->data = new uint8_t[l];
}

SocketMessage::~SocketMessage() {
	delete[] this->data;
}

//Set the message length and opcode.
void SocketMessage::refreshSocketData(const uint8_t opcode, const uint16_t l) {
	delete[] this->data;

	this->opcode = opcode;
	this->l = l;

	this->data = new uint8_t[l];
}

//Set the message data.
void SocketMessage::refreshSocketData(uint8_t* data) {
	for(int i=0;i<this->l;i+=1)
		this->data[i] = data[i];
}

AMirrorSocket::AMirrorSocket() {
	remove(SOCKET_PATH);
	umask(0);

	this->server_socket = socket(AF_UNIX, SOCK_STREAM, 0);

	struct sockaddr_un server_address;
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, SOCKET_PATH);

    bind(this->server_socket, (struct sockaddr*) &server_address, sizeof(server_address));
    listen(this->server_socket, 5);

	this->client_socket = accept(this->server_socket, NULL, NULL);
}

//Write a socket message.
void AMirrorSocket::writeSocketMessage(SocketMessage* msg) {
	if(msg->l + 1 > 255)
		return;

	const int byte_l = msg->l + strlen(SOCKET_START) + 3, start_l = strlen(SOCKET_START);

	uint8_t data[byte_l];

	for(int i=0;i<start_l;i+=1)
		data[i] = uint8_t(SOCKET_START[i]);

	data[start_l] = msg->opcode;
	data[start_l + 1] = uint8_t(msg->l + 1);

	for(int i=0;i<msg->l;i+=1)
		data[start_l + 2 + i] = msg->data[i];

	uint8_t checksum = 0;
	for(int i=0;i<byte_l - 1;i+=1)
		checksum ^= data[i];

	data[byte_l-1] = checksum;

	send(client_socket, data, byte_l, 0);
}

//Read a socket message. Return the number of bytes read.
int AMirrorSocket::readSocketMessage(SocketMessage* msg) {
	uint8_t data[DEFAULT_READ_LENGTH];

	const int message_size = recv(this->client_socket, data, DEFAULT_READ_LENGTH, 0);
	
	if(message_size < 0)
        return -1;
	else if(message_size == 0)
		return 0;

    if(message_size < strlen(SOCKET_START) + 1)
        return -1;

    for(uint8_t i=0;i<strlen(SOCKET_START);i+=1) {
        if(data[i] != (uint8_t)SOCKET_START[i])
            return -1;
    }

	const uint8_t opcode = data[strlen(SOCKET_START)], msg_length = data[strlen(SOCKET_START) + 1]-1;
	if(msg_length > message_size - strlen(SOCKET_START) - 2)
		return -1;
	
	uint8_t msg_data[msg_length];
	for(int i=0;i<msg_length;i+=1)
		msg_data[i] = data[i+strlen(SOCKET_START) + 2];

	uint8_t checksum = 0;
	for(int i=0;i<message_size - 1;i+=1)
		checksum ^= data[i];

	if(checksum != data[message_size - 1])
		return -1;

	msg->refreshSocketData(opcode, msg_length);
	msg->refreshSocketData(msg_data);

	return msg_length;
}

//Get the client socket.
int AMirrorSocket::getClient() {
	return this->client_socket;
}

//Write a message to the client socket.
void writeSocketMessage(SocketMessage* msg, const int socket) {
	if(msg->l + 1 > 255)
		return;

	if(socket < 0)
		return;

	const int byte_l = msg->l + strlen(SOCKET_START) + 3, start_l = strlen(SOCKET_START);

	uint8_t data[byte_l];

	for(int i=0;i<start_l;i+=1)
		data[i] = uint8_t(SOCKET_START[i]);

	data[start_l] = msg->opcode;
	data[start_l + 1] = uint8_t(msg->l + 1);

	for(int i=0;i<msg->l;i+=1)
		data[start_l + 2 + i] = msg->data[i];

	uint8_t checksum = 0;
	for(int i=0;i<byte_l - 1;i+=1)
		checksum ^= data[i];

	data[byte_l-1] = checksum;

	send(socket, data, byte_l, 0);
}

//Socket thread function.
void *socketThread(void* parameters_v) {
	SocketHandlerParameters* parameters = (SocketHandlerParameters*)parameters_v;
	AMirrorSocket* amirror_socket = NULL;

	while(*parameters->running) {
		if(parameters->client_socket < 0 || amirror_socket == NULL) {
			amirror_socket = new AMirrorSocket();
			parameters->client_socket = amirror_socket->getClient();
		} else {
			SocketMessage rx_msg(0, DEFAULT_READ_LENGTH);

			const int socket_byte_count = amirror_socket->readSocketMessage(&rx_msg);

			if(socket_byte_count > 0) {
				if(rx_msg.opcode == OPCODE_AIBUS_RECEIVE) {
					char rx_buf[rx_msg.l];
					for(int i=0;i<rx_msg.l;i+=1)
						rx_buf[i] = char(rx_msg.data[i]);

					aiserialWrite(*parameters->ai_serial, rx_buf, rx_msg.l);
				}
			} else if(socket_byte_count == 0) { //Socket closed.
				parameters->client_socket = -1;
				delete amirror_socket;
			}
		}
	}

	if(amirror_socket != NULL)
		delete amirror_socket;

	void* result;
	return result;
}