#include "Mirror_Window.h"

MirrorWindow::MirrorWindow(AttributeList *attribute_list) : NavWindow(attribute_list), 
	title_box(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y, this->w-MAIN_TITLE_AREA_X, TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 50, &this->color_profile->text),
	message_box(renderer, 25, 50 + 42, 800, 60, ALIGN_H_L, ALIGN_V_M, 36, &this->color_profile->text) {

	title_box.setText(getString(LOCALE_STRING_MIRROR, attribute_list->locale));

	if(this->attribute_list->phone_type != 0) {
		if(this->attribute_list->phone_name.length() > 0)
			message_box.setText(getString(LOCALE_STRING_MIRROR_WAITING_1, attribute_list->locale) +
								this->attribute_list->phone_name
								+ getString(LOCALE_STRING_MIRROR_WAITING_2, attribute_list->locale));
		else
			message_box.setText(getString(LOCALE_STRING_MIRROR_WAITING_GENERIC, attribute_list->locale));
	} else
		message_box.setText(getString(LOCALE_STRING_MIRROR_NOT_CONNECTED, attribute_list->locale));

	this->writeConnectDisconnectMessage(true);
}

MirrorWindow::~MirrorWindow() {
	this->attribute_list->phone_active = false;
}

void MirrorWindow::refreshWindow() {
	if(this->active && attribute_list->phone_type != 0)
		this->writeConnectDisconnectMessage(true);

	if(this->attribute_list->phone_type != 0) {
		if(this->attribute_list->phone_name.length() > 0)
			message_box.setText(getString(LOCALE_STRING_MIRROR_WAITING_1, attribute_list->locale) +
								this->attribute_list->phone_name
								+ getString(LOCALE_STRING_MIRROR_WAITING_2, attribute_list->locale));
		else
			message_box.setText(getString(LOCALE_STRING_MIRROR_WAITING_GENERIC, attribute_list->locale));
	} else
		message_box.setText(getString(LOCALE_STRING_MIRROR_NOT_CONNECTED, attribute_list->locale));

	this->title_box.renderText();
	this->message_box.renderText();
}

void MirrorWindow::drawWindow() {
	if(!this->active)
		return;

	this->title_box.drawText();
	this->message_box.drawText();
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

	if(connect) {
		connect_data[2] = 0x1;
	} else {

	}

	connect_msg.refreshAIData(connect_data);
	attribute_list->aibus_handler->writeAIData(&connect_msg, attribute_list->mirror_connected);

	attribute_list->phone_active = connect;
}