#include "Si4735_AidF.h"

Si4735Controller::Si4735Controller(const uint8_t reset_pin, const int8_t address_state, ParameterList* parameters) {
	this->parameters = parameters;

	this->reset_pin = reset_pin;
	this->address_state = address_state;

	this->tuner = new SI4735();
}

Si4735Controller::~Si4735Controller() {
	delete this->tuner;
}

//Start the controller, phase 1.
void Si4735Controller::init1() {
	this->tuner->setDeviceI2CAddress(address_state);
	this->tuner->setup(reset_pin, POWER_UP_FM);
}

//Start the controller, phase 2.
void Si4735Controller::init2() {
	this->tuner->setFM(parameters->fm_lower_limit, parameters->fm_upper_limit, parameters->fm_start, parameters->fm_inc);

	delay(500);
	this->tuner->setRdsConfig(3, 3, 3, 3, 3);
	this->tuner->setFifoCount(1);
	this->tuner->setVolume(63);
	this->tuner->setFmBlendStereoThreshold(FM_STEREO_THRESH);
}

//Loop function.
void Si4735Controller::loop() {
	if(parameters->tune_changed)
		last_frequency_change = 0;
}

//Set the desired frequency, within the tuning range.
uint16_t Si4735Controller::setFrequency(const uint16_t des_freq) {
	tuner->setFrequency(des_freq);
	delay(30);
	return tuner->getFrequency();
}

//Get the current frequency. The number returned is the tuned frequency * 1000.
uint16_t Si4735Controller::getFrequency() {
	delay(30);
	return tuner->getFrequency();
}

//Turn the radio output on or off.
void Si4735Controller::setPower(const bool power) {
	this->setPower(power, parameters->last_sub);
}

//Turn the radio output on or off and set its function..
void Si4735Controller::setPower(const bool power, const uint8_t function) {
	if(power) {
		if(function == SUB_FM1)
			this->tuner->setFM(parameters->fm_lower_limit, parameters->fm_upper_limit, parameters->fm1_tune, parameters->fm_inc);
		else if(function == SUB_FM2)
			this->tuner->setFM(parameters->fm_lower_limit, parameters->fm_upper_limit, parameters->fm2_tune, parameters->fm_inc);
		else if(function == SUB_AM)
			this->tuner->setAM(parameters->am_lower_limit, parameters->am_upper_limit, parameters->am_tune, parameters->am_inc);

		if(function != SUB_AM) {
			this->tuner->setRdsConfig(3, 3, 3, 3, 3);
			this->tuner->setFifoCount(1);
		}
	} else
		tuner->powerDown();
}

//Increment the tuned frequency.
uint16_t Si4735Controller::incrementFrequency(const uint8_t count) {
	for(uint8_t i=0;i<count;i+=1)
		tuner->setFrequencyUp();

	return tuner->getFrequency();
}


//Decrement the tuned frequency.
uint16_t Si4735Controller::decrementFrequency(const uint8_t count) {
	for(uint8_t i=0;i<count;i+=1)
		tuner->setFrequencyDown();

	return tuner->getFrequency();
}

//Get the RSSI.
uint8_t Si4735Controller::getRSSI() {
	tuner->getStatus();
	return tuner->getReceivedSignalStrengthIndicator();
}

//Start seeking.
void Si4735Controller::startSeek(const bool seek_up) {
	if(seek_up)
		tuner->seekStation(1, 1);
	else
		tuner->seekStation(0, 1);

	seeking = true;
}

//Reset the frequency change timer.
void Si4735Controller::resetFrequencyChange() {
	this->last_frequency_change = 0;
}

//Return whether the tuner is seeking.
bool Si4735Controller::getSeeking(uint16_t* frequency) {
	if(!seeking)
		return false;
	
	tuner->getStatus(1, 0);
	delay(30);
	*frequency = tuner->getFrequency();

	if(parameters->last_sub == SUB_FM1) {
		if(parameters->fm1_tune != *frequency)
			last_frequency_change = 0;
	} else if(parameters->last_sub == SUB_FM2) {
		if(parameters->fm2_tune != *frequency)
			last_frequency_change = 0;
	}

	if(last_frequency_change > 250)
		seeking = false;

	return seeking;
}

