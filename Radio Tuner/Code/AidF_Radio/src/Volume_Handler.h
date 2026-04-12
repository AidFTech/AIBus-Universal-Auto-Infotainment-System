#include <stdint.h>
#include <MCP4251.h>
#include <Arduino.h>

#include "AIBus_Handler.h"
#include "Parameter_List.h"

#ifndef volume_handler_h
#define volume_handler_h

#define DEFAULT_SLIDER_RANGE 32
#define DEFAULT_TONE_RANGE 256
#define DEFAULT_VOL_RANGE 64

#define MAX_ATTENUATION 20
#define MAX_ATTENUATION_RATIO 0.1
#define BASS_X 2411.438532
#define BASS_R 220.0
#define TREBLE_X 3900.85644
#define TREBLE_R 220.0

class VolumeHandler {
public:
	VolumeHandler(MCP4251* vol_mcp, MCP4251* treble_mcp, MCP4251* bass_mcp, MCP4251* fader_mcp, ParameterList* parameters, AIBusHandler* ai_handler);

	void init();

	bool handleAIBus(AIData *msg);
	void setAIBusParameter(AIData *msg);

	void setVolRange(const uint16_t vol_range);
	uint16_t getVolRange();

	void setVolume(const uint16_t volume);
	void setBass(int bass);
	void setTreble(int treble);

	void setBalance(const int16_t balance);
	void setFader(const int16_t fader);
	
	uint16_t getVolume();
	uint16_t getBass();
	uint16_t getTreble();
	
	int16_t getBalance();
	int16_t getFader();

	bool getVolumeChanged();
	void setUseAuxLevel(const bool use_aux_level);

	void refreshSVC();
private:
	MCP4251 *vol_mcp, *treble_mcp, *bass_mcp, *fader_mcp;
	AIBusHandler* ai_handler;

	ParameterList* parameters;

	uint16_t vol_range = DEFAULT_VOL_RANGE;
	uint16_t volume = 0, treble = DEFAULT_TONE_RANGE, bass = 0;
	int16_t balance = 0, fader = 0;

	bool volume_changed = false;

	bool use_aux_level = false;

	void setVolume();
	void setVolumeDisplay();
};

#endif
