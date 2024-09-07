#include "Volume_Handler.h"

VolumeHandler::VolumeHandler(MCP4251* vol_mcp, MCP4251* treble_mcp, MCP4251* bass_mcp, MCP4251* fader_mcp, ParameterList* parameters, AIBusHandler* ai_handler) {
	this->ai_handler = ai_handler;
	this->parameters = parameters;

	this->vol_mcp = vol_mcp;
	this->treble_mcp = treble_mcp;
	this->bass_mcp = bass_mcp;
	this->fader_mcp = fader_mcp;

	this->vol_mcp->begin();
	this->treble_mcp->begin();
	this->bass_mcp->begin();
	this->fader_mcp->begin();

	this->vol_mcp->DigitalPotSetWiperPosition(0, 0);
	this->vol_mcp->DigitalPotSetWiperPosition(1, 0);
	this->fader_mcp->DigitalPotSetWiperPosition(0, 0);
	this->fader_mcp->DigitalPotSetWiperPosition(1, 0);

	this->treble_mcp->DigitalPotSetWiperPosition(0, 0x100);
	this->treble_mcp->DigitalPotSetWiperPosition(1, 0x100);
	this->bass_mcp->DigitalPotSetWiperPosition(0, 0);
	this->bass_mcp->DigitalPotSetWiperPosition(1, 0);
}

//Handle an AIBus message.
bool VolumeHandler::handleAIBus(AIData *msg) {
	if(msg->receiver == ID_RADIO && msg->sender == ID_NAV_SCREEN) {
		if(msg->l >= 3 && msg->data[0] == 0x32 && msg->data[1] == 0x6) {
			uint16_t new_volume = volume;
			if(msg->data[2]&0x10 != 0)
				new_volume += msg->data[2]&0xF;
			else
				new_volume -= msg->data[2]&0xF;
			setVolume(new_volume);
			return true;
		}
	}

	return false;
}

//Set the volume range.
void VolumeHandler::setVolRange(const uint16_t vol_range) {
	this->vol_range = vol_range;
}

//Get the volume range.
uint16_t VolumeHandler::getVolRange() {
	return this->vol_range;
}

//Refresh the volumes.
void VolumeHandler::setVolume() {
	setVolume(volume);
}

//Set the volume.
void VolumeHandler::setVolume(const uint16_t volume) {
	this->volume = volume;
	if(this->volume > this->vol_range)
		this->volume = this->vol_range;

	if(!this->parameters->digital_amp) { //Analog mode.
		uint16_t lf_vol = volume*DEFAULT_RANGE/vol_range, rf_vol = volume*DEFAULT_RANGE/vol_range, lr_vol = volume*DEFAULT_RANGE/vol_range, rr_vol = volume*DEFAULT_RANGE/vol_range;
		if(balance < 0) { //Left balance.
			rf_vol = rf_vol*(DEFAULT_RANGE/2 - abs(balance))/(DEFAULT_RANGE/2);
			rr_vol = rr_vol*(DEFAULT_RANGE/2 - abs(balance))/(DEFAULT_RANGE/2);
		} else if(balance > 0) { //Right balance.
			lf_vol = lf_vol*(DEFAULT_RANGE/2 - abs(balance))/(DEFAULT_RANGE/2);
			lr_vol = lr_vol*(DEFAULT_RANGE/2 - abs(balance))/(DEFAULT_RANGE/2);
		}

		if(fader > 0) { //Front fader.
			lr_vol = lr_vol*(DEFAULT_RANGE/2 - abs(fader))/(DEFAULT_RANGE/2);
			rr_vol = rr_vol*(DEFAULT_RANGE/2 - abs(fader))/(DEFAULT_RANGE/2);
		} else if(fader < 0) { //Rear fader.
			lf_vol = lf_vol*(DEFAULT_RANGE/2 - abs(fader))/(DEFAULT_RANGE/2);
			rf_vol = rf_vol*(DEFAULT_RANGE/2 - abs(fader))/(DEFAULT_RANGE/2);
		}

		this->vol_mcp->DigitalPotSetWiperPosition(0, rf_vol);
		this->vol_mcp->DigitalPotSetWiperPosition(1, lf_vol);
		this->fader_mcp->DigitalPotSetWiperPosition(0, rr_vol);
		this->fader_mcp->DigitalPotSetWiperPosition(1, lr_vol);
	} else { //Digital mode.
		uint8_t volume_data[] = {0x32, 0x6, (this->volume&0xFF00)>>8, this->volume&0xFF};
		AIData volume_msg(sizeof(volume_data), ID_RADIO, ID_AMPLIFIER);
		volume_msg.refreshAIData(volume_data);

		ai_handler->writeAIData(&volume_msg, parameters->amp_connected);
	}
}

//Get the set volume.
uint16_t VolumeHandler::getVolume() {
	return this->volume;
}