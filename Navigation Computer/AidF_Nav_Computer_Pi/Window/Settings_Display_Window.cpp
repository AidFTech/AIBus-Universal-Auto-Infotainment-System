#include "Settings_Display_Window.h"

Settings_Display_Window::Settings_Display_Window(AttributeList *attribute_list) : Settings_Window(attribute_list, 3, "Display Settings", NEXT_WINDOW_SETTINGS_MAIN) {
	initSettingsMain();
}

void Settings_Display_Window::handleEnterButton() {
	const int selected = this->settings_menu->getSelected() - 1;

	if(selected < 0)
		return;

	if(this->settings_display_menu == SETTINGS_DISPLAY_MENU_MAIN) {
		switch(selected) {
		case 0:
			attribute_list->next_window = NEXT_WINDOW_SETTINGS_COLOR;
			break;
		case 1:
			initSettingsNight();
			break;
		}
	}
}

void Settings_Display_Window::initSettingsMain() {
	this->settings_display_menu = SETTINGS_DISPLAY_MENU_MAIN;
	
	this->clearMenu();

	this->title_block->setText("Display Settings");

	this->settings_menu->setItem("Colors", 0);
	this->settings_menu->setItem("Day/Night Mode", 1);
	this->settings_menu->setItem("Upper Header", 2);

	this->refreshWindow();

	this->settings_menu->setSelected(1);
}

void Settings_Display_Window::initSettingsNight() {
	const int8_t last_menu = this->settings_display_menu;
	this->settings_display_menu = SETTINGS_DISPLAY_MENU_DAYNIGHT;
	
	this->clearMenu();

	this->title_block->setText("Day/Night Mode");

	this->settings_menu->setItem("Auto", 0);
	this->settings_menu->setItem("Day", 1);
	this->settings_menu->setItem("Night", 2);

	this->refreshWindow();

	if(last_menu != SETTINGS_DISPLAY_MENU_DAYNIGHT)
		this->settings_menu->setSelected(1);
}

void Settings_Display_Window::handleBackButton() {
	if(this->settings_display_menu == SETTINGS_DISPLAY_MENU_MAIN)
		Settings_Window::handleBackButton();
	else {
		this->initSettingsMain();
	}
}
