#include <stdint.h>
#include <string>

#include "Nav_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../AIBus_Handler.h"

#ifndef consumption_window_h
#define consumption_window_h

#define MSG46_ECON 0x01

#define CONSUMPTION_MAIN_AREA_Y MAIN_TITLE_AREA_Y + 65
#define CONSUMPTION_MAIN_AREA_HEIGHT 42

#define TRIP_INFO_COUNT 8

class Consumption_Window : public NavWindow {
public:
	Consumption_Window(AttributeList *attribute_list);
	~Consumption_Window();

	void drawWindow();
	void refreshWindow();
	bool handleAIBus(AIData* ai_d);

private:
	void requestConsumptionInfo();

	AIBusHandler* aibus_handler;

	TextBox* title_box;
	TextBox* split_info_box_left[TRIP_INFO_COUNT];
	TextBox* split_info_box_right[TRIP_INFO_COUNT];
};

#endif