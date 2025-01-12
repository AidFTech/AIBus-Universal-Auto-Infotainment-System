#include "Settings_Color_Picker.h"

Color_Picker_Window::Color_Picker_Window(AttributeList* attribute_list) : NavWindow(attribute_list) {
	this->title_block = new TextBox(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y, this->w-MAIN_TITLE_AREA_X, TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 55, &this->color_profile->text);
	title_block->setText("Color Selection");

	this->last_day_night_setting = attribute_list->day_night_settings;
	attribute_list->day_night_settings = DAY_NIGHT_DAY;
	setColorProfile(attribute_list->color_profile, *attribute_list->day_profile);

	this->color_menu = new NavMenu(this->attribute_list, 0, MAIN_TITLE_AREA_Y + 55, attribute_list->w*4/9, 40, 9, ALIGN_H_L, 36, 9, false, "");

	this->color_menu->setItem("Background", 0);
	this->color_menu->setItem("Text", 1);
	this->color_menu->setItem("Button", 2);
	this->color_menu->setItem("Selection", 3);
	this->color_menu->setItem("Headerbar", 4);
	this->color_menu->setItem("Outline", 5);

	this->color_menu->setSelected(1);

	selected_color = &this->attribute_list->color_profile->background;
	
	if(this->attribute_list->color_profile->background == this->attribute_list->color_profile->background2)
		solid_br = true;

	for(int i=0;i<SLIDER_COUNT;i+=1) {
		this->color_slider[i] = new NavSlider(attribute_list, attribute_list->w*5/9, getSliderY(i), attribute_list->w/3, 25, 255, 0, false);
		
		this->slider_value[i] = new TextBox(renderer, attribute_list->w*8/9 + 10, getSliderY(i), attribute_list->w/9, 25, ALIGN_H_L, ALIGN_V_M, 25, &this->color_profile->text);
		this->color_slider[i]->setValueText(this->slider_value[i]);
	}

	setSliderValues();
	this->setDayNightOption();
}

Color_Picker_Window::~Color_Picker_Window() {
	delete this->title_block;
	delete this->color_menu;

	for(int i=0;i<SLIDER_COUNT;i+=1) {
		delete this->color_slider[i];
		delete this->slider_value[i];
	}
}

void Color_Picker_Window::drawWindow() {
	this->title_block->drawText();
	this->color_menu->drawMenu();

	int slider_count = 3;
	if(color_option == COLOR_OPTION_BACKGROUND)
		slider_count = SLIDER_COUNT;

	for(int i=0;i<slider_count;i+=1)
		this->color_slider[i]->drawSlider();

	if(color_option_active && !color_slider_active) {
		uint8_t last_r, last_g, last_b, last_a;
		SDL_GetRenderDrawColor(renderer, &last_r, &last_g, &last_b, &last_a);

		AidFColorProfile* color_profile = attribute_list->color_profile;
		const uint32_t selection_color = color_profile->selection;

		const uint16_t selection_box_spacing = 10;
		
		SDL_SetRenderDrawColor(renderer, getRedComponent(selection_color), getGreenComponent(selection_color), getBlueComponent(selection_color), getAlphaComponent(selection_color));
		for(uint8_t i=0;i<5;i+=1) {
			SDL_Rect selection_rect{attribute_list->w/2 + i,
									getSliderY(color_element) - selection_box_spacing + i,
									attribute_list->w/2 - 2*i,
									25 + 2*selection_box_spacing - 2*i};
			SDL_RenderDrawRect(renderer, &selection_rect);
		}

		SDL_SetRenderDrawColor(renderer, last_r, last_g, last_b, last_a);
	}
}

void Color_Picker_Window::refreshWindow() {
	this->title_block->renderText();
	this->color_menu->refreshItems();

	for(uint8_t i=0;i<SLIDER_COUNT;i+=1) {
		this->slider_value[i]->renderText();
	}
}

