#include "BM83_AidF.h"

BM83::BM83(Stream* serial, AIBusHandler* ai_handler, PhoneTextHandler* text_handler, ParameterList* parameters) {
	this->serial = serial;
	this->ai_handler = ai_handler;
	this->parameter_list = parameters;
	this->text_handler = text_handler;
	this->pin_code = random(0,10000);
}

BM83::BM83(Stream* serial, AIBusHandler* ai_handler, PhoneTextHandler* text_handler, ParameterList* parameters, const uint16_t pin_code) {
	this->serial = serial;
	this->ai_handler = ai_handler;
	this->parameter_list = parameters;
	this->text_handler = text_handler;
	if(pin_code < 10000)
		this->pin_code = pin_code;
	else
		this->pin_code = pin_code%10000;
}

BM83::~BM83() {
	if(active_device != NULL)
		delete active_device;
}

//Loop function.
void BM83::loop() {
	if(serial->available() >= 5) {
		const int avail = serial->available();
		uint8_t data[avail];

		serial->readBytes(data, avail);

		int start = -1;
		for(int i=0;i<avail;i+=1) {
			if(data[i] == BM83_START_BYTE)
				start = i;

			if(start >= 0)
				break;
		}

		if(start >= 0) {
			while(start < avail) {
				if(avail - start < 5)
					break;

				const uint16_t new_l = (data[start+1]<<8) + data[start+2];

				uint8_t msg_data[new_l + 5];
				for(int i=0;i<sizeof(msg_data);i+=1)
					msg_data[i] = data[start + i];

				BM83Data rx_msg = getBM83Message(msg_data, sizeof(msg_data));

				if(rx_msg.opcode != BM83_ACK) {
					uint8_t tx_data[] = {rx_msg.opcode};
					BM83Data tx_msg(sizeof(tx_data), BM83_ACK);
					
					tx_msg.refreshBMData(tx_data);
					sendBM83Message(&tx_msg);
				}
				
				handleBM83Message(&rx_msg);

				start += new_l + 5;
			}
		}
	}
}

//Start up the BM83.
void BM83::init() {
	this->sendPowerOn();
	this->sendDeviceName("AidF Audio System");
	this->sendDevicePIN();
}

//Handle a BT AIBus message.
bool BM83::handleAIBus(AIData* ai_msg) {
	if(ai_msg->receiver != ID_PHONE)
		return false;

	bool ack = true;

	if(ai_msg->sender == ID_NAV_COMPUTER) {
		if(ai_msg->l >= 3 && ai_msg->data[0] == 0x2B && ai_msg->data[1] == 0x6C) { //Menu item selected.
			if(call_status == CALL_STATUS_IDLE && active_device == NULL) { //Phone not connected.
				switch(ai_msg->data[2]) {
				case 1: //Activate pairing mode.
					this->activatePairing();
					break;
				}
			}
		}
	}

	if(ack)
		ai_handler->sendAcknowledgement(ID_PHONE, ai_msg->sender);

	return false;
}

//Send the power on command.
void BM83::sendPowerOn() {
	uint8_t power_press[] = {0x0, 0x51};
	uint8_t power_release[] = {0x0, 0x52};

	BM83Data power_press_msg(sizeof(power_press), 0x2);
	power_press_msg.refreshBMData(power_press);

	BM83Data power_release_msg(sizeof(power_release), 0x2);
	power_release_msg.refreshBMData(power_release);

	sendBM83Message(&power_press_msg);
	sendBM83Message(&power_release_msg);
}

//Send the device name.
void BM83::sendDeviceName(String name) {
	if(name.length() > 32)
		name = name.substring(0, 32);

	uint8_t name_bytes[name.length()];
	for(int i=0;i<name.length();i+=1)
		name_bytes[i] = uint8_t(name[i]);
	
	BM83Data name_msg(name.length(), 0x5);
	name_msg.refreshBMData(name_bytes);

	sendBM83Message(&name_msg);
}

//Send the stored PIN.
void BM83::sendDevicePIN() {
	this->sendDevicePIN(pin_code);
}

//Send the desired PIN code.
void BM83::sendDevicePIN(uint16_t pin) {
	if(pin >= 10000)
		pin %= 10000;

	uint8_t pin_data[4];
	pin_data[0] = pin/1000 + '0';
	pin_data[1] = (pin/100)%10 + '0';
	pin_data[2] = (pin/10)%10 + '0';
	pin_data[3] = pin%10 + '0';

	BM83Data pin_msg(sizeof(pin_data), 0x6);
	sendBM83Message(&pin_msg);
}

