#include "Settings_Main_Window.h"

Settings_Main_Window::Settings_Main_Window(AttributeList *attribute_list) : Settings_Window(attribute_list, SETTING_COUNT, "Settings", NEXT_WINDOW_NULL) {
	this->settings_menu->setItem("Display Settings", 0);
	this->settings_menu->setItem("Info Settings", 1);
	this->settings_menu->setItem("Clock Settings", 2);
	this->settings_menu->setItem("Unit/Format Settings", 3);
	if(attribute_list->canslator_connected)
		this->settings_menu->setItem("Vehicle Settings", 4);
	if(attribute_list->radio_connected)
		this->settings_menu->setItem("Audio Settings", 5);

	this->settings_menu->setSelected(1);
}

void Settings_Main_Window::handleEnterButton() {
	const int selected = this->settings_menu->getSelected() - 1;

	if(selected < 0)
		return;

	switch(selected) {
		case 0:
			attribute_list->next_window = NEXT_WINDOW_SETTINGS_DISPLAY;
			break;
		case 1:
			attribute_list->next_window = NEXT_WINDOW_SETTINGS_INFO;
			break;
		case 2:
			attribute_list->next_window = NEXT_WINDOW_SETTINGS_CLOCK;
			break;
		case 3:
			attribute_list->next_window = NEXT_WINDOW_SETTINGS_FORMAT;
			break;
		case 4:
			attribute_list->next_window = NEXT_WINDOW_SETTINGS_EXT;
			sendSettingsMenuRequest(ID_CANSLATOR);
			break;
		case 5:
			attribute_list->next_window = NEXT_WINDOW_SETTINGS_EXT;
			sendSettingsMenuRequest(ID_RADIO);
			break;
	}
}

//Send a menu request.
void Settings_Main_Window::sendSettingsMenuRequest(const uint8_t receiver) {
	uint8_t request_data[] = {0x2B, 0x45};
	AIData request_msg(sizeof(request_data), ID_NAV_COMPUTER, receiver);
	request_msg.refreshAIData(request_data);

	attribute_list->aibus_handler->cacheTxMessage(&request_msg);
}