#include "Phone_Window.h"

PhoneWindow::PhoneWindow(AttributeList *attribute_list) : NavWindow(attribute_list) {
	main_area_box[0] = new TextBox(this->renderer, TITLE_AREA_X + 64, TITLE_AREA_Y, this->w - (TITLE_AREA_X + 64), AREA_H, ALIGN_H_L, ALIGN_V_M, 50, &this->color_profile->text);
	for(uint8_t i=1;i<6;i+=1)
		main_area_box[i] = new TextBox(this->renderer, TITLE_AREA_X, MAIN_AREA_Y + MAIN_AREA_HEIGHT*i, FULL_AREA_W, AREA_H, ALIGN_H_L, ALIGN_V_M, 36, &this->color_profile->text);
	
	for(uint8_t i=0;i<2;i+=1)
		subtitle_area_box[i] = new TextBox(this->renderer, this->w - TITLE_AREA_X - SUB_AREA_WIDTH, TITLE_AREA_Y + AREA_H + SUB_AREA_HEIGHT*(1-i%2), SUB_AREA_WIDTH, SUB_AREA_HEIGHT, ALIGN_H_R, ALIGN_V_M, 26, &this->color_profile->text);
	subtitle_area_box[2] = new TextBox(this->renderer, TITLE_AREA_X + 64, TITLE_AREA_Y + AREA_H, HALF_AREA_W, SUB_AREA_HEIGHT, ALIGN_H_L, ALIGN_V_M, 29, &this->color_profile->text);

	//Clear all the "changed" bools.
	for(uint8_t i=0;i<6;i+=1)
		main_area_change[i] = false;
	
	for(uint8_t i=0;i<3;i+=1)
		subtitle_area_change[i] = false;

	this->side_menu = new NavMenu(this->attribute_list, this->w/2, MAIN_AREA_Y + MAIN_AREA_HEIGHT, this->w/2, MAIN_AREA_HEIGHT, 5, ALIGN_H_R, 36, 5, false, "");

	uint8_t phone_img_data[sizeof(image_data_Phone)];
	for(int i=0;i<sizeof(image_data_Phone);i+=1)
		phone_img_data[i] = image_data_Phone[i];

	SDL_Surface* phone_surface = SDL_CreateRGBSurfaceWithFormatFrom(phone_img_data, 64, 82, 1, (64+7)/8, SDL_PIXELFORMAT_INDEX1MSB);
	SDL_Color colors[] = {{0,0,0,0}, getSDLColor(attribute_list->color_profile->text)};
	SDL_SetPaletteColors(phone_surface->format->palette, colors, 0, 2);

	this->phone_texture = SDL_CreateTextureFromSurface(renderer, phone_surface);
	SDL_SetTextureBlendMode(this->phone_texture, SDL_BLENDMODE_BLEND);

	SDL_FreeSurface(phone_surface);
}

PhoneWindow::~PhoneWindow() {
	delete this->side_menu;
	delete this->settings_menu;

	for(uint8_t i=0;i<6;i+=1)
		delete this->main_area_box[i];
	
	for(uint8_t i=0;i<3;i+=1)
		delete this->subtitle_area_box[i];
}

void PhoneWindow::drawWindow() {
	if(!this->active)
		return;

	if(this->settings_menu == NULL || !this->settings_menu_active) {
		main_area_box[0]->drawText();
		if(!settings_menu_prep) {
			this->side_menu->drawMenu();

			for(uint8_t i=1;i<6;i+=1)
				main_area_box[i]->drawText();

			for(uint8_t i=0;i<3;i+=1)
				subtitle_area_box[i]->drawText();
		}

		SDL_Rect phone_rect = {TITLE_AREA_X, TITLE_AREA_Y, 64, 82};
		SDL_RenderCopy(this->renderer, this->phone_texture, NULL, &phone_rect);
	} else {
		if(this->settings_menu->getY() >= TITLE_AREA_Y + AREA_H + SUB_AREA_HEIGHT) {
			main_area_box[0]->drawText();
			for(uint8_t i=0;i<3;i+=1)
				subtitle_area_box[i]->drawText();
		}
		
		this->settings_menu->drawMenu();
	}
}

