#include <stdint.h>
#include <string>

#include "Nav_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../AIBus_Handler.h"

#ifndef settings_window_h
#define settings_window_h

#define SETTING_COUNT 10

class Settings_Window : public NavWindow {
public:
	Settings_Window(AttributeList *attribute_list, const uint16_t setting_count, std::string header, const int back_index);
	~Settings_Window();

	virtual void drawWindow();
	virtual void refreshWindow();
	virtual bool handleAIBus(AIData* ai_d);

protected:
	virtual void handleEnterButton();
	virtual void handleBackButton();
	
	virtual void clearMenu();

	virtual void resizeMenu(const uint16_t new_count);

	TextBox *title_block;
	NavMenu *settings_menu;
	
	int back_index = 0;

	bool allow_ext_menu = false;
	uint8_t ext_menu_sender = ID_NAV_COMPUTER;
};

#endif
