#include <stdint.h>
#include <Arduino.h>

#ifndef bluetooth_device_h
#define bluetooth_device_h

class BluetoothDevice {
public:
    uint8_t mac_id[6];
    String device_name;
    uint8_t device_id;
    uint8_t device_number;
};

#endif