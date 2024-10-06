#include "AIBus_Handler.h"

#ifdef RPI_UART
AIBusHandler::AIBusHandler(std::string port) {
	this->cached_msg = std::vector<AIData>(0);
	gpioCfgSetInternals(1<<10);
	gpioInitialise();

	char c_port[port.length() + 1];
	for(uint8_t i=0;i<port.length();i+=1)
		c_port[i] = port[i];

	c_port[port.length()] = '\0';
	
	const int test_port = aiserialOpen(c_port);
	if(test_port<0) {
		ai_port = 0; //TODO: Throw an error.
	} else 
		this->ai_port = test_port;

	gpioSetMode(AI_RX, PI_INPUT);
	gpioSetPullUpDown(AI_RX, PI_PUD_UP);
}
#else
AIBusHandler::AIBusHandler() {
	this->cached_bytes = std::vector<uint8_t>(0);
	std::cout<<"Ready!\nEnter the sender, receiver, and data. Separate all characters with a space. Do not include the checksum.\n";
}
#endif

AIBusHandler::~AIBusHandler() {
	if(this->ai_port >= 0)
		aiserialClose(this->ai_port);
	#ifdef RPI_UART
	gpioTerminate();
	#endif
}

#ifndef RPI_UART
int AIBusHandler::connectAIPort(std::string port) {
	char c_port[port.length() + 1];
	for(uint8_t i=0;i<port.length();i+=1)
		c_port[i] = port[i];

	c_port[port.length()] = '\0';
	
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
bool AIBusHandler::readAIData(AIData* ai_d) {
	return readAIData(ai_d, true);
}

//Read AIBus data.
bool AIBusHandler::readAIData(AIData* ai_d, const bool cache) {
	ai_d->refreshAIData(0, 0, 0);
	if(port_connected) {
		if(cache && cached_msg.l > 0) {
			ai_d->refreshAIData(cached_msg);

			if(cached_bytes.size() > 4) {
				const int l = cached_bytes.at(1);
				if(cached_bytes.size() < l + 2) {
					cached_bytes.clear();
					return true;
				}

				uint8_t data[l+2];
				for(int i=0;i<l+2;i+=1) {
					data[i] = cached_bytes.at(0);
					cached_bytes.erase(cached_bytes.begin());
				}

				readAIByteData(&cached_msg, data, sizeof(data));

			} else {
				if(cached_bytes.size() > 0)
					cached_bytes.clear();

				cached_msg.refreshAIData(0,0,0);
			}
			return true;
		} else if(aiserialBytesAvailable(this->ai_port) >= 2) {
			const uint8_t s = uint8_t(aiserialReadByte(this->ai_port));
			const uint8_t l = uint8_t(aiserialReadByte(this->ai_port));

			if(l<2) {
				while(aiserialBytesAvailable(this->ai_port) > 0)
					aiserialReadByte(this->ai_port);
				return false;
			}
			
			clock_t start = clock();
			while(aiserialBytesAvailable(this->ai_port) < l) {
				#ifdef RPI_UART
				if(gpioRead(AI_RX) == 0)
					start = clock();
				#endif
				
				if((clock() - start)/(CLOCKS_PER_SEC/1000) > 5) {
					clock_t clear_time = clock();
					
					while((clock() - clear_time)/(CLOCKS_PER_SEC/1000000) < 20) {
						#ifdef RPI_UART
						if(gpioRead(AI_RX) == 0)
							clear_time = clock();
						#endif
						if(aiserialBytesAvailable(this->ai_port) > 0) {
							aiserialReadByte(this->ai_port);
							clear_time = clock();
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

			#if !defined(RPI_UART) && defined(AIBUS_PRINT)
			printBytes(ai_d);
			#endif
			return true;
		} else {
			return false;
		}

	} else {
		#ifndef RPI_UART
		std::string ai_t;
		std::getline(std::cin, ai_t);

		if(ai_t.empty())
			return false;

		if(ai_t.find('/') != std::string::npos) { //Possible serial start.
			const int success = connectAIPort(ai_t);
			if(success >= 0)
				std::cout<<ai_t<<" connection successful!\n";
			return false;
		}

		int pos = 0;
		std::vector<uint8_t> d_v;
		while(pos < ai_t.length()) {
			int space_index = ai_t.find_first_of(' ', pos);
			if(space_index == pos) {
				pos += 1;
				continue;
			}

			if(space_index < 0)
				space_index = ai_t.length();

			std::string substr = ai_t.substr(pos, space_index-pos);
			
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
void AIBusHandler::writeAIData(AIData* ai_d) {
	writeAIData(ai_d, ai_d->receiver != 0xFF && ai_d->data[0] != 0x80);
}

//Write AIBus data. Wait for an acknowledgement message if acknowledge is true.
void AIBusHandler::writeAIData(AIData* ai_d, const bool acknowledge) {
	if(port_connected) {
		uint8_t* data = new uint8_t[ai_d->l + 4];
		ai_d->getBytes(data);

		while(aiserialBytesAvailable(this->ai_port) >= 2) {
			AIData msg;
			if(readAIData(&msg, false)) {
				if(msg.sender != ai_d->sender && (msg.receiver == ai_d->sender || msg.receiver == 0xFF) && msg.l >= 1 && msg.data[0] != 0x80) {
					sendAcknowledgement(msg.receiver, msg.sender);
					
					if(cached_msg.l <= 0)
						cached_msg.refreshAIData(msg);
					else {
						uint8_t data[msg.l+4];
						msg.getBytes(data);

						for(int i=0;i<sizeof(data);i+=1)
							cached_bytes.push_back(data[i]);
					}
				}
			}
		}

		if(ai_d->l > 1 || (ai_d->l >= 1 && ai_d->data[0] != 0x80)) {
			clock_t start = clock();
			#ifndef RPI_UART
			int cached_bytes = aiserialBytesAvailable(this->ai_port);
			#endif
			while((clock() - start)/(CLOCKS_PER_SEC/1000000) < 20) {
				#ifdef RPI_UART
				if(gpioRead(AI_RX) == 0)
					start = clock();
				#else
				if(cached_bytes != aiserialBytesAvailable(this->ai_port)) {
					cached_bytes = aiserialBytesAvailable(this->ai_port);
					start = clock();
				}
				#endif
			}
			start = clock();
			while((clock() - start)/(CLOCKS_PER_SEC/1000000) < 20);
		}

		for(uint8_t i=0;i<ai_d->l+4;i+=1)
			aiserialWriteByte(this->ai_port, data[i]);

		if(ai_d->l > 1 || (ai_d->l >= 1 && ai_d->data[0] != 0x80)) {
			while(aiserialBytesAvailable(this->ai_port) >= 2) {
				AIData msg;
				if(readAIData(&msg, false)) {
					if(msg.sender != ai_d->sender && (msg.receiver == ai_d->sender || msg.receiver == 0xFF) && msg.l >= 1 && msg.data[0] != 0x80) {
						sendAcknowledgement(msg.receiver, msg.sender);
						
						if(cached_msg.l <= 0)
							cached_msg.refreshAIData(msg);
						else {
							uint8_t data[msg.l+4];
							msg.getBytes(data);

							for(int i=0;i<sizeof(data);i+=1)
								cached_bytes.push_back(data[i]);
						}
					}
				}
			}
		}

		if(acknowledge)
			awaitAcknowledgement(ai_d);
	} else {
		#ifndef RPI_UART
		printBytes(ai_d);
		#endif
	}
}

//Send the acknowledgement message.
void AIBusHandler::sendAcknowledgement(const uint8_t sender, const uint8_t receiver) {
	AIData ack_msg(1, sender, receiver);
	ack_msg.data[0] = 0x80;

	writeAIData(&ack_msg, false);
}

//Wait for the acknowledgement message.
bool AIBusHandler::awaitAcknowledgement(AIData* ai_d) {
	clock_t repeat_time = clock();
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
						uint8_t data[new_msg.l+4];
						new_msg.getBytes(data);

						for(int i=0;i<sizeof(data);i+=1)
							cached_bytes.push_back(data[i]);
					}
				}
				repeat_time = clock();
			}
		}

		if((clock()-repeat_time)/(CLOCKS_PER_SEC/1000) > REPEAT_DELAY && !acknowledge) {
			writeAIData(ai_d, false);
			repeat_time = clock();
			tries += 1;
		}
	}
	
	return acknowledge;
}

//Get the number of available bytes.
int AIBusHandler::getAvailableBytes() {
	return getAvailableBytes(true);
}

//Get the number of available bytes.
int AIBusHandler::getAvailableBytes(const bool cache) {
	if(port_connected) {
		if(!cache || cached_msg.l <= 0)
			return aiserialBytesAvailable(this->ai_port);
		else
			return cached_msg.l + 4;
	} else
		return -1;
}

//Get a pointer to the port.
int* AIBusHandler::getPortPointer() {
	return &this->ai_port;
}

//Get whether the port is connected.
bool AIBusHandler::getConnected() {
	return port_connected;
}

//Cache any pending messages.
bool AIBusHandler::cachePending() {
	if(!port_connected)
		return false;

	if(aiserialBytesAvailable(ai_port) > 0) {
		AIData ai_msg;
		if(readAIData(&ai_msg)) {
			if(ai_msg.receiver == ID_NAV_COMPUTER || ai_msg.receiver == 0xFF) {
				if(ai_msg.sender != ID_NAV_COMPUTER && ai_msg.l >= 1 && ai_msg.data[0] != 0x80) {
					sendAcknowledgement(ai_msg.receiver, ai_msg.sender);

					if(cached_msg.l <= 0)
						cached_msg.refreshAIData(ai_msg);
					else {
						uint8_t data[ai_msg.l+4];
						ai_msg.getBytes(data);

						for(int i=0;i<sizeof(data);i+=1)
							cached_bytes.push_back(data[i]);
					}
				}
				return true;
			}
		}
	}
	
	return false;
}

//Cache a message.
void AIBusHandler::cacheMessage(AIData* ai_msg) {
	if(ai_msg->receiver == ID_NAV_COMPUTER || ai_msg->receiver == 0xFF) {
		if(ai_msg->sender != ID_NAV_COMPUTER && ai_msg->l >= 1 && ai_msg->data[0] != 0x80) {
			sendAcknowledgement(ai_msg->receiver, ai_msg->sender);

			if(cached_msg.l <= 0)
				cached_msg.refreshAIData(*ai_msg);
			else {
				uint8_t data[ai_msg->l+4];
				ai_msg->getBytes(data);

				for(int i=0;i<sizeof(data);i+=1)
					cached_bytes.push_back(data[i]);
			}
		}
	}
}

#ifndef RPI_UART
uint16_t stringToNumber(std::string str) {
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

void printBytes(AIData* ai_d) {
	const uint8_t l = ai_d->l + 4;
	uint8_t* data = new uint8_t[l];
	ai_d->getBytes(data);
	#ifndef RPI_UART
	for(uint8_t i=0;i<l;i+=1)
		std::cout<<std::hex<<int(data[i])<<" "<<std::dec;
	std::cout<<'\n';
	#endif
	delete[] data;
}
