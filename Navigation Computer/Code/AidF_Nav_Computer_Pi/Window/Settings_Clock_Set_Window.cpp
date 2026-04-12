#include "Settings_Clock_Set_Window.h"

SettingsClockSetWindow::SettingsClockSetWindow(AttributeList* attribute_list) : NavWindow(attribute_list), 
	title_box(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y, attribute_list->w-MAIN_TITLE_AREA_X, TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 55, &attribute_list->color_profile->text),
	msg_box(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y + 55, attribute_list->w - MAIN_TITLE_AREA_X, TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 26, &attribute_list->color_profile->text) {
	const uint8_t font_size = attribute_list->h/5 <= 255 ? attribute_list->h/5 : 255;

	title_box.setText(getString(LOCALE_STRING_SET_CLOCK, attribute_list->locale));
	msg_box.setText(getString(LOCALE_STRING_CANT_SET_CLOCK, attribute_list->locale));

	this->time_set_menu = new NavMenu(attribute_list, 0, this->h/2-font_size/2, this->w*3/10, font_size*7/5, 2, ALIGN_H_C, font_size, 1, true, "");
	this->colon = new TextBox(renderer, this->w*5/20, this->h/2-font_size/2, this->w/10, font_size*7/5, ALIGN_H_L, ALIGN_V_M, font_size, &attribute_list->color_profile->text);
	this->colon->setText(":");
	this->am_pm_text = new TextBox(renderer, this->w*3/5, this->h/2-font_size/2, this->w*3/10, font_size*7/5, ALIGN_H_C, ALIGN_V_M, font_size, &attribute_list->color_profile->text);

	setClockItems(attribute_list->hour, attribute_list->minute);
	time_set_menu->setSelected(1);

	auto_set = attribute_list->auto_clock && attribute_list->hour >= 0 && attribute_list->minute >= 0;
}

SettingsClockSetWindow::~SettingsClockSetWindow() {
	delete this->time_set_menu;
	delete this->am_pm_text;
	delete this->colon;
}

//Refresh the window.
void SettingsClockSetWindow::refreshWindow() {
	title_box.renderText();

	this->time_set_menu->refreshItems();
	this->am_pm_text->renderText();
	this->colon->renderText();
}

//Draw the window.
void SettingsClockSetWindow::drawWindow() {
	if(exited) {
		attribute_list->next_window = NEXT_WINDOW_SETTINGS_CLOCK;
		return;
	}

	if(selected == TIME_ITEM_NONE && (hour != attribute_list->hour || minute != attribute_list->minute)) {
		hour = attribute_list->hour;
		minute = attribute_list->minute;
		setClockItems(hour, minute);
	}

	title_box.drawText();
	if(auto_set)
		msg_box.drawText();

	this->time_set_menu->drawMenu();
	this->colon->drawText();
	this->am_pm_text->drawText();

	if(selected != TIME_ITEM_NONE) {
		int16_t x = 0, y = 0;

		time_set_menu->getSelectedIndexPosition(&x, &y);

		const int arrow_h = 24;

		x += MAIN_TITLE_AREA_X;
		y += arrow_h*6/5;

		SDL_RenderTriangle(renderer, NULL, x - arrow_h/2, y + arrow_h, x + arrow_h/2, y + arrow_h, x, y, getSDLColor(color_profile->selection));

		y += this->h*7/25 - 2*arrow_h*6/5;
		SDL_RenderTriangle(renderer, NULL, x - arrow_h/2, y - arrow_h, x + arrow_h/2, y - arrow_h, x, y, getSDLColor(color_profile->selection));
	}
}

//Exit the window.
void SettingsClockSetWindow::exitWindow() {
	exited = true;
	NavWindow::exitWindow();
}

