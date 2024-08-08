#include "../Window_Handler.h"
#include "../Menu/Nav_Menu.h"
#include "../AIBus_Handler.h"

#include <iostream>

#ifndef audio_window_h
#define audio_window_h

#define AREA_W 400
#define AREA_H 60

#define TITLE_AREA_X 25
#define TITLE_AREA_Y 50
#define MAIN_AREA_Y TITLE_AREA_Y + 65

#define SUB_AREA_X 150
#define SUB_AREA_Y 40

#define MAIN_AREA_HEIGHT 42

#define SUB_AREA_WIDTH AREA_W
#define SUB_AREA_HEIGHT 30

#define FUNCTION_AREA_WIDTH 800/6
#define FUNCTION_AREA_HEIGHT 64

#define FULL_AREA_W 800 - TITLE_AREA_X
#define HALF_AREA_W 800/2 - TITLE_AREA_X

#define MAIN_AREA_GROUP 0
#define SUB_AREA_GROUP 1
#define FUNCTION_AREA_GROUP 2
#define BUTTON_AREA_GROUP 0xB

class Audio_Window : public NavWindow {
public:
	Audio_Window(AttributeList *attribute_list);
	~Audio_Window();

	void drawWindow();
	void refreshWindow();

	bool handleAIBus(AIData* msg);
	void setText(const uint8_t group, const uint8_t area, std::string text);
private:
	void fillText(const uint8_t group, const uint8_t area);
	void interpretAudioScreenChange(AIData* ai_b);
	void refreshAudioScreen();

	void interpretMenuChange(AIData* ai_b);
	void handleEnterButton();

	void initializeSettingsMenu(AIData* ai_b);
	void sendMenuClose();
	void sendMenuClose(const uint8_t receiver);

	std::string main_area_text[6];
	TextBox* main_area_box[6];
	bool main_area_change[6];

	std::string subtitle_area_text[3];
	TextBox* subtitle_area_box[3];
	bool subtitle_area_change[3];

	std::string function_area_text[6];
	TextBox* function_area_box[6];
	bool function_area_change[6];

	NavMenu* audio_menu, *settings_menu = NULL;
	uint8_t settings_menu_sender = ID_RADIO;
	bool settings_menu_active = false, settings_menu_prep = false;
};

#endif
