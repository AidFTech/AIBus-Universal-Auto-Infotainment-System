#include "Nav_Window.h"

#include "../AidF_Color_Profile.h"
#include "../Serial_AIBus_Handler.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"

#include "../Symbol/Triangle.h"
#include "../Locale/Locale.h"

#ifndef settings_clock_set_window_h
#define settings_clock_set_window_h

enum time_item : int8_t {
	TIME_ITEM_NONE = -1,
	TIME_ITEM_HOUR,
	TIME_ITEM_MINUTE
};

class SettingsClockSetWindow : public NavWindow {
public:
	SettingsClockSetWindow(AttributeList* attribute_list);
	~SettingsClockSetWindow();

	void refreshWindow();
	void drawWindow();

	bool handleAIBus(AIData* ai_d);
private:
	TextBox title_box, msg_box;

	NavMenu* time_set_menu;
	TextBox* am_pm_text, *colon;

	time_item selected = TIME_ITEM_NONE;

	int8_t hour = -1, minute = -1;
	bool auto_set = false;

	void setClockItems(const int8_t hour, const int8_t minute);
	void incrementTime(const int steps, const bool up);
	void setTime();
};

#endif