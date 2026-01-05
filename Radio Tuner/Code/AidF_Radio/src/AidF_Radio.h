#include <Arduino.h>

#include <elapsedMillis.h>
#include <MCP4251.h>
#include <RTCx.h>

#include <Wire.h>

#include "AIBus.h"
#include "AIBus_Handler.h"
#include "Locale.h"
#include "Audio_Source.h"
#include "Text_Handler.h"
#include "Parameter_List.h"
#include "Si4735_AidF.h"
#include "Volume_Handler.h"
#include "Background_Tune_Handler.h"
#include "ADC_Handler.h"

#include "Text_Split.h"

#include "Radio_EEPROM.h"
#include "Radio_RAM.h"

#ifndef aidf_radio_h
#define aidf_radio_h

#if defined(DXCORE)
#define AI_RX PIN_PA7
#define TUNER_RESET PIN_PC0
#define IDAT PIN_PA2
#define ICLK PIN_PA3

#define AUDIO_SW PIN_PC1 //Audio switch.
#define NAV_SW PIN_PC2 //Nav audio switch. Input.
#define AUDIO_ON_SW PIN_PC3 //Audio on/off. Audio on when low.
#define POWER_ON_SW PIN_PD0 //Output, vehicle power is on when high.
#define AUX_SW PIN_PD1 //Input, aux port mechanical switch.

#define DAC_FILTER_MODE PIN_PD2 //Output, digital filter mode.
#define DAC_MUTE PIN_PD3 //Output, analog mute.

#define ADC_CS PIN_PD4
#define VOL_CS PIN_PD5
#define TREBLE_CS PIN_PD6
#define BASS_CS PIN_PF2
#define FADE_CS PIN_PD7

#define NAV_MUTE PIN_PF3
#define RAM_CS PIN_PF4
#define DIGITAL_ERROR PIN_PF5

#else
#define AI_RX 3
#define TUNER_RESET 6
#define IDAT A4
#define ICLK A5

#define AUDIO_SW 7 //Audio switch.
#define NAV_SW 9 //Nav audio switch. Input.
#define AUDIO_ON_SW 10 //Audio on/off. Audio on when low.
#define POWER_ON_SW 13 //Output, vehicle power is on when high.
#define AUX_SW 14 //Input, aux port mechanical switch.

#define DAC_FILTER_MODE 15 //Output, digital filter mode.
#define DAC_MUTE 22 //Output, analog mute.

#define ADC_CS 16
#define VOL_CS 17
#define TREBLE_CS 18
#define BASS_CS 19
#define FADE_CS 20

#define NAV_MUTE 21
#define RAM_CS 22
#define DIGITAL_ERROR 23
#endif

#define SOURCE_COUNT 16
#define SOURCE_PING_DELAY 5000
#define COMPUTER_PING_DELAY 4000
#define SCREEN_PING_DELAY 5000
#define PARAMETER_DELAY 250

#define DISPLAY_INFO_TIMER 750
#define IMID_TIMER 300

#define DOOR_TIMER 30000
#define CONTROL_TIMER 7000

#define SOURCE_CHANGE_TIMER 1000

#define SOURCE_CHECK_TIMER 10000

#define RDS_SEGMENT_COUNT 12
#define RDS_IMID_TIMER 3000

#define AISerial Serial

class AidFRadio {
public:
	void setup();
	void loop();
	
private:
	ParameterList parameters;

	AIBusHandler aibus_handler = AIBusHandler(&AISerial, AI_RX, ID_RADIO);

	TextHandler text_handler = TextHandler(&aibus_handler, &parameters);

	MCP4251 vol_controller = MCP4251(VOL_CS, 10000, 0, 10000, 0);
	MCP4251 treble_controller = MCP4251(TREBLE_CS, 50000, 0, 50000, 0);
	MCP4251 bass_controller = MCP4251(BASS_CS, 10000, 0, 10000, 0);
	MCP4251 fade_controller = MCP4251(FADE_CS, 10000, 0, 10000, 0);

	VolumeHandler volume_handler = VolumeHandler(&vol_controller, &treble_controller, &bass_controller, &fade_controller, &parameters, &aibus_handler);

	Si4735Controller tuner = Si4735Controller(TUNER_RESET, HIGH, &parameters), br_tuner = Si4735Controller(TUNER_RESET, LOW, &parameters);
	BackgroundTuneHandler background_tuner = BackgroundTuneHandler(&br_tuner, &parameters);
	SourceHandler source_handler = SourceHandler(&aibus_handler, &text_handler, &tuner, &background_tuner, &parameters, &volume_handler, SOURCE_COUNT);

	PCM9211Handler adc_handler = PCM9211Handler(ADC_CS);

	elapsedMillis aibus_timer, source_text_timer;
	elapsedMillis src_ping_timer, computer_ping_timer, parameter_timer, screen_ping_timer;

	bool source_text_timer_enabled = false;

	bool info_timer_enabled = false;
	elapsedMillis info_timer;

	bool imid_timer_enabled = false;
	elapsedMillis imid_timer;

	elapsedMillis rds_imid_timer;
	int rds_imid_index = 0;

	elapsedMillis background_tune_timer = 0;

	elapsedMillis source_check_timer = 0;
	bool source_check_enabled = true;

	String rds_program_split[RDS_SEGMENT_COUNT];

	bool* power_on = &parameters.power_on, *digital_mode = &parameters.digital_mode;
	bool key_on = false; //True if the key has been at any time during this power cycle.
	bool rtc_on = false; //True if the clock is on and functional.

	bool source_change_timer_enable = false;
	elapsedMillis source_change_timer;

	elapsedMillis door_timer;
	bool door_timer_enabled = false;

	elapsedMillis control_timer;

	SRAMHandler sram_handler = SRAMHandler(RAM_CS);

	volatile bool tuner_reset = false;
	
	void handleAIBus(AIData* msg);
	void sendTunedFrequencyMessage(const uint8_t sub_id);
	void setTunerFrequency(const uint8_t sub_id);
	void pingActiveSource();
	void pingComputer();
	void screenInit();
	void setSourceName();
	double getSpeed(AIData* msg);
	void clearFMData();
	void getScreenControlRequest(const bool all);
	void sendIMIDPing();
	void sendIMIDRequest();
	void normalizePresets();
	void sendAudioLightMessage(const bool audio_on);

	void fullPowerOn();
	void powerOff();
};

void setup();
void loop();

int getSourceRank(const void* source_a, const void* source_b);

#endif
