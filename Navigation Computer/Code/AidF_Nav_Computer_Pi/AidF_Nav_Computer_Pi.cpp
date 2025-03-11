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

	int* socket_list[] = {&this->socket_parameters.client_socket};

	#ifdef RPI_UART
	this->aibus_handler = new AIBusHandler("/dev/ttyS0", socket_list, 1);
	#else
	this->aibus_handler = new AIBusHandler(socket_list, 1);
	#endif

	this->window_handler = new Window_Handler(this->renderer, this->br, this->lw, this->lh, &this->active_color_profile, this->aibus_handler);
	this->attribute_list = window_handler->getAttributeList();

	this->attribute_list->day_profile = &this->day_profile;
	this->attribute_list->night_profile = &this->night_profile;

	audio_window = new Audio_Window(attribute_list);
	phone_window = new PhoneWindow(attribute_list);
	main_window = new Main_Menu_Window(attribute_list);
	misc_window = new NavWindow(attribute_list);

	this->window_handler->setActiveWindow(main_window);
	this->window_handler->setText("--:--", 0);

	this->canslator_connected = &attribute_list->canslator_connected;
	this->radio_connected = &attribute_list->radio_connected;
	this->mirror_connected = &attribute_list->mirror_connected;

	this->socket_parameters.running = &this->running;
	this->socket_parameters.ai_serial = aibus_handler->getPortPointer();

	pthread_create(&socket_thread, NULL, socketThread, (void *)&socket_parameters);
	pthread_create(&frame_thread, NULL, frameThread, (void*)&attribute_list->frame);
}

