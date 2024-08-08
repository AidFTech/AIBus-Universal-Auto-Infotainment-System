#include "Consumption_Window.h"

Consumption_Window::Consumption_Window(AttributeList *attribute_list) : NavWindow(attribute_list) {
	this->aibus_handler = attribute_list->aibus_handler;
	this->requestConsumptionInfo();

	title_box = new TextBox(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y, this->w-MAIN_TITLE_AREA_X, TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 40, &this->color_profile->text);
	title_box->setText("Consumption");

	for(uint8_t i=0;i<TRIP_INFO_COUNT;i+=1) {
		split_info_box_left[i] = new TextBox(renderer, MAIN_TITLE_AREA_X, CONSUMPTION_MAIN_AREA_Y+i*CONSUMPTION_MAIN_AREA_HEIGHT, this->w-2*MAIN_TITLE_AREA_X, CONSUMPTION_MAIN_AREA_HEIGHT, ALIGN_H_L, ALIGN_V_M, 32, &this->color_profile->text);
		split_info_box_right[i] = new TextBox(renderer, this->w/2, CONSUMPTION_MAIN_AREA_Y+i*CONSUMPTION_MAIN_AREA_HEIGHT, this->w-MAIN_TITLE_AREA_X, CONSUMPTION_MAIN_AREA_HEIGHT, ALIGN_H_R, ALIGN_V_M, 32, &this->color_profile->text);
	}
}

Consumption_Window::~Consumption_Window() {
	delete title_box;

	for(uint8_t i=0;i<TRIP_INFO_COUNT;i+=1) {
		delete split_info_box_left[i];
		delete split_info_box_right[i];
	}
}

void Consumption_Window::requestConsumptionInfo() {
	AIData request_message(2, ID_NAV_COMPUTER, ID_CANSLATOR);

	request_message.data[0] = 0x45;
	request_message.data[1] = MSG46_ECON;

	aibus_handler->writeAIData(&request_message, attribute_list->canslator_connected);
}

void Consumption_Window::drawWindow() {
	title_box->drawText();

	for(uint8_t i=0;i<TRIP_INFO_COUNT;i+=1) {
		split_info_box_left[i]->drawText();
		split_info_box_right[i]->drawText();
	}
}

void Consumption_Window::refreshWindow() {
	this->title_box->renderText();

	for(uint8_t i=0;i<TRIP_INFO_COUNT;i+=1) {
		split_info_box_left[i]->renderText();
		split_info_box_right[i]->renderText();
	}
}

bool Consumption_Window::handleAIBus(AIData* ai_d) {
	if(ai_d->data[0] == 0x46 && ai_d->sender == ID_CANSLATOR) { //Trip information message.
		aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
		if(ai_d->data[1] == MSG46_ECON) {
			const uint8_t row = ai_d->data[2]&0xF;
			const bool right_column = (ai_d->data[2]&0x80) != 0;
			
			if(row >= TRIP_INFO_COUNT)
				return true;

			std::string entry_name = "";
			for(uint8_t i=0;i<ai_d->l-3;i+=1)
				entry_name += ai_d->data[i+3];

			if(right_column)
				split_info_box_right[row]->setText(entry_name);
			else
				split_info_box_left[row]->setText(entry_name);
		}
		return true;
	} else
		return false;
}
