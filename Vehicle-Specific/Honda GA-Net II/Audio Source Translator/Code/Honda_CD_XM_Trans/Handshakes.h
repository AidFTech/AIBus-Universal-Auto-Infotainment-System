#include "IEBus_Handler.h"

#include <stdint.h>
#include <Arduino.h>

#ifndef handshakes_h
#define handshakes_h

void sendWideHandshake(IEBusHandler* driver); //Send the introductory handshake to all devices.
bool checkForMessages(IEBusHandler* driver, IE_Message* the_message);

void sendFunctionMessage(IEBusHandler* driver, const bool change, const uint16_t recipient, uint8_t* data, uint16_t data_l);

#endif
