#include "Settings_Clock_Window.h"

Settings_Clock_Window::Settings_Clock_Window(AttributeList *attribute_list) : Settings_Window(attribute_list, SETTINGS_CLOCK_MENU_LEN, getMenuTitle(MENU_INDEX_SETTINGS_CLOCK, attribute_list->locale), NEXT_WINDOW_SETTINGS_MAIN) {
	initClockMain();
}

//Initialize the main clock menu.
void Settings_Clock_Window::initClockMain() {
	this->settings_clock_menu = SETTINGS_CLOCK_MENU_MAIN;

	MenuList main_menu = getMenu(MENU_INDEX_SETTINGS_CLOCK, attribute_list->locale);
	this->title_block->setText(main_menu.title);

	this->clearMenu();

	for(int i=0;i<main_menu.size();i+=1)
		this->settings_menu->setItem(main_menu[i], i);

	this->refreshWindow();

	this->settings_menu->setSelected(1);
}

//Initialize the clock format menu.
void Settings_Clock_Window::initClockFormat() {
	const settings_clock_menu_t last_menu = this->settings_clock_menu;
	this->settings_clock_menu = SETTINGS_CLOCK_MENU_FORMAT;

	MenuList clock_menu = getMenu(MENU_INDEX_SETTINGS_CLOCK_FORMAT, attribute_list->locale);
	this->title_block->setText(clock_menu.title);
	
	this->clearMenu();

	std::string msg_12h;
	if(attribute_list->display_12h)
		msg_12h = "#CON ";
	else
		msg_12h = "#COF ";

	std::string msg_24h;
	if(!attribute_list->display_12h)
		msg_24h = "#CON ";
	else
		msg_24h = "#COF ";

	msg_12h += clock_menu.getLocalEntry(MENU_INDEX_SETTINGS_CLOCK_FORMAT_12H);
	msg_24h += clock_menu.getLocalEntry(MENU_INDEX_SETTINGS_CLOCK_FORMAT_24H);

	this->settings_menu->setItem(msg_12h, clock_menu.getLocalIndex(MENU_INDEX_SETTINGS_CLOCK_FORMAT_12H));
	this->settings_menu->setItem(msg_24h, clock_menu.getLocalIndex(MENU_INDEX_SETTINGS_CLOCK_FORMAT_24H));

	this->refreshWindow();

	if(last_menu != SETTINGS_CLOCK_MENU_FORMAT)
		this->settings_menu->setSelected(1);
}

//Initialize the auto clock device window.
void Settings_Clock_Window::initClockAuto() {
	const settings_clock_menu_t last_menu = this->settings_clock_menu;
	this->settings_clock_menu = SETTINGS_CLOCK_MENU_AUTO;

	MenuList clock_menu = getMenu(MENU_INDEX_SETTINGS_AUTO_SET, attribute_list->locale);
	this->title_block->setText(clock_menu.title);
	this->clearMenu();

	bool timekeeper_found = false;

	std::string msg_canslator;
	if(attribute_list->timekeeper == ID_CANSLATOR) {
		msg_canslator = "#CON ";
		timekeeper_found = true;
	} else
		msg_canslator = "#COF ";

	msg_canslator += clock_menu.getLocalEntry(MENU_INDEX_SETTINGS_AUTO_SET_VEHICLE);

	std::string msg_gps;
	if(!timekeeper_found && attribute_list->timekeeper == ID_GPS_ANTENNA) {
		msg_gps = "#CON ";
		timekeeper_found = true;
	} else
		msg_gps = "#COF ";
	msg_gps += clock_menu.getLocalEntry(MENU_INDEX_SETTINGS_AUTO_SET_GPS);

	std::string msg_radio;
	if(!timekeeper_found && attribute_list->timekeeper == ID_RADIO && attribute_list->auto_clock) {
		msg_radio = "#CON ";
		timekeeper_found = true;
	} else
		msg_radio = "#COF ";
	msg_radio += clock_menu.getLocalEntry(MENU_INDEX_SETTINGS_AUTO_SET_RADIO);

	std::string msg_off;
	if(!timekeeper_found)
		msg_off = "#CON ";
	else
		msg_off = "#COF ";
	msg_off += clock_menu.getLocalEntry(MENU_INDEX_SETTINGS_AUTO_SET_MANUAL);

	this->settings_menu->setItem(msg_canslator, clock_menu.getLocalIndex(MENU_INDEX_SETTINGS_AUTO_SET_VEHICLE)); //TODO: Only if vehicle has a timekeeper.
	if(attribute_list->gps_antenna_connected)
		this->settings_menu->setItem(msg_gps, clock_menu.getLocalIndex(MENU_INDEX_SETTINGS_AUTO_SET_GPS));
	this->settings_menu->setItem(msg_radio, clock_menu.getLocalIndex(MENU_INDEX_SETTINGS_AUTO_SET_RADIO));
	this->settings_menu->setItem(msg_off, clock_menu.getLocalIndex(MENU_INDEX_SETTINGS_AUTO_SET_MANUAL));

	if(last_menu != SETTINGS_CLOCK_MENU_AUTO)
		this->settings_menu->setSelected(1);
}

