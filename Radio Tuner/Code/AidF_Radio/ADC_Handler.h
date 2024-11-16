#include <Arduino.h>
#include <SPI.h>

#ifndef adc_handler_h
#define adc_handler_h

#define REG_ERROR_OUTPUT 0x20
#define REG_OSCILLATION_CONTROL 0x24
#define REG_ERROR_CAUSE 0x25
#define REG_ADC_CLK 0x26
#define REG_ERROR_INT0_OUTPUT 0x2C
#define REG_ERROR_INT1_OUTPUT 0x2D
#define REG_DIR_AUTO_CLOCK 0x30
#define REG_SOURCE_SCLK 0x31
#define REG_SOURCE_SECONDARY_CLOCK1 0x32
#define REG_SOURCE_SECONDARY_CLOCK2 0x33
#define REG_DIR_INPUT_SOURCE 0x34
#define REG_RECOUT0_SOURCE 0x35
#define REG_RECOUT1_SOURCE 0x36
#define REG_POWER_CONTROL 0x40
#define REG_ADC_FUNCTION_CTRL 0x42
#define REG_ADC_ATTEN 0x46
#define REG_ADC_FUNCTION_CTRL2 0x48
#define REG_DIT_FUNCTION0 0x60
#define REG_DIT_FUNCTION1 0x61
#define REG_DIT_FUNCTION2 0x62
#define REG_OUTPUT_PORT 0x6B
#define REG_AUX_OUTPUT 0x6C
#define REG_MPIOCA_HIZ 0x6E
#define REG_MPIOABC_GROUP 0x6F
#define REG_MPO_SETTING 0x78

class PCM9211Handler {
public:
	PCM9211Handler(const int pcm9211_sel);

	void init();
	void powerOff();

	void setADCOn();
	void setDigitalOut();

	uint8_t getOutputPort();
	
private:
	int pcm9211_sel = -1;
	
	void setMPOOutputs(const uint8_t mpo1, const uint8_t mpo0);

	void writeRegister(const uint8_t reg, const uint8_t value);
	uint8_t readRegister(const uint8_t reg);
};

#endif
