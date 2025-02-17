#include "Audio_Window.h"

//Audio window constructor.
Audio_Window::Audio_Window(AttributeList *attribute_list) : NavWindow(attribute_list) {
	main_area_box[0] = new TextBox(this->renderer, TITLE_AREA_X, TITLE_AREA_Y, AREA_W, AREA_H, ALIGN_H_L, ALIGN_V_M, 65, &this->color_profile->text);
	for(uint8_t i=1;i<6;i+=1)
		main_area_box[i] = new TextBox(this->renderer, TITLE_AREA_X, MAIN_AREA_Y + MAIN_AREA_HEIGHT*i, FULL_AREA_W, AREA_H, ALIGN_H_L, ALIGN_V_M, 36, &this->color_profile->text);
	
	for(uint8_t i=0;i<2;i+=1)
		subtitle_area_box[i] = new TextBox(this->renderer, this->w - TITLE_AREA_X - SUB_AREA_WIDTH, SUB_AREA_Y + SUB_AREA_HEIGHT*(1-i%2), SUB_AREA_WIDTH, SUB_AREA_HEIGHT, ALIGN_H_R, ALIGN_V_M, 26, &this->color_profile->text);
	subtitle_area_box[2] = new TextBox(this->renderer, TITLE_AREA_X, TITLE_AREA_Y + AREA_H, HALF_AREA_W, SUB_AREA_HEIGHT, ALIGN_H_L, ALIGN_V_M, 29, &this->color_profile->text);

	for(uint8_t i=0;i<6;i+=1)
		function_area_box[i] = new TextBox(this->renderer, FUNCTION_AREA_WIDTH*i, this->h - FUNCTION_AREA_HEIGHT, FUNCTION_AREA_WIDTH, FUNCTION_AREA_HEIGHT, ALIGN_H_C, ALIGN_V_M, 26, &this->color_profile->text);
	
	this->audio_menu = new NavMenu(this->attribute_list, this->w/2, MAIN_AREA_Y + MAIN_AREA_HEIGHT, this->w/2, MAIN_AREA_HEIGHT, 5, ALIGN_H_R, 36, 5, false, "");
	
	//Clear all the "changed" bools.
	for(uint8_t i=0;i<6;i+=1)
		main_area_change[i] = false;
	
	for(uint8_t i=0;i<3;i+=1)
		subtitle_area_change[i] = false;
		
	for(uint8_t i=0;i<6;i+=1)
		function_area_change[i] = false;
}

//Audio window destructor.
Audio_Window::~Audio_Window() {
	delete this->audio_menu;
	delete this->settings_menu;

	for(uint8_t i=0;i<6;i+=1)
		delete this->main_area_box[i];
	
	for(uint8_t i=0;i<3;i+=1)
		delete this->subtitle_area_box[i];
	
	for(uint8_t i=0;i<6;i+=1)
		delete this->function_area_box[i];
}