//Fills the parameter list with stereo and RDS data.
void Si4735Controller::getParameters(ParameterList* parameters, const uint8_t setting) {
	if(setting >= 2) { //AM.
		return;
	}

	tuner->getStatus();
	tuner->getCurrentReceivedSignalQuality();

	tuner->rdsBeginQuery();

	const uint8_t rssi = tuner->getCurrentRSSI();
	parameters->fm_stereo = rssi > FM_STEREO_THRESH;

	parameters->has_rds = tuner->getRdsReceived();

	if(parameters->has_rds) {
		getCallsign(&parameters->rds_station_name);

		getRdsInfo(&parameters->rds_program_name, false);

		uint16_t year, month, day, hour = parameters->hour, minute = parameters->min;
		const int16_t last_hour = parameters->hour, last_min = parameters->min;

		if(tuner->getRdsDateTime(&year, &month, &day, &hour, &minute) && parameters->fm_stereo) {
			parameters->hour = hour;
			parameters->min = minute;
			parameters->minute_timer = 0;
		}
	}
}

//Fill string rds with RDS program info. Return true if successful.
bool Si4735Controller::getRdsInfo(String* rds) {
	return getRdsInfo(rds, true);
}

//Fill string rds with callsign. 
bool Si4735Controller::getCallsign(String* rds) {
	const uint8_t status_request_bytes[] = {0x4};
	tuner->sendCommand(FM_RDS_STATUS, sizeof(status_request_bytes), status_request_bytes);

	uint8_t status_response_bytes[12];
	tuner->getCommandResponse(sizeof(status_response_bytes), status_response_bytes);

	if((status_response_bytes[0]&0x11) == 0)
		return false;
	
	uint16_t rds_a = (status_response_bytes[4]<<8) | status_response_bytes[5];
	
	//TODO: Station names outside the US.
	//Thanks to: https://www.fmsystems-inc.com/rds-pi-code-formula-station-callsigns/
	if(rds_a < 4096)
		return false;
	
	if(rds_a >= 21672) {
		rds_a -= 21672;
		*rds = "W";
	} else {
		rds_a -= 4096;
		*rds = "K";
	}

	const char callsign_letters[] = {rds_a/(26*26) + 'A', (rds_a/26)%26 + 'A', rds_a%26 + 'A', '\0'};
	*rds += callsign_letters;

	return true;
}

//Fill string rds with RDS program info. Return true if successful.
bool Si4735Controller::getRdsInfo(String* rds, const bool init) {
	if(init){
		tuner->rdsBeginQuery();

		if(!tuner->getRdsReceived())
			return false;
	}

	const char* c_text = tuner->getRdsProgramInformation();
			
	if(c_text != NULL) {
		const String text = String(c_text);

		int last_space = text.length();
		for(int i=text.length() - 1; i >= 0; i -= 1) {
			bool space_found = false;

			if(text.charAt(i) > 0x20) {
				last_space = i+1;
				space_found = true;
			}

			if(space_found)
				break;
		}

		*rds = text.substring(0, last_space);
		return true;
	}
	return false;
}

//Clear the internal RDS buffers.
void Si4735Controller::clearRds() {
	tuner->clearRdsBuffer();
}

ParameterList* Si4735Controller::getParameterList() {
	return this->parameters;
}

/*String getRDSString(uint16_t* registers) {
	String message = "";
	for(uint8_t i=0;i<4;i+=1) {
		char letter_a = (registers[i]&0xFF00)>>8, letter_b = (registers[i]&0xFF);

		if(letter_a < 0)
			letter_a = ' ';
		if(letter_b < 0)
			letter_b = ' ';

		message += letter_a;
		message += letter_b;
	}

	for(int i=message.length()-1;i>=0;i-=1) {
		if(message.charAt(i) != ' ' && message.charAt(i) != '\0') {
			message = message.substring(0, i+1);
			break;
		}
	}

	return message;
}*/
