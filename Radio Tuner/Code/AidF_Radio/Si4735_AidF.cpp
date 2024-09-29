#include "Si4735_AidF.h"

Si4735Controller::Si4735Controller(const uint8_t reset_pin, const int8_t address_state, AIBusHandler* ai_handler, ParameterList* parameters, TextHandler* text_handler) {
	this->ai_handler = ai_handler;
	this->parameters = parameters;
	this->text_handler = text_handler;

	this->reset_pin = reset_pin;
	this->address_state = address_state;

	this->tuner = new SI4735();
}

Si4735Controller::~Si4735Controller() {
	delete this->tuner;
}

//Start the controller.
void Si4735Controller::init() {
	this->tuner->setup(reset_pin, POWER_UP_FM);
	this->tuner->setDeviceI2CAddress(address_state);
}

//Loop function.
void Si4735Controller::loop() {

}

//Set the desired frequency, within the tuning range.
uint16_t Si4735Controller::setFrequency(const uint16_t des_freq) {
	tuner->setFrequency(des_freq);
	return tuner->getFrequency();
}

//Get the current frequency. The number returned is the tuned frequency * 1000.
uint16_t Si4735Controller::getFrequency() {
	return tuner->getFrequency();
}

//Turn the radio output on or off.
void Si4735Controller::setPower(const bool power) {
	this->setPower(power, parameters->last_sub);
}

//Turn the radio output on or off and set its function..
void Si4735Controller::setPower(const bool power, const uint8_t function) {
	if(power) {
		uint8_t si_function = POWER_UP_FM;
		if(function == SUB_AM)
			si_function = POWER_UP_AM;

		tuner->setup(reset_pin, si_function);
		tuner->radioPowerUp();
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
	return tuner->getReceivedSignalStrengthIndicator();
}

//Start seeking.
void Si4735Controller::startSeek(const bool seek_up) {
	if(seek_up)
		tuner->seekStation(1, 1);
	else
		tuner->seekStation(0, 1);
}

//Return whether the tuner is seeking.
bool Si4735Controller::getSeeking(uint16_t* frequency) {
	tuner->getStatus(1, 0);
	return !tuner->getTuneCompleteTriggered();
}

//Fills the parameter list with stereo and RDS data.
void Si4735Controller::getParameters(ParameterList* parameters, const uint8_t setting) {
	if(setting >= 2) { //AM.
		return;
	}
	
	parameters->fm_stereo = tuner->getCurrentPilot();

	tuner->rdsBeginQuery();
	parameters->has_rds = tuner->getRdsReceived();

	if(parameters->has_rds) {
		const char* c_name = tuner->getRdsStationName();
		parameters->rds_station_name = String(c_name);

		const char* c_text = tuner->getRdsProgramInformation();
		parameters->rds_program_name = String(c_text);

		uint16_t year, month, day, hour, minute;
		if(tuner->getRdsDateTime(&year, &month, &day, &hour, &minute)) {
			parameters->hour = hour;
			parameters->min = minute;
		}
	}
}

AIBusHandler* Si4735Controller::getAIHandler() {
	return this->ai_handler;
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