bool Color_Picker_Window::handleAIBus(AIData* ai_d) {
	if(!this->active)
		return false;

	AIBusHandler* aibus_handler = this->attribute_list->aibus_handler;

	if(ai_d->sender == ID_NAV_SCREEN) {
		if(ai_d->l >= 3 && ai_d->data[0] == 0x32) { //Knob turn.
			const bool clockwise = (ai_d->data[2]&0x10) != 0;
			const uint8_t steps = ai_d->data[2]&0xF;

			if(!this->color_option_active) {
				if(!clockwise) {
					for(uint8_t i=0;i<steps;i+=1)
						this->color_menu->incrementSelected();
				} else {
					for(uint8_t i=0;i<steps;i+=1)
						this->color_menu->decrementSelected();
				}
			} else {
				if(!this->color_slider_active) {
					if(!clockwise) {
						for(uint8_t i=0;i<steps;i+=1)
							incrementColorElement();
					} else {
						for(uint8_t i=0;i<steps;i+=1)
							decrementColorElement();
					}
				} else {
					if(clockwise) {
						for(uint8_t i=0;i<steps;i+=1)
							incrementSlider();
					} else {
						for(uint8_t i=0;i<steps;i+=1)
							decrementSlider();
					}
				}
			}

			aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
			return true;
		} else if(ai_d->l >= 3 && ai_d->data[0] == 0x30) { //Button press.
			if(ai_d->data[1] >= 0x28 && ai_d->data[1] <= 0x29 && (ai_d->data[2] >> 6) == 0x2) {
				int8_t control = -1;
				switch(ai_d->data[1]) {
					case 0x28:
						control = NAV_UP;
						break;
					case 0x29:
						control = NAV_DOWN;
						break;
					default:
						return false;
				}

				if(!this->color_option_active) {
					this->color_menu->navigateSelected(control);
				} else {
					if(!this->color_slider_active) {
						if(control == NAV_UP)
							decrementColorElement();
						else if(control == NAV_DOWN)
							incrementColorElement();
					} else {

					}
				}

				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
				return true;
			} else {
				bool answered = false;
				const uint8_t control = ai_d->data[1], state = ai_d->data[2]>>6;
				if(control == 0x7 && state == 0x2) { //Enter button.
					this->handleEnterButton();
					answered = true;
				} else if((control == 0x27 || control == 0x51) && state == 0x2) { //Back button.
					if(control == 0x51 || !this->color_option_active) {
						this->attribute_list->next_window = NEXT_WINDOW_SETTINGS_COLOR;
						this->attribute_list->day_night_settings = last_day_night_setting;
						//TODO: Ping the Canslator.
					} else {
						if(!color_slider_active) {
							this->color_option_active = false;
							this->color_slider_active = false;
							color_menu->setSelected(color_option);
						} else {
							this->color_slider_active = false;
							this->color_slider[color_element]->setSelected(false);
						}
					}
					answered = true;
				} else if(control == 0x20 && state == 0x2) {
					 this->attribute_list->day_night_settings = last_day_night_setting;
				} else if(control == 0x26) //Audio button, to prevent it from being used.
					answered = true;
				
				if(answered)
					aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
				
				return answered;
			}
		}
	}

	return false;
}

void Color_Picker_Window::incrementColorElement() {
	uint8_t color_element_limit = 3;
	if(color_option == COLOR_OPTION_BACKGROUND)
		color_element_limit = SLIDER_COUNT;

	if(color_element < color_element_limit - 1)
		color_element += 1;
	else
		color_element = 0;
}

void Color_Picker_Window::decrementColorElement() {
	uint8_t color_element_limit = 3;
	if(color_option == COLOR_OPTION_BACKGROUND)
		color_element_limit = SLIDER_COUNT;

	if(color_element > 0)
		color_element -= 1;
	else
		color_element = color_element_limit - 1;
}

int16_t Color_Picker_Window::getSliderY(const uint8_t s) {
	if(s < 3)
		return MAIN_TITLE_AREA_Y + 60 + s*40;
	else
		return MAIN_TITLE_AREA_Y + 60 + (s+1)*40;
}

void Color_Picker_Window::handleEnterButton() {
	if(!color_option_active) {
		const int selected = this->color_menu->getSelected() - 1;

		if(selected < 0)
			return;

		if(selected >= 7) {
			switch(selected) {
			case 7:
				attribute_list->day_night_settings = DAY_NIGHT_DAY;
				setColorProfile(attribute_list->color_profile, *attribute_list->day_profile);
				setDayNightOption();
				break;
			case 8:
				attribute_list->day_night_settings = DAY_NIGHT_NIGHT;
				setColorProfile(attribute_list->color_profile, *attribute_list->night_profile);
				setDayNightOption();
				break;
			}
			setDayNightOption();
		} else {
			color_element = 0;

			color_option_active = true;
			color_slider_active = false;
			
			color_option = selected + 1;
			setSliderValues();
		}
	} else {
		if(!color_slider_active) {
			color_slider_active = true;
			color_slider[color_element]->setSelected(true);
		} else {
			color_slider_active = false;
			color_slider[color_element]->setSelected(false);
		}
	}
}

void Color_Picker_Window::setDayNightOption() {
	std::string day_sel = "#COF";
	if(this->attribute_list->day_night_settings == DAY_NIGHT_DAY)
		day_sel = "#CON";
	
	std::string night_sel = "#COF";
	if(this->attribute_list->day_night_settings == DAY_NIGHT_NIGHT)
		night_sel = "#CON";

	this->color_menu->setItem(day_sel + "  Day", 7);
	this->color_menu->setItem(night_sel + "  Night", 8);
	setSliderValues();

	solid_br = false;
	if(attribute_list->color_profile->background == attribute_list->color_profile->background2)
		solid_br = true;
}