void PhoneWindow::refreshWindow() {
	for(uint8_t i=0;i<6;i+=1)
		main_area_box[i]->renderText();

	for(uint8_t i=0;i<3;i+=1)
		subtitle_area_box[i]->renderText();

	if(this->side_menu != NULL)
		this->side_menu->refreshItems();
	if(this->settings_menu != NULL)
		this->settings_menu->refreshItems();

	uint8_t phone_img_data[sizeof(image_data_Phone)];
	for(int i=0;i<sizeof(image_data_Phone);i+=1)
		phone_img_data[i] = image_data_Phone[i];

	SDL_Surface* phone_surface = SDL_CreateRGBSurfaceWithFormatFrom(phone_img_data, 64, 82, 1, (64+7)/8, SDL_PIXELFORMAT_INDEX1MSB);
	SDL_Color colors[] = {{0,0,0,0}, getSDLColor(attribute_list->color_profile->text)};
	SDL_SetPaletteColors(phone_surface->format->palette, colors, 0, 2);

	this->phone_texture = SDL_CreateTextureFromSurface(renderer, phone_surface);
	SDL_SetTextureBlendMode(this->phone_texture, SDL_BLENDMODE_BLEND);

	SDL_FreeSurface(phone_surface);
}

//Handle an AIBus message. Return true if the message is meant for the phone screen.
bool PhoneWindow::handleAIBus(AIData* msg) {
	AIBusHandler* aibus_handler = this->attribute_list->aibus_handler;

	if(msg->data[0] == 0x21 && msg->data[1] == 0xA5) { //Write data.
		aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
		this->interpretPhoneScreenChange(msg);
		return true;
	} else if(msg->sender == ID_NAV_SCREEN) { //Button pressed on screen.
		if(!this->active)
			return false;
		
		if(msg->data[0] == 0x32 && msg->data[1] == 0x7) {
			this->interpretMenuChange(msg);
			aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
			return true;
		} else if(msg->data[0] == 0x30) {
			const uint8_t button = msg->data[1], state = msg->data[2]>>6;
			if(button == 0x7 && state == 0x2) { //Enter button.
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				this->handleEnterButton();
				return true;
			} else if ((button == 0x27 || (button == 0x51 && this->settings_menu_active)) && state == 0x2) { //Menu or back button.
				if(this->settings_menu_active) {
					this->settings_menu_active = false;
					aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
					this->settings_menu_prep = false;
					sendMenuClose();
					return true;
				} else
					return false;
			} else if(button == 0x51 && state == 0x2 && !this->settings_menu_active) {
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				
				uint8_t menu_request_data[] = {0x2B, 0x4C};
				AIData menu_request_msg(sizeof(menu_request_data), ID_NAV_COMPUTER, ID_PHONE);
				menu_request_msg.refreshAIData(menu_request_data);

				aibus_handler->writeAIData(&menu_request_msg);
				return true;
			} else if(button == 0x20 && state == 0x2) { //Home button.
				this->active = false;
				this->attribute_list->next_window = NEXT_WINDOW_MAIN;
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				return true;
			} else if((button == 0x28 || button == 0x29 || button == 0x2A || button == 0x2B) && state == 0x2)  {
				this->interpretMenuChange(msg);
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				return true;
			} else
				return false;
		} else
			return false;
	} else
		return false;
}

void PhoneWindow::setText(const uint8_t group, const uint8_t area, std::string text) {
	if (text.compare("") == 0)
		text = " ";
	
	if(group == MAIN_AREA_GROUP && area < 6) {
		main_area_text[area] = text;
		main_area_change[area] = true;
	}
	else if(group == SUB_AREA_GROUP && area < 3) {
		subtitle_area_text[area] = text;
		subtitle_area_change[area] = true;
	}
	else if(group == BUTTON_AREA_GROUP && area < 5) {
		this->side_menu->setItem(text, area);
		return;
	}

	//this->fillText(group, area);
}

//Change the phone screen.
void PhoneWindow::interpretPhoneScreenChange(AIData* ai_b) {
	if(ai_b->l < 3)
		return;

	if(ai_b->data[0] != 0x21 && ai_b->data[1] != 0xA5)
		return;

	const uint8_t group = (ai_b->data[2]&0xF0)>>4;

	if(group == 0xF) {
		if(side_menu->getFilledTextItems() > 0) {
			for(uint8_t i=1;i<6;i+=1)
				main_area_box[i]->setWidth(HALF_AREA_W);
				
			if(side_menu->getSelected() == 0)
				side_menu->incrementSelected();
		} else {
			for(uint8_t i=1;i<6;i+=1)
				main_area_box[i]->setWidth(FULL_AREA_W);
			side_menu->setSelected(0);
		}
	
		refreshPhoneScreen();
		return;
	}

	if(ai_b->l < 3 || (group > 0x1 && group != 0xB))
		return;

	const uint8_t area = ai_b->data[2]&0xF;

	 //Fill a field.
	if(area >= 6)
		return;

	std::string text = "";
	for(uint8_t i=3;i<ai_b->l; i+=1)
		text += ai_b->data[i];

	setText(group, area, text);

	if(group == BUTTON_AREA_GROUP) {
		if(side_menu->getFilledTextItems() > 0) {
			for(uint8_t i=1;i<6;i+=1)
				main_area_box[i]->setWidth(HALF_AREA_W);
			if(side_menu->getSelected() == 0)
				side_menu->incrementSelected();
		} else {
			for(uint8_t i=1;i<6;i+=1)
				main_area_box[i]->setWidth(FULL_AREA_W);
			side_menu->setSelected(0);
		}
	}

	refreshPhoneScreen();
}