void Settings_Clock_Window::handleEnterButton() {
	const int selected = this->settings_menu->getSelected() - 1;

	if(selected < 0)
		return;

	if(this->settings_clock_menu == SETTINGS_CLOCK_MENU_MAIN) {
		MenuList menu = getMenu(MENU_INDEX_SETTINGS_CLOCK, attribute_list->locale);

		switch(menu.getGlobalIndex(selected)) {
		case MENU_INDEX_SETTINGS_CLOCK_CLOCK_FORMAT: //Clock format.
			this->initClockFormat();
			break;
		case MENU_INDEX_SETTINGS_CLOCK_AUTO_SET: //Auto clock.
			this->initClockAuto();
			break;
		default:
			break;
		}
	} else if(this->settings_clock_menu == SETTINGS_CLOCK_MENU_FORMAT) {
		AIBusHandler* ai_handler = attribute_list->aibus_handler;
		uint8_t time_set_byte = 0x0;

		if(attribute_list->auto_clock)
			time_set_byte |= 0x1;
		else
			time_set_byte |= 0x2;

		MenuList menu = getMenu(MENU_INDEX_SETTINGS_CLOCK_FORMAT, attribute_list->locale);

		switch(menu.getGlobalIndex(selected)) {
		case MENU_INDEX_SETTINGS_CLOCK_FORMAT_12H: //12h.
			time_set_byte |= 0x80;
			break;
		case MENU_INDEX_SETTINGS_CLOCK_FORMAT_24H: //24h.
			time_set_byte &= (~0x80);
			break;
		default:
			return;
		}

		if(selected == 0 || selected == 1) {
			uint8_t format_data[] = {0x1D, time_set_byte};
			AIData format_msg(sizeof(format_data), ID_NAV_COMPUTER, attribute_list->timekeeper);
			format_msg.refreshAIData(format_data);

			bool ack = true;
			if(attribute_list->timekeeper == ID_RADIO && !attribute_list->radio_connected)
				ack = false;
			if(attribute_list->timekeeper == ID_CANSLATOR && !attribute_list->canslator_connected)
				ack = false;
			if(attribute_list->timekeeper == 0xFF || attribute_list->timekeeper == ID_NAV_COMPUTER)
				ack = false;
			
			ai_handler->writeAIData(&format_msg, ack);

			initClockMain();
		}
	} else if(this->settings_clock_menu == SETTINGS_CLOCK_MENU_AUTO) {
		const uint8_t last_timekeeper = attribute_list->timekeeper;
		const bool last_auto = attribute_list->auto_clock;

		uint8_t new_timekeeper = last_timekeeper;
		bool new_auto = last_auto;

		MenuList menu = getMenu(MENU_INDEX_SETTINGS_AUTO_SET, attribute_list->locale);

		switch(menu.getGlobalIndex(selected)) {
		case MENU_INDEX_SETTINGS_AUTO_SET_VEHICLE: //Canslator.
			new_timekeeper = ID_CANSLATOR;
			new_auto = true;
			break;
		case MENU_INDEX_SETTINGS_AUTO_SET_GPS: //GPS.
			if(attribute_list->gps_antenna_connected) {
				new_timekeeper = ID_GPS_ANTENNA;
				new_auto = true;
			} else
				return;
			break;
		case MENU_INDEX_SETTINGS_AUTO_SET_RADIO: //Radio.
			new_timekeeper = ID_RADIO;
			new_auto = true;
			break;
		case MENU_INDEX_SETTINGS_AUTO_SET_MANUAL: //Radio, manual.
			new_timekeeper = ID_RADIO;
			new_auto = false;
			break;
		default:
			return;
		}

		AIBusHandler* ai_handler = attribute_list->aibus_handler;

		if(new_timekeeper != last_timekeeper) {
			uint8_t cancel_data[] = {0x1D, 0x0};
			AIData cancel_msg(sizeof(cancel_data), ID_NAV_COMPUTER, last_timekeeper, cancel_data);

			bool ack = true;
			if(last_timekeeper == ID_RADIO && !attribute_list->radio_connected)
				ack = false;
			if(last_timekeeper == ID_CANSLATOR && !attribute_list->canslator_connected)
				ack = false;
			if(last_timekeeper == 0xFF || last_timekeeper == ID_NAV_COMPUTER)
				ack = false;
			
			ai_handler->writeAIData(&cancel_msg, ack);

			uint8_t start_data[] = {0x1D, 0x0};
			if(new_auto)
				start_data[1] |= 0x1;
			else
				start_data[1] |= 0x2;

			if(attribute_list->display_12h)
				start_data[1] |= 0x80;

			AIData start_msg(sizeof(start_data), ID_NAV_COMPUTER, new_timekeeper);
			start_msg.refreshAIData(start_data);

			ack = true;
			if(new_timekeeper == ID_RADIO && !attribute_list->radio_connected)
				ack = false;
			if(new_timekeeper == ID_CANSLATOR && !attribute_list->canslator_connected)
				ack = false;

			ai_handler->writeAIData(&start_msg, ack);
		} else if(new_auto != last_auto) {
			uint8_t start_data[] = {0x1D, 0x0};
			if(new_auto)
				start_data[1] |= 0x1;
			else
				start_data[1] |= 0x2;

			if(attribute_list->display_12h)
				start_data[1] |= 0x80;

			AIData start_msg(sizeof(start_data), ID_NAV_COMPUTER, new_timekeeper);
			start_msg.refreshAIData(start_data);

			bool ack = true;
			if(new_timekeeper == ID_RADIO && !attribute_list->radio_connected)
				ack = false;
			if(new_timekeeper == ID_CANSLATOR && !attribute_list->canslator_connected)
				ack = false;

			ai_handler->writeAIData(&start_msg, ack);
		}

		this->initClockMain();
	}
}

void Settings_Clock_Window::handleBackButton() {
	if(this->settings_clock_menu != SETTINGS_CLOCK_MENU_MAIN)
		this->initClockMain();
	else
		Settings_Window::handleBackButton();
}
