#include <stdint.h>

#include "../Window/Nav_Window.h"
#include "Vehicle_Info_Parameters.h"

class VehicleInfoWindow : public NavWindow {
public:
    VehicleInfoWindow(AttributeList *attribute_list, InfoParameters* info_parameters);

private:
    InfoParameters* info_parameters;
    TextBox* title_box;
};
