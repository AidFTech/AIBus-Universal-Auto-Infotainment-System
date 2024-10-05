#include "AMirror_Socket.h"

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

//Read a socket message.
int AMirrorSocket::readSocketMessage(SocketMessage* msg) {
	uint8_t data[DEFAULT_READ_LENGTH];

	const int message_size = recv(this->client_socket, data, DEFAULT_READ_LENGTH, 0);
	
	if(message_size <= 0)
        return -1;

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
	for(int i=0;i<msg_length;i+=1)
		checksum ^= msg_data[i];

	if(checksum != data[message_size - 1])
		return -1;

	msg->refreshSocketData(opcode, msg_length);
	msg->refreshSocketData(msg_data);

	return msg_length;
}

//Socket thread function.
void *socketThread(void* parameters_v) {
	SocketHandlerParameters* parameters = (SocketHandlerParameters*)parameters_v;

	AMirrorSocket amirror_socket;

	SocketRecvHandlerParameters recv_paramters;
	recv_paramters.main_parameters = parameters;
	recv_paramters.socket = &amirror_socket;

	while(*parameters->running) {
		if(parameters->aidata_tx.size() > 4) { //An AIBus message is ready to send.
			const int l = parameters->aidata_tx.at(1);
			if(parameters->aidata_tx.size() < l + 2) {
				parameters->aidata_tx.clear();
				continue;
			}

			uint8_t data[l+2];
			for(int i=0;i<l+2;i+=1) {
				data[i] = parameters->aidata_tx.at(0);
				parameters->aidata_tx.erase(parameters->aidata_tx.begin());
			}

			SocketMessage socket_msg(OPCODE_AIBUS_SEND, sizeof(data));
			socket_msg.refreshSocketData(data);

			amirror_socket.writeSocketMessage(&socket_msg);
		}
	}
}

//Socket receive thread function.
void *socketReceiveThread(void* parameters_v) {
	SocketRecvHandlerParameters* recv_parameters = (SocketRecvHandlerParameters*)parameters_v;
	SocketHandlerParameters* parameters = recv_parameters->main_parameters;

	AMirrorSocket* socket_handler = recv_parameters->socket;
	
	while(parameters->running) {
		SocketMessage rx_msg(0, DEFAULT_READ_LENGTH);

		if(socket_handler->readSocketMessage(&rx_msg) > 0) {
			if(rx_msg.opcode == OPCODE_AIBUS_RECEIVE) {
				for(int i=0;i<rx_msg.l;i+=1)
					parameters->aidata_rx.push_back(rx_msg.data[i]);
			}
		}
	}
}