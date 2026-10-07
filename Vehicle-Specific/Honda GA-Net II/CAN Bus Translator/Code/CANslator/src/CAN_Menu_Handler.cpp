#include "CAN_Menu_Handler.h"

CANMenuHandler::CANMenuHandler(AIBusHandler* ai_handler, ParameterList* parameter_list) {
	this->ai_handler = ai_handler;
	this->parameter_list = parameter_list;
}

//Get the active menu.
active_menu_t CANMenuHandler::getActiveMenu() {
	return this->active_menu;
}

//Set the active menu.
void CANMenuHandler::setActiveMenu(const active_menu_t active_menu) {
	this->active_menu = active_menu;
}

//Send the intiial menu msesage. Return whether a menu can be generated.
bool CANMenuHandler::startMenu(const uint8_t count, const uint8_t rows, const bool loop, String title) {
	uint8_t start_menu_data[title.length() + 12];
	
	unsigned int div = count/rows;
	if(count%rows != 0)
		div += 1;

	//TODO: Screen width.
	const uint16_t x = 0, y = 140, width = 800/div;
	
	start_menu_data[0] = 0x2B;
	start_menu_data[1] = 0x50;
	start_menu_data[2] = rows&0x7F;
	start_menu_data[3] = count;
	start_menu_data[4] = (x&0xFF00) >> 8;
	start_menu_data[5] = x&0xFF;
	start_menu_data[6] = (y&0xFF00) >> 8;
	start_menu_data[7] = y&0xFF;
	start_menu_data[8] = (width&0xFF00)>>8;
	start_menu_data[9] = width&0xFF;
	start_menu_data[10] = 0x0;
	start_menu_data[11] = 0x23;
	
	for(unsigned int i=0;i<title.length();i+=1)
		start_menu_data[i+12] = uint8_t(title.charAt(i));
	
	if(loop)
		start_menu_data[2] |= 0x80;
	
	AIData start_menu_msg(sizeof(start_menu_data), ID_CANSLATOR, ID_NAV_COMPUTER, start_menu_data);
	ai_handler->writeAIData(&start_menu_msg);

	elapsedMillis cancel_wait;
	while(cancel_wait < 20) {
		AIData ai_msg;
		if(ai_handler->dataAvailable() > 0) {
			if(ai_handler->readAIData(&ai_msg, false)) {
				if(ai_msg.l >= 2 && ai_msg.sender == ID_NAV_COMPUTER && ai_msg.receiver == ID_CANSLATOR && ai_msg.data[0] == 0x2B && ai_msg.data[1] == 0x40) { //No menu available.
					return false;
				} else if(ai_msg.receiver == ID_CANSLATOR) {
					ai_handler->cacheMessage(&ai_msg);
				}
			}
		}
	}

	return true;
}

//Append an item to the menu at the index defined by position.
void CANMenuHandler::appendMenu(const uint8_t position, String text) {
	uint8_t append_menu_data[text.length() + 3];
	
	append_menu_data[0] = 0x2B;
	append_menu_data[1] = 0x51;
	append_menu_data[2] = position;
	
	for(unsigned int i=0;i<text.length();i+=1)
		append_menu_data[i+3] = uint8_t(text.charAt(i));
	
	AIData append_menu_msg(sizeof(append_menu_data), ID_CANSLATOR, ID_NAV_COMPUTER, append_menu_data);
	ai_handler->writeAIData(&append_menu_msg);
}

//Display the menu.
void CANMenuHandler::displayMenu(const uint8_t selected) {
	uint8_t display_menu_data[] = {0x2B, 0x52, selected};
	AIData display_menu_msg(sizeof(display_menu_data), ID_CANSLATOR, ID_NAV_COMPUTER, display_menu_data);

	ai_handler->writeAIData(&display_menu_msg);
}

//Add a slider to an entry.
void CANMenuHandler::addSlider(const uint8_t position, String text, const uint8_t slider_pos, const uint8_t slider_max, const bool selected) {
	uint8_t slider_data[6 + text.length()];
	slider_data[0] = 0x2B;
	slider_data[1] = 0x54;
	slider_data[2] = position&0xFF;
	slider_data[3] = slider_pos;
	slider_data[4] = slider_max;
	slider_data[5] = selected;
	for(int i=0;i<text.length();i+=1)
		slider_data[i+6] = uint8_t(text[i]);

	AIData slider_msg(sizeof(slider_data), ID_CANSLATOR, ID_NAV_COMPUTER, slider_data);
	ai_handler->writeAIData(&slider_msg, parameter_list->computer_connected);
}

