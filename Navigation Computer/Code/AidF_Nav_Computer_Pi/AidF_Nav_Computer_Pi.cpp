#include "AidF_Nav_Computer_Pi.h"

//Must be compiled with options -lSDL2 -lSDL2_ttf -lpigpio -lrt

AidF_Nav_Computer::AidF_Nav_Computer(SDL_Window* window, const uint16_t lw, const uint16_t lh) {
	this->lw = lw;
	this->lh = lh;

	this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	
	this->night_profile.background = DEFAULT_BACKGROUND_NIGHT;
	this->night_profile.background2 = DEFAULT_BACKGROUND_NIGHT;
	this->night_profile.text = DEFAULT_TEXT_NIGHT;
	this->night_profile.button = DEFAULT_BUTTON_NIGHT;
	this->night_profile.selection = DEFAULT_SELECTION_NIGHT;
	this->night_profile.headerbar = DEFAULT_HEADERBAR_NIGHT;
	this->night_profile.outline = DEFAULT_OUTLINE_NIGHT;

	//TODO: Read in a file for the last saved color profile.
	this->getBackground();

	#ifdef RPI_UART
	this->aibus_handler = new AIBusHandler("/dev/ttyAMA0");
	#else
	this->aibus_handler = new AIBusHandler();
	#endif

	this->window_handler = new Window_Handler(this->renderer, this->br, this->lw, this->lh, &this->active_color_profile, this->aibus_handler);
	this->attribute_list = window_handler->getAttributeList();

	this->attribute_list->day_profile = &this->day_profile;
	this->attribute_list->night_profile = &this->night_profile;

	audio_window = new Audio_Window(attribute_list);
	phone_window = new PhoneWindow(attribute_list);
	main_window = new Main_Menu_Window(attribute_list);
	this->window_handler->setActiveWindow(main_window);
	this->window_handler->setText("--:--", 0);

	this->canslator_connected = &attribute_list->canslator_connected;
	this->radio_connected = &attribute_list->radio_connected;

	this->socket_parameters.running = &this->running;
	this->socket_parameters.ai_serial = aibus_handler->getPortPointer();

	pthread_create(&socket_thread, NULL, socketThread, (void *)&socket_parameters);
}

AidF_Nav_Computer::~AidF_Nav_Computer() {
	SDL_DestroyRenderer(this->renderer);
	delete this->br;
	delete this->aibus_handler;
	delete this->window_handler;

	delete audio_window;
	delete main_window;
	delete misc_window;
	delete phone_window;

	pthread_join(socket_thread, NULL);
}