//Handle an AIBus message. Return true if the message is meant for the audio screen.
bool Audio_Window::handleAIBus(AIData* msg) {
	if(msg->receiver != ID_NAV_COMPUTER)
		return false;
	
	AIBusHandler* aibus_handler = this->attribute_list->aibus_handler;

	if(msg->l >= 2 && (msg->data[0] == 0x20 || msg->data[0] == 0x23) && (msg->data[1]&0x60) == 0x60) {
		aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
		this->interpretAudioScreenChange(msg);
		return true;
	} else if (msg->data[0] == 0x2B) { //Menu-related commands.
		bool ack = true;

		if(msg->data[1] == 0x5A) { //Initialize a new audio menu.
			if(this->settings_menu_active) {
				uint8_t data[] = {0x2B, 0x40};
				AIData no_menu_msg(2, ID_NAV_COMPUTER, msg->sender);
				no_menu_msg.refreshAIData(data);

				aibus_handler->writeAIData(&no_menu_msg);
			} else {
				this->initializeSettingsMenu(msg);
			}
		} else if(msg->data[1] == 0x51 && this->settings_menu != NULL) { //Add a menu entry.
			if(msg->sender != this->settings_menu_sender)
				return false;

			const uint8_t entry = msg->data[2];
			std::string entry_name = "";
			
			for(uint8_t i=0;i<msg->l-3;i+=1)
				entry_name += msg->data[i+3];
			
			this->settings_menu->setItem(entry_name, entry);
		} else if(msg->data[1] == 0x52 && this->settings_menu != NULL) { //Display the menu.
			if(msg->sender != this->settings_menu_sender)
				return false;
				
			if(!active) {
				ack = false;
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				
				attribute_list->next_window = NEXT_WINDOW_AUDIO;

				if(attribute_list->phone_active && attribute_list->phone_type != 0 && attribute_list->mirror_connected) {
					uint8_t mirror_off_data[] = {0x48, 0x8E, 0x0};
					AIData mirror_off_msg(sizeof(mirror_off_data), ID_NAV_COMPUTER, ID_ANDROID_AUTO);

					mirror_off_msg.refreshAIData(mirror_off_data);
					aibus_handler->writeAIData(&mirror_off_msg, attribute_list->mirror_connected);

					attribute_list->phone_active = false;
				}
			}

			const uint8_t selection = msg->data[2];
			this->settings_menu->setSelected(selection);
			this->settings_menu->setTextItems();
			this->settings_menu_active = true;
			this->settings_menu_prep = false;
		} else if(msg->data[1] == 0x53 && this->settings_menu != NULL) { //Change the menu title.
			if(msg->sender != this->settings_menu_sender)
				return false;

			std::string title = "";
			for(uint8_t i=2;i<msg->l;i+=1)
				title += msg->data[i];

			this->settings_menu->setTitle(title);
		} else if(msg->data[1] == 0x54 && this->settings_menu != NULL && msg->l >= 6) { //Create a menu slider.
			const uint8_t index = msg->data[2];
			const uint8_t max = msg->data[4], value = msg->data[3];
			const bool sel = msg->data[5] != 0;

			std::string value_text = "";
			for(uint8_t i=6;i<msg->l;i+=1)
				value_text += msg->data[i];

			if(this->settings_menu->getSlider(index) != NULL) {
				NavSlider* slider = this->settings_menu->getSlider(index);
				if(max != slider->getMax()) {
					this->settings_menu->setSlider(value_text, max, value, index);
					this->settings_menu->getSlider(index)->setSelected(sel);
				} else {
					slider->setValue(value);
					slider->setSelected(sel);
					
					TextBox* slider_text = this->settings_menu->getSliderTextBox(index);
					if(slider_text != NULL)
						slider_text->setText(value_text);
				}
			} else {
				this->settings_menu->setSlider(value_text, max, value, index);
				this->settings_menu->getSlider(index)->setSelected(sel);
			}

		} else if(msg->data[1] == 0x4A && this->settings_menu != NULL) { //Clear the menu.
			this->settings_menu_active = false;
			this->settings_menu_prep = false;

			ack = false;
			aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
			sendMenuClose(msg->sender);
		}

		if(ack)
			aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);

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
				
				uint8_t menu_request_data[] = {0x2B, 0x4A};
				AIData menu_request_msg(sizeof(menu_request_data), ID_NAV_COMPUTER, ID_RADIO);
				menu_request_msg.refreshAIData(menu_request_data);

				aibus_handler->writeAIData(&menu_request_msg);
				return true;
			} else if(button == 0x20 && state == 0x2) { //Home button.
				this->active = false;
				this->attribute_list->next_window = NEXT_WINDOW_MAIN;
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				return true;
			} else if(button == 0x26 && state == 0x2 && this->active) { //Audio button.
				this->active = false;
				this->attribute_list->next_window = NEXT_WINDOW_LAST;
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

void Audio_Window::drawWindow() {
	if(!this->active)
		return;

	if(this->settings_menu == NULL || !this->settings_menu_active) {
		main_area_box[0]->drawText();
		if(!settings_menu_prep) {
			this->audio_menu->drawMenu();

			for(uint8_t i=1;i<6;i+=1)
				main_area_box[i]->drawText();

			for(uint8_t i=0;i<3;i+=1)
				subtitle_area_box[i]->drawText();
			
			for(uint8_t i=0;i<6;i+=1)
				function_area_box[i]->drawText();
		}
	} else {
		if(this->settings_menu->getY() >= TITLE_AREA_Y + AREA_H + SUB_AREA_HEIGHT) {
			main_area_box[0]->drawText();
			for(uint8_t i=0;i<3;i+=1)
				subtitle_area_box[i]->drawText();
		}
		
		this->settings_menu->drawMenu();
	}
}

void Audio_Window::refreshWindow() {
	for(uint8_t i=0;i<6;i+=1)
		main_area_box[i]->renderText();

	for(uint8_t i=0;i<3;i+=1)
		subtitle_area_box[i]->renderText();
	
	for(uint8_t i=0;i<6;i+=1)
		function_area_box[i]->renderText();

	if(this->audio_menu != NULL)
		this->audio_menu->refreshItems();
	if(this->settings_menu != NULL)
		this->settings_menu->refreshItems();
}

//Exit window function.
void Audio_Window::exitWindow() {
	if(this->settings_menu_active) {
		this->settings_menu_active = false;
		this->settings_menu_prep = false;
		sendMenuClose();
	}
}

//Change the audio screen.
void Audio_Window::interpretAudioScreenChange(AIData* ai_b) {
	if(ai_b->l < 2)
		return;

	const uint8_t group = ai_b->data[1]&0xF;
	const bool refresh = (ai_b->data[1]&0x10) != 0;

	if(ai_b->data[0] == 0x20 && group == 0xF) { //Clear all fields.
		for(uint8_t i=0;i<6;i+=1)
			setText(MAIN_AREA_GROUP, i, "");
		for(uint8_t i=0;i<3;i+=1)
			setText(SUB_AREA_GROUP, i, "");
		for(uint8_t i=0;i<6;i+=1)
			setText(FUNCTION_AREA_GROUP, i, "");
		for(uint8_t i=0;i<5;i+=1)
			setText(BUTTON_AREA_GROUP, i, "");

		if(refresh)
			refreshAudioScreen();
		
		if(audio_menu->getFilledTextItems() > 0) {
			for(uint8_t i=1;i<6;i+=1)
				main_area_box[i]->setWidth(HALF_AREA_W);
		} else {
			for(uint8_t i=1;i<6;i+=1)
				main_area_box[i]->setWidth(FULL_AREA_W);
		}
		
		return;
	} else if(ai_b->data[0] == 0x23 && group == 0xF) {
		if(audio_menu->getFilledTextItems() > 0) {
			for(uint8_t i=1;i<6;i+=1)
				main_area_box[i]->setWidth(HALF_AREA_W);
				
			if(audio_menu->getSelected() == 0)
				audio_menu->incrementSelected();
		} else {
			for(uint8_t i=1;i<6;i+=1)
				main_area_box[i]->setWidth(FULL_AREA_W);
			audio_menu->setSelected(0);
		}
	
		refreshAudioScreen();
		return;
	}

	if(ai_b->l < 3 || (group > 0x2 && group != 0xB))
		return;

	const uint8_t area = ai_b->data[2];

	if(ai_b->data[0] == 0x20) { //Clear a field.
		if(area == 0xFF) {
			for(uint8_t i=0;i<6;i+=1)
				setText(group, i, "");
		} else
			setText(group, area, "");
	} else if(ai_b->data[0] == 0x23) { //Fill a field.
		if(area >= 6)
			return;

		std::string text = "";
		for(uint8_t i=3;i<ai_b->l; i+=1)
			text += ai_b->data[i];

		setText(group, area, text);
	}

	if(group == BUTTON_AREA_GROUP) {
		if(audio_menu->getFilledTextItems() > 0) {
			for(uint8_t i=1;i<6;i+=1)
				main_area_box[i]->setWidth(HALF_AREA_W);
			if(audio_menu->getSelected() == 0)
				audio_menu->incrementSelected();
		} else {
			for(uint8_t i=1;i<6;i+=1)
				main_area_box[i]->setWidth(FULL_AREA_W);
			audio_menu->setSelected(0);
		}
	}

	if(refresh)
		refreshAudioScreen();
}

void Audio_Window::refreshAudioScreen() {
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
	for(uint8_t i=0;i<6;i+=1) {
		if(function_area_change[i]) {
			fillText(FUNCTION_AREA_GROUP, i);
			function_area_change[i] = false;
		}
	}
}

void Audio_Window::interpretMenuChange(AIData* ai_b) {
	if(ai_b->sender == ID_NAV_SCREEN) {
		if(ai_b->data[0] == 0x32) {
			const bool clockwise = (ai_b->data[2]&0x10) != 0;
			const uint8_t steps = ai_b->data[2]&0xF;

			if(this->settings_menu == NULL || !this->settings_menu_active) {
				if(this->audio_menu != NULL) {
					if(clockwise) {
						for(uint8_t i=0;i<steps;i+=1)
							this->audio_menu->incrementSelected();
					} else {
						for(uint8_t i=0;i<steps;i+=1)
							this->audio_menu->decrementSelected();
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
				if(this->audio_menu != NULL) {
					if(ai_b->data[1] == 0x29)
						this->audio_menu->incrementSelected();
					else if(ai_b->data[1] == 0x28)
						this->audio_menu->decrementSelected();
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

void Audio_Window::handleEnterButton() {
	AIBusHandler* aibus_handler = this->attribute_list->aibus_handler;

	aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ID_NAV_SCREEN);
	if(this->settings_menu_active && this->settings_menu != NULL) {
		uint8_t data[] = {0x2B, 0x60, uint8_t(this->settings_menu->getSelected())};
		AIData enter_msg(3, ID_NAV_COMPUTER, this->settings_menu_sender);
		enter_msg.refreshAIData(data);

		aibus_handler->writeAIData(&enter_msg);
	} else {
		uint8_t data[] = {0x2B, 0x6A, uint8_t(this->audio_menu->getSelected())};
		AIData enter_msg(3, ID_NAV_COMPUTER, ID_RADIO);
		enter_msg.refreshAIData(data);

		aibus_handler->writeAIData(&enter_msg);
	}
}

void Audio_Window::initializeSettingsMenu(AIData* ai_b) {
	const uint8_t count = ai_b->data[3], rows = ai_b->data[2]&0x7F;
	const bool loop = (ai_b->data[2]&0x80) != 0;
	const int16_t x = (ai_b->data[4]<<8)|ai_b->data[5], y = (ai_b->data[6]<<8)|ai_b->data[7];
	const uint16_t w = (ai_b->data[8]<<8)|ai_b->data[9], h = (ai_b->data[10]<<8)|ai_b->data[11];
	std::string title = "";
	if(ai_b->l > 12) {
		for(uint8_t i=0;i<ai_b->l-12;i+=1)
			title += char(ai_b->data[i+12]);
	}

	this->settings_menu_sender = ai_b->sender;
	
	//if(this->settings_menu != NULL)
	//	delete this->settings_menu;

	this->settings_menu = new NavMenu(attribute_list, x, y, w, h, count, -1, h*6/7, rows, loop, title);
	settings_menu_prep = true;
}

void Audio_Window::setText(const uint8_t group, const uint8_t area, std::string text) {
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
	else if(group == FUNCTION_AREA_GROUP && area < 6) {
		function_area_text[area] = text;
		function_area_change[area] = true;
	}
	else if(group == BUTTON_AREA_GROUP && area < 5) {
		this->audio_menu->setItem(text, area);
		return;
	}

	//this->fillText(group, area);
}

void Audio_Window::fillText(const uint8_t group, const uint8_t area) {

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
	} else if(group == FUNCTION_AREA_GROUP) {
		if(area >= 6)
			return;
		
		function_area_box[area]->setText(function_area_text[area]);
		if(this->active)
			function_area_box[area]->drawText();
	}
}

void Audio_Window::sendMenuClose() {
	this->sendMenuClose(this->settings_menu_sender);
}

void Audio_Window::sendMenuClose(const uint8_t receiver) {
	AIData close_msg(2, ID_NAV_COMPUTER, receiver);
	uint8_t data[] = {0x2B, 0x40};
	close_msg.refreshAIData(data);

	this->attribute_list->aibus_handler->writeAIData(&close_msg);
}
