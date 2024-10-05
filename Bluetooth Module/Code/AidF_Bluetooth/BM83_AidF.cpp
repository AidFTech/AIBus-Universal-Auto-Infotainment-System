#include "BM83_AidF.h"

BM83::BM83(Stream* serial, AIBusHandler* ai_handler) {
	this->serial = serial;
	this->ai_handler = ai_handler;
	this->pin_code = random(0,10000);
}

BM83::BM83(Stream* serial, AIBusHandler* ai_handler, const uint16_t pin_code) {
	this->serial = serial;
	this->ai_handler = ai_handler;
	if(pin_code < 10000)
		this->pin_code = pin_code;
	else
		this->pin_code = pin_code%10000;
}

//Loop function.
void BM83::loop() {
	if(serial->available() >= 5) {
		
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

//Send a message to the BM83.
void BM83::sendBM83Message(BM83Data* msg) {
	const int full_l = msg->l + 1;

	uint8_t msg_bytes[msg->l + 5];
	msg_bytes[0] = 0xAA;
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