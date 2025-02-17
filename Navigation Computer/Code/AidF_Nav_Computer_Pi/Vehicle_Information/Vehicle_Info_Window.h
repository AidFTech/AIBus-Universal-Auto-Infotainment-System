#include <stdint.h>
#include <time.h>

#include "../Window/Nav_Window.h"

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

#include "DRL_Img.h"
#include "SideMarkers_Img.h"
#include "LowBeam_Img.h"
#include "HighBeam_Img.h"
#include "FrontFog_Img.h"
#include "RearFog_Img.h"
#include "Silhouette_Img.h"

#include "Hybrid_Img.h"
#include "Power_Flow_Arrow.h"

class VehicleInfoWindow : public NavWindow {
public:
	VehicleInfoWindow(AttributeList *attribute_list, InfoParameters* info_parameters);
	~VehicleInfoWindow();

	void refreshWindow();
	void drawWindow();

private:
	InfoParameters* info_parameters;
	TextBox* title_box;

	SDL_Texture* drl_texture = NULL,
				*side_texture = NULL,
				*lowbeam_texture = NULL,
				*highbeam_texture = NULL,
				*frontfog_texture = NULL,
				*rearfog_texture = NULL;

	SDL_Texture* electric_motor_texture = NULL,
				*engine_texture = NULL;
	
	SDL_Texture* silhouette_texture = NULL, *silhouette_outline_texture = NULL;
};

#endif
