#include <stdint.h>

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

#include <sdbus-c++/sdbus-c++.h>

#ifndef bta_device_h
#define bta_device_h

#define MAC_LEN 6

using namespace std;
using namespace sdbus;

class BTADevice {
public:
	BTADevice(ObjectPath path, uint8_t address[]);
	BTADevice(ObjectPath path, uint8_t address[], string device_name);

	uint8_t* getMacAddress();
	string getMacString();

	string getDeviceName();

	ObjectPath getPath();

private:
	string device_name;
	uint8_t address[MAC_LEN];

	ObjectPath path;
};

void getMacFromString(string mac_str, uint8_t* mac_addr);

#endif