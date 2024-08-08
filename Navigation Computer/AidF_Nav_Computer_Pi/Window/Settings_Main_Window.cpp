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
	}
}