//Clear the menu. Return whether successful.
bool CANMenuHandler::clearMenu() {
	uint8_t clear_menu_data[] = {0x2B, 0x40};
	AIData clear_menu_msg(sizeof(clear_menu_data), ID_CANSLATOR, ID_NAV_COMPUTER, clear_menu_data);

	ai_handler->writeAIData(&clear_menu_msg);

	bool canceled = false;
	elapsedMillis cancel_wait;
	while(cancel_wait < 20) {
		AIData ai_msg;
		if(ai_handler->dataAvailable() > 0) {
			if(ai_handler->readAIData(&ai_msg)) {
				if(ai_msg.l >= 2 && ai_msg.receiver == ID_CANSLATOR && ai_msg.sender == ID_NAV_COMPUTER && ai_msg.data[0] == 0x2B && ai_msg.data[1] == 0x40) { //Menu cleared.
					ai_handler->sendAcknowledgement(ID_CANSLATOR, ai_msg.sender);
					canceled = true;
					break;
				}
			} else if(ai_msg.receiver == ID_CANSLATOR) {
				ai_handler->sendAcknowledgement(ID_CANSLATOR, ai_msg.sender);
				ai_handler->cacheMessage(&ai_msg);
			}
		}
	}

	if(!canceled)
		return false;

	return true;
}

//Create the main settings menu.
void CANMenuHandler::createMainSettingsMenu() {
	if(this->getActiveMenu() != ACTIVE_MENU_NONE) {
		if(!this->clearMenu())
			return;
	}

	MenuList main_settings = getMenu(MENU_INDEX_SETTINGS_MAIN, parameter_list->locale);

	const uint8_t setting_count = uint8_t(main_settings.size());
	if(!this->startMenu(setting_count, setting_count, false, main_settings.title))
		return;

	this->setActiveMenu(ACTIVE_MENU_SETTINGS_MAIN);

	for(int i=0;i<setting_count;i+=1) {
		if(main_settings.getGlobalIndex(i) == MENU_INDEX_SETTINGS_AMBIENT_LIGHTING && !parameter_list->has_ambient_lighting)
			continue;

		this->appendMenu(i, main_settings[i]);
	}

	this->displayMenu(1);
}

//Create the comfort/convenience settings menu.
void CANMenuHandler::createComfortConvenienceMenu(const bool light_sensor_connected, const bool rain_sensor_connected) {
	bool selection_settings[] = {true, parameter_list->parking_lights, parameter_list->wiper_door_off, parameter_list->auto_wiper};
	bool display_settings[] = {light_sensor_connected, light_sensor_connected, light_sensor_connected&rain_sensor_connected, light_sensor_connected&rain_sensor_connected};

	const active_menu_t last_menu = this->getActiveMenu();

	MenuList comfort_settings = getMenu(MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE, parameter_list->locale);

	const uint8_t setting_count = uint8_t(comfort_settings.size());
	if(last_menu != ACTIVE_MENU_SETTINGS_COMFORT_CONVENIENCE &&
		!this->startMenu(setting_count, setting_count, false, comfort_settings.title))
		return;

	this->setActiveMenu(ACTIVE_MENU_SETTINGS_COMFORT_CONVENIENCE);

	for(int i=0;i<comfort_settings.size(); i+=1) {
		if(!display_settings[i])
			continue;

		const menu_index_t setting = comfort_settings.getGlobalIndex(i);
		
		switch(setting) {
		case MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE_HEADLIGHT_TEMP:
			this->appendMenu(i, comfort_settings[i]);
			break;
		case MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE_PARKING_LIGHTS:
		case MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE_RAINSENSOR:
		case MENU_INDEX_SETTINGS_COMFORT_CONVENIENCE_WIPER_DOOR_OFF:
			this->appendMenu(i, String(selection_settings[i] ? "#RON " : "#ROF ") + comfort_settings[i]);
			break;
		default:
			break;
		}
	}

	if(last_menu != ACTIVE_MENU_SETTINGS_COMFORT_CONVENIENCE)
		this->displayMenu(1);
}

