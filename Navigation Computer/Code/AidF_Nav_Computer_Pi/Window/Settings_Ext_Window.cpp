#include "Settings_Ext_Window.h"

Settings_Ext_Window::Settings_Ext_Window(AttributeList *attribute_list) : Settings_Window(attribute_list, SETTING_COUNT, "", NEXT_WINDOW_SETTINGS_MAIN) {
	this->allow_ext_menu = true;
}

void Settings_Ext_Window::exitWindow() {
	uint8_t back_data[] = {0x2B, 0x40};
	AIData back_msg(sizeof(back_data), ID_NAV_COMPUTER, this->ext_menu_sender);
	back_msg.refreshAIData(back_data);

	if(ext_menu_sender != ID_NAV_COMPUTER)
		attribute_list->aibus_handler->writeAIData(&back_msg);
}

void Settings_Ext_Window::handleEnterButton() {
	const int selected = this->settings_menu->getSelected() - 1;

	if(selected < 0)
		return;
	
	uint8_t enter_data[] = {0x2B, 0x60, uint8_t((selected+1)&0xFF)};
	AIData enter_msg(sizeof(enter_data), ID_NAV_COMPUTER, this->ext_menu_sender);
	enter_msg.refreshAIData(enter_data);
	
	attribute_list->aibus_handler->writeAIData(&enter_msg);
}

void Settings_Ext_Window::handleBackButton() {
	uint8_t back_data[] = {0x2B, 0x40};
	AIData back_msg(sizeof(back_data), ID_NAV_COMPUTER, this->ext_menu_sender);
	back_msg.refreshAIData(back_data);

	if(ext_menu_sender != ID_NAV_COMPUTER)
		attribute_list->aibus_handler->writeAIData(&back_msg);
	else
		Settings_Window::handleBackButton();
}