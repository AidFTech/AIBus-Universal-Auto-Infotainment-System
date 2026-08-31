#include "Serial_AIBus_Handler.h"

#ifdef SOCKET_SERVER
SerialAIBusHandler::SerialAIBusHandler(string port, int** socket_list, const int socket_l, const uint8_t id,  unsigned long* timer) {
#else
SerialAIBusHandler::SerialAIBusHandler(string port, const uint8_t id,  unsigned long* timer) {
#endif
	this->cached_rx = vector<AIData>(0);
	this->cached_tx = vector<AIData>(0);
	this->cached_ack = vector<bool>(0);
	
	#ifdef RPI_UART
	static const char* const chip_path = "/dev/gpiochip0";

	chip = gpiod_chip_open(chip_path);
	line_ai_rx = getGPIOLine(chip, AI_RX);
	#endif

	const char* c_port = port.c_str();
	
	const int test_port = aiserialOpen(c_port);
	if(test_port<0) {
		ai_port = -1; //TODO: Throw an error.
		#ifndef RPI_UART
		cout<<"AIBus not connected."<<endl;
		#endif
	} else {
		this->ai_port = test_port;
		#ifndef RPI_UART
		port_connected = true;
		#endif
	}

	#ifdef RPI_UART
	setPinMode(line_ai_rx, PIN_MODE_INPUT_PULLUP);
	#endif

	#ifdef SOCKET_SERVER
	this->socket_list = new int*[socket_l];
	for(int i=0;i<socket_l;i+=1)
		this->socket_list[i] = socket_list[i];

	this->socket_l = socket_l;
	#endif

	this->timer = timer;
	this->id = id;
}

SerialAIBusHandler::~SerialAIBusHandler() {
	if(this->ai_port >= 0)
		aiserialClose(this->ai_port);
	#ifdef RPI_UART
	gpiod_line_release(line_ai_rx);
	gpiod_chip_close(chip);
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
	return readAIData(ai_d, cache, multiple, false);
}

//Read AIBus data.
bool SerialAIBusHandler::readAIData(AIData* ai_d, const bool cache, const bool multiple, const bool threaded) {
	if(!threaded) {
		/*while(thread_locked && cache && multiple) {
			if(main_locked)
				break;
		}*/
		if(thread_locked && cache && multiple)
			return false;
	}

	const bool last_main_locked = main_locked;
	main_locked = true;
	ai_d->refreshAIData(0, 0, 0);
	if(port_connected) {
		if(cache && cached_rx.size() > 0) {
			ai_d->refreshAIData(cached_rx.at(0));
			cached_rx.erase(cached_rx.begin());
			
			writeToSocket(ai_d);
			main_locked = last_main_locked;
			return true;
		} else if(aiserialBytesAvailable(this->ai_port) >= 4) {
			if(multiple && multi_expected_size > 0 && multi_cache.size() > 0 && *timer - last_multi_message > 500) {
				const uint8_t sender = multi_cache[0].sender;

				multi_cache.clear();
				multi_expected_size = -1;

				requestResend(sender);
			}

			const uint8_t s = uint8_t(aiserialReadByte(this->ai_port));
			const uint8_t l = uint8_t(aiserialReadByte(this->ai_port));

			if(l > AIDATA_LIMIT + 5) { //Length was misread.
				main_locked = last_main_locked;
				return false;
			}

			if(l<2) {
				while(aiserialBytesAvailable(this->ai_port) > 0)
					aiserialReadByte(this->ai_port);
				main_locked = last_main_locked;
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
						//#ifdef RPI_UART
						//if(!readPin(chip, AI_RX))
						//	clear_time = *this->timer;
						//#endif
						if(aiserialBytesAvailable(this->ai_port) > 0) {
							aiserialReadByte(this->ai_port);
							clear_time = *this->timer;
						}
					}
					main_locked = last_main_locked;
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
					main_locked = last_main_locked;
					return false;
				}
			}

			if(sizeof(d) == 3 && d[0] == s && d[1] == l && d[2] == r) {
				main_locked = last_main_locked;
				return false;
			}

			ai_d->refreshAIData(l-2, s, r);
			
			for(uint8_t i=0;i<ai_d->l;i+=1)
				ai_d->data[i] = d[i];

			if(multi_cache.size() > 0 && multiple && ai_d->l >= 3 && ai_d->data[0] == 0x91) {
				if(multi_cache[0].sender != ai_d->sender || multi_cache[0].receiver != ai_d->receiver)
					return false;
			}

			if(getID(r) && ai_d->l > 0 && ai_d->data[0] != 0x80)
				sendAcknowledgement(r, s);

			if(ai_d->l >= 1 && ai_d->data[0] == 0x92) { //Resend.
				writeToSocket(ai_d);

				if(!getID(ai_d->receiver))
					return false;
				
				for(auto tx_msg: recent_tx) {
					if(tx_msg.receiver == s && tx_msg.sender == r) {
						writeAIData(&tx_msg);
						break;
					}
				}

				return false;
			}

			if(multiple && ai_d->l >= 3 && ai_d->data[0] == 0x91 && (getID(ai_d->receiver) || ai_d->receiver == 0xFF)) { //Multiple messages are on the way.
				last_multi_message = *timer;
				if(multi_expected_size <= 0) {
					const int msg_count = ai_d->data[1];

					if(msg_count <= 0) {
						main_locked = last_main_locked;
						return false;
					}
					
					multi_expected_size = msg_count;
				}

				writeToSocket(ai_d);

				#if !defined(RPI_UART) && defined(AIBUS_PRINT)
				printBytes(ai_d);
				#endif

				multi_cache.push_back(*ai_d);

				if(multi_cache.size() >= multi_expected_size) {
					vector<uint8_t> bytes_vec;

					for(int m=0; m<multi_expected_size;m+=1) {
						AIData msg;
						for(auto test_msg: multi_cache) {
							if(test_msg.l < 3 || test_msg[0] != 0x91 || test_msg[1] != multi_expected_size)
								continue;

							if(test_msg[2] != m)
								continue;

							if(test_msg.sender != ai_d->sender || test_msg.receiver != ai_d->receiver)
								continue;

							msg = test_msg;
						}

						for(int i=3;i<msg.l;i+=1)
							bytes_vec.push_back(msg[i]);
					}

					uint8_t bytes[bytes_vec.size()];
					for(int i=0;i<bytes_vec.size();i+=1)
						bytes[i] = bytes_vec[i];

					AIData msg(sizeof(bytes), ai_d->sender, ai_d->receiver, bytes);
					//cached_rx.push_back(msg);

					multi_cache.clear();
					multi_expected_size = -1;

					ai_d->refreshAIData(msg);
					main_locked = last_main_locked;
					return true;
				}

				main_locked = last_main_locked;
				return false;
			}

			#if !defined(RPI_UART) && defined(AIBUS_PRINT)
			printBytes(ai_d);
			#endif
			
			writeToSocket(ai_d);
			main_locked = last_main_locked;
			return true;
		} else {
			main_locked = last_main_locked;
			return false;
		}
	} else {
		#ifndef RPI_UART
		string ai_t;
		getline(cin, ai_t);

		if(ai_t.empty()) {
			main_locked = last_main_locked;
			return false;
		}

		if(ai_t.find('/') != string::npos) { //Possible serial start.
			const int success = connectAIPort(ai_t);
			if(success >= 0)
				cout<<ai_t<<" connection successful!\n";
			main_locked = last_main_locked;
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

		if(d_v.size() < 2) {
			main_locked = last_main_locked;
			return false;
		}
		
		ai_d->refreshAIData(d_v.size()-2, d_v.at(0), d_v.at(1));
		for(uint8_t i=0;i<d_v.size()-2;i+=1)
			ai_d->data[i] = d_v.at(i+2);
		
		main_locked = last_main_locked;
		return true;
		#else
		main_locked = last_main_locked;
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
	/*while(thread_locked);
	main_locked = true;
	cached_tx.push_back(*ai_d);
	cached_ack.push_back(acknowledge);
	main_locked = false;*/
	return writeAIBusToSerial(ai_d, acknowledge);
}

//Write AIBus data to the cache and send as a thread.
void SerialAIBusHandler::writeToCache(AIData* ai_d, const bool acknowledge) {
	while(thread_locked);
	main_locked = true;
	cached_tx.push_back(*ai_d);
	cached_ack.push_back(acknowledge);
	main_locked = false;

	if(!cache_thread_enabled) {
		cache_thread_enabled = true;
		pthread_create(&cache_thread, NULL, flushCacheThread, (void*)this);
	}
}

//Write AIBus data to the serial port.
bool SerialAIBusHandler::writeAIBusToSerial(AIData* ai_d, const bool acknowledge) {
	bool sent = true;

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

		{
			unsigned long start = *this->timer;
			int current_cached_bytes = aiserialBytesAvailable(this->ai_port);
			while((*this->timer - start) < 1) {
				#ifdef RPI_UART
				//if(!chip) {
				//	if(!readPin(chip, AI_RX))
				//		start = *this->timer;
				//} else
				#endif
				if(current_cached_bytes != aiserialBytesAvailable(this->ai_port)) {
					current_cached_bytes = aiserialBytesAvailable(this->ai_port);
					start = *this->timer;
				}
			}
			start = *this->timer;
			while((*this->timer - start) < 1);
		}

		bool ack = false;
		for(int i=0;i<sizeof(ai_group)/sizeof(AIData);i+=1) {
			ack = writeAIBusToSerial(&ai_group[i], acknowledge);
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

		const unsigned long cache_timer = *timer;

		while(aiserialBytesAvailable(this->ai_port) >= 2 && *timer - cache_timer < 5) {
			AIData msg;
			if(readAIData(&msg, false)) {
				if(msg.sender != ai_d->sender && (msg.receiver == ai_d->sender || msg.receiver == 0xFF) && msg.l >= 1 && msg.data[0] != 0x80) {
					cached_rx.push_back(msg);
				}
			}
		}

		{
			unsigned long start = *this->timer;
			int current_cached_bytes = aiserialBytesAvailable(this->ai_port);
			while((*this->timer - start) < 5) {
				#ifdef RPI_UART
				//if(!chip) {
				//	if(!readPin(chip, AI_RX))
				//		start = *this->timer;
				//} else
				#endif
				if(current_cached_bytes != aiserialBytesAvailable(this->ai_port)) {
					current_cached_bytes = aiserialBytesAvailable(this->ai_port);
					start = *this->timer;
				}
			}
			start = *this->timer;
			while((*this->timer - start) < 1);
		}

		if(ai_d->l == 2 && ai_d->data[0] == ai_d->sender && data[sizeof(data)-1] == ai_d->receiver) {
			AIData ext_msg(ai_d->l + 1, ai_d->sender, ai_d->receiver);
			for(int i=0;i<ai_d->l;i+=1)
				ext_msg[i] = ai_d->data[i];
			ext_msg[ext_msg.l-1] = 0;

			uint8_t ext_data[ext_msg.l + 4];
			ext_msg.getBytes(ext_data);

			aiserialWrite(this->ai_port, (char*)ext_data, sizeof(ext_data));
		} else {
			aiserialWrite(this->ai_port, (char*)data, sizeof(data));
		}

		/*while(aiserialBytesAvailable(this->ai_port) >= 2) {
			AIData msg;
			if(readAIData(&msg, false)) {
				if(msg.sender != ai_d->sender && (msg.receiver == ai_d->sender || msg.receiver == 0xFF) && msg.l >= 1 && msg.data[0] != 0x80) {
					cached_vec.push_back(msg);
				}
			}
		}*/

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

	writeAIBusToSerial(&ack_msg, false);
}

//Send a resend request.
void SerialAIBusHandler::requestResend(const uint8_t r) {
	requestResend(id, r);
}

//Send a resend request.
void SerialAIBusHandler::requestResend(const uint8_t s, const uint8_t r) {
	if(r == 0xFF)
		return;

	uint8_t resend_data[] = {0x92};
	AIData resend_msg(sizeof(resend_data), s, r, resend_data);
	writeAIData(&resend_msg);
}

//Wait for the acknowledgement message.
bool SerialAIBusHandler::awaitAcknowledgement(AIData* ai_d) {
	unsigned long repeat_time = *this->timer;
	bool acknowledge = false;
	uint8_t tries = 0;

	while(!acknowledge && tries < MAX_REPEAT) {
		AIData new_msg;
		if(readAIData(&new_msg, false, true, true)) {
			if(new_msg.sender == ai_d->sender)
				continue;

			if(new_msg.sender == ai_d->receiver && new_msg.receiver == ai_d->sender && new_msg.data[0] == 0x80) {
				acknowledge = true;
				break;
			} else {
				if(new_msg.sender != ai_d->sender && (new_msg.receiver == ai_d->sender || new_msg.receiver == 0xFF) && new_msg.l >= 1 && new_msg.data[0] != 0x80) {
					cached_rx.push_back(new_msg);
				}
				repeat_time = *this->timer;
			}
		}

		for(int i=0;i<cached_rx.size();i+=1) {
			AIData* rx_msg = &cached_rx.at(i);
			if(rx_msg->sender == ai_d->receiver && rx_msg->receiver == ai_d->sender && rx_msg->l > 0 && rx_msg->data[0] == 0x80) {
				acknowledge = true;
				break;
			}
		}

		if(acknowledge)
			break;

		if((*this->timer-repeat_time) > REPEAT_DELAY && !acknowledge) {
			if(ai_d->l == 2 && ai_d->data[0] != 0xA1) {
				AIData padded_msg(ai_d->l + 1, ai_d->sender, ai_d->receiver, ai_d->data);
				padded_msg[padded_msg.l-1] = 0x0;
				writeAIBusToSerial(&padded_msg, false);
			} else
				writeAIBusToSerial(ai_d, false);
				
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
		if(!cache || cached_rx.size() <= 0)
			return aiserialBytesAvailable(this->ai_port);
		else
			return cached_rx.at(0).l + 4;
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
	return cachePending(false);
}

//Cache any pending messages.
bool SerialAIBusHandler::cachePending(const bool threaded) {
	if(!port_connected)
		return false;

	if(aiserialBytesAvailable(ai_port) > 0) {
		AIData ai_msg;
		if(readAIData(&ai_msg, false, true, threaded)) {
			if(ai_msg.receiver == this->id || ai_msg.receiver == 0xFF) {
				if(ai_msg.sender != this->id && ai_msg.l >= 1 && ai_msg.data[0] != 0x80) {
					cached_rx.push_back(ai_msg);
					return true;
				}
			}
		}
	}
	
	return false;
}

//Cache a message.
void SerialAIBusHandler::cacheMessage(AIData* ai_msg) {
	if(ai_msg->receiver == this->id || ai_msg->receiver == 0xFF) {
		if(ai_msg->sender != this->id && ai_msg->l >= 1 && ai_msg->data[0] != 0x80) {
			cached_rx.push_back(*ai_msg);
		}
	}
}

//Send all cached messages. Return whether successful.
bool SerialAIBusHandler::flushCached() {
	cache_thread_enabled = true;
	int l = cached_tx.size();
	unsigned long cache_timer = *timer;

	//Wait for the cache to reach its full size.
	while(*timer - cache_timer < 20) {
		if(cached_tx.size() != l) {
			l = cached_tx.size();
			cache_timer = *timer;
		}
	}

	bool ack = true;

	while(main_locked);

	thread_locked = true;

	while(this->cached_tx.size() > 0 && this->cached_ack.size() > 0) {
		ack &= writeAIBusToSerial(&this->cached_tx.at(0), cached_ack.at(0));

		cached_tx.erase(cached_tx.begin());
		cached_ack.erase(cached_ack.begin());
	}
	thread_locked = false;

	cache_thread_enabled = false;
	return ack;
}

//Read a multi-send message. Threaded.
void SerialAIBusHandler::readMultiple(const uint8_t sender, const uint8_t receiver, vector<uint8_t> data, const int message_count) {
	thread_locked = true;

	uint8_t full_data[AIDATA_LIMIT*message_count];
	for(int i=0;i<data.size();i+=1)
		full_data[i] = data.at(i);

	int full_length = data.size();

	unsigned long start = *timer;
	int m = 1;
	bool message_complete = false;
	while(m < message_count && *timer - start < 200) {
		AIData test_msg;
		if(readAIData(&test_msg, false, false, true)) {
			if(test_msg.l < 3 || test_msg.receiver != receiver || test_msg.sender != sender || test_msg.data[0] != 0x91 || test_msg.data[1] != message_count) {
				if((test_msg.receiver == receiver || test_msg.receiver == 0xFF) && test_msg.l > 0 && test_msg.data[0] != 0x80)
					cacheMessage(&test_msg); 
				continue;
			}

			const uint8_t index = test_msg[2];

			if(index < m)
				continue;

			for(int i=0;i<test_msg.l-3;i+=1)
				full_data[(AIDATA_LIMIT)*index + i] = test_msg[i+3];

			full_length += test_msg.l - 3;

			m += 1;
			start = *timer;
		}

		if(m == message_count) {
			message_complete = true;
			break;
		}
	}

	if(message_complete) {
		AIData ai_d(full_length, sender, receiver, full_data);
		cached_rx.push_back(ai_d);
	}
	
	thread_locked = false;
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

#ifdef RPI_UART
//Get the GPIO chip.
gpiod_chip* SerialAIBusHandler::getChip() {
	return this->chip;
}

//Get a line.
gpiod_line* getGPIOLine(gpiod_chip* chip, const int pin) {
	return gpiod_chip_get_line(chip, pin);
}

//Set a GPIO pin mode.
void setPinMode(gpiod_line* line, const pin_mode_t mode) {
	switch(mode) {
	case PIN_MODE_INPUT_PULLUP:
		gpiod_line_request_input_flags(line, "Consumer", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
		break;
	case PIN_MODE_INPUT:
		gpiod_line_request_input(line, "Consumer");
		break;
	case PIN_MODE_OUTPUT:
		gpiod_line_request_output(line, "Consumer", 0);
		break;
	default:
		break;
	}
}

//Read a GPIO pin.
bool readPin(gpiod_line* line) {
	const bool val = gpiod_line_get_value(line) > 0;
	return val;
}

//Write the provided state to the GPIO pin.
int writePin(gpiod_line* line, const bool state) {
	return gpiod_line_set_value(line, state ? 1 : 0);
}
#endif

void* flushCacheThread(void* v_aibus_handler) {
	SerialAIBusHandler* aibus_handler = (SerialAIBusHandler*)v_aibus_handler;
	aibus_handler->flushCached();

	void* the_return;
	return the_return;
}
