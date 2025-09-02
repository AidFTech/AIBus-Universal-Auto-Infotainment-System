#include "Window_Handler.h"

Window_Handler::Window_Handler(SDL_Renderer* renderer, Background* br, const uint16_t lw, const uint16_t lh, AidFColorProfile* active_profile, AIBusHandler* aibus_handler) {
	this->renderer = renderer;
	this->br = br;
	this->w = lw;
	this->h = lh;
	this->color_profile = active_profile;
	
	this->attribute_list = new AttributeList();

	this->attribute_list->br = this->br;
	this->attribute_list->color_profile = this->color_profile;
	this->attribute_list->w = this->w;
	this->attribute_list->h = this->h;
	this->attribute_list->renderer = this->renderer;

	this->active_window = nullptr;
	this->last_window = nullptr;

	this->aibus_handler = aibus_handler;
	this->attribute_list->aibus_handler = aibus_handler;

	this->vehicle_info_paramters = new InfoParameters();

	this->header_box[0] = new TextBox(renderer, CLOCK_SPACING, 0, this->w/3 - CLOCK_SPACING, CLOCK_HEIGHT, ALIGN_H_L, ALIGN_V_M, CLOCK_HEIGHT*6/7, &this->color_profile->text);
	this->header_box[1] = new TextBox(renderer, this->w/3, 0, this->w/3, CLOCK_HEIGHT, ALIGN_H_C, ALIGN_V_M, CLOCK_HEIGHT*6/7, &this->color_profile->text);
	this->header_box[2] = new TextBox(renderer, 2*this->w/3, 0, this->w/3 - CLOCK_SPACING, CLOCK_HEIGHT, ALIGN_H_R, ALIGN_V_M, CLOCK_HEIGHT*6/7, &this->color_profile->text);

	this->audio_header = new TextBox(renderer, 0, CLOCK_HEIGHT, this->w, CLOCK_HEIGHT, ALIGN_H_C, ALIGN_V_M, CLOCK_HEIGHT*6/7, &this->color_profile->text);
}

Window_Handler::~Window_Handler() {
	delete this->attribute_list;
	delete this->vehicle_info_paramters;

	delete this->header_box[0];
	delete this->header_box[1];
	delete this->header_box[2];

	delete this->audio_header;

	if(delete_last && last_window != NULL) {
		delete last_window;
		last_window = nullptr;
	}

	if(delete_active && active_window != NULL) {
		delete active_window;
		active_window = nullptr;
	}
}

void Window_Handler::drawWindow() {
	this->active_window->drawWindow();

	if(typeid(*this->active_window) != typeid(IntroWindow))
		this->drawClockHeader();
}

void Window_Handler::refresh() {
	this->active_window->refreshWindow();
	
	for(uint8_t i=0;i<3;i+=1)
		this->header_box[i]->renderText();
		
	this->audio_header->renderText();
}

void Window_Handler::clearWindow() {
	this->br->drawBackground(this->renderer, 0,0,this->w,this->h);
}

void Window_Handler::setText(std::string text, const uint8_t pos) {
	if(pos < 0 || pos > 2)
		return;
	
	this->header_box[pos]->setText(text);
}

//Set the audio header text.
void Window_Handler::setAudioText(std::string text) {
	this->audio_text = text;
	this->audio_header->setText(text);
}

AttributeList* Window_Handler::getAttributeList() {
	return this->attribute_list;
}

InfoParameters* Window_Handler::getVehicleInfo() {
	return this->vehicle_info_paramters;
}

AIBusHandler* Window_Handler::getAIBusHandler() {
	return this->aibus_handler;
}

Background* Window_Handler::getBackground() {
	return this->br;
}

uint16_t Window_Handler::getWidth() {
	return this->w;
}

uint16_t Window_Handler::getHeight() {
	return this->h;
}

SDL_Renderer* Window_Handler::getRenderer() {
	return this->renderer;
}

AidFColorProfile* Window_Handler::getColorProfile() {
	return this->color_profile;
}

NavWindow* Window_Handler::getLastWindow() {
	return this->last_window;
}

//Get the active window.
NavWindow* Window_Handler::getActiveWindow() {
	return this->active_window;
}

//Set the active window.
void Window_Handler::setActiveWindow(NavWindow* new_window, const bool delete_last) {
	if(delete_last &&
			this->last_window != new_window &&
			this->last_window != active_window &&
			this->last_window != NULL) {
		delete this->last_window;
	}

	this->last_window = this->active_window;

	if(this->active_window != NULL && this->active_window != nullptr)
		this->active_window->setActive(false);

	this->active_window = new_window;

	if(this->active_window != NULL && this->active_window != nullptr)
		this->active_window->setActive(true);
}

