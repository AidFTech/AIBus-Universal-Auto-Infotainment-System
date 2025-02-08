#include "Mirror_Window.h"

MirrorWindow::MirrorWindow(AttributeList *attribute_list) : NavWindow(attribute_list) {
	title_box = new TextBox(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y, this->w-MAIN_TITLE_AREA_X, TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 50, &this->color_profile->text);
	title_box->setText("Phone Mirror");

	message_box = new TextBox(renderer, 25, 50 + 42, 800, 60, ALIGN_H_L, ALIGN_V_M, 36, &this->color_profile->text);

	if(this->attribute_list->phone_type != 0) {
		if(this->attribute_list->phone_name.length() > 0)
			message_box->setText("Waiting for " + this->attribute_list->phone_name + ".");
		else
			message_box->setText("Waiting for phone to connect.");
	} else {
		message_box->setText("Phone not connected.");
	}

	this->writeConnectDisconnectMessage(true);
}

MirrorWindow::~MirrorWindow() {
	this->attribute_list->phone_active = false;

	delete title_box;
	delete message_box;
}

void MirrorWindow::refreshWindow() {
	if(this->active && attribute_list->phone_type != 0)
		this->writeConnectDisconnectMessage(true);

	if(this->attribute_list->phone_type != 0) {
		if(this->attribute_list->phone_name.length() > 0)
			message_box->setText("Waiting for " + this->attribute_list->phone_name + ".");
		else
			message_box->setText("Waiting for phone to connect.");
	} else {
		message_box->setText("Phone not connected.");
	}

	this->title_box->renderText();
	this->message_box->renderText();
}

void MirrorWindow::drawWindow() {
	if(!this->active)
		return;

	this->title_box->drawText();
	this->message_box->drawText();
}

void MirrorWindow::exitWindow() {
	this->writeConnectDisconnectMessage(false);
}

//Send the connect or disconnect message to the mirror.
void MirrorWindow::writeConnectDisconnectMessage(const bool connect) {
	if((attribute_list->phone_active && connect) || (!connect && !attribute_list->phone_active))// //Phone is already active, we don't need to send this.
		return;

	uint8_t connect_data[] = {0x48, 0x8E, 0x0};
	AIData connect_msg(sizeof(connect_data), ID_NAV_COMPUTER, ID_ANDROID_AUTO);

	if(connect)
		connect_data[2] = 0x1;

	connect_msg.refreshAIData(connect_data);
	attribute_list->aibus_handler->writeAIData(&connect_msg, attribute_list->mirror_connected);

	attribute_list->phone_active = connect;
}