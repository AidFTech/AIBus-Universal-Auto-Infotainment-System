#include "Settings_Window.h"

Settings_Window::Settings_Window(AttributeList *attribute_list, const uint16_t setting_count, std::string header, const next_window_t back_index) : NavWindow(attribute_list) {
	this->title_block = new TextBox(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y, this->w-MAIN_TITLE_AREA_X, TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 55, &this->color_profile->text);
	title_block->setText(header);

	this->settings_menu = new NavMenu(this->attribute_list, 0, MAIN_TITLE_AREA_Y + 55, attribute_list->w, 40, setting_count, ALIGN_H_L, 36, setting_count, false, "");
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

	SerialAIBusHandler* aibus_handler = this->attribute_list->aibus_handler;

	if(ai_d->l >= 2 && ai_d->data[0] == 0x2B) { //Menu message.
		if(!allow_ext_menu)
			return false;

		if(ai_d->data[1] == 0x50 && ai_d->l >= 12) { //New menu.
			const uint8_t rows = ai_d->data[2]&0x7F, ml = ai_d->data[3];
			const bool loop = (ai_d->data[2]&0x80) != 0;
			const int16_t x = (ai_d->data[4]<<8)|ai_d->data[5], y = (ai_d->data[6]<<8)|ai_d->data[7];
			const uint16_t w = (ai_d->data[8]<<8)|ai_d->data[9], h = (ai_d->data[10]<<8)|ai_d->data[11];

			std::string menu_title = "";
			for(int i=12;i<ai_d->l;i+=1)
				menu_title += char(ai_d->data[i]);

			if(settings_menu != NULL)
				delete this->settings_menu;

			this->settings_menu = new NavMenu(attribute_list, x, y, w, h, ml, -1, h*6/7, rows, loop, "");
			this->title_block->setText(menu_title);

			this->ext_menu_sender = ai_d->sender;

			aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
			return true;
		} else if(ai_d->data[1] == 0x51 && ai_d->l >= 3) { //New menu item.
			if(ai_d->sender != this->ext_menu_sender)
				return false;

			std::string menu_item = "";
			for(int i=3;i<ai_d->l;i+=1)
				menu_item += char(ai_d->data[i]);

			const uint16_t index = ai_d->data[2];
			this->settings_menu->setItem(menu_item, index);

			aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
			return true;
		} else if(ai_d->data[1] == 0x52 && ai_d->l >= 3) { //Select the item.
			if(ai_d->sender != this->ext_menu_sender)
				return false;

			const uint16_t index = ai_d->data[2];
			this->settings_menu->setSelected(index);
			this->settings_menu->setTextItems();

			aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
			return true;
		} else if(ai_d->data[1] == 0x53) { //Set the title.
			if(ai_d->sender != this->ext_menu_sender)
				return false;

			std::string menu_title = "";
			for(int i=2;i<ai_d->l;i+=1)
				menu_title += char(ai_d->data[i]);

			this->title_block->setText(menu_title);

			aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
			return true;
		} else if(ai_d->data[1] == 0x40) { //Clear the menu.
			attribute_list->next_window = NEXT_WINDOW_SETTINGS_MAIN;

			aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
			return true;
		}
		//TODO: Sliders.
		
		return false;
	} else if(ai_d->sender == ID_NAV_SCREEN) {
		if(this->settings_menu->handleAIBus(ai_d)) {
			aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
			return true;
		}

		if(ai_d->l >= 3 && ai_d->data[0] == 0x30) { //Button press.
			bool answered = false;
			const uint8_t control = ai_d->data[1], state = ai_d->data[2]>>6;
			if(control == 0x7 && state == 0x2) { //Enter button.
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
				this->handleEnterButton();
				answered = true;
			} else if((control == 0x27 || control == 0x51) && state == 0x2) { //Back button.
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
				this->handleBackButton();
				answered = true;
			}
			
			return answered;
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
	
	if(!allow_ext_menu) {
		if(settings_menu != NULL)
			delete this->settings_menu;

		this->settings_menu = new NavMenu(this->attribute_list, 0, MAIN_TITLE_AREA_Y + 55, attribute_list->w, 40, SETTING_COUNT, ALIGN_H_L, 36, SETTING_COUNT, false, "");
	}

	this->settings_menu->setSelected(0);
}

void Settings_Window::resizeMenu(const uint16_t new_count) {
	if(settings_menu != NULL)
		delete this->settings_menu;

	this->settings_menu = new NavMenu(this->attribute_list, 0, MAIN_TITLE_AREA_Y + 55, attribute_list->w, 40, new_count, ALIGN_H_L, 36, new_count, false, "");
}