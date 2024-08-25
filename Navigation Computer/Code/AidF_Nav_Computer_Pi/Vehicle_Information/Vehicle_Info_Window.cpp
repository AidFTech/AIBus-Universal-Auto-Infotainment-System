#include "Vehicle_Info_Window.h"

VehicleInfoWindow::VehicleInfoWindow(AttributeList *attribute_list, InfoParameters* info_parameters) : NavWindow(attribute_list) {
	this->info_parameters = info_parameters;

	title_box = new TextBox(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y, this->w-MAIN_TITLE_AREA_X, TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 40, &this->color_profile->text);
	if(info_parameters->hybrid_system_present)
		title_box->setText("Hybrid Power Flow");
	else
		title_box->setText("Vehicle Information");
}

VehicleInfoWindow::~VehicleInfoWindow() {

}

void VehicleInfoWindow::refreshWindow() {
	this->title_box->renderText();
}

void VehicleInfoWindow::drawWindow() {
	if(!this->active)
		return;
	
	this->title_box->drawText();
}