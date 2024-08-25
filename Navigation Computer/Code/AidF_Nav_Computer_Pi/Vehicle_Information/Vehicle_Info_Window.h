#include <stdint.h>

#ifndef vehicle_info_window_h
#define vehicle_info_window_h

#include "../Window/Nav_Window.h"
#include "Vehicle_Info_Parameters.h"

class VehicleInfoWindow : public NavWindow {
public:
    VehicleInfoWindow(AttributeList *attribute_list, InfoParameters* info_parameters);
    ~VehicleInfoWindow();

    void refreshWindow();
    void drawWindow();

private:
    InfoParameters* info_parameters;
    TextBox* title_box;
};

#endif