#include <stdint.h>
#include <SI4735.h>
#include <elapsedMillis.h>

#include "Parameter_List.h"
#include "Text_Handler.h"
#include "Radio_Time.h"

#ifndef si4735_aidf_h
#define si4735_aidf_h

#define FM_STEREO_THRESH 30
#define QUEUE_TIMER 500

class Si4735Controller {
public:
	Si4735Controller(const uint8_t reset_pin, const int8_t address_state, ParameterList* parameters);
	~Si4735Controller();

	void init1();
	void init2();
	void loop();

	bool getQueued();

	void queueFrequency(const uint16_t des_freq);
	uint16_t setFrequency(const uint16_t des_freq);
	uint16_t getFrequency();

	void setPower(const bool power);
	void setPower(const bool power, const uint8_t function);

	void setLimits();

	uint16_t incrementFrequency(const uint8_t count = 1);
	uint16_t decrementFrequency(const uint8_t count = 1);

	void resetFrequencyChange();

	void getParameters(ParameterList* parameters, const uint8_t setting);
	bool getRdsInfo(String* rds);
	bool getCallsign(String* rds);
	bool getDateTime();
	bool getDateTimePresent();
	void clearRds();

	uint8_t getRSSI();

	void startSeek(const bool seek_up);
	void startScan();
	bool getSeeking(uint16_t* frequency);

	ParameterList* getParameterList();
private:
	ParameterList* parameters;
	SI4735 tuner;

	uint8_t reset_pin;
	int8_t address_state = LOW;

	elapsedMillis last_frequency_change = 0;
	bool seeking = false;

	uint16_t queued_frequency = 0;
	bool queued_frequency_set = false;
	elapsedMillis queue_timer;

	bool getRdsInfo(String* rds, const bool init);
	bool getDateTime(ParameterList* parameters);
};

//String getRDSString(uint16_t* registers);

#endif
