#include "Settings_Color_Window.h"

Settings_Color_Window::Settings_Color_Window(AttributeList *attribute_list) : Settings_Window(attribute_list, 2, "Colors", NEXT_WINDOW_SETTINGS_DISPLAY) {
	initColorMainMenu();
}

void Settings_Color_Window::handleEnterButton() {
	const int selected = this->settings_menu->getSelected() - 1;
	const uint16_t l = this->settings_menu->getLength();

	if(selected < 0)
		return;

	if(this->color_menu == SETTINGS_COLOR_MENU_MAIN) {
		if(selected == l-2)
			this->attribute_list->next_window = NEXT_WINDOW_SETTINGS_COLOR_PICKER;
	}
}

void Settings_Color_Window::initColorMainMenu() {
	this->color_menu = SETTINGS_COLOR_MENU_MAIN;
	this->clearMenu();

	this->title_block->setText("Colors");

	//TODO: Load in preset colors from a configuration file.

	const uint16_t l = this->settings_menu->getLength();
	this->settings_menu->setItem("Custom", l-2);
	this->settings_menu->setItem("Save Preset", l-1);
	
	this->settings_menu->setSelected(l-2);
}