void AidF_Nav_Computer::loop() {
	SDL_Event event;
	if(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
				if(event.key.keysym.sym == SDLK_ESCAPE)
					running = false;
				break;
		}
	}
	
	this->window_handler->checkNextWindow(misc_window, audio_window, phone_window, main_window);

	if(attribute_list->day_night_settings == DAY_NIGHT_DAY) 
		setDayNight(false);
	else if(attribute_list->day_night_settings == DAY_NIGHT_NIGHT)
		setDayNight(true);

	if(attribute_list->background_changed) {
		attribute_list->background_changed = false;
		this->getBackground();
	}
	
	if(attribute_list->text_changed) {
		attribute_list->text_changed = false;
		
		if(this->audio_window != NULL)
			this->audio_window->refreshWindow();
		if(this->phone_window != NULL)
			this->phone_window->refreshWindow();
		if(this->main_window != NULL)
			this->main_window->refreshWindow();
		this->window_handler->refresh();
	}

	this->br->drawBackground(renderer, 0, 0, lw, lh);
	this->window_handler->drawWindow();
	SDL_RenderPresent(renderer);

	AIData ai_msg;
	do {
		if(!this->aibus_handler->getConnected() || this->aibus_handler->getAvailableBytes() > 0) {
			if(this->aibus_handler->readAIData(&ai_msg)) {
				aibus_read_time = clock();

				{
					uint8_t ai_bytes[ai_msg.l + 4];
					ai_msg.getBytes(ai_bytes);

					SocketMessage ai_sock_msg(OPCODE_AIBUS_SEND, sizeof(ai_bytes));
					ai_sock_msg.refreshSocketData(ai_bytes);
					writeSocketMessage(&ai_sock_msg, socket_parameters.client_socket);
				}

				if(!*canslator_connected && ai_msg.sender == ID_CANSLATOR)
					*canslator_connected = true;

				if(!*radio_connected && ai_msg.sender == ID_RADIO)
					*radio_connected = true;

				if(ai_msg.sender == ID_NAV_COMPUTER)
					continue;

				if(ai_msg.sender != ID_NAV_COMPUTER && (ai_msg.receiver == ID_NAV_COMPUTER || ai_msg.receiver == 0xFF) && ai_msg.l >= 1 && ai_msg.data[0] != 0x80) {
					bool answered = false;

					if(ai_msg.receiver == 0xFF && ai_msg.data[0] == 0xA1)
						answered = handleBroadcastMessage(&ai_msg);

					if(!answered)
						answered = audio_window->handleAIBus(&ai_msg);
					if(!answered)
						answered = phone_window->handleAIBus(&ai_msg);
					if(!answered && this->window_handler->getActiveWindow() != NULL  && this->window_handler->getActiveWindow() != audio_window)
						answered = this->window_handler->getActiveWindow()->handleAIBus(&ai_msg);
					if(!answered) { //Handle the AIBus message directly.
						if(ai_msg.receiver == ID_NAV_COMPUTER)
							this->aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_msg.sender);

						if(ai_msg.sender == ID_NAV_SCREEN && ai_msg.data[0] == 0x30) { //Button press.
							const uint8_t button = ai_msg.data[1], state = ai_msg.data[2]>>6;
							if(button == 0x26 && state == 0x2) { //Audio button.
								this->window_handler->getAttributeList()->next_window = NEXT_WINDOW_AUDIO;
							} else if(button == 0x50 && state == 0x2) { //Phone button.
								this->window_handler->getAttributeList()->next_window = NEXT_WINDOW_PHONE;
							} else if(button == 0x20 && state == 0x2) { //Home button.
								this->window_handler->getAttributeList()->next_window = NEXT_WINDOW_MAIN;
							}
						} else if(ai_msg.sender == ID_CANSLATOR && ai_msg.l >= 1 && ai_msg.data[0] == 0x11) { //Light info.
							InfoParameters* info_parameters = window_handler->getVehicleInfo();
							setLightState(&ai_msg, info_parameters);
						} else if(ai_msg.sender == ID_PHONE && ai_msg.l >= 3 && ai_msg.data[0] == 0x21 && ai_msg.data[1] == 0x00 && ai_msg.data[2] == 0x1) { //Open the phone window.
							this->window_handler->getAttributeList()->next_window = NEXT_WINDOW_PHONE;
						}
					}
				}
			}
		}
	} while(aibus_handler->getConnected() && (clock() - aibus_read_time)/(CLOCKS_PER_SEC/1000) < AIBUS_WAIT);
}

