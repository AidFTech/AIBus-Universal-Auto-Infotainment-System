#include <stdint.h>
#include <SPI.h>
#include <SRAM_23LC.h>

#include "Audio_Source.h"
#include "Volume_Handler.h"
#include "Background_Tune_Handler.h"
#include "Parameter_List.h"

#ifndef radio_ram_h
#define radio_ram_h

#define AIDF_RAM_HEADER "AidF AIA-AR100"

#define ADDR_AIDF_HEADER 0

#define CLOCK_MODE_BROADCAST_AUTO 0x1
#define CLOCK_MODE_BROADCAST_MANUAL 0x2
#define CLOCK_MODE_RTC_INIT 0x4
#define CLOCK_MODE_12H 0x80

enum ADDR {
	SELECTED_SOURCE = sizeof(AIDF_RAM_HEADER),
	SELECTED_SUBSOURCE,
	AUDIO_ON,
	FM1_FREQ,
	FM2_FREQ = FM1_FREQ + sizeof(uint16_t),
	AM_FREQ = FM2_FREQ + sizeof(uint16_t),
	CLOCK_FREQ = AM_FREQ + sizeof(uint16_t),
	CLOCK_MODE = CLOCK_FREQ + sizeof(int16_t),
	VOL,
	MAX_VOL = VOL + sizeof(uint16_t),
	TREBLE = MAX_VOL + sizeof(uint16_t),
	BASS = TREBLE + sizeof(uint16_t),
	BALANCE = BASS + sizeof(uint16_t),
	FADER = BALANCE + sizeof(int16_t),
	SOURCE_COUNT = FADER + sizeof(int16_t),
	FM_STATION_COUNT,
	FM1_PRESETS,
	FM2_PRESETS = FM1_PRESETS + sizeof(uint16_t)*PRESET_COUNT,
	AM_PRESETS = FM2_PRESETS + sizeof(uint16_t)*PRESET_COUNT,
	RDS_DISPLAY_MODE = AM_PRESETS + sizeof(uint16_t)*PRESET_COUNT,
	AUX_NAV_LEVEL,
	SOURCE_FUNCTION_FILTER,
	SVC_SETTING,

	SOURCE_START = 0x100,
	FM_STATION_START = 0x400,
};

enum SOURCE_ADDR {
	SOURCE_ID,
	SOURCE_SUB_ID
};

//Radio startup parameters.
struct StartParams {
	uint8_t selected_source = 0, selected_subsource = 0;
	uint16_t fm1_freq = 0, fm2_freq = 0, am_freq = 0;
	int16_t clock_freq = -1;

	uint8_t clock_mode;

	bool audio_on = false;
	
	uint16_t vol = 0, max_vol = DEFAULT_VOL_RANGE, treble = 0, bass = 0;
	int16_t balance = 0, fader = 0;

	svc_setting_t svc_setting;
	header_rds_setting_t rds_setting;
	source_button_t source_button_setting;
	uint8_t aux_level, nav_cut;
	bool dac_latency, steering_control_preset;
};

class SRAMHandler {
public:
	SRAMHandler(const uint8_t ram_cs);

	void begin();

	void writeHeader();
	bool getValid();

	void clearRAM();

	void setStartParams(StartParams* start_params);
	void getStartParams(StartParams* start_params);

	void setRAMPresets(ParameterList* parameter_list);
	void getRAMPresets(ParameterList* parameter_list);

	uint8_t getSourceCount();
	void setSources(const uint16_t l, AudioSource* source_list);
	void getSources(const uint16_t l, AudioSource* source_list);

	void setFrequencies(BackgroundTuneHandler* tuner);
	void getFrequencies(BackgroundTuneHandler* tuner);
private:
	SRAM_23LC sram;

	uint16_t readUint16(const uint32_t addr);
	int16_t readInt16(const uint32_t addr);

	void writeUint16(const uint32_t addr, const uint16_t data);
	void writeInt16(const uint32_t addr, const int16_t data);
};

#endif