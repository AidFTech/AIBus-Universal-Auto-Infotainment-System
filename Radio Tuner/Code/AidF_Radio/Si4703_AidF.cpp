#include "Si4703_AidF.h"

//Si4703 radio tuner controller.
Si4703Controller::Si4703Controller(const uint16_t reset_pin, const uint16_t enable_pin, const int16_t SDA_pin, const int16_t SCL_pin, AIBusHandler* ai_handler, ParameterList* parameters, TextHandler* text_handler) {
	this->reset_pin = reset_pin;
	this->enable_pin = enable_pin;
	this->SDA_pin = SDA_pin;
	this->SCL_pin = SCL_pin;
	
	pinMode(this->reset_pin, OUTPUT);
	pinMode(this->enable_pin, OUTPUT);
	pinMode(this->SDA_pin, OUTPUT);
	pinMode(this->SCL_pin, OUTPUT);
	
	digitalWrite(this->reset_pin, HIGH);
	digitalWrite(this->enable_pin, LOW);

	Wire.setClock(I2C_CLOCK);

	this->ai_handler = ai_handler;
	this->parameters = parameters;
	this->text_handler = text_handler;
}

//Loop function.
void Si4703Controller::loop() {

}

//Set the desired frequency, within the tuning range.
uint16_t Si4703Controller::setFrequency(const uint16_t des_freq) {
	readRegisters();
	registers[REG_CHANNEL] &= 0xFE00;
	registers[REG_PWRCFG] |= 0x4000;
	
	uint16_t sub = 7600, inc = 5;
	if((registers[REG_SYSCFG2]&0x00C0) == 0)
		sub = 8750;
	
	if((registers[REG_SYSCFG2]&0x0030) == 0)
		inc = 20;
	else if((registers[REG_SYSCFG2]&0x0010) != 0)
		inc = 10;
	else if((registers[REG_SYSCFG2]&0x0020) != 0)
		inc = 5;
	
	uint16_t bin_freq = (des_freq - sub)/inc;
	
	registers[REG_CHANNEL] |= (bin_freq&0x1FF) | 0x8000; //Set channel and start tuning.
	writeRegisters();
	
	elapsedMillis tuning_timer;
	do {
		readRegisters();
	} while((registers[REG_STATUS] & 0x4000) != 0 && tuning_timer < TUNE_WAIT);
	if(tuning_timer >= TUNE_WAIT)
		return getFrequency();
	
	readRegisters();
	registers[REG_CHANNEL] &= 0x7FFF;
	writeRegisters();
	
	tuning_timer = 0;
	do {
		readRegisters();
	} while((registers[REG_STATUS] & 0x4000) == 0 && tuning_timer < TUNE_WAIT);
	if(tuning_timer >= TUNE_WAIT)
		return getFrequency();
	
	return getFrequency();//bin_freq*inc+sub;
}

//Get the current frequency. The number returned is the tuned frequency * 1000.
uint16_t Si4703Controller::getFrequency() {
	readRegisters();
	return calculateFrequency();
}

//Calculate the tuned frequency without reading registers.
uint16_t Si4703Controller::calculateFrequency() {
	uint16_t sub = 7600, inc = 5;
	if((registers[REG_SYSCFG2]&0x00C0) == 0)
		sub = 8750;
	
	if((registers[REG_SYSCFG2]&0x0030) == 0)
		inc = 20;
	else if((registers[REG_SYSCFG2]&0x0010) != 0)
		inc = 10;
	else if((registers[REG_SYSCFG2]&0x0020) != 0)
		inc = 5;
	
	uint16_t bin_freq = registers[REG_READCH]&0x1FF;
	
	return bin_freq*inc+sub;
}

