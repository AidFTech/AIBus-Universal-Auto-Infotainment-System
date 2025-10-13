#include "Settings_Main_Window.h"

Settings_Main_Window::Settings_Main_Window(AttributeList *attribute_list) : Settings_Window(attribute_list, SETTING_COUNT, getMenuTitle(MENU_INDEX_SETTINGS_MAIN, attribute_list->locale), NEXT_WINDOW_NULL) {
	MenuList menu_list = getMenu(MENU_INDEX_SETTINGS_MAIN, attribute_list->locale);
	
	for(int i=0;i<menu_list.size();i+=1) {
		if(menu_list.getGlobalIndex(i) == MENU_INDEX_SETTINGS_MAIN_VEHICLE && attribute_list->canslator_connected)
			this->settings_menu->setItem(menu_list[i], i);
		else if(menu_list.getGlobalIndex(i) == MENU_INDEX_SETTINGS_MAIN_AUDIO && attribute_list->radio_connected)
			this->settings_menu->setItem(menu_list[i], i);
		else
			this->settings_menu->setItem(menu_list[i], i);
	}

	this->settings_menu->setSelected(1);
}

void Settings_Main_Window::handleEnterButton() {
	const int selected = this->settings_menu->getSelected() - 1;

	if(selected < 0)
		return;

	MenuList main_menu = getMenu(MENU_INDEX_SETTINGS_MAIN, attribute_list->locale);

	switch(main_menu.getGlobalIndex(selected)) {
	case MENU_INDEX_SETTINGS_MAIN_DISPLAY:
		attribute_list->next_window = NEXT_WINDOW_SETTINGS_DISPLAY;
		break;
	case MENU_INDEX_SETTINGS_MAIN_INFO:
		attribute_list->next_window = NEXT_WINDOW_SETTINGS_INFO;
		break;
	case MENU_INDEX_SETTINGS_MAIN_CLOCK:
		attribute_list->next_window = NEXT_WINDOW_SETTINGS_CLOCK;
		break;
	case MENU_INDEX_SETTINGS_MAIN_UNIT_FORMAT:
		attribute_list->next_window = NEXT_WINDOW_SETTINGS_FORMAT;
		break;
	case MENU_INDEX_SETTINGS_MAIN_VEHICLE:
		attribute_list->next_window = NEXT_WINDOW_SETTINGS_EXT;
		sendSettingsMenuRequest(ID_CANSLATOR);
		break;
	case MENU_INDEX_SETTINGS_MAIN_AUDIO:
		attribute_list->next_window = NEXT_WINDOW_SETTINGS_EXT;
		sendSettingsMenuRequest(ID_RADIO);
		break;
	default:
		break;
	}
}

//Send a menu request.
void Settings_Main_Window::sendSettingsMenuRequest(const uint8_t receiver) {
	uint8_t request_data[] = {0x2B, 0x45};
	AIData request_msg(sizeof(request_data), ID_NAV_COMPUTER, receiver, request_data);

	attribute_list->aibus_handler->cacheTxMessage(&request_msg);
}