#include "BTA_Device.h"

BTADevice::BTADevice(ObjectPath path, uint8_t address[]) {
	for(int i=0;i<MAC_LEN;i+=1)
		this->address[i] = address[i];

	device_name = getMacString();
	this->path = path;
}

BTADevice::BTADevice(ObjectPath path, uint8_t address[], string device_name) {
	for(int i=0;i<MAC_LEN;i+=1)
		this->address[i] = address[i];

	this->device_name = device_name;
	this->path = path;
}

//Get the MAC address.
uint8_t* BTADevice::getMacAddress() {
	return this->address;
}

//Get the MAC address in string form.
string BTADevice::getMacString() {
	string mac_str = "";

	for(int i=0;i<MAC_LEN;i+=1) {
		stringstream hex_stream;
		hex_stream<<hex<<int(address[i]);

		mac_str += hex_stream.str();
		if(i < MAC_LEN - 1)
			mac_str += ":";
	}
	transform(mac_str.begin(), mac_str.end(), mac_str.begin(), ::toupper);
	return mac_str;
}

//Get the device name.
string BTADevice::getDeviceName() {
	return this->device_name;
}

//Get the object path.
ObjectPath BTADevice::getPath() {
	return this->path;
}

//Get a MAC address from a string.
void getMacFromString(string mac_str, uint8_t* mac_addr) {
	int m = 0;
	uint8_t current_digit = 0;
	for(int i=0;i<mac_str.length();i+=1) {
		if(isdigit(mac_str[i])) {
			current_digit <<= 4;
			char mac_char[] = {mac_str[i]};
			current_digit |= stoi(mac_char);
		} else {
			switch(mac_str[i]) { //Check for hex.
			case 'a':
			case 'A':
				current_digit <<= 4;
				current_digit |= 0xA;
				break;
			case 'b':
			case 'B':
				current_digit <<= 4;
				current_digit |= 0xB;
				break;
			case 'c':
			case 'C':
				current_digit <<= 4;
				current_digit |= 0xC;
				break;
			case 'd':
			case 'D':
				current_digit <<= 4;
				current_digit |= 0xD;
				break;
			case 'e':
			case 'E':
				current_digit <<= 4;
				current_digit |= 0xE;
				break;
			case 'f':
			case 'F':
				current_digit <<= 4;
				current_digit |= 0xF;
				break;
			default: //End of digit.
				mac_addr[m] = current_digit;
				current_digit = 0;
				m += 1;
				break;
			}
		}

		if(i>=mac_str.length() - 1) {
			mac_addr[m] = current_digit;
			current_digit = 0;
			m += 1;
		}

		if(m >= MAC_LEN)
			break;
	}
}