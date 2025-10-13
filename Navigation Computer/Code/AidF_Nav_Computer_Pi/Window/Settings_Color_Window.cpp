#include "Settings_Color_Window.h"

Settings_Color_Window::Settings_Color_Window(AttributeList *attribute_list) : Settings_Window(attribute_list, 2, getMenuTitle(MENU_INDEX_SETTINGS_COLORS, attribute_list->locale), NEXT_WINDOW_SETTINGS_DISPLAY) {
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
		else if(selected == l - 1) {
			//TODO: Save a new preset.
		} else {
			std::string selected_profile = this->settings_menu->getSelectedString();
			getIniColorProfile(attribute_list->day_profile, attribute_list->night_profile, selected_profile);
			saveIniColorProfile(*attribute_list->day_profile, *attribute_list->night_profile, ACTIVE_COLOR);

			if(attribute_list->night)
				setColorProfile(attribute_list->color_profile, *attribute_list->night_profile);
			else
				setColorProfile(attribute_list->color_profile, *attribute_list->day_profile);
			
			attribute_list->background_changed = true;
			attribute_list->text_changed = true;
		}
	}
}

//Create the main color menu.
void Settings_Color_Window::initColorMainMenu() {
	this->color_menu = SETTINGS_COLOR_MENU_MAIN;
	this->clearMenu();

	MenuList main_menu = getMenu(MENU_INDEX_SETTINGS_COLORS, attribute_list->locale);
	this->title_block->setText(main_menu.title);

	std::vector<std::string> presets = getIniProfileList();
	const uint16_t l = presets.size() + 2;
	
	this->resizeMenu(l);

	for(int i=0;i<l-2;i+=1)
		this->settings_menu->setItem(presets.at(i), i);

	this->settings_menu->setItem(main_menu.getLocalEntry(MENU_INDEX_SETTINGS_COLORS_CUSTOM), l-2);
	this->settings_menu->setItem(main_menu.getLocalEntry(MENU_INDEX_SETTINGS_COLORS_SAVE), l-1);
	
	this->settings_menu->setSelected(1);
}
