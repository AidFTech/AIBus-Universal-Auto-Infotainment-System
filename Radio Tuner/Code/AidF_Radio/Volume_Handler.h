#include <stdint.h>
#include <MCP4251.h>

#include "AIBus_Handler.h"
#include "Parameter_List.h"

#ifndef volume_handler_h
#define volume_handler_h

#define DEFAULT_SLIDER_RANGE 32
#define DEFAULT_TONE_RANGE 256

class VolumeHandler {
public:
	VolumeHandler(MCP4251* vol_mcp, MCP4251* treble_mcp, MCP4251* bass_mcp, MCP4251* fader_mcp, ParameterList* parameters, AIBusHandler* ai_handler);

	bool handleAIBus(AIData *msg);
	void setAIBusParameter(AIData *msg);

	void setVolRange(const uint16_t vol_range);
	uint16_t getVolRange();

	void setVolume(const uint16_t volume);
	void setBass(const uint16_t bass);
	void setTreble(const uint16_t treble);

	void setBalance(const int16_t balance);
	void setFader(const int16_t fader);
	
	uint16_t getVolume();
	uint16_t getBass();
	uint16_t getTreble();
	
	int16_t getBalance();
	int16_t getFader();
private:
	MCP4251 *vol_mcp, *treble_mcp, *bass_mcp, *fader_mcp;
	AIBusHandler* ai_handler;

	ParameterList* parameters;

	uint16_t vol_range = DEFAULT_TONE_RANGE;
	uint16_t volume = 0, treble = DEFAULT_TONE_RANGE, bass = 0;
	int16_t balance = 0, fader = 0;

	void setVolume();
};

#endif