//Start the controller.
void Si4703Controller::init() {
	//Acknowledgement to the works of Nathan Seidle and Simon Monk.
	
	//pinMode(reset_pin, OUTPUT);
	//pinMode(SDA_pin, OUTPUT);
	//pinMode(enable_pin, OUTPUT);
	
	digitalWrite(enable_pin, LOW);
	
	digitalWrite(SDA_pin, LOW);
	digitalWrite(reset_pin, LOW);
	delay(1);
	digitalWrite(reset_pin, HIGH);
	delay(1);
	
	Wire.begin();

	readRegisters();
	
	registers[7] = 0x8100;
	registers[REG_PWRCFG] |= 0x400;
	writeRegisters();
	
	delay(500);
	
	readRegisters();
	registers[REG_PWRCFG] = 0x1;//0x4001; //Power on
	registers[REG_SYSCFG1] |= 0x1000; //RDS
	//registers[REG_SYSCFG1] |= 0xC000; //Interrupts.
	registers[REG_SYSCFG2] |= 0x000F; //Full volume
	
	#ifndef NA
	registers[REG_SYSCFG1] |= 0x0800;
	#endif
	
	#ifdef JP
	registers[REG_SYSCFG2] |= 0x0080;
	#endif
	
	#if defined EU || defined JP
	registers[REG_SYSCFG2] |= 0x0010;
	#endif
	
	writeRegisters();
	
	delay(110);
}

//Read register values to the register variables.
void Si4703Controller::readRegisters() {
	digitalWrite(this->enable_pin, HIGH);
	Wire.requestFrom(0x10, 32);
	
	bool cached = false;
	
	elapsedMillis comm_timer;
	int last_available = Wire.available();
	while(Wire.available() < 32) {
		if(Wire.available() > last_available) {
			last_available = Wire.available();
			comm_timer = 0;
		}

		bool has_ai = false;
		if(!cached) {
			has_ai = ai_handler->cachePending(ID_RADIO);
			cached = true;
		}
		
		if(comm_timer > COMM_WAIT)
			return;

		if(has_ai)
			parameters->ai_pending = true;
	}
	
	if(!cached)
		ai_handler->cachePending(ID_RADIO);
	
	for(uint8_t i = 0x0A; i <= 0x0F; i+=1) {
		this->registers[i] = (Wire.read() << 8)&0xFF00;
		this->registers[i] |= Wire.read() & 0xFF;
	}
	
	for(uint8_t i = 0; i<= 0x9; i+=1) {
		this->registers[i] = (Wire.read() << 8)&0xFF00;
		this->registers[i] |= Wire.read() & 0xFF;
	}
	digitalWrite(this->enable_pin, LOW);
}

//Write values from the register variables to the registers.
bool Si4703Controller::writeRegisters() {
	digitalWrite(this->enable_pin, HIGH);
	Wire.beginTransmission(0x10);
	
	for(uint8_t i = 2; i < 8; i+=1) {
		uint8_t msb = registers[i] >> 8;
		uint8_t lsb = registers[i] & 0xFF;
		
		Wire.write(msb);
		Wire.write(lsb);
	}
	
	uint8_t result = Wire.endTransmission();
	digitalWrite(this->enable_pin, LOW);
	
	if(result != 0)
		return false;
	else
		return true;
}

