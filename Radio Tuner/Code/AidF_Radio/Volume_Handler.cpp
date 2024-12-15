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

	this->treble_mcp->DigitalPotSetWiperPosition(0, treble);
	this->treble_mcp->DigitalPotSetWiperPosition(1, treble);
	this->bass_mcp->DigitalPotSetWiperPosition(0, bass);
	this->bass_mcp->DigitalPotSetWiperPosition(1, bass);
}

//Handle an AIBus message.
bool VolumeHandler::handleAIBus(AIData *msg) {
	if(msg->receiver == ID_RADIO && msg->sender == ID_NAV_SCREEN) {
		if(msg->l >= 3 && msg->data[0] == 0x32) {
			if(msg->data[1] == 0x6) {
				ai_handler->sendAcknowledgement(ID_RADIO, msg->sender);

				uint16_t new_volume = volume;
				if((msg->data[2]&0x10) != 0)
					new_volume += msg->data[2]&0xF;
				else
					new_volume -= msg->data[2]&0xF;
				setVolume(new_volume);
				return true;
			}
		}
	}

	return false;
}

//Set a tone parameter via AIBus.
void VolumeHandler::setAIBusParameter(AIData *msg) {
	if(parameters->bass_adjust
		|| parameters->treble_adjust
		|| parameters->balance_adjust
		|| parameters->fader_adjust) {

		if(msg->l >= 3 && msg->data[0] == 0x30) {
			ai_handler->sendAcknowledgement(ID_RADIO, msg->sender);

			const uint8_t button = msg->data[1], state = msg->data[2] >> 6;
			if(button == 0x2A && state == 2) {
				const int increment = -DEFAULT_TONE_RANGE/DEFAULT_SLIDER_RANGE;

				if(parameters->bass_adjust)
					setBass(getBass() + increment);
				else if(parameters->treble_adjust)
					setTreble(treble + increment);
				else if(parameters->balance_adjust)
					setBalance(balance + increment);
				else if(parameters->fader_adjust)
					setFader(fader + increment);

				return;
			} else if(button == 0x2B && state == 2) {
				const int increment = DEFAULT_TONE_RANGE/DEFAULT_SLIDER_RANGE;

				if(parameters->bass_adjust)
					setBass(getBass() + increment);
				else if(parameters->treble_adjust)
					setTreble(treble + increment);
				else if(parameters->balance_adjust)
					setBalance(balance + increment);
				else if(parameters->fader_adjust)
					setFader(fader + increment);

				return;
			}
		} else if(msg->l >= 3 && msg->data[0] == 0x32 && msg->data[1] == 0x7) {
			const bool clockwise = (msg->data[2]&0x10) != 0;
			const uint8_t steps = msg->data[2]&0xF;

			int increment = steps*DEFAULT_TONE_RANGE/DEFAULT_SLIDER_RANGE;
			if(!clockwise)
				increment = -increment;

			if(parameters->bass_adjust)
				setBass(getBass() + increment);
			else if(parameters->treble_adjust)
				setTreble(treble + increment);
			else if(parameters->balance_adjust)
				setBalance(balance + increment);
			else if(parameters->fader_adjust)
				setFader(fader + increment);
		}
	}
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
		uint16_t lf_vol = volume*DEFAULT_TONE_RANGE/vol_range, rf_vol = volume*DEFAULT_TONE_RANGE/vol_range, lr_vol = volume*DEFAULT_TONE_RANGE/vol_range, rr_vol = volume*DEFAULT_TONE_RANGE/vol_range;
		if(balance < 0) { //Left balance.
			rf_vol = rf_vol*(DEFAULT_TONE_RANGE/2 - abs(balance))/(DEFAULT_TONE_RANGE/2);
			rr_vol = rr_vol*(DEFAULT_TONE_RANGE/2 - abs(balance))/(DEFAULT_TONE_RANGE/2);
		} else if(balance > 0) { //Right balance.
			lf_vol = lf_vol*(DEFAULT_TONE_RANGE/2 - abs(balance))/(DEFAULT_TONE_RANGE/2);
			lr_vol = lr_vol*(DEFAULT_TONE_RANGE/2 - abs(balance))/(DEFAULT_TONE_RANGE/2);
		}

		if(fader > 0) { //Front fader.
			lr_vol = lr_vol*(DEFAULT_TONE_RANGE/2 - abs(fader))/(DEFAULT_TONE_RANGE/2);
			rr_vol = rr_vol*(DEFAULT_TONE_RANGE/2 - abs(fader))/(DEFAULT_TONE_RANGE/2);
		} else if(fader < 0) { //Rear fader.
			lf_vol = lf_vol*(DEFAULT_TONE_RANGE/2 - abs(fader))/(DEFAULT_TONE_RANGE/2);
			rf_vol = rf_vol*(DEFAULT_TONE_RANGE/2 - abs(fader))/(DEFAULT_TONE_RANGE/2);
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

//Set the bass.
void VolumeHandler::setBass(const uint16_t bass) {
	this->bass = DEFAULT_TONE_RANGE - bass;

	if(this->bass > DEFAULT_TONE_RANGE)
		this->bass = DEFAULT_TONE_RANGE;

	this->bass_mcp->DigitalPotSetWiperPosition(0, bass);
	this->bass_mcp->DigitalPotSetWiperPosition(1, bass);
}

//Set the treble.
void VolumeHandler::setTreble(const uint16_t treble) {
	this->treble = treble;

	if(this->treble > DEFAULT_TONE_RANGE)
		this->treble = DEFAULT_TONE_RANGE;

	this->treble_mcp->DigitalPotSetWiperPosition(0, treble);
	this->treble_mcp->DigitalPotSetWiperPosition(1, treble);
}

//Set the balance.
void VolumeHandler::setBalance(const int16_t balance) {
	this->balance = balance;

	if(this->balance > DEFAULT_TONE_RANGE/2)
		this->balance = DEFAULT_TONE_RANGE/2;
	else if(this->balance < -DEFAULT_TONE_RANGE/2)
		this->balance = -DEFAULT_TONE_RANGE/2;

	setVolume(this->volume);
}

//Set the fader.
void VolumeHandler::setFader(const int16_t fader) {
	this->fader = fader;

	if(this->fader > DEFAULT_TONE_RANGE/2)
		this->fader = DEFAULT_TONE_RANGE/2;
	else if(this->fader < -DEFAULT_TONE_RANGE/2)
		this->fader = -DEFAULT_TONE_RANGE/2;

	setVolume(this->volume);
}

//Get the set volume.
uint16_t VolumeHandler::getVolume() {
	return this->volume;
}

//Get the set bass.
uint16_t VolumeHandler::getBass() {
	return DEFAULT_TONE_RANGE - this->bass;
}

//Get the set treble.
uint16_t VolumeHandler::getTreble() {
	return this->treble;
}

//Get the set balance.
int16_t VolumeHandler::getBalance() {
	return this->balance;
}

//Get the set fader.
int16_t VolumeHandler::getFader() {
	return this->fader;
}
