#include "Settings_Secret_Window.h"

Settings_Secret_Window::Settings_Secret_Window(AttributeList* attribute_list) : Settings_Window(attribute_list, attribute_list->ping_device_list.size() + 1, getMenuTitle(MENU_INDEX_SETTINGS_SECRET, attribute_list->locale), NEXT_WINDOW_SETTINGS_MAIN) {
	MenuList menu_list = getMenu(MENU_INDEX_SETTINGS_SECRET, attribute_list->locale);

	for(int i=0;i<menu_list.size();i+=1) {
		this->settings_menu->setItem(menu_list[i], i);
	}

	this->settings_menu->setSelected(1);

	vector<uint8_t>* ping_device_list = &attribute_list->ping_device_list;

	attribute_list->aibus_handler->setAckCache(true);

	for(auto device: *ping_device_list) {
		switch(device) {
		case 0:
		case ID_NAV_COMPUTER:
		case ID_COMPUTER_PROXY:
		case 0xFF:
			continue;
		default:
			break;
		}

		uint8_t req_data[] = {0x2B, 0x57, 'D', 'E', 'V', 'S', 'E', 'T'};
		AIData req_msg(sizeof(req_data), ID_NAV_COMPUTER, device, req_data);
		attribute_list->aibus_handler->writeAIData(&req_msg);
	}
}

bool Settings_Secret_Window::handleAIBus(AIData* ai_msg) {
	if(!home_held && !menu_held && !enter_held && Settings_Window::handleAIBus(ai_msg))
		return true;

	if(ai_msg->l >= 2 && ai_msg->data[0] == 0x2B) { //Menu message...
		if(ai_msg->data[1] == 0x57) { //Secondary settings menu name.
			string menu_name = "";
			for(int i=2;i<ai_msg->l;i+=1)
				menu_name += char(ai_msg->data[i]);

			addNestedRequestOption(menu_name, ai_msg->sender);
			return true;
		}
	} else if(ai_msg->sender == ID_NAV_SCREEN && ai_msg->l >= 3) {
		if(ai_msg->data[0] == 0x30) {
			const bool held = ai_msg->data[2] == 0x40;
			const uint8_t button = ai_msg->data[1];

			bool answer = false;
			if((button == 0x20 && home_held) || (button == 0x51 && menu_held) || (button == 0x7 && enter_held))
				answer = true;

			const bool was_held = (button == 0x20 && home_held) || (button == 0x51 && menu_held) || (button == 0x7 && enter_held);

			if(button == 0x20)
				home_held &= held;
			else if(button == 0x51)
				menu_held &= held;
			else if(button == 0x7)
				enter_held &= held;

			if(!was_held) //Not a button that was previously held.
				return Settings_Window::handleAIBus(ai_msg);

			return answer;
		}
	} 

	return false;
}

void Settings_Secret_Window::exitWindow() {
	exited = true;
}

void Settings_Secret_Window::drawWindow() {
	if(exited) {
		attribute_list->next_window = NEXT_WINDOW_SETTINGS_MAIN;
		return;
	}

	Settings_Window::drawWindow();
}

//Add a nested device settings request option to the menu.
void Settings_Secret_Window::addNestedRequestOption(const string option_name, const uint8_t device_id) {
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

	MenuList menu_list = getMenu(MENU_INDEX_SETTINGS_SECRET, attribute_list->locale);

	for(int i=0;i<setting_name.size();i+=1)
		settings_menu->setItem(setting_name[i], menu_list.size() + i);
}

void Settings_Secret_Window::handleEnterButton() {
	const int selected = this->settings_menu->getSelected() - 1;

	if(selected < 0)
		return;

	MenuList main_menu = getMenu(MENU_INDEX_SETTINGS_SECRET, attribute_list->locale);

	if(selected - main_menu.size() < setting_id.size()) {
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

		attribute_list->next_window = NEXT_WINDOW_SETTINGS_SECRET_EXT;
		
		uint8_t dev_setting_request_data[] = {0x2B, 0x55, 'D', 'E', 'V', 'S', 'E', 'T'};
		AIData dev_setting_request_msg(sizeof(dev_setting_request_data), ID_NAV_COMPUTER, device_id, dev_setting_request_data);

		attribute_list->aibus_handler->writeToCache(&dev_setting_request_msg, true);
	}
}