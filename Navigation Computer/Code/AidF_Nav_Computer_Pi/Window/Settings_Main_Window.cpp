#include "Settings_Main_Window.h"

Settings_Main_Window::Settings_Main_Window(AttributeList *attribute_list) : Settings_Window(attribute_list, SETTING_COUNT + attribute_list->ping_device_list.size(), getMenuTitle(MENU_INDEX_SETTINGS_MAIN, attribute_list->locale), NEXT_WINDOW_NULL) {
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

	vector<uint8_t>* ping_device_list = &attribute_list->ping_device_list;

	for(int i=0;i<ping_device_list->size();i+=1) {
		switch(ping_device_list->at(i)) {
		case 0:
		case ID_NAV_COMPUTER:
		case ID_COMPUTER_PROXY:
		case ID_CANSLATOR:
		case ID_RADIO:
		case 0xFF:
			continue;
		default:
			break;
		}

		uint8_t req_data[] = {0x2B, 0x57, 'S', 'E', 'T', 'T', 'I', 'N', 'G'};
		AIData req_msg(sizeof(req_data), ID_NAV_COMPUTER, ping_device_list->at(i), req_data);
		attribute_list->aibus_handler->writeToCache(&req_msg, true);
	}

	ping_device_list->clear(); //TODO: DO we really want to do this?
}

bool Settings_Main_Window::handleAIBus(AIData* ai_msg) {
	if(Settings_Window::handleAIBus(ai_msg))
		return true;

	if(ai_msg->l >= 2 && ai_msg->data[0] == 0x2B) { //Menu message...
		if(ai_msg->data[1] == 0x57) { //Secondary settings menu name.
			string menu_name = "";
			for(int i=2;i<ai_msg->l;i+=1)
				menu_name += char(ai_msg->data[i]);

			addNestedRequestOption(menu_name, ai_msg->sender);
			return true;
		}
	}

	return false;
}

void Settings_Main_Window::handleEnterButton() {
	const int selected = this->settings_menu->getSelected() - 1;

	if(selected < 0)
		return;

	MenuList main_menu = getMenu(MENU_INDEX_SETTINGS_MAIN, attribute_list->locale);

	if(selected < main_menu.size()) {
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
	} else if(selected - main_menu.size() < setting_id.size()) {
		const uint8_t device_id = setting_id.at(selected - main_menu.size());
		bool found = false;

		for(int i=0;i<attribute_list->ping_device_list.size();i+=1) {
			if(attribute_list->ping_device_list.at(i) == device_id) {
				found = true;
				break;
			}
		}

		if(!found)
			return;

		attribute_list->next_window = NEXT_WINDOW_SETTINGS_EXT;
		sendSettingsMenuRequest(device_id);
	}
}

//Add a nested device settings request option to the menu.
void Settings_Main_Window::addNestedRequestOption(const string option_name, const uint8_t device_id) {
	int ins_ind = -1;
	for(int i=0;i<setting_id.size();i+=1) {
		if(setting_id[i] > device_id) {
			ins_ind = i;
			break;
		} else if(setting_id[i] == device_id) //ID already exists.
			return;
	}

	if(ins_ind >= 0) {
		setting_id.insert(setting_id.begin() + ins_ind, device_id);
		setting_name.insert(setting_name.begin() + ins_ind, option_name);
	} else {
		setting_id.push_back(device_id);
		setting_name.push_back(option_name);
	}

	MenuList menu_list = getMenu(MENU_INDEX_SETTINGS_MAIN, attribute_list->locale);

	for(int i=0;i<setting_name.size();i+=1)
		settings_menu->setItem(setting_name[i], menu_list.size() + i);
}

//Send a menu request.
void Settings_Main_Window::sendSettingsMenuRequest(const uint8_t receiver) {
	uint8_t request_data[] = {0x2B, 0x45};
	AIData request_msg(sizeof(request_data), ID_NAV_COMPUTER, receiver, request_data);

	attribute_list->aibus_handler->writeToCache(&request_msg, true);
}