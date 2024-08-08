#include "../Window_Handler.h"
#include "../Menu/Nav_Menu.h"
#include "../AIBus_Handler.h"

#ifndef main_window_h
#define main_window_h

#define MAIN_TITLE_HEIGHT 50

class Main_Menu_Window : public NavWindow {
public:
	Main_Menu_Window(AttributeList *attribute_list);
	~Main_Menu_Window();

	void drawWindow();
	void refreshWindow();

	bool handleAIBus(AIData* msg);
private:
	TextBox* title_box;
	NavMenu* main_menu;

	void setMainMenu();
	void interpretMenuChange(AIData* ai_b);
	void handleEnterButton();
};

#endif
