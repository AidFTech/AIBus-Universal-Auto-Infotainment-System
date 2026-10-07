#include "AIBus_Interface.h"

//Write an ignition status AIBus message.
void writeAIBusIgnitionStatus(AIBusHandler* aibus_handler, const aibus_ignition_status ignition_status) {
	uint8_t msg[] = {
		0xA1,			//Indicates a broadcast body-control message for general vehicle events (e.g. doors opening, lights turning on/off, etc.)
		0x2,			//Within the above, indicates a key position message.
		ignition_status	//The ignition status as defined in the enumeration.
	};

	AIData ai_msg(sizeof(msg), ID_CANSLATOR, 0xFF, msg);
	aibus_handler->writeAIData(&ai_msg, false); //Broadcast messages to 0xFF do not require acknowledgement.
}

//Write an illumination status AIBus message.
void writeAIBusIlluminationStatus(AIBusHandler* aibus_handler, const uint8_t brightness, const bool ill_on, const bool night_mode) {
	uint8_t msg[] = {
		0xA1,			//Indicates a broadcast body-control message for general vehicle events (e.g. doors opening, lights turning on/off, etc.)
		0x10,			//Within the above, indicates a brightness/interior illumination message.
		brightness,		//The desired brightness, out of 255.
		uint8_t(		//The encoded illumination/night mode message.
			0 | (ill_on ? AIBUS_ILL_STATUS_ILL_ON : 0)
		| (night_mode ? AIBUS_ILL_STATUS_NIGHT : 0)) 
	};

	AIData ai_msg(sizeof(msg), ID_CANSLATOR, 0xFF, msg);
	aibus_handler->writeAIData(&ai_msg, false); //Broadcast messages to 0xFF do not require acknowledgement.
}

//Write an exterior light status AIBus message.
void writeAIBusExtLightStatus(AIBusHandler* aibus_handler, const aibus_ext_light_status status) {
	uint8_t msg[] = {
		0xA1,			//Indicates a broadcast body-control message for general vehicle events (e.g. doors opening, lights turning on/off, etc.)
		0x11,			//Within the above, indicates an exterior light message.
		status.raw[0],
		status.raw[1],
	};

	AIData ai_msg(sizeof(msg), ID_CANSLATOR, 0xFF, msg);
	aibus_handler->writeAIData(&ai_msg, false); //Broadcast messages to 0xFF do not require acknowledgement.
}

//Write a door status AIBus message.
void writeAIBusDoorStatus(AIBusHandler *aibus_handler, const uint8_t door_status) {
	uint8_t msg[] = {
		0xA1,			//Indicates a broadcast body-control message for general vehicle events (e.g. doors opening, lights turning on/off, etc.)
		0x43,			//Within the above, indicates a door status message.
		door_status		//The door open/closed status as defined in the header file.
	};

	AIData ai_msg(sizeof(msg), ID_CANSLATOR, 0xFF, msg);
	aibus_handler->writeAIData(&ai_msg, false); //Broadcast messages to 0xFF do not require acknowledgement.
}

//Write a time AIBus message. Normally this is done by the radio.
void writeAIBusTime(AIBusHandler* aibus_handler, const uint8_t hour, const uint8_t minute, const uint8_t second, const bool use_12h, const bool auto_clock) {
	const uint8_t option_hour = hour | (auto_clock ? AIBUS_TIME_AUTO : 0) | (use_12h ? AIBUS_TIME_12H : 0);
	
	uint8_t msg[] = {
		0xA1,			//Indicates a broadcast body-control message for general vehicle events (e.g. doors opening, lights turning on/off, etc.)
		0x1F,			//Within the above, indicates a common parameter.
		0x1,			//Within the above, indicates current time.
		option_hour,	//The hour with display options defined.
		minute,			//The minute.
		second			//The second.
	};

	AIData ai_msg(sizeof(msg), ID_CANSLATOR, 0xFF, msg);
	aibus_handler->writeAIData(&ai_msg, false); //Broadcast messages to 0xFF do not require acknowledgement.
}

//Write an outside temperature AIBus message.
void writeAIBusOutsideTemp(AIBusHandler* aibus_handler, double temp, const bool fahrenheit, const int decimal_count) {
	aibus_temperature_config temperature_config;
	temperature_config.data.fahrenheit = fahrenheit;	

	temperature_config.data.negative = temp < 0;
	if(temp < 0)
		temp *= -1;
	
	int dec = 0;
	while(temp - int(temp) > 0 || dec < decimal_count) {
		temp *= 10;
		dec += 1;
	}

	temperature_config.data.decimal_count = dec;

	const uint64_t norm_temp = uint64_t(temp);
	uint64_t byte_ref = 0x100;

	int bytes = 1;

	while(byte_ref <= norm_temp && bytes < 7) {
		byte_ref <<= 8;
		bytes += 1;
	}

	temperature_config.data.byte_count = bytes;

	AIData ai_msg(4 + bytes, ID_CANSLATOR, 0xFF);
	ai_msg[0] = 0xA1;		//Indicates a broadcast body-control message for general vehicle events (e.g. doors opening, lights turning on/off, etc.)
	ai_msg[1] = 0x1F;		//Within the above, indicates a common parameter.
	ai_msg[2] = 0x3; 		//Within the above, indicates outside temperature.
	ai_msg[3] = temperature_config.raw[0];
							//The temperature configuration.
	for(int i=0;i<bytes;i+=1)
		ai_msg[i+4] = (norm_temp >> ((bytes-i-1)*8))&0xFF; //The temperature.

	aibus_handler->writeAIData(&ai_msg, false); //Broadcast messages to 0xFF do not require acknowledgement.
}

//Write a coolant temperature AIBus message.
void writeAIBusCoolantTemp(AIBusHandler* aibus_handler, double temp, const bool fahrenheit, const int decimal_count) {
	aibus_temperature_config temperature_config;
	temperature_config.data.fahrenheit = fahrenheit;	

	temperature_config.data.negative = temp < 0;
	if(temp < 0)
		temp *= -1;
	
	int dec = 0;
	while(temp - int(temp) > 0 || dec < decimal_count) {
		temp *= 10;
		dec += 1;
	}

	temperature_config.data.decimal_count = dec;

	const uint64_t norm_temp = uint64_t(temp);
	uint64_t byte_ref = 0x100;

	int bytes = 1;

	while(byte_ref <= norm_temp && bytes < 7) {
		byte_ref <<= 8;
		bytes += 1;
	}

	temperature_config.data.byte_count = bytes;

	AIData ai_msg(4 + bytes, ID_CANSLATOR, 0xFF);
	ai_msg[0] = 0xA1;		//Indicates a broadcast body-control message for general vehicle events (e.g. doors opening, lights turning on/off, etc.)
	ai_msg[1] = 0x1F;		//Within the above, indicates a common parameter.
	ai_msg[2] = 0x5; 		//Within the above, indicates coolant temperature.
	ai_msg[3] = temperature_config.raw[0];
							//The temperature configuration.
	for(int i=0;i<bytes;i+=1)
		ai_msg[i+4] = (norm_temp >> ((bytes-i-1)*8))&0xFF; //The temperature.

	aibus_handler->writeAIData(&ai_msg, false); //Broadcast messages to 0xFF do not require acknowledgement.
}