#include <stdint.h>
#include <MCP4251.h>

#include "AIBus_Handler.h"

#ifndef volume_handler_h
#define volume_handler_h

#define DEFAULT_RANGE 256

class VolumeHandler {
public:
	VolumeHandler(MCP4251* vol_mcp, MCP4251* treble_mcp, MCP4251* bass_mcp, MCP4251* fader_mcp, AIBusHandler* ai_handler);

	void setVolRange(const uint16_t vol_range);
	uint16_t getVolRange();

	void setVolume(const uint16_t volume);
	uint16_t getVolume();
private:
	MCP4251 *vol_mcp, *treble_mcp, *bass_mcp, *fader_mcp;
	AIBusHandler* ai_handler;

	uint16_t vol_range = DEFAULT_RANGE;
	uint16_t volume = 0, treble = DEFAULT_RANGE, bass = DEFAULT_RANGE;
	int16_t balance = 0, fader = 0;
};

#endif
