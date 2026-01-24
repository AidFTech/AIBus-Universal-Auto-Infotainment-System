#include "Serial_AIBus_Handler.h"

#ifdef RPI_UART
#ifdef SOCKET_SERVER
SerialAIBusHandler::SerialAIBusHandler(string port, int** socket_list, const int socket_l, const uint8_t id,  unsigned long* timer) {
#else
SerialAIBusHandler::SerialAIBusHandler(string port, const uint8_t id,  unsigned long* timer) {
#endif
	this->cached_vec = vector<AIData>(0);
	gpioCfgSetInternals(1<<10);
	gpioInitialise();

	const char* c_port = port.c_str();
	
	const int test_port = aiserialOpen(c_port);
	if(test_port<0) {
		ai_port = 0; //TODO: Throw an error.
		//cout<<"AIBus not connected.\n";
	} else 
		this->ai_port = test_port;

	gpioSetMode(AI_RX, PI_INPUT);
	gpioSetPullUpDown(AI_RX, PI_PUD_UP);

	#ifdef SOCKET_SERVER
	this->socket_list = new int*[socket_l];
	for(int i=0;i<socket_l;i+=1)
		this->socket_list[i] = socket_list[i];

	this->socket_l = socket_l;
	#endif

	this->timer = timer;
	this->id = id;
}
#else
#ifdef SOCKET_SERVER
SerialAIBusHandler::SerialAIBusHandler(int** socket_list, const int socket_l, const uint8_t id, unsigned long* timer) {
#else
SerialAIBusHandler::SerialAIBusHandler(const uint8_t id, unsigned long* timer) {
#endif
	this->cached_vec = vector<AIData>(0);
	cout<<"Ready!\nEnter the sender, receiver, and data. Separate all characters with a space. Do not include the checksum.\n";

	#ifdef SOCKET_SERVER
	this->socket_list = new int*[socket_l];
	for(int i=0;i<socket_l;i+=1)
		this->socket_list[i] = socket_list[i];

	this->socket_l = socket_l;
	#endif
	this->timer = timer;
	this->id = id;
}
#endif

SerialAIBusHandler::~SerialAIBusHandler() {
	if(this->ai_port >= 0)
		aiserialClose(this->ai_port);
	#ifdef RPI_UART
	gpioTerminate();
	#endif

	#ifdef SOCKET_SERVER
	delete[] this->socket_list;
	#endif
}

#ifndef RPI_UART
int SerialAIBusHandler::connectAIPort(string port) {
	const char* c_port = port.c_str();
	
	const int test_port = aiserialOpen(c_port);
	if(test_port<0) {
		return -1; //TODO: Throw an error.
	} else {
		this->ai_port = test_port;
		port_connected = true;
		return this->ai_port;
	}
}
#endif

//Read AIBus data.
bool SerialAIBusHandler::readAIData(AIData* ai_d) {
	return readAIData(ai_d, true);
}

//Read AIBus data.
bool SerialAIBusHandler::readAIData(AIData* ai_d, const bool cache, const bool multiple) {
	ai_d->refreshAIData(0, 0, 0);
	if(port_connected) {
		if(cache && cached_msg.l > 0) {
			ai_d->refreshAIData(cached_msg);

			if(cached_vec.size() > 0) {
				cached_msg.refreshAIData(cached_vec.at(0));
				cached_vec.erase(cached_vec.begin());
			} else {
				cached_msg.refreshAIData(0,0,0);
			}
			
			writeToSocket(ai_d);
			return true;
		} else if(aiserialBytesAvailable(this->ai_port) >= 2) {
			const uint8_t s = uint8_t(aiserialReadByte(this->ai_port));
			const uint8_t l = uint8_t(aiserialReadByte(this->ai_port));

			if(l<2) {
				while(aiserialBytesAvailable(this->ai_port) > 0)
					aiserialReadByte(this->ai_port);
				return false;
			}
			
			unsigned long start = *this->timer;
			while(aiserialBytesAvailable(this->ai_port) < l) {
				#ifdef RPI_UART
				//if(gpioRead(AI_RX) == 0)
				//	start = *this->timer;
				#endif
				
				if((*this->timer - start) > 5) {
					unsigned long clear_time = *this->timer;
					
					while((*this->timer - clear_time) < 1) {
						#ifdef RPI_UART
						if(gpioRead(AI_RX) == 0)
							clear_time = *this->timer;
						#endif
						if(aiserialBytesAvailable(this->ai_port) > 0) {
							aiserialReadByte(this->ai_port);
							clear_time = *this->timer;
						}
					}
					return false;
				}
			}

			const uint8_t r = uint8_t(aiserialReadByte(this->ai_port));

			char d_c[l-1];
			aiserialRead(this->ai_port, d_c, l-1);
			
			uint8_t d[l-1];
			for(uint8_t i=0;i<l-1;i+=1)
				d[i] = uint8_t(d_c[i]);
			
			{
				uint8_t chex[l+2];
				chex[0] = s;
				chex[1] = l;
				chex[2] = r;
				for(uint8_t i=0;i<l-1;i+=1)
					chex[i+3] = d[i];

				if(!checkValidity(chex,l+2)) {
					while(aiserialBytesAvailable(this->ai_port) > 0)
						aiserialReadByte(this->ai_port);
					return false;
				}
			}

			ai_d->refreshAIData(l-2, s, r);
			
			for(uint8_t i=0;i<ai_d->l;i+=1)
				ai_d->data[i] = d[i];

			if(multiple && ai_d->l >= 3 && ai_d->data[0] == 0x91 && (getID(ai_d->receiver) || ai_d->receiver == 0xFF)) { //Multiple messages are on the way.
				if(getID(ai_d->receiver))
					sendAcknowledgement(ai_d->receiver, ai_d->sender);

				if(ai_d->data[2] != 0)
					return false;

				const int msg_count = ai_d->data[1];
				uint8_t full_data[msg_count*AIDATA_LIMIT];
				int full_length = ai_d->l - 3;

				if(msg_count <= 0)
					return true;

				writeToSocket(ai_d);

				for(int i=0;i<ai_d->l - 3;i+=1)
					full_data[i] = ai_d->data[i+3];
				int m = 1;
				unsigned long ai_timer = *timer;


				AIData test_msg;
				while(m < msg_count && *timer - ai_timer < 200) {
					if(readAIData(&test_msg, true, false)) {
						if(test_msg.l < 3 || test_msg.receiver != ai_d->receiver || test_msg.sender != ai_d->sender || test_msg.data[0] != 0x91 || test_msg.data[1] != msg_count) {
							if((test_msg.receiver == ai_d->receiver || test_msg.receiver == 0xFF) && test_msg.l > 0 && test_msg.data[0] != 0x80)
								cacheMessage(&test_msg); 
							continue;
						}

						if(getID(test_msg.receiver))
							sendAcknowledgement(test_msg.receiver, test_msg.sender);

						for(int i=0;i<test_msg.l-3;i+=1)
							full_data[(AIDATA_LIMIT)*m + i] = test_msg[i+3];

						full_length += test_msg.l - 3;

						m += 1;
						ai_timer = *timer;
					}

					if(*timer - ai_timer > 200)
						return false;
				}

				ai_d->refreshAIData(full_length, ai_d->sender, ai_d->receiver);
				ai_d->refreshAIData(full_data);

				#if !defined(RPI_UART) && defined(AIBUS_PRINT)
				printBytes(ai_d);
				#endif

				return true;
			}

			#if !defined(RPI_UART) && defined(AIBUS_PRINT)
			printBytes(ai_d);
			#endif
			
			writeToSocket(ai_d);
			return true;
		} else {
			return false;
		}

	} else {
		#ifndef RPI_UART
		string ai_t;
		getline(cin, ai_t);

		if(ai_t.empty())
			return false;

		if(ai_t.find('/') != string::npos) { //Possible serial start.
			const int success = connectAIPort(ai_t);
			if(success >= 0)
				cout<<ai_t<<" connection successful!\n";
			return false;
		}

		int pos = 0;
		vector<uint8_t> d_v;
		while(pos < ai_t.length()) {
			int space_index = ai_t.find_first_of(' ', pos);
			if(space_index == pos) {
				pos += 1;
				continue;
			}

			if(space_index < 0)
				space_index = ai_t.length();

			string substr = ai_t.substr(pos, space_index-pos);
			
			const uint8_t data = stringToNumber(substr)&0xFF;
			d_v.push_back(data);

			pos = space_index + 1;
		}

		if(d_v.size() < 2)
			return false;
		
		ai_d->refreshAIData(d_v.size()-2, d_v.at(0), d_v.at(1));
		for(uint8_t i=0;i<d_v.size()-2;i+=1)
			ai_d->data[i] = d_v.at(i+2);
		
		return true;
		#else
		return false;
		#endif
	}
}

//Read AIBus data from bytes.
bool readAIByteData(AIData* ai_d, uint8_t* data, const uint8_t d_l) {
	if(d_l < 2)
		return false;
	
	const uint8_t l=data[1]-2;
	if(!checkValidity(data, l+4))
		return false;
	
	ai_d->refreshAIData(l, data[0], data[2]);
	
	for(uint8_t i=0;i<ai_d->l;i+=1)
		ai_d->data[i] = data[i+3];
	
	return true;
}

//Write AIBus data.
bool SerialAIBusHandler::writeAIData(AIData* ai_d) {
	return writeAIData(ai_d, ai_d->receiver != 0xFF && ai_d->data[0] != 0x80);
}

//Write AIBus data. Wait for an acknowledgement message if acknowledge is true.
bool SerialAIBusHandler::writeAIData(AIData* ai_d, const bool acknowledge) {
	bool sent = true;

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

		{
			unsigned long start = *this->timer;
			#ifndef RPI_UART
			int current_cached_bytes = aiserialBytesAvailable(this->ai_port);
			#endif
			while((*this->timer - start) < 1) {
				#ifdef RPI_UART
				if(gpioRead(AI_RX) == 0)
					start = *this->timer;
				#else
				if(current_cached_bytes != aiserialBytesAvailable(this->ai_port)) {
					current_cached_bytes = aiserialBytesAvailable(this->ai_port);
					start = *this->timer;
				}
				#endif
			}
			start = *this->timer;
			while((*this->timer - start) < 1);
		}

		bool ack = false;
		for(int i=0;i<sizeof(ai_group)/sizeof(AIData);i+=1) {
			ack = writeAIData(&ai_group[i], acknowledge);
			if(!ack && acknowledge)
				return false;

			const unsigned long start = *timer;
			while(*timer - start < 5);
		}
		return ack;
	}

	if(port_connected) {
		uint8_t data[ai_d->l + 4];
		ai_d->getBytes(data);

		while(aiserialBytesAvailable(this->ai_port) >= 2) {
			AIData msg;
			if(readAIData(&msg, false)) {
				if(msg.sender != ai_d->sender && (msg.receiver == ai_d->sender || msg.receiver == 0xFF) && msg.l >= 1 && msg.data[0] != 0x80) {
					sendAcknowledgement(msg.receiver, msg.sender);
					
					if(cached_msg.l <= 0)
						cached_msg.refreshAIData(msg);
					else {
						/*uint8_t data[msg.l+4];
						msg.getBytes(data);

						for(int i=0;i<sizeof(data);i+=1)
							cached_vec.push_back(data[i]);*/
						cached_vec.push_back(msg);
					}
				}
			}
		}

		{
			unsigned long start = *this->timer;
			#ifndef RPI_UART
			int current_cached_bytes = aiserialBytesAvailable(this->ai_port);
			#endif
			while((*this->timer - start) < 1) {
				#ifdef RPI_UART
				if(gpioRead(AI_RX) == 0)
					start = *this->timer;
				#else
				if(current_cached_bytes != aiserialBytesAvailable(this->ai_port)) {
					current_cached_bytes = aiserialBytesAvailable(this->ai_port);
					start = *this->timer;
				}
				#endif
			}
			start = *this->timer;
			while((*this->timer - start) < 1);
		}

		for(uint8_t i=0;i<ai_d->l+4;i+=1)
			aiserialWriteByte(this->ai_port, data[i]);

		while(aiserialBytesAvailable(this->ai_port) >= 2) {
			AIData msg;
			if(readAIData(&msg, false)) {
				if(msg.sender != ai_d->sender && (msg.receiver == ai_d->sender || msg.receiver == 0xFF) && msg.l >= 1 && msg.data[0] != 0x80) {
					sendAcknowledgement(msg.receiver, msg.sender);
					
					if(cached_msg.l <= 0)
						cached_msg.refreshAIData(msg);
					else {
						/*uint8_t data[msg.l+4];
						msg.getBytes(data);

						for(int i=0;i<sizeof(data);i+=1)
							cached_vec.push_back(data[i]);*/
						cached_vec.push_back(msg);
					}
				}
			}
		}

		if(acknowledge)
			sent = awaitAcknowledgement(ai_d);
	} else {
		#ifndef RPI_UART
		printBytes(ai_d);
		#endif
	}
	return sent;
}

//Determine whether the provided ID is valid.
bool SerialAIBusHandler::getID(const uint8_t id) {
	return this->id == id;
}

//Send the acknowledgement message.
void SerialAIBusHandler::sendAcknowledgement(const uint8_t sender, const uint8_t receiver) {
	AIData ack_msg(1, sender, receiver);
	ack_msg.data[0] = 0x80;

	writeAIData(&ack_msg, false);
}

//Wait for the acknowledgement message.
bool SerialAIBusHandler::awaitAcknowledgement(AIData* ai_d) {
	unsigned long repeat_time = *this->timer;
	bool acknowledge = false;
	uint8_t tries = 0;

	while(!acknowledge && tries < MAX_REPEAT) {
		AIData new_msg;
		if(readAIData(&new_msg, false)) {
			if(new_msg.sender == ai_d->sender)
				continue;

			if(new_msg.sender == ai_d->receiver && new_msg.receiver == ai_d->sender && new_msg.data[0] == 0x80){
				acknowledge = true;
				break;
			} else {
				if(new_msg.sender != ai_d->sender && (new_msg.receiver == ai_d->sender || new_msg.receiver == 0xFF) && new_msg.l >= 1 && new_msg.data[0] != 0x80) {
					sendAcknowledgement(new_msg.receiver, new_msg.sender);
					
					if(cached_msg.l <= 0)
						cached_msg.refreshAIData(new_msg);
					else {
						/*uint8_t data[new_msg.l+4];
						new_msg.getBytes(data);

						for(int i=0;i<sizeof(data);i+=1)
							cached_vec.push_back(data[i]);*/
						cached_vec.push_back(new_msg);
					}
				}
				repeat_time = *this->timer;
			}
		}

		if((*this->timer-repeat_time) > REPEAT_DELAY && !acknowledge) {
			if(ai_d->l == 2 && ai_d->data[0] != 0xA1) {
				AIData padded_msg(ai_d->l + 1, ai_d->sender, ai_d->receiver, ai_d->data);
				padded_msg[padded_msg.l-1] = 0x0;
				writeAIData(&padded_msg, false);
			} else
				writeAIData(ai_d, false);
				
			repeat_time = *this->timer;
			tries += 1;
		}
	}
	
	return acknowledge;
}

//Get the number of available bytes.
int SerialAIBusHandler::getAvailableBytes() {
	return getAvailableBytes(true);
}

//Get the number of available bytes.
int SerialAIBusHandler::getAvailableBytes(const bool cache) {
	if(port_connected) {
		if(!cache || cached_msg.l <= 0)
			return aiserialBytesAvailable(this->ai_port);
		else
			return cached_msg.l + 4;
	} else
		return -1;
}

//Get a pointer to the port.
int* SerialAIBusHandler::getPortPointer() {
	return &this->ai_port;
}

//Get whether the port is connected.
bool SerialAIBusHandler::getConnected() {
	return port_connected;
}

//Cache any pending messages.
bool SerialAIBusHandler::cachePending() {
	if(!port_connected)
		return false;

	if(aiserialBytesAvailable(ai_port) > 0) {
		AIData ai_msg;
		if(readAIData(&ai_msg)) {
			if(ai_msg.receiver == this->id || ai_msg.receiver == 0xFF) {
				if(ai_msg.sender != this->id && ai_msg.l >= 1 && ai_msg.data[0] != 0x80) {
					sendAcknowledgement(ai_msg.receiver, ai_msg.sender);

					if(cached_msg.l <= 0)
						cached_msg.refreshAIData(ai_msg);
					else {
						/*uint8_t data[ai_msg.l+4];
						ai_msg.getBytes(data);

						for(int i=0;i<sizeof(data);i+=1)
							cached_vec.push_back(data[i]);*/
						cached_vec.push_back(ai_msg);
					}
				}
				return true;
			}
		}
	}
	
	return false;
}

//Cache a message.
void SerialAIBusHandler::cacheMessage(AIData* ai_msg) {
	if(ai_msg->receiver == this->id || ai_msg->receiver == 0xFF) {
		if(ai_msg->sender != this->id && ai_msg->l >= 1 && ai_msg->data[0] != 0x80) {
			sendAcknowledgement(ai_msg->receiver, ai_msg->sender);

			if(cached_msg.l <= 0)
				cached_msg.refreshAIData(*ai_msg);
			else {
				/*uint8_t data[ai_msg->l+4];
				ai_msg->getBytes(data);

				for(int i=0;i<sizeof(data);i+=1)
					cached_vec.push_back(data[i]);*/
				cached_vec.push_back(*ai_msg);
			}
		}
	}
}

//Cache a message to be sent later.
void SerialAIBusHandler::cacheTxMessage(AIData* ai_msg) {
	if(this->cached_tx.l == 0)
		this->cached_tx.refreshAIData(*ai_msg);
}

//Send a cached message. Return whether successful.
bool SerialAIBusHandler::flushCached() {
	if(this->cached_tx. l <= 0)
		return true;
	
	bool ack = false;
	ack = writeAIData(&this->cached_tx);

	cached_tx.refreshAIData(0,0,0);

	return ack;
}

//Write a message to a socket.
void SerialAIBusHandler::writeToSocket(AIData* ai_d) {
	#ifdef SOCKET_SERVER
	for(int i=0;i<this->socket_l;i+=1) {
		if(*this->socket_list[i] >= 0) {
			uint8_t ai_bytes[ai_d->l + 4];
			ai_d->getBytes(ai_bytes);

			SocketMessage ai_sock_msg(OPCODE_AIBUS_SEND, sizeof(ai_bytes));
			ai_sock_msg.refreshSocketData(ai_bytes);
			writeSocketMessage(&ai_sock_msg, *this->socket_list[i]);
		}
	}
	#endif
}

#ifndef RPI_UART
uint16_t stringToNumber(string str) {
	uint16_t the_return = 0;
	
	for(int i=0;i<str.length() && i < 4;i+=1) {
		the_return <<= 4;
		switch(str[i]) {
			case '0':
				the_return |= 0x0;
				break;
			case '1':
				the_return |= 0x1;
				break;
			case '2':
				the_return |= 0x2;
				break;
			case '3':
				the_return |= 0x3;
				break;
			case '4':
				the_return |= 0x4;
				break;
			case '5':
				the_return |= 0x5;
				break;
			case '6':
				the_return |= 0x6;
				break;
			case '7':
				the_return |= 0x7;
				break;
			case '8':
				the_return |= 0x8;
				break;
			case '9':
				the_return |= 0x9;
				break;
			case 'A':
			case 'a':
				the_return |= 0xA;
				break;
			case 'B':
			case 'b':
				the_return |= 0xB;
				break;
			case 'C':
			case 'c':
				the_return |= 0xC;
				break;
			case 'D':
			case 'd':
				the_return |= 0xD;
				break;
			case 'E':
			case 'e':
				the_return |= 0xE;
				break;
			case 'F':
			case 'f':
				the_return |= 0xF;
				break;
		}
	}

	return the_return;
}
#endif

//Determine whether a message the initialization message.
bool getInitMessage(AIData* ai_d) {
	if(ai_d->l < 2)
		return false;

	if(ai_d->data[0] == 0x4A && ai_d->data[1] == 0x1F)
		return true;
	else
		return false;
}

//Determine whether a message is the poweroff message.
bool getPowerOffMessage(AIData* ai_d) {
	if(ai_d->l < 1)
		return false;
	
	if(ai_d->data[0] == 0xA0)
		return true;
	else
		return false;
}

void printBytes(AIData* ai_d) {
	const uint8_t l = ai_d->l + 4;
	uint8_t data[l];
	ai_d->getBytes(data);
	#ifndef RPI_UART
	for(uint8_t i=0;i<l;i+=1)
		cout<<hex<<int(data[i])<<" "<<dec;
	cout<<'\n';
	#endif
}
