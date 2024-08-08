#include "Main_Menu_Window.h"

Main_Menu_Window::Main_Menu_Window(AttributeList *attribute_list) : NavWindow(attribute_list) {
	title_box = new TextBox(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y, this->w-MAIN_TITLE_AREA_X, MAIN_TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 55, &this->color_profile->text);
	title_box->setText("Main Menu");

	this->main_menu = new NavMenu(attribute_list, 0, MAIN_TITLE_AREA_Y + MAIN_TITLE_HEIGHT*11/10, w/2, 42, 12, -1, 36, 6, true, "");
	setMainMenu();
}

Main_Menu_Window::~Main_Menu_Window() {
	delete this->title_box;
	delete this->main_menu;
}

void Main_Menu_Window::drawWindow() {
	if(!this->active)
		return;
	
	this->title_box->drawText();
	this->main_menu->drawMenu();
}

void Main_Menu_Window::refreshWindow() {
	this->title_box->renderText();
	this->main_menu->refreshItems();
}

void Main_Menu_Window::setMainMenu() {
	main_menu->setItem("Navigation", 0);
	main_menu->setItem("Audio", 1);
	main_menu->setItem("Phone", 2);
	main_menu->setItem("Settings", 5);
	main_menu->setItem("Monitor Off", 6);
	main_menu->setItem("Consumption", 11);
	main_menu->setItem("Power Flow", 10);
	main_menu->setItem("Phone Mirror", 9);

	main_menu->setSelected(1);
}

bool Main_Menu_Window::handleAIBus(AIData* msg) {
	if(!this->active)
		return false;

	AIBusHandler* aibus_handler = this->attribute_list->aibus_handler;

	if(msg->sender == ID_NAV_SCREEN) { //From the screen.
		if(msg->data[0] == 0x32 && msg->data[1] == 0x7) { //Knob turn.
			aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
			interpretMenuChange(msg);
			return true;
		} else if(msg->data[0] == 0x30) { //Button press.
			const uint8_t button = msg->data[1], state = msg->data[2]>>6;
			if(button == 0x7 && state == 0x2) { //Enter button.
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				handleEnterButton();
				return true;
			} else if((button == 0x28 || button == 0x29 || button == 0x2A || button == 0x2B) && state == 0x2) {
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				interpretMenuChange(msg);
				return true;
			} else if(button == 0x51 && state == 0x2) {
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				attribute_list->next_window = NEXT_WINDOW_SETTINGS_MAIN;
				return true;
			} else
				return false;
		} else
			return false;
	} else
		return false;
}

void Main_Menu_Window::interpretMenuChange(AIData* ai_b) {
	if(ai_b->data[0] == 0x32) {
		const bool clockwise = (ai_b->data[2]&0x10) != 0;
		const uint8_t steps = ai_b->data[2]&0xF;

		if(!clockwise) {
			for(uint8_t i=0;i<steps;i+=1)
				this->main_menu->incrementSelected();
		} else {
			for(uint8_t i=0;i<steps;i+=1)
				this->main_menu->decrementSelected();
		}
	} else if(ai_b->data[0] == 0x30){
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

		this->main_menu->navigateSelected(control);
	}
}

void Main_Menu_Window::handleEnterButton() {
	const int16_t option = this->main_menu->getSelected() - 1;
	if(option < 0)
		return;

	this->main_menu->setSelected(1);
	switch(option) {
		case 0:
			attribute_list->next_window = NEXT_WINDOW_MAP;
			break;
		case 1:
			attribute_list->next_window = NEXT_WINDOW_AUDIO;
			break;
		case 5:
			attribute_list->next_window = NEXT_WINDOW_SETTINGS_MAIN;
			break;
		case 6:
			this->main_menu->setSelected(1);
			{
				AIData screen_off_msg(2, ID_NAV_COMPUTER, ID_NAV_SCREEN);
				//TODO: Cut all controls to the screen.
				screen_off_msg.data[0] = 0x10;
				screen_off_msg.data[1] = 0x0;
				this->attribute_list->aibus_handler->writeAIData(&screen_off_msg);
			}
			break;
		case 11:
			attribute_list->next_window = NEXT_WINDOW_CONSUMPTION;
			break;
	}
}
