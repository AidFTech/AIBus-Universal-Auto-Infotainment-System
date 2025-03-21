#include "ADC_Handler.h"

PCM9211Handler::PCM9211Handler(const int pcm9211_sel) {
	SPI.begin();
	this->pcm9211_sel = pcm9211_sel;
}

//Start the ADC. Thanks to https://e2e.ti.com/support/audio-group/audio/f/audio-forum/262000/pcm9211-configuration-issue
void PCM9211Handler::init() {
	delay(100);
	writeRegister(REG_POWER_CONTROL, 0x0);
	delay(30);
	writeRegister(REG_POWER_CONTROL, 0x33);
	delay(40);
	writeRegister(REG_POWER_CONTROL, 0xC0);
	
	writeRegister(REG_SOURCE_SCLK, 0x1A);
	writeRegister(REG_SOURCE_SECONDARY_CLOCK2, 0x22);
	writeRegister(REG_ERROR_OUTPUT, 0x0);
	writeRegister(REG_ERROR_CAUSE, 0x1);
	writeRegister(REG_OSCILLATION_CONTROL, 0x0);
	writeRegister(REG_ADC_CLK, 0x81);
	writeRegister(REG_SOURCE_SECONDARY_CLOCK2, 0x22);

	writeRegister(REG_ADC_FUNCTION_CTRL, 0x2);
	writeRegister(REG_ADC_FUNCTION_CTRL2, 0x0);
	writeRegister(REG_ADC_ATTEN, 0xD7);

	writeRegister(REG_DIT_FUNCTION0, 0x22);
	writeRegister(REG_DIT_FUNCTION1, 0x10);
	writeRegister(REG_DIT_FUNCTION2, 0x0);

	writeRegister(REG_RECOUT0_SOURCE, 0x0F);
	setMPOOutputs(0x0, 0xE);

	writeRegister(REG_MPIOCA_HIZ, 0xF);
	writeRegister(REG_MPIOABC_GROUP, 0x40);

	writeRegister(REG_OUTPUT_PORT, 0x22);
	writeRegister(REG_DIR_INPUT_SOURCE, 0xCF);
	writeRegister(REG_OUTPUT_MUTE, 0xCC);
}

//Turn the ADC off.
void PCM9211Handler::powerOff() {
	writeRegister(REG_POWER_CONTROL, 0x33);
	writeRegister(REG_OUTPUT_PORT, 0x33);
}

//Set the output to the ADC.
void PCM9211Handler::setADCOn() {
	writeRegister(REG_OUTPUT_PORT, 0x22);
	writeRegister(REG_RECOUT0_SOURCE, 0xF);
	writeRegister(REG_DIR_INPUT_SOURCE, 0xCF);
}

//Set the output to digital input.
void PCM9211Handler::setDigitalOut() {
	writeRegister(REG_OUTPUT_PORT, 0x33);
	writeRegister(REG_RECOUT0_SOURCE, 0xF);
	writeRegister(REG_DIR_INPUT_SOURCE, 0xCF);
}

//Set the MPO0 and MPO1 outputs.
void PCM9211Handler::setMPOOutputs(const uint8_t mpo1, const uint8_t mpo0) {
	writeRegister(REG_MPO_SETTING, ((mpo1&0xF)<<4) | mpo0&0xF);
}

//Write to a register.
void PCM9211Handler::writeRegister(const uint8_t reg, const uint8_t value) {
	digitalWrite(pcm9211_sel, LOW);
	SPI.transfer(reg&0x7F);
	SPI.transfer(value);
	digitalWrite(pcm9211_sel, HIGH);
}

//Read a register.
uint8_t PCM9211Handler::readRegister(const uint8_t reg) {
	digitalWrite(pcm9211_sel, LOW);
	SPI.transfer((reg&0x7F) | 0x80);
	const uint8_t value = SPI.transfer(0xFF);

	digitalWrite(pcm9211_sel, HIGH);
	
	return value;
}
