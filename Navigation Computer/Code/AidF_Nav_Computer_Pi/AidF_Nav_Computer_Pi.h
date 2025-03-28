#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <fstream>
#include <vector>
#include <time.h>
#include <pthread.h>

#include "AidF_Color_Profile.h"
#include "Window_Handler.h"
#include "AIBus_Handler.h"
#include "Ini_Context.h"

#include "Background/Nav_Background.h"
#include "Background/Nav_Solid_Background.h"
#include "Background/Nav_Gradient_Background.h"

#include "Window/Audio_Window.h"
#include "Window/Phone_Window.h"

#include "Window/Main_Menu_Window.h"
#include "Window/Consumption_Window.h"
#include "Window/Settings_Main_Window.h"
#include "Window/Settings_Display_Window.h"

#include "Vehicle_Information/Vehicle_Info_Parameters.h"
#include "Vehicle_Information/Power_Flow_Arrow.h"

#include "Socket/AMirror_Socket.h"

#ifndef aidf_nav_computer_pi_h
#define aidf_nav_computer_pi_h

#define DEFAULT_W 800
#define DEFAULT_H 480

#define AIBUS_WAIT 5

#define SOCKET_AIBUS_TEST

#define RESOLUTION_FILE "./AidF_Nav_Resolution.ini"

class AidF_Nav_Computer {
public:
	bool running = true;

	AidF_Nav_Computer(SDL_Window* window, const uint16_t lw, const uint16_t lh);
	~AidF_Nav_Computer();

	void loop();
	uint16_t getWidth();
	uint16_t getHeight();
	SDL_Renderer* getRenderer();

	AidFColorProfile* getColorProfile();
private:
	bool handleBroadcastMessage(AIData* ai_d);
	void setDayNight(const bool night);
	void setMirrorColors();

	void getBackground();

	uint8_t key_position = 1, door_position = 0;

	uint16_t lw, lh;
	bool night = false;

	bool* canslator_connected, *radio_connected, *mirror_connected;

	SDL_Renderer* renderer;
	AidFColorProfile active_color_profile, day_profile, night_profile;

	AIBusHandler* aibus_handler;

	Background* br;

	Window_Handler* window_handler;
	AttributeList* attribute_list;

	Audio_Window* audio_window;
	PhoneWindow* phone_window;
	Main_Menu_Window* main_window;
	NavWindow* misc_window;

	pthread_t socket_thread, frame_thread;
	SocketHandlerParameters socket_parameters;

	clock_t aibus_read_time = clock();
	
	bool vol_timer_enabled = false;
	clock_t vol_timer = clock();
};

void setup(AidF_Nav_Computer* nav_computer);
void loop(AidF_Nav_Computer* nav_computer);

int main(int argc, char* args[]);

void getResolution(int* w, int* h);
void saveResolution(const int w, const int h);

void *frameThread(void* frame_v);

#endif