#include <stdint.h>
#include <SPI.h>
#include <SRAM_23LC.h>

#include "Audio_Source.h"
#include "Volume_Handler.h"
#include "Background_Tune_Handler.h"

#ifndef radio_ram_h
#define radio_ram_h

#define AIDF_RAM_HEADER "AidF AIA-AR100"

#define ADDR_AIDF_HEADER 0

enum ADDR {
	SELECTED_SOURCE = sizeof(AIDF_RAM_HEADER),
	SELECTED_SUBSOURCE,
	AUDIO_ON,
	FM1_FREQ,
	FM2_FREQ = FM1_FREQ + sizeof(uint16_t),
	AM_FREQ = FM2_FREQ + sizeof(uint16_t),
	CLOCK_FREQ = AM_FREQ + sizeof(uint16_t),
	VOL = CLOCK_FREQ + sizeof(int16_t),
	MAX_VOL = VOL + sizeof(uint16_t),
	TREBLE = MAX_VOL + sizeof(uint16_t),
	BASS = TREBLE + sizeof(uint16_t),
	BALANCE = BASS + sizeof(uint16_t),
	FADER = BALANCE + sizeof(int16_t),
	SOURCE_COUNT = FADER + sizeof(int16_t),
	FM_STATION_COUNT,

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

	bool audio_on = false;
	
	uint16_t vol = 0, max_vol = DEFAULT_VOL_RANGE, treble = 0, bass = 0;
	int16_t balance = 0, fader = 0;
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