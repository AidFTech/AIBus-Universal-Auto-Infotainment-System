#include <Arduino.h>
#include <MCP23S08.h>

#ifndef aux_light_control_h
#define aux_light_control_h

#define AUX_LIGHT_DRL_L 0
#define AUX_LIGHT_DRL_R 1
#define AUX_LIGHT_TURN_L 2
#define AUX_LIGHT_TURN_R 3
#define AUX_LIGHT_REAR_FOG_L 4
#define AUX_LIGHT_REAR_FOG_R 5
#define AUX_LIGHT_AUX_TAIL 6
#define AUX_LIGHT_AUX_PROJECTOR 7

class AuxLightController {
public:
	AuxLightController(const uint8_t aux_light_cs, const uint8_t addr);

	void init();

	void setLeftDRL(const bool state);
	void setRightDRL(const bool state);
	void setLeftTurn(const bool state);
	void setRightTurn(const bool state);

	void setLeftRFog(const bool state);
	void setRightRFog(const bool state);

	void setTail(const bool state);
	void setProjector(const bool state);
private:
	MCP23S08 light_controller;
};

#endif