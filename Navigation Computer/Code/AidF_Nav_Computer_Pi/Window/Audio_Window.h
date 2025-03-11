#include "../Window_Handler.h"
#include "../Menu/Nav_Menu.h"
#include "../AIBus_Handler.h"

#ifndef audio_window_h
#define audio_window_h

#define SUB_AREA_WIDTH 400

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

	void exitWindow();

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

	bool getSettingsMenuValid(const uint8_t sender);

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

	//Dimensions.
	int area_w = 400, area_h = 60;

	int title_area_x = 25;
	int title_area_y = 50;
	int main_area_y = title_area_y + 65;

	int sub_area_x = 150;
	int sub_area_y = 40;

	int main_area_height = 42;

	int sub_area_height = 30;

	int function_area_width = 800/6, function_area_height = 64;

	int full_area_w = 800 - title_area_x;
	int half_area_w = 800/2 - title_area_x;
};

#endif
