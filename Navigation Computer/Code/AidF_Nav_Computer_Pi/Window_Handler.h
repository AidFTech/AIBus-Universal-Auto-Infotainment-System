#include <SDL2/SDL.h>
#include <string>

#include "Background/Nav_Background.h"
#include "AidF_Color_Profile.h"
#include "Text_Box.h"
#include "AIBus/AIBus.h"
#include "Window/Nav_Window.h"
#include "Window/Attribute_List.h"
#include "AIBus_Handler.h"

#include "Window/Consumption_Window.h"
#include "Window/Settings_Main_Window.h"
#include "Window/Settings_Display_Window.h"
#include "Window/Settings_Color_Window.h"
#include "Window/Settings_Color_Picker.h"

#include "Vehicle_Information/Vehicle_Info_Parameters.h"
#include "Vehicle_Information/Vehicle_Info_Window.h"

#include "Window/Mirror_Window.h"

#ifndef nav_window_handler_h
#define nav_window_handler_h

#define CLOCK_HEIGHT 40
#define CLOCK_SPACING 25

class Window_Handler {
public:
	Window_Handler(SDL_Renderer* renderer, Background* br, const uint16_t lw, const uint16_t lh, AidFColorProfile* active_profile, AIBusHandler* aibus_handler);
	~Window_Handler();

	void drawWindow();
	void clearWindow();

	Background* getBackground();
	uint16_t getWidth();
	uint16_t getHeight();

	void setText(std::string text, const uint8_t pos);

	void refresh();

	SDL_Renderer* getRenderer();
	AidFColorProfile* getColorProfile();

	AttributeList* getAttributeList();
	InfoParameters* getVehicleInfo();

	AIBusHandler* getAIBusHandler();
	
	NavWindow* getLastWindow();
	NavWindow* getActiveWindow();
	void setActiveWindow(NavWindow* new_window);

	void checkNextWindow(NavWindow* misc_window, NavWindow* audio_window, NavWindow* phone_window, NavWindow* main_window);
private:
	void drawClockHeader();

	SDL_Renderer* renderer;
	Background* br;
	AidFColorProfile* color_profile;

	uint16_t w, h;

	TextBox* header_box[3];
	
	NavWindow* active_window, *last_window;
	AttributeList *attribute_list;
	InfoParameters *vehicle_info_paramters;

	AIBusHandler* aibus_handler;
};

#endif
