#include <stdint.h>

#include "../Window/Nav_Window.h"

#include "Vehicle_Info_Parameters.h"

#ifndef vehicle_info_window_h
#define vehicle_info_window_h

#include "DRL_Img.h"
#include "SideMarkers_Img.h"
#include "LowBeam_Img.h"
#include "HighBeam_Img.h"
#include "FrontFog_Img.h"
#include "RearFog_Img.h"

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
};

#endif