void Color_Picker_Window::incrementSlider() {
	const uint8_t value = color_slider[color_element]->getValue();
	setSlider(value + 1, color_element);
}

void Color_Picker_Window::decrementSlider() {
	const uint8_t value = color_slider[color_element]->getValue();
	setSlider(value - 1, color_element);
}

void Color_Picker_Window::setSlider(const uint8_t slider_value, const uint8_t slider) {
	NavSlider* aff_slider = color_slider[slider];
	aff_slider->setValue(slider_value);
	
	AidFColorProfile* color_profile = this->attribute_list->color_profile;

	if(slider == 0) { //Set red.
		*this->selected_color &= 0xFFFFFF;
		*this->selected_color |= (aff_slider->getValue() << 24);
		if(color_option == COLOR_OPTION_BACKGROUND && solid_br) {
			color_profile->background2 &= 0xFFFFFF;
			color_profile->background2 |= (aff_slider->getValue() << 24);
		}
	} else if(slider == 1) { //Set green.
		*this->selected_color &= 0xFF00FFFF;
		*this->selected_color |= (aff_slider->getValue() << 16);
		if(color_option == COLOR_OPTION_BACKGROUND && solid_br) {
			color_profile->background2 &= 0xFF00FFFF;
			color_profile->background2 |= (aff_slider->getValue() << 16);
		}
	} else if(slider == 2) { //Set blue.
		*this->selected_color &= 0xFFFF00FF;
		*this->selected_color |= (aff_slider->getValue() << 8);
		if(color_option == COLOR_OPTION_BACKGROUND && solid_br) {
			color_profile->background2 &= 0xFFFF00FF;
			color_profile->background2 |= (aff_slider->getValue() << 8);
		}
	} else if(slider == 3 && color_option == COLOR_OPTION_BACKGROUND) { //Background red.
		solid_br = false;
		color_profile->background2 &= 0xFFFFFF;
		color_profile->background2 |= (aff_slider->getValue() << 24);
	} else if(slider == 4 && color_option == COLOR_OPTION_BACKGROUND) { //Background green.
		solid_br = false;
		color_profile->background2 &= 0xFF00FFFF;
		color_profile->background2 |= (aff_slider->getValue() << 16);
	} else if(slider == 5 && color_option == COLOR_OPTION_BACKGROUND) { //Background blue.
		solid_br = false;
		color_profile->background2 &= 0xFFFF00FF;
		color_profile->background2 |= (aff_slider->getValue() << 8);
	}

	if(color_option == COLOR_OPTION_BACKGROUND || slider >= 3)
		this->attribute_list->background_changed = true;
	else if(color_option == COLOR_OPTION_TEXT)
		this->attribute_list->text_changed = true;

	this->refreshDayNightProfile();
	if(this->attribute_list->background_changed)
		setSliderValues();
}

void Color_Picker_Window::refreshDayNightProfile() {
	if(attribute_list->day_night_settings == DAY_NIGHT_DAY)
		setColorProfile(attribute_list->day_profile, *attribute_list->color_profile);
	else if(attribute_list->day_night_settings == DAY_NIGHT_NIGHT)
		setColorProfile(attribute_list->night_profile, *attribute_list->color_profile);
}

void Color_Picker_Window::setSliderValues() {
	AidFColorProfile* color_profile = this->attribute_list->color_profile;

	switch(color_option) {
		case COLOR_OPTION_BACKGROUND:
			this->selected_color = &color_profile->background;
			break;
		case COLOR_OPTION_BUTTON:
			this->selected_color = &color_profile->button;
			break;
		case COLOR_OPTION_HEADERBAR:
			this->selected_color = &color_profile->headerbar;
			break;
		case COLOR_OPTION_OUTLINE:
			this->selected_color = &color_profile->outline;
			break;
		case COLOR_OPTION_SELECTION:
			this->selected_color = &color_profile->selection;
			break;
		case COLOR_OPTION_TEXT:
			this->selected_color = &color_profile->text;
			break;
		default:
			return;
	}
	
	if(color_profile->background != color_profile->background2)
		solid_br = false;
	else
		solid_br = true;

	color_slider[0]->setValue(getRedComponent(*this->selected_color));
	color_slider[1]->setValue(getGreenComponent(*this->selected_color));
	color_slider[2]->setValue(getBlueComponent(*this->selected_color));

	color_slider[3]->setValue(getRedComponent(color_profile->background2));
	color_slider[4]->setValue(getGreenComponent(color_profile->background2));
	color_slider[5]->setValue(getBlueComponent(color_profile->background2));
}
