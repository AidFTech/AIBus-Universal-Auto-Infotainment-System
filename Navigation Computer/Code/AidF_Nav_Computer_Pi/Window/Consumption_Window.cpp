#include "Consumption_Window.h"

Consumption_Window::Consumption_Window(AttributeList *attribute_list) : NavWindow(attribute_list),
	title_box(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y, this->w-MAIN_TITLE_AREA_X, TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 40, &this->color_profile->text) {
	this->aibus_handler = attribute_list->aibus_handler;
	this->requestConsumptionInfo();

	title_box.setText(getString(LOCALE_STRING_CONSUMPTION, attribute_list->locale));

	for(uint8_t i=0;i<TRIP_INFO_COUNT;i+=1) {
		split_info_box_left[i] = new TextBox(renderer, MAIN_TITLE_AREA_X, CONSUMPTION_MAIN_AREA_Y+i*CONSUMPTION_MAIN_AREA_HEIGHT, this->w/2-MAIN_TITLE_AREA_X, CONSUMPTION_MAIN_AREA_HEIGHT, ALIGN_H_L, ALIGN_V_M, 32, &this->color_profile->text);
		split_info_box_right[i] = new TextBox(renderer, this->w/2, CONSUMPTION_MAIN_AREA_Y+i*CONSUMPTION_MAIN_AREA_HEIGHT, this->w/2-MAIN_TITLE_AREA_X, CONSUMPTION_MAIN_AREA_HEIGHT, ALIGN_H_R, ALIGN_V_M, 32, &this->color_profile->text);
	}
}

Consumption_Window::~Consumption_Window() {
	for(uint8_t i=0;i<TRIP_INFO_COUNT;i+=1) {
		delete split_info_box_left[i];
		delete split_info_box_right[i];
	}

	if(settings_menu != NULL)
		delete settings_menu;
}

void Consumption_Window::requestConsumptionInfo() {
	AIData request_message(2, ID_NAV_COMPUTER, ID_CANSLATOR);

	request_message.data[0] = 0x45;
	request_message.data[1] = MSG46_ECON;

	aibus_handler->writeAIData(&request_message, attribute_list->canslator_connected);
}

void Consumption_Window::drawWindow() {
	title_box.drawText();

	if(this->settings_menu == NULL) {
		for(uint8_t i=0;i<TRIP_INFO_COUNT;i+=1) {
			split_info_box_left[i]->drawText();
			split_info_box_right[i]->drawText();
		}
	} else {
		this->settings_menu->drawMenu();
	}
}

void Consumption_Window::refreshWindow() {
	this->title_box.renderText();

	for(uint8_t i=0;i<TRIP_INFO_COUNT;i+=1) {
		split_info_box_left[i]->renderText();
		split_info_box_right[i]->renderText();
	}
	this->settings_menu->refreshItems();
}

void Consumption_Window::exitWindow() {
	if(this->settings_menu != NULL) {
		uint8_t back_data[] = {0x2B, 0x40};
		AIData back_msg(sizeof(back_data), ID_NAV_COMPUTER, ID_CANSLATOR);
		back_msg.refreshAIData(back_data);
		
		attribute_list->aibus_handler->writeAIData(&back_msg);
	}
}

