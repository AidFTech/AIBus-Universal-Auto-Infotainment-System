#include <stdint.h>

#include "../Window_Handler.h"
#include "../Menu/Nav_Menu.h"
#include "../AIBus_Handler.h"

#include "../Locale/Locale.h"

#ifndef main_window_h
#define main_window_h

#define MAIN_TITLE_HEIGHT 50

enum main_menu_item : uint8_t {
	MAIN_MENU_ITEM_NAVIGATION,
	MAIN_MENU_ITEM_AUDIO,
	MAIN_MENU_ITEM_PHONE,
	MAIN_MENU_ITEM_SETTINGS = 5,
	MAIN_MENU_ITEM_MONITOR_OFF,
	MAIN_MENU_ITEM_MIRROR = 9,
	MAIN_MENU_ITEM_INFORMATION,
	MAIN_MENU_ITEM_CONSUMPTION
};

static const main_menu_item MENU_INDEX[] = {
	MAIN_MENU_ITEM_NAVIGATION,
	MAIN_MENU_ITEM_AUDIO,
	MAIN_MENU_ITEM_PHONE,
	MAIN_MENU_ITEM_SETTINGS,
	MAIN_MENU_ITEM_MONITOR_OFF,
	MAIN_MENU_ITEM_CONSUMPTION,
	MAIN_MENU_ITEM_INFORMATION,
	MAIN_MENU_ITEM_MIRROR,
};

class Main_Menu_Window : public NavWindow {
public:
	Main_Menu_Window(AttributeList *attribute_list);

	void drawWindow();
	void refreshWindow();

	bool handleAIBus(AIData* msg);
private:
	TextBox title_box;
	NavMenu main_menu;

	void setMainMenu();
	void interpretMenuChange(AIData* ai_b);
	void handleEnterButton();
};

#endif