//Create the ambient lighting menu.
void CANMenuHandler::createAmbientLightingMenu(const bool slider_only) {
	const MenuList light_settings = getMenu(MENU_INDEX_SETTINGS_AMBIENT_LIGHTING, parameter_list->locale);
	const active_menu_t last_menu = this->getActiveMenu();
	const uint8_t setting_count = uint8_t(light_settings.size());

	if(last_menu != ACTIVE_MENU_SETTINGS_AMBIENT_LIGHTING &&
		!this->startMenu(setting_count, setting_count, false, light_settings.title))
		return;

	this->setActiveMenu(ACTIVE_MENU_SETTINGS_AMBIENT_LIGHTING);

	for(int i=0;i<light_settings.size();i+=1) {
		if(slider_only && light_settings.getGlobalIndex(i) != MENU_INDEX_SETTINGS_AMBIENT_LIGHTING_BRIGHTNESS)
			continue;

		String setting = light_settings[i];

		if(light_settings.getGlobalIndex(i) == MENU_INDEX_SETTINGS_AMBIENT_LIGHTING_DOOR)
			setting = parameter_list->ambient_light_enable_door ? "#RON " : "#ROF " + setting;
		else if(light_settings.getGlobalIndex(i) == MENU_INDEX_SETTINGS_AMBIENT_LIGHTING_INTERIOR)
			setting = parameter_list->ambient_light_enable_int ? "#RON " : "#ROF " + setting;

		this->appendMenu(i, setting);

		if(light_settings.getGlobalIndex(i) == MENU_INDEX_SETTINGS_AMBIENT_LIGHTING_BRIGHTNESS) {
			const uint8_t pos = long(parameter_list->ambient_light_brightness)*100/255;
			addSlider(i, String(pos) + "%", pos, 100, parameter_list->ambient_slide_active);
		}
	}

	if(last_menu != ACTIVE_MENU_SETTINGS_AMBIENT_LIGHTING)
		this->displayMenu(1);
}

//Create the headlight/temperature integration menu.
void CANMenuHandler::createHeadlightTempereatureMenu() {
	MenuList headlight_settings = getMenu(MENU_INDEX_SETTINGS_HEADLIGHT, parameter_list->locale);
	const active_menu_t last_menu = this->getActiveMenu();

	const uint8_t setting_count = uint8_t(headlight_settings.size());
	if(last_menu != ACTIVE_MENU_SETTINGS_HEADLIGHT_INTEGRATION &&
		!this->startMenu(setting_count, setting_count, false, headlight_settings.title))
		return;

	this->setActiveMenu(ACTIVE_MENU_SETTINGS_HEADLIGHT_INTEGRATION);

	bool selected[headlight_settings.size()];
	for(int i=0;i<headlight_settings.size();i+=1)
		selected[i] = (i == parameter_list->headlight_temp_setting);

	for(int i=0;i<headlight_settings.size();i+=1)
		this->appendMenu(i, String(selected[i] ? "#CON " : "#COF ") + headlight_settings[i]);

	if(last_menu != ACTIVE_MENU_SETTINGS_HEADLIGHT_INTEGRATION)
		this->displayMenu(1);
}

//Create the "secret" dev menu.
void CANMenuHandler::createSecretMenu() {
	const MenuList secret_menu = getMenu(MENU_INDEX_SECRET_MAIN, parameter_list->locale);
	const active_menu_t last_menu = this->getActiveMenu();

	const uint8_t setting_count = uint8_t(secret_menu.size());

	if(last_menu != ACTIVE_MENU_SECRET_MAIN &&
		!this->startMenu(setting_count, setting_count, false, secret_menu.title))
		return;

	this->setActiveMenu(ACTIVE_MENU_SECRET_MAIN);

	for(int i=0;i<secret_menu.size();i+=1)
		appendMenu(i, secret_menu[i]);

	if(last_menu != ACTIVE_MENU_SECRET_MAIN)
		this->displayMenu(1);
}