void PhoneWindow::refreshPhoneScreen() {
	for(uint8_t i=0;i<6;i+=1) {
		if(main_area_change[i]) {
			fillText(MAIN_AREA_GROUP, i);
			main_area_change[i] = false;
		}
	}
	for(uint8_t i=0;i<3;i+=1) {
		if(subtitle_area_change[i]) {
			fillText(SUB_AREA_GROUP, i);
			subtitle_area_change[i] = false;
		}
	}
}

void PhoneWindow::fillText(const uint8_t group, const uint8_t area) {
	if(group == MAIN_AREA_GROUP) {
		if(area >= 6)
			return;

		main_area_box[area]->setText(main_area_text[area]);
		
		if(this->active)
			main_area_box[area]->drawText();
	} else if(group == SUB_AREA_GROUP) {
		if(area >= 3)
			return;
		
		subtitle_area_box[area]->setText(subtitle_area_text[area]);
		if(this->active)
			subtitle_area_box[area]->drawText();
	}
}

void PhoneWindow::interpretMenuChange(AIData* ai_b) {
	if(ai_b->sender == ID_NAV_SCREEN) {
		if(ai_b->data[0] == 0x32) {
			const bool clockwise = (ai_b->data[2]&0x10) != 0;
			const uint8_t steps = ai_b->data[2]&0xF;

			if(this->settings_menu == NULL || !this->settings_menu_active) {
				if(this->side_menu != NULL) {
					if(clockwise) {
						for(uint8_t i=0;i<steps;i+=1)
							this->side_menu->incrementSelected();
					} else {
						for(uint8_t i=0;i<steps;i+=1)
							this->side_menu->decrementSelected();
					}
				}
			} else {
				if(!clockwise) {
					for(uint8_t i=0;i<steps;i+=1)
						this->settings_menu->incrementSelected();
				} else {
					for(uint8_t i=0;i<steps;i+=1)
						this->settings_menu->decrementSelected();
				}
			}
		} else if(ai_b->data[0] == 0x30) {
			if(this->settings_menu == NULL || !this->settings_menu_active) {
				if(this->side_menu != NULL) {
					if(ai_b->data[1] == 0x29)
						this->side_menu->incrementSelected();
					else if(ai_b->data[1] == 0x28)
						this->side_menu->decrementSelected();
				}
			} else {
				int8_t control = -1;
				switch(ai_b->data[1]) {
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
						return;
				}
				this->settings_menu->navigateSelected(control);
			}
		}
	}
}

void PhoneWindow::handleEnterButton() {
	AIBusHandler* aibus_handler = this->attribute_list->aibus_handler;

	aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ID_NAV_SCREEN);
	if(this->settings_menu_active && this->settings_menu != NULL) {
		uint8_t data[] = {0x2B, 0x60, uint8_t(this->settings_menu->getSelected())};
		AIData enter_msg(3, ID_NAV_COMPUTER, this->settings_menu_sender);
		enter_msg.refreshAIData(data);

		aibus_handler->writeAIData(&enter_msg);
	} else {
		uint8_t data[] = {0x2B, 0x6C, uint8_t(this->side_menu->getSelected())};
		AIData enter_msg(3, ID_NAV_COMPUTER, ID_PHONE);
		enter_msg.refreshAIData(data);

		aibus_handler->writeAIData(&enter_msg);
	}
}

void PhoneWindow::sendMenuClose() {
	this->sendMenuClose(this->settings_menu_sender);
}

void PhoneWindow::sendMenuClose(const uint8_t receiver) {
	AIData close_msg(2, ID_NAV_COMPUTER, receiver);
	uint8_t data[] = {0x2B, 0x40};
	close_msg.refreshAIData(data);

	this->attribute_list->aibus_handler->writeAIData(&close_msg);
}