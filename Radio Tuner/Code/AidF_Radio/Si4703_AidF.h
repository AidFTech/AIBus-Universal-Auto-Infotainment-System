//Acknowledgement to the works of Nathan Seidle and Simon Monk.

#include <stdint.h>
#include <Wire.h>
#include <Arduino.h>
#include <elapsedMillis.h>

#include "Parameter_List.h"
#include "AIBus_Handler.h"
#include "Text_Handler.h"

#ifndef si4703_aidf_h
#define si4703_aidf_h

#define REG_DEVID 0x00
#define REG_CHIPID 0x01
#define REG_PWRCFG 0x02
#define REG_CHANNEL 0x03
#define REG_SYSCFG1 0x04
#define REG_SYSCFG2 0x05
#define REG_SYSCFG3 0x06
#define REG_TEST1 0x07
#define REG_STATUS 0x0A
#define REG_READCH 0x0B
#define REG_RDSA 0x0C
#define REG_RDSB 0x0D
#define REG_RDSC 0x0E
#define REG_RDSD 0x0F

#define INC_50k 0
#define INC_100k 1
#define INC_200k 2

#define COMM_WAIT 50
#define TUNE_WAIT 2000

#define I2C_CLOCK 400000
#define RDS_WAIT 15000

//#define NA //North America
#define EU //Europe
//#define AU //Australia
//#define JP //Japan

class Si4703Controller {
	public:
		Si4703Controller(const uint16_t reset_pin, const uint16_t enable_pin, const int16_t SDA_pin, const int16_t SCL_pin, AIBusHandler* ai_handler, ParameterList* parameters, TextHandler* text_handler);
		void init();
		void loop();

		uint16_t setFrequency(const uint16_t des_freq);
		uint16_t getFrequency();
		bool getRDS(uint16_t* messages);
		bool getRDSErrors(const bool read);
		
		void getParameters(ParameterList* parameters, const uint8_t setting);
		uint8_t getRSSI();
		bool getSeeking(uint16_t* frequency);

		void setPower(const bool power);

		uint16_t incrementFrequency(const uint8_t count = 1);
		uint16_t incrementFrequency(const uint16_t current_frequency, const uint8_t count);
		uint16_t decrementFrequency(const uint8_t count = 1);
		uint16_t decrementFrequency(const uint16_t current_frequency, const uint8_t count);
		
		void startSeek(const bool seek_up);

		AIBusHandler* getAIHandler();
		ParameterList* getParameterList();
		
	private:
		uint16_t reset_pin, enable_pin;
		int16_t SDA_pin, SCL_pin;
	
		uint16_t registers[16];
		
		bool seeking = false;

		AIBusHandler* ai_handler;
		ParameterList* parameters;
		TextHandler* text_handler;
	
		void readRegisters();
		bool writeRegisters();

		bool getRDSTextErrors();

		uint16_t calculateFrequency();
};

String decodeRDS(uint16_t* rds, Si4703Controller *tuner);
int8_t decodeRDS(uint16_t* rds, Si4703Controller *tuner, String* char_text);

String getRDSString(uint16_t* registers);

#endif
