#include "Aux_Light_Control.h"

AuxLightController::AuxLightController(const uint8_t aux_light_cs, const uint8_t addr) :
	light_controller(aux_light_cs, addr) {
	
}

//Initialize the MCP.
void AuxLightController::init() {
	light_controller.begin();

	light_controller.pinModeIO(AUX_LIGHT_DRL_L, OUTPUT);
	light_controller.pinModeIO(AUX_LIGHT_DRL_R, OUTPUT);
	light_controller.pinModeIO(AUX_LIGHT_TURN_L, OUTPUT);
	light_controller.pinModeIO(AUX_LIGHT_TURN_R, OUTPUT);
	light_controller.pinModeIO(AUX_LIGHT_REAR_FOG_L, OUTPUT);
	light_controller.pinModeIO(AUX_LIGHT_REAR_FOG_R, OUTPUT);
	light_controller.pinModeIO(AUX_LIGHT_AUX_PROJECTOR, OUTPUT);

	light_controller.digitalWriteIO(AUX_LIGHT_DRL_L, false);
	light_controller.digitalWriteIO(AUX_LIGHT_DRL_R, false);
	light_controller.digitalWriteIO(AUX_LIGHT_TURN_L, false);
	light_controller.digitalWriteIO(AUX_LIGHT_TURN_R, false);
	light_controller.digitalWriteIO(AUX_LIGHT_REAR_FOG_L, false);
	light_controller.digitalWriteIO(AUX_LIGHT_REAR_FOG_R, false);
	light_controller.digitalWriteIO(AUX_LIGHT_AUX_PROJECTOR, false);
}

//Set the left DRL.
void AuxLightController::setLeftDRL(const bool state) {
	light_controller.digitalWriteIO(AUX_LIGHT_DRL_L, state);
}

//Set the right DRL.
void AuxLightController::setRightDRL(const bool state) {
	light_controller.digitalWriteIO(AUX_LIGHT_DRL_R, state);
}

//Set the left turn signal.
void AuxLightController::setLeftTurn(const bool state) {
	light_controller.digitalWriteIO(AUX_LIGHT_TURN_L, state);
}

//Set the right turn signal.
void AuxLightController::setRightTurn(const bool state) {
	light_controller.digitalWriteIO(AUX_LIGHT_TURN_R, state);
}

//Set the left rear foglight.
void AuxLightController::setLeftRFog(const bool state) {
	light_controller.digitalWriteIO(AUX_LIGHT_REAR_FOG_L, state);
}

//Set the right rear foglight.
void AuxLightController::setRightRFog(const bool state) {
	light_controller.digitalWriteIO(AUX_LIGHT_REAR_FOG_R, state);
}

//Set the tail.
void AuxLightController::setTail(const bool state) {
	light_controller.digitalWriteIO(AUX_LIGHT_AUX_TAIL, state);
}

//Set the projector.
void AuxLightController::setProjector(const bool state) {
	light_controller.digitalWriteIO(AUX_LIGHT_AUX_PROJECTOR, state);
}