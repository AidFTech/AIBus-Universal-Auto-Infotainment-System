#include <stdint.h>
#include <SI4735.h>

#include "AIBus_Handler.h"
#include "Parameter_List.h"
#include "Text_Handler.h"

#ifndef si4735_aidf_h
#define si4735_aidf_h

#define FM_STEREO_THRESH 50

class Si4735Controller {
public:
	Si4735Controller(const uint8_t reset_pin, const int8_t address_state, AIBusHandler* ai_handler, ParameterList* parameters, TextHandler* text_handler);
	~Si4735Controller();

	void init1();
	void init2();
	void loop();

	uint16_t setFrequency(const uint16_t des_freq);
	uint16_t getFrequency();

	void setPower(const bool power);
	void setPower(const bool power, const uint8_t function);

	uint16_t incrementFrequency(const uint8_t count = 1);
	uint16_t decrementFrequency(const uint8_t count = 1);

	void resetFrequencyChange();

	void getParameters(ParameterList* parameters, const uint8_t setting);

	uint8_t getRSSI();

	void startSeek(const bool seek_up);
	bool getSeeking(uint16_t* frequency);

	AIBusHandler* getAIHandler();
	ParameterList* getParameterList();

	bool no_seek = false; //Set to true for background tuner.
private:
	AIBusHandler* ai_handler;
	ParameterList* parameters;
	TextHandler* text_handler;

	SI4735* tuner;

	uint8_t reset_pin;
	int8_t address_state = LOW;

	elapsedMillis last_frequency_change = 0;
};

//String getRDSString(uint16_t* registers);

#endif
