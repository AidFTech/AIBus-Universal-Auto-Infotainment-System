#include <stdint.h>
#include <time.h>

#include "../Window/Nav_Window.h"
#include "../Menu/Nav_Menu.h"

#include "Vehicle_Info_Parameters.h"
#include "../AidF_Color_Profile.h"

#ifndef vehicle_info_window_h
#define vehicle_info_window_h

#define COLOR_EV1 0xFFE97FFF
#define COLOR_EV2 0xFFD300FF
#define COLOR_REG1 0x7FFF7FFF
#define COLOR_REG2 0x0EE30EFF
#define COLOR_ENG1 0xFF6B54FF
#define COLOR_ENG2 0xDC2508FF

#define COLOR_ASSIST1 0x5454FFFF
#define COLOR_ASSIST2 0xA4A4FFFF

enum info_active_menu : uint8_t {
	INFO_ACTIVE_MENU_NONE,
	INFO_ACTIVE_MENU_MAIN,
	INFO_ACTIVE_MENU_PARAM,
};

#include "DRL_Img.h"
#include "SideMarkers_Img.h"
#include "LowBeam_Img.h"
#include "HighBeam_Img.h"
#include "FrontFog_Img.h"
#include "RearFog_Img.h"
#include "Silhouette_Img.h"

#include "Hybrid_Img.h"
#include "Power_Flow_Arrow.h"

#include "../Locale/Locale.h"

#include "../Saved_Settings.h"

#define PARAM_H 80

#define CHARGE_ASSIST_W 400
#define CHARGE_ASSIST_H 30

#define INFO_SETTING_COUNT 9

using namespace std;

class VehicleInfoWindow : public NavWindow {
public:
	VehicleInfoWindow(AttributeList *attribute_list, InfoParameters* info_parameters);
	~VehicleInfoWindow();

	void refreshWindow();
	void drawWindow();

	bool handleAIBus(AIData* ai_d);

private:
	InfoParameters* info_parameters;
	TextBox title_box;

	//Info display:
	TextBox* param_titles[PARAM_COUNT];
	TextBox* param_text[PARAM_COUNT];
	info_param* param_index;

	//Light display:
	SDL_Texture* drl_texture = NULL,
				*side_texture = NULL,
				*lowbeam_texture = NULL,
				*highbeam_texture = NULL,
				*frontfog_texture = NULL,
				*rearfog_texture = NULL;

	//Hybrid power flow:
	SDL_Texture* electric_motor_texture = NULL,
				*engine_texture = NULL;
	
	SDL_Texture* silhouette_texture = NULL, *silhouette_outline_texture = NULL;

	NavMenu* settings_menu = NULL;

	info_active_menu active_menu = INFO_ACTIVE_MENU_NONE;
	uint8_t active_param = 0;

	void handleEnterButton();
	void refreshParam(TextBox* title, TextBox* text, const info_param param);
	
	void createDefaultSettingsMenu();
	void createParamSettingsMenu(const uint8_t active_param);

	void saveParamSettings();
};

#endif
