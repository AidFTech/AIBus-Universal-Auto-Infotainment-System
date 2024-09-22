#include <stdint.h>
#include <elapsedMillis.h>
#include <MCP4251.h>

#include "AIBus.h"
#include "AIBus_Handler.h"
#include "BM83_AidF.h"

#ifdef MEGACOREX
#define AI_RX PIN_PA7
#define POWER_ON PIN_PC2
#define DIGITAL_OUT PIN_PC3

#define BM83_MFB PIN_PD0
#define BM83_MODE PIN_PD1
#define BM83_RESET PIN_PD2

#define SPDIF_RST PIN_PD3
#define DAC_MUTE PIN_PD4
#define DAC_FILTER PIN_PD5
#define BIAS_CS PIN_PD6
#define EEPROM_CS PIN_PD7

#else
#define AI_RX 4
#define POWER_ON 5
#define DIGITAL_OUT 6

#define BM83_MFB 7
#define BM83_MODE 8
#define BM83_RESET 9

#define SPDIF_RESET 14
#define DAC_MUTE 15
#define DAC_FILTER 16
#define BIAS_CS 17
#define EEPROM_CS 18
#endif

#define AISerial Serial

#if !defined(HAVE_HWSERIAL1)
#include <SoftwareSerial.h>
SoftwareSerial BM83Serial(2, 3);
#else
#define BM83Serial Serial1
#endif

AIBusHandler ai_handler(&AISerial, AI_RX);
BM83 bm83_handler(&BM83Serial);

//Arduino setup.
void setup() {
	AISerial.begin(AI_BAUD);
	BM83Serial.begin(115200);
}

//Arduino loop.
void loop() {
	
}
