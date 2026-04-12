#include "Settings_Info_Window.h"

Settings_Info_Window::Settings_Info_Window(AttributeList* attribute_list, InfoParameters* info_parameters) : Settings_Window(attribute_list, 4, getString(LOCALE_STRING_INFO_WINDOW_SETTINGS_HEADER, attribute_list->locale), NEXT_WINDOW_SETTINGS_MAIN) {
	initInfoMenuMain();
	this->info_parameters = info_parameters;
}

//Initialize the main info menu.
void Settings_Info_Window::initInfoMenuMain() {
	settings_info_menu = SETTINGS_INFO_MENU_MAIN;

	const MenuList main_menu = getMenu(MENU_INDEX_INFORMATION_MAIN, attribute_list->locale);
	title_block->setText(getString(LOCALE_STRING_INFO_WINDOW_SETTINGS_HEADER, attribute_list->locale));

	this->clearMenu();
	this->resizeMenu(main_menu.size());

	const int start = main_menu.getLocalIndex(MENU_INDEX_INFORMATION_MAIN_DISP_1);

	for(int i=start;i<=main_menu.getLocalIndex(MENU_INDEX_INFORMATION_MAIN_DISP_4);i+=1)
		settings_menu->setItem(main_menu[i], i-start);

	refreshWindow();
	settings_menu->setSelected(1);
}

//Initialize the parameters settings menu.
void Settings_Info_Window::initParamSettingsMenu(const uint8_t active_param) {
	settings_info_menu = SETTINGS_INFO_MENU_PARAM;

	this->active_param = active_param;

	const MenuList parameter_menu = getMenu(MENU_INDEX_INFORMATION_PARAM, attribute_list->locale);
	title_block->setText(parameter_menu.title + to_string(active_param + 1));

	this->clearMenu();
	this->resizeMenu(parameter_menu.size());

	for(int i=0;i<parameter_menu.size();i+=1) {
		string option = "";
		if(info_parameters->param_index[active_param] == i)
			option = "#CON ";
		else
			option = "#COF ";

		option += parameter_menu[i];

		bool sel = true;
		if(i != 0) {
			for(int j=0;j<PARAM_COUNT;j+=1) {
				if(info_parameters->param_index[j] == i && j != active_param)
					sel = false;	
				if(!sel)
					break;
			}
		}

		if(sel)
			settings_menu->setItem(option, i);
	}

	settings_menu->setSelected(1);
}

//Handle the Enter button.
void Settings_Info_Window::handleEnterButton() {
	const int selected = this->settings_menu->getSelected() - 1;

	if(selected < 0)
		return;

	if(settings_info_menu == SETTINGS_INFO_MENU_MAIN) {
		initParamSettingsMenu(uint8_t(selected));
	} else if(settings_info_menu == SETTINGS_INFO_MENU_PARAM) {
		info_parameters->param_index[active_param] = (info_param)selected;
		initInfoMenuMain();
		saveParamSettings();
	}
}

//Handle the Back button.
void Settings_Info_Window::handleBackButton() {
	if(settings_info_menu == SETTINGS_INFO_MENU_PARAM)
		initInfoMenuMain();
	else
		Settings_Window::handleBackButton();
}

//Save parameter settings to a file.
void Settings_Info_Window::saveParamSettings() {
	uint8_t params[PARAM_COUNT];
	for(int i=0;i<PARAM_COUNT;i+=1)
		params[i] = (uint8_t)this->info_parameters->param_index[i];

	saveVehicleInfoParams(this->info_parameters->display_cruise, this->info_parameters->draw_charge_assist, params, PARAM_COUNT);
}