AidF_Nav_Computer::~AidF_Nav_Computer() {
	attribute_list->frame = -1;

	SDL_DestroyRenderer(this->renderer);
	delete this->br;
	delete this->aibus_handler;
	delete this->window_handler;

	delete audio_window;
	delete main_window;
	delete misc_window;
	delete phone_window;

	pthread_join(socket_thread, NULL);
	pthread_join(frame_thread, NULL);
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

	if((attribute_list->background_changed || attribute_list->text_changed) && mirror_connected) {
		this->setMirrorColors();
	}

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
	
	if(this->vol_timer_enabled && (clock() - vol_timer)/(CLOCKS_PER_SEC/1000) >= 700) {
		this->vol_timer_enabled = false;
		//TODO: Other data here?
		this->window_handler->setText("", 1);
	}

	this->br->drawBackground(renderer, 0, 0, lw, lh);
	this->window_handler->drawWindow();
	SDL_RenderPresent(renderer);

	AIData ai_msg;
	do {
		if(!this->aibus_handler->getConnected() || this->aibus_handler->getAvailableBytes() > 0) {
			if(this->aibus_handler->readAIData(&ai_msg)) {
				aibus_read_time = clock();

				if(!*canslator_connected && ai_msg.sender == ID_CANSLATOR)
					*canslator_connected = true;

				if(!*radio_connected && ai_msg.sender == ID_RADIO)
					*radio_connected = true;

				if(!*mirror_connected && ai_msg.sender == ID_ANDROID_AUTO) {
					*mirror_connected = true;
					this->setMirrorColors();
				}

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

						if(ai_msg.sender == ID_RADIO && ai_msg.l >= 3 && ai_msg.data[0] == 0x26) { //Volume bar.
							const uint8_t vol = ai_msg.data[1];
							std::string vol_text = "Vol: " + std::to_string(vol);
							
							this->window_handler->setText(vol_text, 1);
							
							this->vol_timer_enabled = true;
							this->vol_timer = clock();
							
							answered = true;
						} else if(ai_msg.sender == ID_NAV_SCREEN && ai_msg.l >= 3 && ai_msg.data[0] == 0x30) { //Button press.
							const uint8_t button = ai_msg.data[1], state = ai_msg.data[2]>>6;
							if(button == 0x26 && state == 0x2) { //Audio button.
								attribute_list->next_window = NEXT_WINDOW_AUDIO;
							} else if(button == 0x50 && state == 0x2) { //Phone button.
								attribute_list->next_window = NEXT_WINDOW_PHONE;
							} else if(button == 0x20 && state == 0x2) { //Home button.
								attribute_list->next_window = NEXT_WINDOW_MAIN;
							}
						} else if(ai_msg.sender == ID_CANSLATOR && ai_msg.l >= 1 && ai_msg.data[0] == 0x11) { //Light info.
							InfoParameters* info_parameters = window_handler->getVehicleInfo();
							setLightState(&ai_msg, info_parameters);
							answered = true;
						} else if((ai_msg.sender == ID_RADIO || ai_msg.sender == ID_ANDROID_AUTO) && ai_msg.l >= 3 && ai_msg.data[0] == 0x27 && ai_msg.data[1] == 0x30 && ai_msg.data[2] == 0x26) { //Open the audio window.
							if(attribute_list->phone_active && attribute_list->mirror_connected && attribute_list->phone_type != 0) {
								uint8_t mirror_off_data[] = {0x48, 0x8E, 0x0};
								AIData mirror_off_msg(sizeof(mirror_off_data), ID_NAV_COMPUTER, ID_ANDROID_AUTO);

								mirror_off_msg.refreshAIData(mirror_off_data);
								aibus_handler->writeAIData(&mirror_off_msg, attribute_list->mirror_connected);
								
								attribute_list->phone_active = false;
							}

							this->window_handler->getAttributeList()->next_window = NEXT_WINDOW_AUDIO;
						} else if(ai_msg.sender == ID_PHONE && ai_msg.l >= 3 && ai_msg.data[0] == 0x21 && ai_msg.data[1] == 0x00 && ai_msg.data[2] == 0x1) { //Open the phone window.
							this->window_handler->getAttributeList()->next_window = NEXT_WINDOW_PHONE;
						} else if(ai_msg.sender == ID_ANDROID_AUTO) {
							if(ai_msg.l >= 3 && ai_msg.data[0] == 0x48 && ai_msg.data[1] == 0x8E) {
								const bool last_mirror_state = attribute_list->phone_active;
								attribute_list->phone_active = ai_msg.data[2] != 0;

								if(last_mirror_state != attribute_list->phone_active) {
									if(attribute_list->phone_active)
										attribute_list->next_window = NEXT_WINDOW_MIRROR;
									else
										attribute_list->next_window = NEXT_WINDOW_MAIN;
								}
							} else if(ai_msg.l >= 2 && ai_msg.data[0] == 0x30) { //Phone type.
								const uint8_t type = ai_msg.data[1];
								attribute_list->phone_type = type;

								//TODO: Only if the window is the mirror window.
								window_handler->getActiveWindow()->refreshWindow();
							} else if(ai_msg.l >= 2 && ai_msg.data[0] == 0x23 && ai_msg.data[1] == 0x30) { //Phone name.
								std::string phone_name = "";
								
								for(int i=2;i<ai_msg.l;i+=1)
									phone_name += char(ai_msg.data[i]);
									
								attribute_list->phone_name = phone_name;

								//TODO: Only if the window is the mirror window.
								window_handler->getActiveWindow()->refreshWindow();
							} else if(ai_msg.l >= 2 && ai_msg.data[0] == 0x2C && ai_msg.data[1] == 0xF0) { //Screen size request.
								const uint16_t w = window_handler->getWidth(), h = window_handler->getHeight();

								uint8_t res_resp_data[] = {0x2C, uint8_t(w>>8), uint8_t(w&0xFF), uint8_t(h>>8), uint8_t(h&0xFF)};
								AIData res_resp_msg(sizeof(res_resp_data), ID_NAV_COMPUTER, ai_msg.sender);
								res_resp_msg.refreshAIData(res_resp_data);

								aibus_handler->writeAIData(&res_resp_msg);
							}
						}
					}

					if(ai_msg.sender == ID_CANSLATOR && !answered) {
						AIData broadcast_msg(ai_msg.l + 1, ai_msg.sender, 0xFF);
						broadcast_msg.data[0] = 0xA1;

						for(int i=0;i<ai_msg.l;i+=1)
							broadcast_msg.data[i+1] = ai_msg.data[i];

						handleBroadcastMessage(&broadcast_msg);
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
		
		if(ai_d->l >= 4 && ai_d->data[2] == 0x1) {
			info_parameters->hybrid_system_present = (ai_d->data[3]&0x7) != 0;
			info_parameters->hybrid_system_type = ai_d->data[3]&0xF;
			info_parameters->charge_assist_meter = (ai_d->data[3]&0x10) != 0;
			if(ai_d->l >= 5)
				info_parameters->hybrid_features = ai_d->data[4];
		} else if(ai_d->l >= 6 && ai_d->data[2] == 0x2) {
			info_parameters->hybrid_status_main = ai_d->data[3];
			info_parameters->hybrid_battery_state = ai_d->data[4];
		}

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

	this->setMirrorColors();
}

//Set the colors at the phone mirror.
void AidF_Nav_Computer::setMirrorColors() {
	if(!attribute_list->mirror_connected)
		return;

	const uint32_t header = attribute_list->color_profile->background, text = attribute_list->color_profile->text;

	uint8_t header_data[] = {0x60, 0x22, uint8_t((header&0xFF000000) >> 24), uint8_t((header&0xFF0000) >> 16), uint8_t((header&0xFF00)>>8)};
	AIData header_msg(sizeof(header_data), ID_NAV_COMPUTER, ID_ANDROID_AUTO);
	header_msg.refreshAIData(header_data);

	uint8_t text_data[] = {0x60, 0x21, uint8_t((text&0xFF000000) >> 24), uint8_t((text&0xFF0000) >> 16), uint8_t((text&0xFF00)>>8)};
	AIData text_msg(sizeof(text_data), ID_NAV_COMPUTER, ID_ANDROID_AUTO);
	text_msg.refreshAIData(text_data);

	const bool connect = aibus_handler->writeAIData(&header_msg, attribute_list->mirror_connected);
	if(attribute_list->mirror_connected && !connect)
		attribute_list->mirror_connected = false;

	aibus_handler->writeAIData(&text_msg, attribute_list->mirror_connected);
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
	int screen_w = DEFAULT_W, screen_h = DEFAULT_H;
	getResolution(&screen_w, &screen_h);

	#ifdef RPI_UART
	//TODO: Load resolution from a file.
	SDL_Window* window = SDL_CreateWindow("AidF", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_FULLSCREEN);
	#else
	SDL_Window* window = SDL_CreateWindow("AidF", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_SHOWN);
	#endif
	//TODO: Hide the cursor.

	AidF_Nav_Computer nav_computer(window, screen_w, screen_h);
	setup(&nav_computer);
	while(nav_computer.running)
		loop(&nav_computer);

	SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();
}

//Frame thread function.
void *frameThread(void* frame_v) {
	int* frame = (int*)frame_v;

	while(*frame >= 0) {
		if(*frame < GRAD_W*3 - 1 && *frame >= 0)
			*frame += 1;
		else if(*frame >= 0)
			*frame = 0;
		
		usleep(1000000/75);
	}

	void* result;
	return result;
}

//Get a resolution from a saved file.
void getResolution(int* w, int* h) {
	std::vector<IniList> dimension_file = loadIniFile(RESOLUTION_FILE);

	bool dimension_loaded = false;

	for(int i=0;i<dimension_file.size();i+=1) {
		if(dimension_file[i].title.compare("AidF_Navigation_Screen_Dimensions") == 0) {
			int new_w = *w, new_h = *h;

			for(int n=0;n<dimension_file[i].l_n;n+=1) {
				if(dimension_file[i].num_vars[n].compare("w") == 0)
					new_w = dimension_file[i].num_values[n];
				else if(dimension_file[i].num_vars[n].compare("h") == 0)
					new_h = dimension_file[i].num_values[n];
			}

			*w = new_w;
			*h = new_h;
			dimension_loaded = true;
			break;
		}
	}

	if(!dimension_loaded) //Dimension file not found.
		saveResolution(*w, *h);
}

//Save a resolution to a file.
void saveResolution(const int w, const int h) {
	IniList dimension_file(2,0);
	
	dimension_file.title = "AidF_Navigation_Screen_Dimensions";
	dimension_file.num_vars[0] = "w";
	dimension_file.num_vars[1] = "h";

	dimension_file.num_values[0] = w;
	dimension_file.num_values[1] = h;

	std::cout<<dimension_file.title<<'\n';

	std::vector<IniList> file_list(0);
	file_list.push_back(dimension_file);

	saveIniFile(RESOLUTION_FILE, file_list);
}