//Activate the pairing mode.
void BM83::activatePairing() {
	uint8_t pair_data[] = {0x0, 0x5D};
	BM83Data pair_msg(sizeof(pair_data), 0x2);
	pair_msg.refreshBMData(pair_data);

	sendBM83Message(&pair_msg);
}

//Send a message to the BM83.
void BM83::sendBM83Message(BM83Data* msg) {
	const int full_l = msg->l + 1;

	uint8_t msg_bytes[msg->l + 5];
	msg_bytes[0] = BM83_START_BYTE;
	msg_bytes[1] = (full_l&0xFF00)>>8;
	msg_bytes[2] = full_l&0xFF;
	msg_bytes[3] = msg->opcode;

	for(int i=0;i<msg->l;i+=1)
		msg_bytes[4+i] = msg->data[i];

	int checksum = 0;
	for(int i=1;i<sizeof(msg_bytes) - 1;i+=1)
		checksum += msg_bytes[i];

	msg_bytes[sizeof(msg_bytes)-1] = (~(checksum&0xFF) + 1)&0xFF;

	serial->write(msg_bytes, msg->l + 5);
	serial->flush();

	//TODO: Acknowledgement.
}

//Decode a BM83 message.
BM83Data BM83::getBM83Message(uint8_t* data, const int l) {
	if(l < 5 || data[0] != BM83_START_BYTE)
		return BM83Data(0,0);

	const uint16_t full_l = (data[1]<<8) + data[2];

	if(full_l < 1)
		return BM83Data(0,0);
	
	BM83Data rx_msg(full_l - 1, data[3]);
	for(int i=0;i<rx_msg.l;i+=1)
		rx_msg.data[i] = data[i+4];

	int checksum = 0;
	for(int i=0;i<l-1;i+=1)
		checksum += data[i];

	uint8_t checksum_b = (~(checksum&0xFF) + 1)&0xFF;

	{
		uint8_t bm83_data[rx_msg.l + 1];
		bm83_data[0] = rx_msg.opcode;

		for(int i=0;i<rx_msg.l;i+=1)
			bm83_data[i+1] = rx_msg.data[i];

		AIData bm83_msg(sizeof(bm83_data), ID_PHONE, 0xFF);
		bm83_msg.refreshAIData(bm83_data);
		ai_handler->writeAIData(&bm83_msg, false);
	}

	if(data[l-1] != checksum_b)
		return BM83Data(0,0);

	return rx_msg;
}

//Read a BM83 message.
void BM83::handleBM83Message(BM83Data* msg) {
	if(msg->opcode == 0x1 && msg->l >= 3) { //Module status.
		const uint8_t state = msg->data[0];

		switch(state) {
		case 0: //Power off.
			bm83_status = BM83_STATUS_OFF;
			sendPowerOn();
			//TODO: Do we want to power the BM83 off before the 555 timer turns power off?
			break;
		case 1: //Pairing mode.
			text_handler->createDisconnectedButtons(true);
			bm83_status = BM83_STATUS_PAIRING;
			break;
		case 2:
			bm83_status = BM83_STATUS_DISCONNECTED; //Power on.
			break;
		case 3: //Pairing successful.
			text_handler->createConnectedButtons();
			bm83_status = BM83_STATUS_CONNECTED;
			break;
		case 4: //Pairing failed.
			text_handler->createDisconnectedButtons(false);
			bm83_status = BM83_STATUS_DISCONNECTED;
			break;
		case 5: //HF link established.
			bm83_status = BM83_STATUS_CONNECTED;
			text_handler->createConnectedButtons();
			break;
		}
	} else if(msg->opcode == 0x2 && msg->l >= 1) { //Call status.
		const uint8_t bm_call_status = msg->data[0];

		uint8_t new_call_status = call_status;
		switch (bm_call_status) {
		case 0: //Idle.
			new_call_status = CALL_STATUS_IDLE;
			break;
		case 1: //Voice dial.
			new_call_status = CALL_STATUS_VOICE;
			//TODO: Check CarPlay?
			break;
		case 2: //Incoming call. No further information is provided.
			new_call_status = CALL_STATUS_INCOMING;
			break;
		case 3: //Outgoing call.
			new_call_status = CALL_STATUS_OUTGOING;
			break;
		case 4: //Active call.
			new_call_status = CALL_STATUS_ACTIVE;
			break;
		case 5: //Call waiting.
		case 6: //Call hold.
			new_call_status = CALL_STATUS_ACTIVE;
			break;
		}

		if(new_call_status != call_status) {
			if(new_call_status == CALL_STATUS_IDLE) {
				uint8_t resume_audio_data[] = {0x21, 0x6, 0x0};
				AIData resume_audio_msg(sizeof(resume_audio_data), ID_PHONE, ID_RADIO);
				resume_audio_msg.refreshAIData(resume_audio_data); 

				ai_handler->writeAIData(&resume_audio_msg, parameter_list->radio_connected);

				this->caller_id = "";
				this->caller_number = "";

				text_handler->writeNaviText("", 0, 2);
				text_handler->writeNaviText("", 0, 3);

				if(active_device != NULL)
					text_handler->createConnectedButtons();
				else
					text_handler->createDisconnectedButtons(false);
			} else if(call_status == CALL_STATUS_IDLE) {
				uint8_t stop_audio_data[] = {0x21, 0x6, 0x1};
				AIData stop_audio_msg(sizeof(stop_audio_data), ID_PHONE, ID_RADIO);
				stop_audio_msg.refreshAIData(stop_audio_data);

				ai_handler->writeAIData(&stop_audio_msg, parameter_list->radio_connected);

				text_handler->createCallButtons();
			}

			String status = "";
			switch(new_call_status) {
			case CALL_STATUS_IDLE:
			case CALL_STATUS_VOICE:
				if(active_device != NULL)
					status = active_device->device_name + " Connected";
				else
					status = "Device Not Connected";
				break;
			case CALL_STATUS_ACTIVE:
				status = "In Call";
				break;
			case CALL_STATUS_INCOMING:
				status = "Incoming Call";
				break;
			case CALL_STATUS_OUTGOING: 
				status = "Dialing";
				break;
			}

			text_handler->writeNaviText(status, 0, 1);
		}

		call_status = new_call_status;
	} else if(msg->opcode == 0x3) { //Caller ID.
		handleCallerID(msg);
	}
}