//Returns any RDS data present.
bool Si4703Controller::getRDS(uint16_t* messages) {
	if(this->seeking)
		return false;
	
	readRegisters();
	if(parameters->ai_pending || ai_handler->dataAvailable() > 0 || parameters->timer_active)
		return false;

	if((registers[REG_STATUS]&0x8000) == 0)
		return false;

	uint16_t new_messages[] = {0,0,0,0};

	uint8_t completed_count = 0;

	bool completed[] = {false, false, false, false};
	bool errors_found = false;

	for(int round=0;round<3;round+=1) {
		elapsedMillis rds_timer = 0;
		uint16_t rds_wait = RDS_WAIT;

		completed_count = 0;
		for(int i=0;i<sizeof(completed)/sizeof(bool);i+=1)
			completed[i] = false;

		errors_found = false;

		while((rds_timer < rds_wait && completed_count < 4) || errors_found) {
			const bool last_errors_found = errors_found;
			const int8_t last_hour = parameters->hour, last_min = parameters->min;
			elapsedMillis delay_timer;

			if((registers[REG_STATUS]&0x8000) != 0) {
				if(ai_handler->cachePending(ID_RADIO) || ai_handler->dataAvailable() > 0)
					parameters->ai_pending = true;
				if(parameters->ai_pending)
					break;

				if((registers[REG_RDSB]&0xF800) == 0x4000) { //Time/date.
					if((registers[REG_READCH]&0x3C00) == 0) {
						this->parameters->hour = (registers[REG_RDSD]>>12) | ((registers[REG_RDSC]&0x1) << 4);
						this->parameters->min = (registers[REG_RDSD]&0xFC0) >> 6;
						
						this->parameters->offset = (registers[REG_RDSD]&0x1F);
						if((registers[REG_RDSD]&0x20)!=0)
							this->parameters->offset *= -1;
					}
					
					if(last_hour != parameters->hour || last_min != parameters->min) {
						parameters->minute_timer = 0;
						text_handler->sendTime();
					}
				}

				bool skip = false;
				const uint8_t index = registers[REG_RDSB]&0x3;

				if((completed[index] && !errors_found) || (registers[REG_RDSB]&0xF000) != 0x0000) {
					delay_timer = 0;
					while(delay_timer < 30);
					readRegisters();
					skip = true;
				}
				if(skip)
					continue;

				new_messages[index] = registers[REG_RDSD];
				completed[index] = true;
				completed_count += 1;
				delay_timer = 0;
				while(delay_timer < 40);

				if(completed_count >= 4) {
					errors_found = getRDSTextErrors();
					if(errors_found && !last_errors_found)
						rds_wait = RDS_WAIT + 2000;

					if(errors_found)
						rds_timer = 0;
				} else
					rds_timer = 0;

				const bool fm_stereo = parameters->fm_stereo;
				parameters->fm_stereo = (registers[REG_STATUS]&0x100) != 0;
				if(fm_stereo != parameters->fm_stereo)
					text_handler->sendStereoMessage(parameters->fm_stereo);
			}

			readRegisters();
			if(parameters->ai_pending)
				break;
		}
		if(parameters->ai_pending)
			break;
	}

	if(parameters->ai_pending || completed_count < 4 || errors_found)
		return false;

	for(int i=0;i<sizeof(new_messages)/sizeof(uint16_t);i+=1)
		messages[i] = new_messages[i];
	
	return true;
}

//Returns whether any RDS errors are present.
bool Si4703Controller::getRDSErrors(const bool read) {
	if(read)
		readRegisters();
	
	if((registers[REG_READCH]&0xFC00) != 0)
		return true;
	
	if((registers[REG_STATUS]&0x0600) != 0)
		return true;
	
	return false;
}

//Returns whether any RDS errors are present in the text message.
bool Si4703Controller::getRDSTextErrors() {
	readRegisters();
	if((registers[REG_READCH]&0xC000) != 0)
		return true;
	else
		return false;
}

//Fills the parameter list with stereo and RDS data.
void Si4703Controller::getParameters(ParameterList* parameters, const uint8_t setting) {
	if(this->seeking)
		return;
	
	readRegisters();
	parameters->fm_stereo = (registers[REG_STATUS]&0x100) != 0;
	
	if(ai_handler->dataAvailable() > 0) {
		parameters->ai_pending = true;
		return;
	}
	
	/*const uint16_t current_frequency = getFrequency();
	if(setting == 0)
		parameters->fm1_tune = current_frequency;
	else if(setting == 1)
		parameters->fm2_tune = current_frequency;*/

	parameters->has_rds = getRDS(parameters->rds_text);
}

uint8_t Si4703Controller::getRSSI() {
	readRegisters();
	return registers[REG_STATUS]&0xFF;
}

//Turn the radio output on or off.
void Si4703Controller::setPower(const bool power) {
	readRegisters();

	if(power)
		registers[REG_PWRCFG] |= 0x4000;
	else
		registers[REG_PWRCFG] &= ~0x4000;

	writeRegisters();
}

//Increment the tuned frequency.
uint16_t Si4703Controller::incrementFrequency(const uint8_t count) {
	return this->incrementFrequency(getFrequency(), count);
}

//Increment the tuned frequency.
uint16_t Si4703Controller::incrementFrequency(const uint16_t current_frequency, const uint8_t count) {
	uint16_t inc = 5;
	
	if((registers[REG_SYSCFG2]&0x0030) == 0)
		inc = 20;
	else if((registers[REG_SYSCFG2]&0x0010) != 0)
		inc = 10;
	else if((registers[REG_SYSCFG2]&0x0020) != 0)
		inc = 5;

	uint16_t ceiling = 10800, floor = 8750;
	if((registers[REG_SYSCFG2]&0xC0) != 0) {
		floor = 7600;
		if((registers[REG_SYSCFG2]&0x80) != 0)
			ceiling = 9000;
	}

	uint16_t desired_frequency = current_frequency;
	for(uint8_t i=0;i<count;i+=1) {
		desired_frequency += inc;
		if(desired_frequency > ceiling)
			desired_frequency = floor;
		setFrequency(desired_frequency);
	}
	return getFrequency();
}

