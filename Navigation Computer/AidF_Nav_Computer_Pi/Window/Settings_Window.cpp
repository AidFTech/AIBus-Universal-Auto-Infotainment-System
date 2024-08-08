#include "Settings_Window.h"

Settings_Window::Settings_Window(AttributeList *attribute_list, const uint16_t setting_count, std::string header, const int back_index) : NavWindow(attribute_list) {
	this->title_block = new TextBox(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y, this->w-MAIN_TITLE_AREA_X, TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 55, &this->color_profile->text);
	title_block->setText(header);

	this->settings_menu = new NavMenu(this->attribute_list, 0, MAIN_TITLE_AREA_Y + 55, attribute_list->w, 40, SETTING_COUNT, ALIGN_H_L, 36, SETTING_COUNT, false, "");
	this->back_index = back_index;
}

Settings_Window::~Settings_Window() {
	delete this->settings_menu;
	delete this->title_block;
}

void Settings_Window::drawWindow() {
	this->title_block->drawText();
	this->settings_menu->drawMenu();
}

void Settings_Window::refreshWindow() {
	this->title_block->renderText();
	this->settings_menu->refreshItems();
}

bool Settings_Window::handleAIBus(AIData* ai_d) {
	if(!this->active)
		return false;

	AIBusHandler* aibus_handler = this->attribute_list->aibus_handler;

	if(ai_d->sender == ID_NAV_SCREEN) {
		if(ai_d->l >= 3 && ai_d->data[0] == 0x32) { //Knob turn.
			const bool clockwise = (ai_d->data[2]&0x10) != 0;
			const uint8_t steps = ai_d->data[2]&0xF;

			if(!clockwise) {
				for(uint8_t i=0;i<steps;i+=1)
					this->settings_menu->incrementSelected();
			} else {
				for(uint8_t i=0;i<steps;i+=1)
					this->settings_menu->decrementSelected();
			}

			aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
			return true;
		} else if(ai_d->l >= 3 && ai_d->data[0] == 0x30) { //Button press.
			if(ai_d->data[1] >= 0x28 && ai_d->data[1] <= 0x2B && (ai_d->data[2] >> 6) == 0x2) {
				int8_t control = -1;
				switch(ai_d->data[1]) {
					case 0x28:
						control = NAV_UP;
						break;
					case 0x29:
						control = NAV_DOWN;
						break;
					case 0x2A:
						control = NAV_LEFT;
						break;
					case 0x2B:
						control = NAV_RIGHT;
						break;
					default:
						return false;
				}

				this->settings_menu->navigateSelected(control);

				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
				return true;
			} else {
				bool answered = false;
				const uint8_t control = ai_d->data[1], state = ai_d->data[2]>>6;
				if(control == 0x7 && state == 0x2) { //Enter button.
					this->handleEnterButton();
					answered = true;
				} else if((control == 0x27 || control == 0x51) && state == 0x2) { //Back button.
					this->handleBackButton();
					answered = true;
				}
				
				if(answered)
					aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
				
				return answered;
			}
		}
	}

	return false;
}

void Settings_Window::handleEnterButton() {
	
}

void Settings_Window::handleBackButton() {
	this->attribute_list->next_window = back_index;
}

void Settings_Window::clearMenu() {
	for(int i=0;i<this->settings_menu->getLength();i+=1)
		this->settings_menu->setItem("", i);
	
	this->settings_menu->setSelected(0);
}
