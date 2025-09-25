#include "Settings_Clock_Window.h"

Settings_Clock_Window::Settings_Clock_Window(AttributeList *attribute_list) : Settings_Window(attribute_list, SETTINGS_CLOCK_MENU_LEN, "Clock Settings", NEXT_WINDOW_SETTINGS_MAIN) {
	initClockMain();
}

//Initialize the main clock menu.
void Settings_Clock_Window::initClockMain() {
	this->settings_clock_menu = SETTINGS_CLOCK_MENU_MAIN;

	this->title_block->setText("Clock Settings");
	this->clearMenu();

	this->settings_menu->setItem("Clock Format", 0);
	this->settings_menu->setItem("Auto Clock Set", 1);
	this->settings_menu->setItem("Set Clock", 2);

	this->refreshWindow();

	this->settings_menu->setSelected(1);
}

//Initialize the clock format menu.
void Settings_Clock_Window::initClockFormat() {
	const int8_t last_menu = this->settings_clock_menu;
	this->settings_clock_menu = SETTINGS_CLOCK_MENU_FORMAT;

	this->title_block->setText("Clock Format");
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

	msg_12h += "12-hour";
	msg_24h += "24-hour";

	this->settings_menu->setItem(msg_12h, 0);
	this->settings_menu->setItem(msg_24h, 1);

	this->refreshWindow();

	if(last_menu != SETTINGS_CLOCK_MENU_FORMAT)
		this->settings_menu->setSelected(1);
}

//Initialize the auto clock device window.
void Settings_Clock_Window::initClockAuto() {
	const int8_t last_menu = this->settings_clock_menu;
	this->settings_clock_menu = SETTINGS_CLOCK_MENU_AUTO;

	this->title_block->setText("Auto Clock Set");
	this->clearMenu();

	bool timekeeper_found = false;

	std::string msg_canslator;
	if(attribute_list->timekeeper == ID_CANSLATOR) {
		msg_canslator = "#CON ";
		timekeeper_found = true;
	} else
		msg_canslator = "#COF ";

	msg_canslator += "From Vehicle";

	std::string msg_gps;
	if(!timekeeper_found && attribute_list->timekeeper == ID_GPS_ANTENNA) {
		msg_gps = "#CON ";
		timekeeper_found = true;
	} else
		msg_gps = "#COF ";
	msg_gps += "From GPS";

	std::string msg_radio;
	if(!timekeeper_found && attribute_list->timekeeper == ID_RADIO && attribute_list->auto_clock) {
		msg_radio = "#CON ";
		timekeeper_found = true;
	} else
		msg_radio = "#COF ";
	msg_radio += "From Radio";

	std::string msg_off;
	if(!timekeeper_found)
		msg_off = "#CON ";
	else
		msg_off = "#COF ";
	msg_off += "Set Clock Manually";

	this->settings_menu->setItem(msg_canslator, 0); //TODO: Only if vehicle has a timekeeper.
	if(attribute_list->gps_antenna_connected)
		this->settings_menu->setItem(msg_gps, 1);
	this->settings_menu->setItem(msg_radio, 2);
	this->settings_menu->setItem(msg_off, 3);

	if(last_menu != SETTINGS_CLOCK_MENU_AUTO)
		this->settings_menu->setSelected(1);
}

void Settings_Clock_Window::handleEnterButton() {
	const int selected = this->settings_menu->getSelected() - 1;

	if(selected < 0)
		return;

	if(this->settings_clock_menu == SETTINGS_CLOCK_MENU_MAIN) {
		switch(selected) {
		case 0: //Clock format.
			this->initClockFormat();
			break;
		case 1: //Auto clock.
			this->initClockAuto();
			break;
		}
	} else if(this->settings_clock_menu == SETTINGS_CLOCK_MENU_FORMAT) {
		AIBusHandler* ai_handler = attribute_list->aibus_handler;
		uint8_t time_set_byte = 0x0;
		if(attribute_list->auto_clock)
			time_set_byte |= 0x1;
		else
			time_set_byte |= 0x2;

		switch(selected) {
		case 0: //12h.
			time_set_byte |= 0x80;
			break;
		case 1: //24h.
			time_set_byte &= (~0x80);
			break;
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

		switch(selected) {
		case 0: //Canslator.
			new_timekeeper = ID_CANSLATOR;
			new_auto = true;
			break;
		case 1: //GPS.
			if(attribute_list->gps_antenna_connected) {
				new_timekeeper = ID_GPS_ANTENNA;
				new_auto = true;
			} else
				return;
			break;
		case 2: //Radio.
			new_timekeeper = ID_RADIO;
			new_auto = true;
			break;
		case 3: //Radio, manual.
			new_timekeeper = ID_RADIO;
			new_auto = false;
			break;
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
