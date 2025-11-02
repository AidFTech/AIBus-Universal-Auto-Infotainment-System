#include "Volume_Handler.h"

VolumeHandler::VolumeHandler(MCP4251* vol_mcp, MCP4251* treble_mcp, MCP4251* bass_mcp, MCP4251* fader_mcp, ParameterList* parameters, AIBusHandler* ai_handler) {
	this->ai_handler = ai_handler;
	this->parameters = parameters;

	this->vol_mcp = vol_mcp;
	this->treble_mcp = treble_mcp;
	this->bass_mcp = bass_mcp;
	this->fader_mcp = fader_mcp;
}

//Initialize the volume manager.
void VolumeHandler::init() {
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
	
	volume_changed = true;
}

//Handle an AIBus message.
bool VolumeHandler::handleAIBus(AIData *msg) {
	if(msg->receiver == ID_RADIO && msg->sender == ID_NAV_SCREEN) { //Volume knob.
		if(msg->l >= 3 && msg->data[0] == 0x32) {
			if(msg->data[1] == 0x6) {
				ai_handler->sendAcknowledgement(ID_RADIO, msg->sender);

				if(!parameters->audio_on)
					return true;
				
				uint16_t new_volume = volume;
				
				const uint8_t inc = msg->data[2]&0xF;
				if((msg->data[2]&0x10) != 0) {
					if(new_volume + inc <= vol_range)
						new_volume += inc;
					else
						new_volume = vol_range;
				} else {
					if(inc <= volume)
						new_volume -= inc;
					else
						new_volume = 0;
				}
				
				setVolume(new_volume);
				return true;
			}
		}
	} else if(msg->receiver == ID_RADIO) {
		if(msg->l >= 4 && msg->data[0] == 0x33 && msg->data[1] == 0x6) { //Volume range.
			ai_handler->sendAcknowledgement(ID_RADIO, msg->sender);

			const uint16_t new_max = (msg->data[2]<<8)|msg->data[3], old_max = this->vol_range;
			this->vol_range = new_max;
			
			this->volume = this->volume*new_max/old_max;
			this->setVolume();
			
			return true;
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
	if(this->volume != volume)
		volume_changed = true;

	this->volume = volume;
	if(this->volume > this->vol_range)
		this->volume = this->vol_range;

	if(!this->parameters->digital_amp) { //Analog mode.
		uint16_t lf_vol = this->volume*DEFAULT_TONE_RANGE/vol_range, rf_vol = this->volume*DEFAULT_TONE_RANGE/vol_range, lr_vol = volume*DEFAULT_TONE_RANGE/vol_range, rr_vol = volume*DEFAULT_TONE_RANGE/vol_range;
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
		uint8_t volume_data[] = {0x32, 0x6, uint8_t((this->volume&0xFF00)>>8), uint8_t(this->volume&0xFF)};
		AIData volume_msg(sizeof(volume_data), ID_RADIO, ID_AMPLIFIER, volume_data);

		ai_handler->writeAIData(&volume_msg, parameters->amp_connected);
	}

	if(volume_changed) 
		setVolumeDisplay();
}

//Set the bass.
void VolumeHandler::setBass(int bass) {
	if(bass > DEFAULT_TONE_RANGE)
		bass = DEFAULT_TONE_RANGE;
	if(bass < 0)
		bass = 0;

	this->bass = DEFAULT_TONE_RANGE - bass;

	uint16_t desired_bass = this->bass;
	
	const float desired_attenuation = this->bass*MAX_ATTENUATION/DEFAULT_TONE_RANGE;
	
	for(int i=0;i<=DEFAULT_TONE_RANGE;i+=1) {
		const float br = (DEFAULT_TONE_RANGE - i)*10000.0/DEFAULT_TONE_RANGE;
		const float ratio = (br + BASS_R)/(br + BASS_R + BASS_X);

		if(20*log10(1.0/ratio) > desired_attenuation) {
			desired_bass = i;
			break;
		}
	}

	this->bass_mcp->DigitalPotSetWiperPosition(0, desired_bass);
	this->bass_mcp->DigitalPotSetWiperPosition(1, desired_bass);
}

//Set the treble.
void VolumeHandler::setTreble(int treble) {
	if(treble > DEFAULT_TONE_RANGE)
		treble = DEFAULT_TONE_RANGE;
	if(treble < 0)
		treble = 0;
	
	this->treble = treble;
	
	uint16_t desired_treble = this->treble;
	
	const float desired_attenuation = (DEFAULT_TONE_RANGE-this->treble)*MAX_ATTENUATION/DEFAULT_TONE_RANGE;

	for(int i=0;i<=DEFAULT_TONE_RANGE;i+=1) {
		const float tr = i*50000.0/DEFAULT_TONE_RANGE;
		const float ratio = TREBLE_X/(tr + TREBLE_R + TREBLE_X);

		if(20*log10(1.0/ratio) > desired_attenuation) {
			desired_treble = DEFAULT_TONE_RANGE - i;
			break;
		}
	}

	this->treble_mcp->DigitalPotSetWiperPosition(0, desired_treble);
	this->treble_mcp->DigitalPotSetWiperPosition(1, desired_treble);
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
	return this->bass <= DEFAULT_TONE_RANGE ? DEFAULT_TONE_RANGE - this->bass : 0;
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

//Return whether the volume changed.
bool VolumeHandler::getVolumeChanged() {
	const bool return_changed = this->volume_changed;
	this->volume_changed = false;

	return return_changed;
}

//Display the volume.
void VolumeHandler::setVolumeDisplay() {
	if(!parameters->power_on)
		return;

	uint8_t max_vol = 255;
	if(this->vol_range < 255)
		max_vol = this->vol_range&0xFF;

	uint8_t set_vol = 255;
	if(this->volume < 255)
		set_vol = this->volume&0xFF;

	uint8_t vol_data[] = {0x26, set_vol, max_vol};
	AIData vol_msg(sizeof(vol_data), ID_RADIO, ID_NAV_COMPUTER, vol_data);

	ai_handler->writeAIData(&vol_msg, parameters->computer_connected);

	vol_msg.receiver = ID_ANDROID_AUTO;
	ai_handler->writeAIData(&vol_msg, parameters->mirror_connected);

	vol_data[0] = 0x62;
	vol_msg.data[0] = 0x62;
	vol_msg.receiver = ID_IMID_SCR;

	ai_handler->writeAIData(&vol_msg, parameters->imid_connected);
}