//Decrement the tuned frequency.
uint16_t Si4703Controller::decrementFrequency(const uint8_t count) {
	return this->decrementFrequency(getFrequency(), count);
}

//Decrement the tuned frequency.
uint16_t Si4703Controller::decrementFrequency(const uint16_t current_frequency, const uint8_t count) {
	uint16_t inc = 5;
	
	if((registers[REG_SYSCFG2]&0x0030) == 0)
		inc = 20;
	else if((registers[REG_SYSCFG2]&0x0010) != 0)
		inc = 10;
	else if((registers[REG_SYSCFG2]&0x0020) != 0)
		inc = 5;

	uint16_t ceiling = 10800, floor = 8750;
	if((registers[REG_SYSCFG2]&0xC0) != 0) {
		floor = 7600;
		if((registers[REG_SYSCFG2]&0x80) != 0)
			ceiling = 9000;
	}

	uint16_t desired_frequency = current_frequency;
	for(uint8_t i=0;i<count;i+=1) {
		desired_frequency -= inc;
		if(desired_frequency < floor)
			desired_frequency = ceiling;
		setFrequency(desired_frequency);
	}
	return getFrequency();
}

//Start seeking.
void Si4703Controller::startSeek(const bool seek_up) {
	readRegisters();
	registers[REG_PWRCFG] |= 0x400;
	
	if(seek_up)
		registers[REG_PWRCFG] |= 0x200;
	else
		registers[REG_PWRCFG] &= ~0x200;
	
	registers[REG_PWRCFG] |= 0x100; //Start seeking.
	writeRegisters();
	this->seeking = true;
}

//Return whether actively seeking and stop seeking if completed.
bool Si4703Controller::getSeeking(uint16_t* frequency) {
	if(this->seeking) {
		*frequency = getFrequency();
		readRegisters();
		if((registers[REG_STATUS]&0x4000) != 0) { //Tuned!
			registers[REG_PWRCFG] &= ~0x100;
			writeRegisters();
			*frequency = getFrequency();
			this->seeking = false;
		}
		return this->seeking;
	} else
		return false;
}

AIBusHandler* Si4703Controller::getAIHandler() {
	return this->ai_handler;
}

ParameterList* Si4703Controller::getParameterList() {
	return this->parameters;
}

String decodeRDS(uint16_t* rds, Si4703Controller *tuner) {
	bool char_read[] = {false, false, false, false};
	
	char rds_return[8];
	uint8_t index = rds[1]&0x03;
	
	do {
		tuner->getRDS(rds);
		index = rds[1]&0x03;index = rds[1]&0x03;
		
		if(/*(rds[1]&0x03) == index && */!tuner->getRDSErrors(false)) {
			rds_return[2*index] = (rds[3]&0xFF00) >> 8;
			rds_return[2*index + 1]= rds[3]&0xFF;
			char_read[index] = true;
			index += 1;
		}

		if(tuner->getAIHandler()->cachePending(ID_RADIO)) {
			tuner->getParameterList()->ai_pending = true;
			return String(rds_return);
		}
	} while(!(char_read[0] && char_read[1] && char_read[2] && char_read[3]));

	String rds_return_str = "";
	for(uint8_t i=0;i<8;i+=1)
		rds_return_str += rds_return[i];
	return rds_return_str;
}

int8_t decodeRDS(uint16_t* rds, Si4703Controller *tuner, String* char_text) {
	if(char_text->length() != 8)
		*char_text = F("        ");
	
	if(tuner->getRDS(rds)) {
		uint8_t index = rds[1]&0x03;
		char_text->setCharAt(index*2, (rds[3]&0xFF00) >> 8);
		char_text->setCharAt(index*2 + 1, rds[3]&0xFF);
		
		return int8_t(index);
	}
	
	return -1;
}

String getRDSString(uint16_t* registers) {
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
}