//Handle a caller ID messasge.
void BM83::handleCallerID(BM83Data* msg) {
	String id_text = "";

	for(int i=1;i<msg->l;i+=1)
		id_text += char(msg->data[i]);

	bool is_text = false;

	for(int i=0;i<id_text.length();i+=1) {
		if(!isDigit(id_text[i])) {
			const char c = id_text[i];
			if(c != '-' && c != '+' && c != '*' && c != '#') {
				is_text = true;
				break;
			}
		}
	}

	uint8_t id_byte = 0x1;
	if(call_status == CALL_STATUS_OUTGOING)
		id_byte = 0x11;

	if(is_text) {
		this->caller_id = id_text;

		uint8_t call_data[] = {0x21, id_byte, 0x4};
		AIData call_msg(sizeof(call_data), ID_PHONE, ID_RADIO);
		call_msg.refreshAIData(call_data);

		ai_handler->writeAIData(&call_msg, parameter_list->radio_connected);

		text_handler->writeNaviText(caller_id, 0, 3);
	} else {
		this->caller_number = id_text;

		uint8_t call_data[] = {0x21, id_byte, 0x3};
		AIData call_msg(sizeof(call_data), ID_PHONE, ID_RADIO);
		call_msg.refreshAIData(call_data);

		ai_handler->writeAIData(&call_msg, parameter_list->radio_connected);

		text_handler->writeNaviText(caller_number, 0, 2);
	}
}

//Connect the specified device.
void BM83::sendConnect(BluetoothDevice* device) {
	uint8_t connect_bytes[] = {0x5,
								device->device_number&0xF - 1,
								0x2, //TODO: Check this one.
								device->mac_id[5],
								device->mac_id[4],
								device->mac_id[3],
								device->mac_id[2],
								device->mac_id[1],
								device->mac_id[0]};

	BM83Data connect_msg(sizeof(connect_bytes), 0x17);

	sendBM83Message(&connect_msg);
}

//Activate a Bluetooth device.
void BM83::activateBTDevice(const uint8_t id) {
	this->active_device = new BluetoothDevice();
	this->active_device->device_id = id;
}

BM83Data::BM83Data(const uint16_t l, const uint8_t opcode) {
	this->data = new uint8_t[l];
	this->l = l;
	this->opcode = l;
}

BM83Data::~BM83Data() {
	delete[] this->data;
}

BM83Data::BM83Data(const BM83Data &copy) {
	this->data = new uint8_t[copy.l];
	this->l = copy.l;
	this->opcode = copy.opcode;
}

void BM83Data::refreshBMData(uint8_t* data) {
	for(int i=0;i<this->l;i+=1)
		this->data[i] = data[i];
}