bool Consumption_Window::handleAIBus(AIData* ai_d) {
	if(ai_d->l >= 3 && ai_d->data[0] == 0x46 && ai_d->sender == ID_CANSLATOR) { //Trip information message.
		if(ai_d->data[1] == MSG46_ECON) {
			const uint8_t row = ai_d->data[2]&0xF;
			const bool right_column = (ai_d->data[2]&0x80) != 0;
			
			if(row >= TRIP_INFO_COUNT)
				return true;

			std::string entry_name = "";
			for(uint8_t i=0;i<ai_d->l-3;i+=1)
				entry_name += ai_d->data[i+3];

			if(right_column)
				split_info_box_right[row]->setText(entry_name);
			else
				split_info_box_left[row]->setText(entry_name);
		}
		return true;
	} else if(ai_d->l >= 2 && ai_d->data[0] == 0x2B && ai_d->sender == ID_CANSLATOR) { //Menu message.
		if(!active)
			return false;

		if(ai_d->l >= 12 && ai_d->data[1] == 0x50) { //New settings menu.
			const uint8_t rows = ai_d->data[2]&0x7F, ml = ai_d->data[3];
			const bool loop = (ai_d->data[2]&0x80) != 0;
			const int16_t x = (ai_d->data[4]<<8)|ai_d->data[5], y = (ai_d->data[6]<<8)|ai_d->data[7];
			const uint16_t w = (ai_d->data[8]<<8)|ai_d->data[9], h = (ai_d->data[10]<<8)|ai_d->data[11];

			std::string menu_title = "";
			for(int i=12;i<ai_d->l;i+=1)
				menu_title += char(ai_d->data[i]);

			if(settings_menu != NULL)
				delete this->settings_menu;

			this->settings_menu = new NavMenu(attribute_list, x, y, w, h, ml, -1, h*6/7, rows, loop, menu_title);
			return true;
		} else if(ai_d->data[1] == 0x51 && ai_d->l >= 3) { //New menu item.
			if(settings_menu == NULL)
				return false;

			std::string menu_item = "";
			for(int i=3;i<ai_d->l;i+=1)
				menu_item += char(ai_d->data[i]);

			const uint16_t index = ai_d->data[2];
			this->settings_menu->setItem(menu_item, index);
			return true;
		} else if(ai_d->data[1] == 0x52 && ai_d->l >= 3) { //Select the item.
			if(settings_menu == NULL)
				return false;

			const uint16_t index = ai_d->data[2];
			this->settings_menu->setSelected(index);
			this->settings_menu->setTextItems();
			return true;
		} else if(ai_d->data[1] == 0x53) { //Set the title.
			if(settings_menu == NULL)
				return false;

			std::string menu_title = "";
			for(int i=2;i<ai_d->l;i+=1)
				menu_title += char(ai_d->data[i]);

			this->settings_menu->setTitle(menu_title);
			return true;
		} else if(ai_d->data[1] == 0x40) { //Clear the menu.
			if(settings_menu == NULL)
				return false;

			this->settings_menu = NULL;
			return true;
		}
	} else if(ai_d->sender == ID_NAV_SCREEN) {
		if(!active)
			return false;

		if(this->settings_menu != NULL) {
			if(this->settings_menu->handleAIBus(ai_d)) {
				return true;
			}
		}

		if(ai_d->l >= 3 && ai_d->data[0] == 0x30) { //Button press.
			bool answered = false;
			const uint8_t control = ai_d->data[1], state = ai_d->data[2]>>6;
			if(control == 0x7 && state == 0x2) { //Enter button.
				this->handleEnterButton();
				answered = true;
			} else if(control == 0x27 && state == 0x2) { //Back button.
				this->handleBackButton();
				answered = true;
			} else if(control == 0x51 && state == 0x2) { //Menu/setup button.
				handleSettingsButton();
				answered = true;
			}
			
			return answered;
		}
	}

	return false;
}

void Consumption_Window::handleEnterButton() {
	if(this->settings_menu != NULL) {
		const int selected = this->settings_menu->getSelected() - 1;

		if(selected < 0)
			return;

		uint8_t enter_data[] = {0x2B, 0x60, uint8_t((selected+1)&0xFF)};
		AIData enter_msg(sizeof(enter_data), ID_NAV_COMPUTER, ID_CANSLATOR);
		enter_msg.refreshAIData(enter_data);
		
		attribute_list->aibus_handler->writeAIData(&enter_msg);
	} else {
		handleSettingsButton();
	}
}

void Consumption_Window::handleBackButton() {
	if(this->settings_menu != NULL) {
		uint8_t back_data[] = {0x2B, 0x40};
		AIData back_msg(sizeof(back_data), ID_NAV_COMPUTER, ID_CANSLATOR);
		back_msg.refreshAIData(back_data);
		
		attribute_list->aibus_handler->writeAIData(&back_msg);
	}
}

void Consumption_Window::handleSettingsButton() {
	if(settings_menu == NULL) {
		const char request_str[] = "CONSINFO";
		AIData request_msg(sizeof(request_str) + 1, ID_NAV_COMPUTER, ID_CANSLATOR);
		request_msg.data[0] = 0x2B;
		request_msg.data[1] = 0x55;

		for(int i=0;i<sizeof(request_str) - 1;i+=1)
			request_msg.data[i+2] = uint8_t(request_str[i]);

		aibus_handler->writeAIData(&request_msg, attribute_list->canslator_connected);
	} else
		handleBackButton();
}