//Check the next window.
void Window_Handler::checkNextWindow(NavWindow* misc_window, NavWindow* audio_window, NavWindow* phone_window, NavWindow* main_window) {
	const int next_window = attribute_list->next_window;
	
	if(next_window != NEXT_WINDOW_NULL)
		this->active_window->exitWindow();

	const bool delete_last = this->last_window != audio_window && this->last_window != phone_window && this->last_window != main_window;
	this->delete_last = delete_last;

	if(next_window == NEXT_WINDOW_AUDIO) {
		audio_window->setActive(true);
		this->setActiveWindow(audio_window, delete_last);
		attribute_list->next_window = NEXT_WINDOW_NULL;
	} else if(next_window == NEXT_WINDOW_MAIN) {
		this->setActiveWindow(main_window, delete_last);
	} else if(next_window == NEXT_WINDOW_PHONE) {
		phone_window->setActive(true);
		this->setActiveWindow(phone_window, delete_last);
		attribute_list->next_window = NEXT_WINDOW_NULL;
	} else if(next_window == NEXT_WINDOW_CONSUMPTION) {
		misc_window = new Consumption_Window(attribute_list);
		this->setActiveWindow(misc_window, delete_last);
	} else if(next_window == NEXT_WINDOW_SETTINGS_MAIN) {
		misc_window = new Settings_Main_Window(attribute_list);
		this->setActiveWindow(misc_window, delete_last);
	} else if(next_window == NEXT_WINDOW_SETTINGS_DISPLAY) {
		misc_window = new Settings_Display_Window(attribute_list);
		this->setActiveWindow(misc_window, delete_last);
	} else if(next_window == NEXT_WINDOW_SETTINGS_COLOR) {
		misc_window = new Settings_Color_Window(attribute_list);
		this->setActiveWindow(misc_window, delete_last);
	} else if(next_window == NEXT_WINDOW_SETTINGS_COLOR_PICKER) {
		misc_window = new Color_Picker_Window(attribute_list);
		this->setActiveWindow(misc_window, delete_last);
	} else if(next_window == NEXT_WINDOW_SETTINGS_CLOCK) {
		misc_window = new Settings_Clock_Window(attribute_list);
		this->setActiveWindow(misc_window, delete_last);
	} else if(next_window == NEXT_WINDOW_SETTINGS_EXT) {
		misc_window = new Settings_Ext_Window(attribute_list);
		this->setActiveWindow(misc_window, delete_last);
	} else if(next_window == NEXT_WINDOW_VEHICLE_INFO) {
		misc_window = new VehicleInfoWindow(attribute_list, vehicle_info_paramters);
		this->setActiveWindow(misc_window, delete_last);
	} else if(next_window == NEXT_WINDOW_MIRROR) {
		misc_window = new MirrorWindow(attribute_list);
		this->setActiveWindow(misc_window, delete_last);
	} else if(next_window == NEXT_WINDOW_LAST) {
		misc_window = this->getLastWindow();
		this->setActiveWindow(misc_window, delete_last);
		misc_window->refreshWindow();
	} else if(next_window != NEXT_WINDOW_NULL) {
		attribute_list->next_window = NEXT_WINDOW_NULL;
	}

	this->delete_active = this->active_window != misc_window && this->active_window != audio_window && this->active_window != phone_window && this->active_window != main_window;

	if(next_window != NEXT_WINDOW_NULL) {
		attribute_list->next_window = NEXT_WINDOW_NULL;
	}
}

//Draw the clock header.
void Window_Handler::drawClockHeader() {
	uint8_t last_r, last_g, last_b, last_a;
	SDL_GetRenderDrawColor(renderer, &last_r, &last_g, &last_b, &last_a);

	const uint32_t headerbar_color = this->color_profile->headerbar, outline_color = this->color_profile->outline;
	
	const int header_height = this->audio_text.length() > 0 ? CLOCK_HEIGHT*2 : CLOCK_HEIGHT;

	SDL_Rect header_rect = {0,0,this->w,header_height};
	SDL_SetRenderDrawColor(renderer, getRedComponent(headerbar_color), getGreenComponent(headerbar_color), getBlueComponent(headerbar_color), getAlphaComponent(headerbar_color));
	SDL_RenderFillRect(renderer, &header_rect);
	SDL_SetRenderDrawColor(renderer, getRedComponent(outline_color), getGreenComponent(outline_color), getBlueComponent(outline_color), getAlphaComponent(outline_color));
	SDL_RenderDrawLine(renderer, 0, header_height, this->w, header_height);

	SDL_SetRenderDrawColor(renderer, last_r, last_g, last_b, last_a);

	for(uint8_t i=0;i<3;i+=1)
		this->header_box[i]->drawText();

	if(this->audio_text.length() > 0)
		this->audio_header->drawText();
}
