#include "Client_AIBus_Handler.h"

SocketMessage::SocketMessage() : SocketMessage(0,0) {

}

SocketMessage::SocketMessage(const uint8_t opcode, const uint16_t l) {
	this->opcode = opcode;
	this->l = l;

	this->data = new uint8_t[l];
}

SocketMessage::~SocketMessage() {
	delete[] this->data;
}

SocketMessage::SocketMessage(const SocketMessage& copy) {
	this->opcode = copy.opcode;
	this->l = copy.l;

	this->data = new uint8_t[l];
	for(int i=0;i<l;i+=1)
		data[i] = copy.data[i];
}

SocketMessage& SocketMessage::operator=(const SocketMessage& copy) {
	delete[] this->data;

	this->opcode = copy.opcode;
	this->l = copy.l;

	this->data = new uint8_t[l];
	for(int i=0;i<l;i+=1)
		data[i] = copy.data[i];

	return *this;
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

ClientAIBusHandler::ClientAIBusHandler(const char* socket_path, uint8_t id) {
	my_id = id;
	refreshSocket(socket_path);
}

ClientAIBusHandler::~ClientAIBusHandler() {
	if(this->network_socket >= 0)
		close(network_socket);
}

//Set the timer pointer.
void ClientAIBusHandler::setTimer(unsigned long* timer) {
	this->timer = timer;
}

//Return whether the RX cache vector can be read.
bool ClientAIBusHandler::getCheckOK() {
	return this->check_ok;
}

//Refresh the socket connection.
void ClientAIBusHandler::refreshSocket(const char* socket_path) {
	if(this->network_socket >= 0)
		return;

	//Create the socket.
	network_socket = socket(AF_UNIX, SOCK_STREAM, 0);

	//Specify the address:
	struct sockaddr_un client_address;
	client_address.sun_family = AF_UNIX;
	strcpy(client_address.sun_path, socket_path);

	//Connect the socket.
	int connection_status = -1;
	do {
		connection_status = connect(network_socket, (struct sockaddr *) &client_address, sizeof(client_address));
	} while(connection_status != 0);

	if(network_socket >= 0) {
		for(int i=0;i<tx_cache.size();i+=1)
			writeSocketMessage(&tx_cache[i]);
		
		tx_cache.clear();
	}
}

//Clear the socket address.
void ClientAIBusHandler::clearSocket() {
	close(network_socket);
	network_socket = -1;
}

//Write a socket message.
void ClientAIBusHandler::writeSocketMessage(SocketMessage* msg) {
	if(msg->l + 1 > 255)
		return;

	if(network_socket < 0) {
		tx_cache.push_back(*msg);
		return;
	}

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

	int avail = rx_cache.size();
	unsigned long last_read = *timer;
	while(*timer - last_read < 20) {
		if(rx_cache.size() > avail)
			last_read = *timer;

		avail = rx_cache.size();
	}

	send(network_socket, data, byte_l, 0);
}

//Read a socket message. Return the number of bytes read.
int ClientAIBusHandler::readSocketMessage(SocketMessage* msg) {
	uint8_t data[DEFAULT_READ_LENGTH];

	const int message_size = recv(this->network_socket, data, DEFAULT_READ_LENGTH, 0);
	
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

	int start = 0;
	vector<uint8_t> full_data_vec(0);

	uint8_t main_opcode = 0;

	while(start < message_size - (strlen(SOCKET_START) + 2)) {
		const uint8_t opcode = data[strlen(SOCKET_START) + start], msg_length = data[strlen(SOCKET_START) + 1 + start]-1;
		if(msg_length > message_size - strlen(SOCKET_START) - 2 - start)
			break;

		if(main_opcode != 0 && opcode != main_opcode) {
			start += strlen(SOCKET_START) + 3 + msg_length;
			continue;
		}

		main_opcode = opcode;
		
		uint8_t msg_data[msg_length];
		for(int i=0;i<msg_length;i+=1)
			msg_data[i] = data[i+strlen(SOCKET_START) + 2 + start];

		uint8_t checksum = 0;
		for(int i=0;i<strlen(SOCKET_START) + 2 + msg_length;i+=1)
			checksum ^= data[i + start];

		if(checksum != data[strlen(SOCKET_START) + 2 + start + msg_length])
			break;

		for(int i=0;i<sizeof(msg_data);i+=1)
			full_data_vec.push_back(msg_data[i]);

		start += strlen(SOCKET_START) + 3 + msg_length;
	}

	uint8_t full_data[full_data_vec.size()];
	for(int i=0;i<sizeof(full_data);i+=1)
		full_data[i] = full_data_vec[i];

	msg->refreshSocketData(main_opcode, sizeof(full_data));
	msg->refreshSocketData(full_data);

	return message_size;
}

//Cache AIBus data from a socket message if valid.
void ClientAIBusHandler::cacheAIData(uint8_t* data, const int l) {
	if(l < 4)
		return;

	vector<uint8_t> data_vec(0);
	for(int i=0;i<l;i+=1)
		data_vec.push_back(data[i]);

	while(data_vec.size() > 0) {
		if(data_vec.size() < 4)
			return;

		const uint8_t s = data_vec[0], r = data_vec[2];
		const int expected_length = data_vec[1] + 2;

		if(data_vec.size() < expected_length)
			return;

		uint8_t chex = 0;
		for(int i=0;i<expected_length-1;i+=1)
			chex ^= data_vec[i];

		if(chex != data_vec[expected_length-1]) { //Invalid message.
			for(int i=0;i<expected_length;i+=1)
				data_vec.erase(data_vec.begin());
			
			continue;
		}
		
		if((r==my_id || r==0xFF) && s != my_id) {
			while(!cache_ok);
			check_ok = false;
			uint8_t ai_data[expected_length - 4];
			for(int i=3;i<expected_length-1;i+=1)
				ai_data[i-3] = data_vec[i];

			AIData new_msg(sizeof(ai_data), s, r, ai_data);

			if(r == my_id && new_msg.l > 0 && new_msg[0] != 0x80)
				sendAcknowledgement(r, s);

			if(new_msg.l >= 1 && new_msg[0] == 0x92) { //Resend.
				check_ok = true;
				for(auto tx_msg: recent_tx) {
					if(tx_msg.receiver == s && tx_msg.sender == r) {
						writeAIData(&tx_msg);
						break;
					}
				}
			} else if(new_msg.l >= 3 && new_msg[0] == 0x91 && (new_msg.receiver == my_id || new_msg.receiver == 0xFF)) {
				const int expected_size = new_msg[1];
				multi_cache.push_back(new_msg);

				if(multi_cache.size() >= expected_size) {
					vector<uint8_t> full_data(0);
					for(int m=0;m<expected_size;m+=1) {
						for(int i=0;i<multi_cache[m].l-3;i+=1)
							full_data.push_back(multi_cache[m][i+3]);
					}

					multi_cache.clear();

					AIData final_message(full_data.size(), s, r, full_data.data());
					rx_cache.push_back(final_message);
				}
			}
			else
				rx_cache.push_back(new_msg);
			
			check_ok = true;
		}

		for(int i=0;i<expected_length;i+=1)
			data_vec.erase(data_vec.begin());
	}
}

//Read AIBus data.
bool ClientAIBusHandler::readAIData(AIData* ai_d) {
	if(!check_ok)
		return false;

	if(rx_cache.empty())
		return false;

	check_ok = false;
	ai_d->refreshAIData(rx_cache[0]);
	rx_cache.erase(rx_cache.begin());
	check_ok = true;

	return true;
}

//Write an AIBus message.
bool ClientAIBusHandler::writeAIData(AIData* ai_d) {
	return writeAIData(ai_d, ai_d->receiver != 0xFF && (ai_d->l > 0 && ai_d->data[0] != 0x80));
}

//Write an AIBus message.
bool ClientAIBusHandler::writeAIData(AIData* ai_d, const bool ack) {
	//Something that may need to be resent later?
	if(ai_d->receiver != 0xFF && ai_d->l >= 1 && ai_d->data[0] != 0x80) {
		int index = -1;
		for(int i=0;i<recent_tx.size();i+=1) {
			AIData tx_msg = recent_tx[i];
			if(tx_msg.receiver == ai_d->receiver && tx_msg.sender == ai_d->sender) {
				index = i;
				break;
			}
		}

		if(index >= 0)
			recent_tx.erase(recent_tx.begin() + index);

		recent_tx.push_back(*ai_d);
	}

	if(ai_d->l > AIDATA_LIMIT + 3) {
		const int count = ai_d->l/AIDATA_LIMIT, r = ai_d->l%AIDATA_LIMIT;
		AIData ai_group[count + (r == 0 ? 0 : 1)];

		for(int i=0;i<sizeof(ai_group)/sizeof(AIData);i+=1) {
			const int l = i<sizeof(ai_group)/sizeof(AIData) - 1 || r == 0 ? AIDATA_LIMIT + 3 : r + 3;
			uint8_t ai_data[l];
			ai_data[0] = 0x91;
			ai_data[1] = sizeof(ai_group)/sizeof(AIData);
			ai_data[2] = i;
			for(int d=0;d<l-3;d+=1)
				ai_data[d+3] = ai_d->data[AIDATA_LIMIT*i + d];

			ai_group[i] = AIData(l, ai_d->sender, ai_d->receiver, ai_data);
		}

		while(!check_ok);
		bool rec_ack = false;
		for(int i=0;i<sizeof(ai_group)/sizeof(AIData);i+=1) {
			rec_ack = writeAIData(&ai_group[i], ack);
			if(!rec_ack && ack)
				return false;

			const unsigned long start = *timer;
			while(*timer - start < 5);
		}
		return rec_ack;
	}

	uint8_t data[ai_d->l + 4];
	ai_d->getBytes(data);

	#ifdef AIBUS_DEBUG
	cout<<"Sent: ";
	for(int i=0;i<sizeof(data);i+=1)
		cout<<hex<<int(data[i])<<' ';
	cout<<endl;
	#endif

	SocketMessage ai_socket_msg(OPCODE_AIBUS_SEND, sizeof(data));
	ai_socket_msg.refreshSocketData(data);

	this->writeSocketMessage(&ai_socket_msg);

	if(!ack || ai_d->receiver == 0xFF || (ai_d->l > 0 && ai_d->data[0] == 0x80) || timer == nullptr || timer == NULL)
		return true;

	while(!check_ok);
	bool rec_ack = false;
	int tries = 0;

	unsigned long last_send = *timer, last_read = *timer;
	const unsigned long first_read = *timer;
	
	while(!rec_ack && tries < 25) {
		if(*timer - first_read <= 1)
			continue;

		if(*timer - last_read <= 2)
			continue;

		last_read = *timer;

		vector<AIData> current_rx;
		while(!check_ok || !cache_ok);
		cache_ok = false;
		check_ok = false;
		for(int i=0;i<rx_cache.size();i+=1) {
			AIData check_msg = rx_cache[i];
			current_rx.push_back(check_msg);
		}

		for(int i=0;i<current_rx.size();i+=1) {
			if(current_rx[i].sender == ai_d->receiver && current_rx[i].receiver == ai_d->sender && current_rx[i].l > 0 && current_rx[i][0] == 0x80) {
				rec_ack = true;
				rx_cache.erase(rx_cache.begin() + i);
				break;
			}
		}
		cache_ok = true;
		check_ok = true;

		if(rec_ack)
			break;

		if(*timer - last_send > (tries > 0 ? 100 : 200)) {
			writeSocketMessage(&ai_socket_msg);
			last_send = *timer;
			tries += 1;
		}
	}

	return rec_ack;
}

//Write an acknowledgment message.
void ClientAIBusHandler::sendAcknowledgement(const uint8_t sender, const uint8_t receiver) {
	uint8_t ack_data[] = {0x80};
	AIData ack_msg(sizeof(ack_data), sender, receiver, ack_data);
	writeAIData(&ack_msg, false);
}

//Get the client socket.
int ClientAIBusHandler::getClient() {
	return this->network_socket;
}

//Get the handler ID.
uint8_t ClientAIBusHandler::getID() {
	return my_id;
}

//Determine whether a message is the initialization message.
bool getInitMessage(AIData* ai_d) {
	if(ai_d->l < 2)
		return false;

	if(ai_d->data[0] == 0x4A && ai_d->data[1] == 0x1F)
		return true;
	else
		return false;
}

//Determine whether a message is the poweroff message.
bool getPoweroffMessage(AIData* ai_d) {
	if(ai_d->l < 1)
		return false;
	
	if(ai_d->data[0] == 0xA0)
		return true;
	else
		return false;
}

//Socket thread function.
void *socketThread(void* parameters_v) {
	ClientHandlerParameters* parameters = (ClientHandlerParameters*)parameters_v;
	ClientAIBusHandler* socket_ptr = parameters->ai_handler;
	
	while(*parameters->running) {
		if(socket_ptr->getClient() < 0) {
			socket_ptr->refreshSocket(parameters->socket_path.c_str());
			continue;
		}

		SocketMessage rx_msg(0, DEFAULT_READ_LENGTH);

		const int socket_byte_count = socket_ptr->readSocketMessage(&rx_msg);

		if(socket_byte_count > 0) {
			if(rx_msg.opcode == OPCODE_AIBUS_RECEIVE) {
				uint8_t data[rx_msg.l];
				for(int i=0;i<rx_msg.l;i+=1)
					data[i] = rx_msg.data[i];

				if(parameters->process)
					socket_ptr->cacheAIData(data, sizeof(data));
			}
		} else if(socket_byte_count == 0) { //Socket closed.
			socket_ptr ->clearSocket();
		}

		usleep(1000);
	}
	
	void* result;
	return result;
}