//Handle an AIBus message.
bool SettingsClockSetWindow::handleAIBus(AIData* ai_d) {
	if(!this->active)
		return false;

	if(ai_d->sender == ID_NAV_COMPUTER)
		return false;

	SerialAIBusHandler* ai_handler = attribute_list->aibus_handler;

	if(this->selected == TIME_ITEM_NONE && this->time_set_menu->handleAIBus(ai_d)) {
		return true;
	} else if(ai_d->receiver == ID_NAV_COMPUTER) {
		if(ai_d->sender == ID_NAV_SCREEN) {
			if(ai_d->l >= 3 && ai_d->data[0] == 0x30) { //Button press.
				const uint8_t control = ai_d->data[1], state = ai_d->data[2]>>6;
				if(control == 0x7 && state == 2) {
					const int selected = time_set_menu->getSelected() - 1;
					if(selected < 0) {
						return true;
					}

					if(this->selected == TIME_ITEM_NONE && !auto_set) {
						switch(selected) {
						case TIME_ITEM_HOUR:
							this->selected = TIME_ITEM_HOUR;
							break;
						case TIME_ITEM_MINUTE:
							this->selected = TIME_ITEM_MINUTE;
							break;
						}
					} else {
						this->selected = TIME_ITEM_NONE;
						setTime();
					}
					return true;
				} else if((control == 0x28 || control == 0x29 || control == 0x2A || control == 0x2B) && state == 2) { //Directional buttons.
					if(auto_set || selected == TIME_ITEM_NONE)
						return false;

					switch(control) {
					case 0x2A:
					case 0x2B:
						selected = TIME_ITEM_NONE;
						setTime();
						time_set_menu->handleAIBus(ai_d);
						break;
					case 0x28:
						incrementTime(1, true);
						break;
					case 0x29:
						incrementTime(1, false);
						break;
					}

					return true;
				} else if((control == 0x27 || control == 0x51) && state == 2) { //Back button.
					if(this->selected == TIME_ITEM_NONE)
						attribute_list->next_window = NEXT_WINDOW_SETTINGS_CLOCK;
					else {
						this->selected = TIME_ITEM_NONE;
						setTime();
					}

					return true;
				} else if(control == 0x20 && state == 2) { //Home button.
					this->selected = TIME_ITEM_NONE;
					setTime();

					attribute_list->next_window = NEXT_WINDOW_MAIN;

					return true;
				} else if(control == 0x26 && selected != TIME_ITEM_NONE) { //Audio button.
					return true;
				} else if (selected != TIME_ITEM_NONE && state == 2){
					this->selected = TIME_ITEM_NONE;
					setTime();
					return true;
				}
			} else if(ai_d->l >= 3 && ai_d->data[0] == 0x32 && ai_d->data[1] == 0x7) { //Knob turn.
				if(auto_set || selected == TIME_ITEM_NONE)
					return false;

				const uint8_t steps = ai_d->data[2]&0xF;
				const bool cw = (ai_d->data[2]&0x10) != 0;
				
				incrementTime(steps, cw);

				return true;
			}
		}
	}
	return false;
}

//Increment the selected time element.
void SettingsClockSetWindow::incrementTime(const int steps, const bool up) {
	const int time_increment = up ? steps : -steps;
	int hour = this->hour, minute = this->minute;

	if(this->hour < 0 || this->minute < 0) {
		hour = 0;
		minute = 0;
	}

	switch(selected) {
	case TIME_ITEM_HOUR:
		hour += time_increment;
		break;
	case TIME_ITEM_MINUTE:
		minute += time_increment;
		break;
	default:
		selected = TIME_ITEM_NONE;
		break;
	}

	while(minute < 0)
		minute += 60;
	while(minute >= 60)
		minute -= 60;

	while(hour < 0)
		hour += 24;
	while(hour >= 24)
		hour -= 24;

	hour &= 0x7F;
	minute &= 0x7F;
	setClockItems(hour, minute);

	this->hour = hour;
	this->minute = minute;
}

//Set the menu items for the current time.
void SettingsClockSetWindow::setClockItems(const int8_t hour, const int8_t minute) {
	const bool display_12h = attribute_list->display_12h;

	if(hour >= 0 && minute >= 0) {
		int hour_display = hour;
		if(display_12h && hour >= 13)
			hour_display = hour - 12;
		else if(display_12h && hour == 0)
			hour_display = 12;

		time_set_menu->setItem(std::to_string(hour_display), TIME_ITEM_HOUR);
		time_set_menu->setItem(minute >= 10 ? std::to_string(minute) : "0" + std::to_string(minute), TIME_ITEM_MINUTE);
	} else {
		if(display_12h)
			time_set_menu->setItem("12", TIME_ITEM_HOUR);
		else
			time_set_menu->setItem("0", TIME_ITEM_HOUR);

		time_set_menu->setItem("00", TIME_ITEM_MINUTE);
	}

	if(display_12h) {
		if(hour < 12)
			am_pm_text->setText("AM");
		else
			am_pm_text->setText("PM");
	} else {
		am_pm_text->setText("");
	}
}

//Set the time via AIBus.
void SettingsClockSetWindow::setTime() {
	if(attribute_list->hour == hour && attribute_list->minute == minute)
		return;

	uint8_t time_data[] = {0xA1, 0x1F, 0x1, uint8_t(hour&0x1F), uint8_t(minute), 0x0};
	AIData time_msg(sizeof(time_data), ID_NAV_COMPUTER, 0xFF, time_data);
	attribute_list->aibus_handler->writeAIData(&time_msg, false);
}