#include "Text_Handler.h"

TextHandler::TextHandler(ClientAIBusHandler* aibus_handler, ParameterList* parameters) {
	this->aibus_handler = aibus_handler;
	this->parameters = parameters;
}

//Write the given text to the group and area on the audio window of the nav screen.
void TextHandler::writeAudioWindowText(string text, const uint8_t group, const uint8_t area, const bool refresh) {
	uint8_t text_data[text.length() + 3];
	text_data[0] = 0x23;
	text_data[1] = 0x60 | (group&0xF) | (refresh ? 0x10 : 0x0);
	text_data[2] = area;
	for(int i=0;i<text.length();i+=1)
		text_data[i+3] = uint8_t(text[i]);

	AIData text_msg(sizeof(text_data), ID_PHONE, ID_NAV_COMPUTER, text_data);
	aibus_handler->writeAIData(&text_msg);
}

//Clear the phone window on the nav screen.
void TextHandler::clearPhoneWindow() {
	for(int i=0;i<6;i+=1)
		writePhoneWindowText("", 0, i);
	for(int i=0;i<3;i+=1)
		writePhoneWindowText("", 1, i);
	for(int i=0;i<5;i+=1)
		writeSideMenuText("", i);
}

//Write the given text to the following group and area on the nav screen.
void TextHandler::writePhoneWindowText(string text, const uint8_t group, const uint8_t area) {
	uint8_t text_data[text.length() + 3];
	text_data[0] = 0x21;
	text_data[1] = 0xA5;
	text_data[2] = ((group&0xF) << 4) | (area&0xF);
	for(int i=0;i<text.length();i+=1)
		text_data[i+3] = uint8_t(text[i]);

	AIData text_msg(sizeof(text_data), ID_PHONE, ID_NAV_COMPUTER, text_data);
	aibus_handler->writeAIData(&text_msg);
}

//Write the given text to the nav computer header.
void TextHandler::writeNavHeaderText(string text) {
	uint8_t header_data[text.length() + 2];
	header_data[0] = 0x22;
	header_data[1] = 0x61;
	for(int i=0;i<text.length();i+=1)
		header_data[i+2] = uint8_t(text[i]);

	AIData header_msg(sizeof(header_data), ID_PHONE, ID_NAV_COMPUTER, header_data);
	aibus_handler->writeAIData(&header_msg);
}

//Write the given text to the side menu in the phone window.
void TextHandler::writeSideMenuText(string text, const uint8_t entry) {
	writePhoneWindowText(text, 0xB, entry);
}

//Write metadata information to the source.
void TextHandler::writeMetadata(string data, const uint8_t recipient, const uint8_t line) {
	uint8_t meta_data[data.length() + 3];
	meta_data[0] = 0x23;
	meta_data[1] = 0x60 | (line&0xF);
	meta_data[2] = 0x1;
	for(int i=0;i<data.length();i+=1)
		meta_data[i+3] = uint8_t(data[i]);

	AIData meta_msg(sizeof(meta_data), ID_PHONE, recipient, meta_data);

	bool ack = true;
	if(recipient == ID_RADIO && !parameters->radio_connected)
		ack = false;
	else if(recipient == ID_IMID_SCR && (parameters->imid_char <= 0 || parameters->imid_lines <= 0))
		ack = false;
	else if(recipient == 0xFF)
		ack = false;

	aibus_handler->writeAIData(&meta_msg, ack);
}

//Clear the current menu.
void TextHandler::clearMenu() {
	uint8_t clear_menu_data[] = {0x2B, 0x40};
	AIData clear_menu_msg(sizeof(clear_menu_data), ID_PHONE, ID_NAV_COMPUTER, clear_menu_data);

	aibus_handler->writeAIData(&clear_menu_msg);

	parameters->current_menu = BTA_MENU_NONE;
}

//Create the list of paired devices.
void TextHandler::createDeviceListMenu(vector<string> device_names) {
	MenuList device_menu = getMenu(MENU_INDEX_DEVICE_LIST, parameters->locale);

	const uint8_t device_count = device_names.size() <= 255 ? device_names.size() : 255;

	this->createMenu(false, device_count, device_count, false, device_menu.title);

	for(int i=0;i<device_count;i+=1)
		this->appendMenu(i, device_names[i]);

	displayMenu(1);
	parameters->current_menu = BTA_MENU_DEVICES;
}

//Create a new menu.
void TextHandler::createMenu(const bool audio, const uint8_t count, const uint8_t rows, const bool loop, string title) {
	uint8_t start_menu_data[title.length() + 12];
	
	unsigned int div = count/rows;
	if(count%rows != 0)
		div += 1;

	const uint16_t x = 0, y = audio ? 140 : 40, width = parameters->screen_w/div;
	
	start_menu_data[0] = 0x2B;
	start_menu_data[1] = audio ? 0x5A : 0x50;
	start_menu_data[2] = rows&0x7F;
	start_menu_data[3] = count;
	start_menu_data[4] = (x&0xFF00) >> 8;
	start_menu_data[5] = x&0xFF;
	start_menu_data[6] = (y&0xFF00) >> 8;
	start_menu_data[7] = y&0xFF;
	start_menu_data[8] = (width&0xFF00)>>8;
	start_menu_data[9] = width&0xFF;
	start_menu_data[10] = 0x0;
	start_menu_data[11] = audio ? 0x23 : 40;
	
	for(unsigned int i=0;i<title.length();i+=1)
		start_menu_data[i+12] = uint8_t(title[i]);
	
	if(loop)
		start_menu_data[2] |= 0x80;
	
	AIData start_menu_msg(sizeof(start_menu_data), ID_PHONE, ID_NAV_COMPUTER, start_menu_data);
	aibus_handler->writeAIData(&start_menu_msg);
}

//Append to the displayed menu.
void TextHandler::appendMenu(const uint8_t position, const string text) {
	uint8_t append_menu_data[text.length() + 3];
	
	append_menu_data[0] = 0x2B;
	append_menu_data[1] = 0x51;
	append_menu_data[2] = position;
	
	for(unsigned int i=0;i<text.length();i+=1)
		append_menu_data[i+3] = uint8_t(text[i]);
	
	AIData append_menu_msg(sizeof(append_menu_data), ID_PHONE, ID_NAV_COMPUTER, append_menu_data);
	
	aibus_handler->writeAIData(&append_menu_msg);
}

//Display the menu.
void TextHandler::displayMenu(const uint8_t selected) {
	uint8_t display_menu_data[] = {0x2B, 0x52, selected};
	AIData display_menu_msg(sizeof(display_menu_data), ID_PHONE, ID_NAV_COMPUTER, display_menu_data);

	aibus_handler->writeAIData(&display_menu_msg);
}