bool AidF_Nav_Computer::handleBroadcastMessage(AIData* ai_d) {
	if(ai_d->sender == ID_CANSLATOR && ai_d->l >= 3 && ai_d->data[1] == 0x2) { //Key position message.
		this->key_position = ai_d->data[2]&0xF;
		
		if(this->key_position == 0x0 && (this->door_position&0xF) != 0)
			this->running = false;
		else
			this->running = true;
		return true;
	} else if(ai_d->sender == ID_CANSLATOR && ai_d->l >= 3 && ai_d->data[1] == 0x43) { //Door message.
		this->door_position = ai_d->data[2];

		if(this->key_position == 0x0 && (this->door_position&0xF) != 0)
			this->running = false;
		else
			this->running = true;
		return true;
	} else if((ai_d->sender == ID_GPS_ANTENNA || ai_d->sender == ID_CANSLATOR || ai_d->sender == ID_RADIO) && ai_d->data[1] == 0x1F) { //Time or day.
		if(ai_d->data[2] == 0x1) { //Time.
			const uint8_t hour = ai_d->data[3], minute = ai_d->data[4];
			if(hour < 24 && minute < 60) {
				//TODO: 12hr time setting.
				if(minute < 10)
					this->window_handler->setText(std::to_string(int(hour))+":0"+std::to_string(int(minute)), 0);
				else
					this->window_handler->setText(std::to_string(int(hour))+":"+std::to_string(int(minute)), 0);
			} else {
				this->window_handler->setText("--:--", 0);
			}
			return true;
		} else if(ai_d->data[2] == 0x2) { //Date.
			const uint16_t year = ai_d->data[3] << 8 | ai_d->data[4];
			const uint8_t month = ai_d->data[5], date = ai_d->data[6];
			if(year != 0xFFFF && month != 0xFF && date != 0xFF) {
				//TODO: Date format.
				this->window_handler->setText(std::to_string(int(month)) + "/" + std::to_string(int(date)) + "/" + std::to_string(int(year)), 2);
			} else {
				this->window_handler->setText(" ", 2);
			}
			return true;
		} else
			return false;
	} else if(ai_d->sender == ID_CANSLATOR && ai_d->data[1] == 0x10) { //Night mode.
		if(this->attribute_list->day_night_settings == DAY_NIGHT_AUTO)
			this->setDayNight((ai_d->data[3]&0x80) != 0);
		return true;
	} else if(ai_d->sender == ID_CANSLATOR && ai_d->l >= 2 && ai_d->data[1] == 0x11) { //Light position.
		InfoParameters* info_parameters = window_handler->getVehicleInfo();
		setLightState(ai_d, info_parameters);
		return true;
	} else if(ai_d->sender == ID_CANSLATOR && ai_d->l >= 2 && ai_d->data[1] == 0x33) { //Hybrid system.
		InfoParameters* info_parameters = window_handler->getVehicleInfo();
		
		if(ai_d->l >= 4 && ai_d->data[2] == 0x1)
			info_parameters->hybrid_system_present = (ai_d->data[3]&0x7) != 0;

		return true;
	} else
		return false;
}

void AidF_Nav_Computer::setDayNight(const bool night) {
	if(this->night == night)
		return;
	
	this->night = night;

	if(night)
		setColorProfile(&active_color_profile, night_profile);
	else
		setColorProfile(&active_color_profile, day_profile);
	
	if(active_color_profile.background == active_color_profile.background2)
		this->br = new BR_Solid(DEFAULT_W, DEFAULT_H, active_color_profile.background);
	else
		this->br = new BR_Gradient(DEFAULT_W, DEFAULT_H, active_color_profile.background, active_color_profile.background2, active_color_profile.vertical, active_color_profile.square);

	if(this->audio_window != NULL)
		this->audio_window->refreshWindow();
	if(this->phone_window != NULL)
		this->phone_window->refreshWindow();
	if(this->main_window != NULL)
		this->main_window->refreshWindow();
	this->window_handler->refresh();
}

AidFColorProfile* AidF_Nav_Computer::getColorProfile() {
	return &this->active_color_profile;
}

uint16_t AidF_Nav_Computer::getWidth() {
	return this->lw;
}

uint16_t AidF_Nav_Computer::getHeight() {
	return this->lh;
}

SDL_Renderer* AidF_Nav_Computer::getRenderer() {
	return this->renderer;
}

void AidF_Nav_Computer::getBackground() {
	if(active_color_profile.background == active_color_profile.background2)
		this->br = new BR_Solid(DEFAULT_W, DEFAULT_H, active_color_profile.background);
	else
		this->br = new BR_Gradient(DEFAULT_W, DEFAULT_H, active_color_profile.background, active_color_profile.background2, active_color_profile.vertical, active_color_profile.square);
}

void setup(AidF_Nav_Computer* nav_computer) {
}

void loop(AidF_Nav_Computer* nav_computer) {
	nav_computer->loop();
}

int main(int argc, char* args[]) {
	SDL_Init(SDL_INIT_VIDEO);
	TTF_Init();
	#ifdef __arm__
	SDL_Window* window = SDL_CreateWindow("AidF", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, DEFAULT_W, DEFAULT_H, SDL_WINDOW_FULLSCREEN);
	#else
	SDL_Window* window = SDL_CreateWindow("AidF", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, DEFAULT_W, DEFAULT_H, SDL_WINDOW_SHOWN);
	#endif
	//TODO: Hide the cursor.

	AidF_Nav_Computer nav_computer(window, DEFAULT_W, DEFAULT_H);
	setup(&nav_computer);
	while(nav_computer.running)
		loop(&nav_computer);

	SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();
}
