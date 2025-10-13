#include "Settings_Display_Window.h"

Settings_Display_Window::Settings_Display_Window(AttributeList *attribute_list) : Settings_Window(attribute_list, 3, getMenuTitle(MENU_INDEX_SETTINGS_DISPLAY, attribute_list->locale), NEXT_WINDOW_SETTINGS_MAIN) {
	initSettingsMain();
}

void Settings_Display_Window::handleEnterButton() {
	const int selected = this->settings_menu->getSelected() - 1;

	if(selected < 0)
		return;

	if(this->settings_display_menu == SETTINGS_DISPLAY_MENU_MAIN) {
		MenuList main_menu = getMenu(MENU_INDEX_SETTINGS_DISPLAY, attribute_list->locale);

		switch(main_menu.getGlobalIndex(selected)) {
		case MENU_INDEX_SETTINGS_DISPLAY_COLORS:
			attribute_list->next_window = NEXT_WINDOW_SETTINGS_COLOR;
			break;
		case MENU_INDEX_SETTINGS_DISPLAY_DAY_NIGHT:
			initSettingsNight();
			break;
		default:
			break;
		}
	}
}

void Settings_Display_Window::initSettingsMain() {
	this->settings_display_menu = SETTINGS_DISPLAY_MENU_MAIN;
	
	this->clearMenu();

	MenuList main_menu = getMenu(MENU_INDEX_SETTINGS_DISPLAY, attribute_list->locale);
	this->title_block->setText(main_menu.title);

	for(int i=0;i<main_menu.size();i+=1)
		this->settings_menu->setItem(main_menu[i], i);

	this->refreshWindow();

	this->settings_menu->setSelected(1);
}

void Settings_Display_Window::initSettingsNight() {
	const settings_display_menu_t last_menu = this->settings_display_menu;
	this->settings_display_menu = SETTINGS_DISPLAY_MENU_DAYNIGHT;
	
	this->clearMenu();

	MenuList daynight_menu = getMenu(MENU_INDEX_SETTINGS_DAY_NIGHT, attribute_list->locale);
	this->title_block->setText(daynight_menu.title);

	for(int i=0;i<daynight_menu.size();i+=1)
		this->settings_menu->setItem(daynight_menu[i], i);

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
