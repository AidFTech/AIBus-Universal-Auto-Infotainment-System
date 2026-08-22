#include "AidF_Nav_Computer_Pi.h"
#include <SDL2/SDL_render.h>
//Must be compiled with options -lSDL2 -lSDL2_ttf -lrt

AidF_Nav_Computer::AidF_Nav_Computer(SDL_Window* window, string port, const uint16_t lw, const uint16_t lh) {
	this->lw = lw;
	this->lh = lh;

	this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	this->amirror_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING, lw, lh);
	this->camera_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING, lw, lh);
	
	this->night_profile.background = DEFAULT_BACKGROUND_NIGHT;
	this->night_profile.background2 = DEFAULT_BACKGROUND_NIGHT;
	this->night_profile.text = DEFAULT_TEXT_NIGHT;
	this->night_profile.button = DEFAULT_BUTTON_NIGHT;
	this->night_profile.selection = DEFAULT_SELECTION_NIGHT;
	this->night_profile.headerbar = DEFAULT_HEADERBAR_NIGHT;
	this->night_profile.outline = DEFAULT_OUTLINE_NIGHT;

	const bool color_set = getIniColorProfile(&this->day_profile, &this->night_profile, ACTIVE_COLOR);
	if(!color_set)
		saveIniColorProfile(this->day_profile, this->night_profile, ACTIVE_COLOR);

	setColorProfile(&this->active_color_profile, this->day_profile);

	this->getBackground();

	int* socket_list[] = {&this->amirror_socket_parameters.client_socket, &this->abta_socket_parameters.client_socket};
	
	this->aibus_handler = new SerialAIBusHandler(port, socket_list, sizeof(socket_list)/sizeof(int*), ID_NAV_COMPUTER, &elapsed_millis.time);

	#ifdef RPI_UART
	gpiod_chip* chip = aibus_handler->getChip();
	this->line_nav_mute = getGPIOLine(chip, GPIO_NAV_MUTE);

	setPinMode(line_nav_mute, PIN_MODE_OUTPUT);
	writePin(line_nav_mute, false);

	//gpioSetMode(GPIO_I2S_MCLK, PI_OUTPUT);
	//gpioWrite(GPIO_I2S_MCLK, PI_LOW);
	//gpioSetMode(GPIO_DAC_MUTE, PI_OUTPUT);
	//gpioWrite(GPIO_DAC_MUTE, PI_HIGH);
	//gpioSetMode(GPIO_USB_PWR, PI_OUTPUT);
	//gpioWrite(GPIO_USB_PWR, PI_HIGH);
	#endif

	this->window_handler = new Window_Handler(this->renderer, this->br, this->lw, this->lh, &this->active_color_profile, this->aibus_handler);
	this->attribute_list = window_handler->getAttributeList();
	{
		InfoParameters* info_parameters = window_handler->getVehicleInfo();
		for(int i=0;i<sizeof(info_parameters->param_index)/sizeof(info_param);i+=1)
			info_parameters->param_index[i] = INFO_PARAM_NONE;

		uint8_t displayed_params_int[sizeof(info_parameters->param_index)/sizeof(info_param)];
		getVehicleInfoParams(&info_parameters->display_cruise, &info_parameters->draw_charge_assist, displayed_params_int, sizeof(displayed_params_int));

		for(int i=0;i<sizeof(displayed_params_int);i+=1)
			info_parameters->param_index[i] = (info_param)displayed_params_int[i];
	}

	{
		NavParameters* nav_parameters = window_handler->getNavParameters();
		nav_parameters->map_path = getMapPath();
	}

	this->attribute_list->day_profile = &this->day_profile;
	this->attribute_list->night_profile = &this->night_profile;
	this->night = &this->attribute_list->night;
	this->attribute_list->window = window;
	this->attribute_list->timer = &elapsed_millis.time;

	audio_window = new Audio_Window(attribute_list);
	phone_window = new PhoneWindow(attribute_list);
	main_window = new Main_Menu_Window(attribute_list);
	misc_window = new NavWindow(attribute_list);

	this->window_handler->setActiveWindow(new IntroWindow(attribute_list), false);
	this->window_handler->setText("--:--", 0);

	getTimekeepingParams(&attribute_list->display_12h, &attribute_list->auto_clock, &attribute_list->timekeeper);

	this->canslator_connected = &attribute_list->canslator_connected;
	this->radio_connected = &attribute_list->radio_connected;
	this->mirror_connected = &attribute_list->mirror_connected;

	this->amirror_socket_parameters.running = &this->running;
	this->amirror_socket_parameters.ai_serial = aibus_handler->getPortPointer();
	this->amirror_socket_parameters.socket_path = AMIRROR_SOCKET_PATH;
	this->amirror_socket_parameters.timer = &elapsed_millis.time;

	this->abta_socket_parameters.running = &this->running;
	this->abta_socket_parameters.ai_serial = aibus_handler->getPortPointer();
	this->abta_socket_parameters.socket_path = BTA_SOCKET_PATH;
	this->abta_socket_parameters.timer = &elapsed_millis.time;

	this->frame_parameters.frame = &attribute_list->frame;
	this->frame_parameters.run = &this->running;
	
	this->elapsed_millis.run = &this->running;

	this->amirror_video_socket_parameters.socket_handler = new ClientVideoSocketHandler(renderer, AMIRROR_VIDEO_IPC_PATH, AMIRROR_VIDEO_SOCKET_PATH, this->lw, this->lh);
	this->amirror_video_socket_parameters.running = &this->running;
	this->amirror_video_socket_parameters.socket_path = AMIRROR_VIDEO_SOCKET_PATH;

	VideoCache* video_cache = this->amirror_video_socket_parameters.socket_handler->getVideoCache();
	SDL_LockTexture(amirror_texture, NULL, &video_cache->pixels, (int*)&video_cache->pitch);
	SDL_UnlockTexture(amirror_texture);

	const string camera_path = getCameraPath();
	if(camera_path.empty())
		camera_video_socket_parameters.socket_path = CAMERA_VIDEO_SOCKET_PATH + string("0");
	else
		camera_video_socket_parameters.socket_path = camera_path;

	camera_handler = new CameraHandler(renderer, attribute_list, &camera_video_socket_parameters, window_handler);

	pthread_create(&amirror_socket_thread, NULL, socketThread, (void *)&amirror_socket_parameters);
	pthread_create(&abta_socket_thread, NULL, socketThread, (void *)&abta_socket_parameters);
	pthread_create(&frame_thread, NULL, frameThread, (void*)&frame_parameters);
	pthread_create(&timer_thread, NULL, millisThread, (void*)&elapsed_millis);
	pthread_create(&amirror_video_thread, NULL, videoPlayThread, (void*)&amirror_video_socket_parameters);

	#ifdef RPI_UART
	uint8_t init_data[] = {0x4A, 0x1F};
	AIData init_msg(sizeof(init_data), ID_NAV_COMPUTER, ID_CANSLATOR, init_data);
	aibus_handler->writeAIData(&init_msg, false);
	#endif

	uint8_t radio_ping_data[] = {0x1};
	AIData radio_ping_msg(sizeof(radio_ping_data), ID_NAV_COMPUTER, ID_RADIO, radio_ping_data);
	aibus_handler->writeAIData(&radio_ping_msg, false);
}

AidF_Nav_Computer::~AidF_Nav_Computer() {
	attribute_list->frame = -1;

	#ifndef RPI_UART
	std::cout<<"Waiting for threads to exit...\n";
	#endif
	pthread_cancel(amirror_socket_thread);
	pthread_cancel(abta_socket_thread);
	pthread_cancel(frame_thread);
	pthread_cancel(timer_thread);
	pthread_cancel(amirror_video_thread);
	pthread_cancel(camera_video_thread);
	#ifndef RPI_UART
	std::cout<<"Threads exited!\n";
	#endif
	
	if(this->amirror_socket_parameters.socket_ptr != nullptr)
		delete this->amirror_socket_parameters.socket_ptr;

	if(this->amirror_video_socket_parameters.socket_handler != nullptr)
		delete this->amirror_video_socket_parameters.socket_handler;

	if(this->camera_video_socket_parameters.socket_handler != nullptr)
		delete this->camera_video_socket_parameters.socket_handler;

	delete camera_handler;

	#ifdef RPI_UART
	gpiod_line_release(line_nav_mute);
	#endif

	SDL_DestroyTexture(amirror_texture);
	SDL_DestroyTexture(camera_texture);
	SDL_DestroyRenderer(this->renderer);
	delete this->br;
	delete this->aibus_handler;
	delete this->window_handler;

	delete audio_window;
	delete main_window;
	delete misc_window;
	delete phone_window;

	#ifdef RPI_UART
	if(!test_mode) 
		system("poweroff");
	#endif
}

//Main object loop.
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

	const bool last_nav_prompt = attribute_list->nav_prompt_active;
	
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

	if(this->vol_timer_enabled && (elapsed_millis.time - vol_timer) >= HEADER_LIMIT_VOLUME) {
		this->vol_timer_enabled = false;
		this->window_handler->setText("", 1);
	}
	
	if(this->header_timer_enabled && (elapsed_millis.time - header_timer) >= HEADER_LIMIT_OTHER) {
		this->header_timer_enabled = false;
		this->window_handler->setAudioText("");
	}

	if(audio_window != NULL)
		audio_window->loop();

	if(camera_video_socket_parameters.socket_handler == nullptr) {
		const string socket_path = camera_video_socket_parameters.socket_path;
		if(exists(path(socket_path))) {
			camera_video_socket_parameters.socket_handler = new ClientVideoSocketHandler(renderer, CAMERA_VIDEO_IPC_PATH, socket_path, this->lw, this->lh);
			camera_video_socket_parameters.running = &this->running;
		}
	}

	ClientVideoSocketHandler* amirror_video_socket_handler = amirror_video_socket_parameters.socket_handler;
		
	unsigned long video_start = elapsed_millis.time;
	do {
		VideoCache *video_cache = amirror_video_socket_handler->getVideoCache();
		
		SDL_LockTexture(amirror_texture, NULL, &video_cache->pixels, (int*)&video_cache->pitch);
		amirror_video_socket_handler->render();

		SDL_UnlockTexture(amirror_texture);
	} while(amirror_video_socket_handler->getRefresh() && elapsed_millis.time - video_start < 15);

	video_start = elapsed_millis.time;
	ClientVideoSocketHandler* camera_video_socket_handler = camera_video_socket_parameters.socket_handler;
	if(camera_video_socket_handler != nullptr) {
		do {
			VideoCache* video_cache = camera_video_socket_handler->getVideoCache();

			SDL_LockTexture(camera_texture, NULL, &video_cache->pixels, (int*)&video_cache->pitch);
			camera_video_socket_handler->render();

			SDL_UnlockTexture(camera_texture);
		} while(camera_video_socket_handler->getRefresh() && elapsed_millis.time - video_start < 15);
	}

	if(attribute_list->phone_active && attribute_list->phone_type != 0 && amirror_video_socket_handler->getVideoInit()) {
		SDL_Rect dest = {0, 0, lw, lh};
		SDL_RenderCopy(renderer, amirror_texture, NULL, &dest);
	} else if(attribute_list->camera_active && camera_video_socket_handler != nullptr && camera_video_socket_handler->getVideoInit()) {
		SDL_Rect dest = {0, 0, lw, lh};
		SDL_RenderCopy(renderer, camera_texture, NULL, &dest);

		camera_handler->overlayCameraInfo(camera_texture);
	} else {
		this->br->drawBackground(renderer, 0, 0, lw, lh);
		this->window_handler->drawWindow();
	}

	SDL_RenderPresent(renderer);

	NavWindow* active_window = this->window_handler->getActiveWindow();
	const bool full_aibus_check = typeid(*active_window) != typeid(VehicleInfoWindow);

	AIData ai_msg;
	do {
		if(!this->aibus_handler->getConnected() || this->aibus_handler->getAvailableBytes() > 0) {
			if(this->aibus_handler->readAIData(&ai_msg)) {
				if(full_aibus_check)
					aibus_read_time = elapsed_millis.time;

				if(!*canslator_connected && ai_msg.sender == ID_CANSLATOR && !getPowerOffMessage(&ai_msg))
					*canslator_connected = true;

				if(!*radio_connected && ai_msg.sender == ID_RADIO && !getInitMessage(&ai_msg) && !getPowerOffMessage(&ai_msg)) {
					*radio_connected = true;

					setMonitorOn(attribute_list->monitor_on);
				}

				if(!attribute_list->gps_antenna_connected && ai_msg.sender == ID_GPS_ANTENNA && !getInitMessage(&ai_msg) && !getPowerOffMessage(&ai_msg))
					attribute_list->gps_antenna_connected = true;

				if(!*mirror_connected && ai_msg.sender == ID_ANDROID_AUTO && !getInitMessage(&ai_msg) && !getPowerOffMessage(&ai_msg)) {
					*mirror_connected = true;
					this->setMirrorColors();
				}

				vector<uint8_t> *ping_device_list = &attribute_list->ping_device_list;
				if(!getInitMessage(&ai_msg) && !getPowerOffMessage(&ai_msg) && ai_msg.sender != 0 && ai_msg.sender != ID_NAV_COMPUTER) {
					bool exists = false;
					int ins_ind = -1;
					for(int i=0;i<ping_device_list->size();i+=1) {
						if(ping_device_list->at(i) > ai_msg.sender) {
							ins_ind = i;
							break;
						} else if(ping_device_list->at(i) == ai_msg.sender) {
							exists = true;
							break;
						}
					}

					if(!exists) {
						if(ins_ind >= 0)
							ping_device_list->insert(ping_device_list->begin() + ins_ind, ai_msg.sender);
						else
							ping_device_list->push_back(ai_msg.sender);
					}
				}

				if(getInitMessage(&ai_msg) || getPowerOffMessage(&ai_msg)) {
					if(ai_msg.sender == ID_RADIO)
						*radio_connected = false;
					else if(ai_msg.sender == ID_ANDROID_AUTO)
						*mirror_connected = false;
					else if(ai_msg.sender == ID_CANSLATOR)
						attribute_list->canslator_connected = false;

					for(int i=0;i<ping_device_list->size();i+=1) {
						if(ping_device_list->at(i) == ai_msg.sender) {
							ping_device_list->erase(ping_device_list->begin() + i);
							break;
						}
					}
				}

				//Send clock settings.
				if(!attribute_list->timekeeper_detected && ai_msg.sender == attribute_list->timekeeper && this->key_position != 0 && !getInitMessage(&ai_msg)) {
					attribute_list->timekeeper_detected = true;

					uint8_t clock_data[] = {0x1D, 0x0};
					if(attribute_list->auto_clock)
						clock_data[1] |= 0x1;
					else
						clock_data[1] |= 0x2;

					if(attribute_list->display_12h)
						clock_data[1] |= 0x80;

					AIData clock_msg(sizeof(clock_data), ID_NAV_COMPUTER, attribute_list->timekeeper);
					clock_msg.refreshAIData(clock_data);
					aibus_handler->writeAIData(&clock_msg);
				}

				if(ai_msg.sender == ID_NAV_COMPUTER)
					continue;

				if(ai_msg.sender != ID_NAV_COMPUTER && (ai_msg.receiver == ID_NAV_COMPUTER || ai_msg.receiver == 0xFF) && ai_msg.l >= 1 && ai_msg.data[0] != 0x80) {
					bool answered = false;

					if(ai_msg.l >= 2 && ai_msg[0] == 0x71) //Turn the monitor on or off.
						setMonitorOn(ai_msg[1] != 0);

					//Reactivate hte monitor if a button is pressed.
					if(!attribute_list->monitor_on && ai_msg.sender == ID_NAV_SCREEN && ai_msg.l >= 3 && (ai_msg[0] == 0x30 || ai_msg[0] == 0x32))
						setMonitorOn(true);

					NavWindow* active_window = this->window_handler->getActiveWindow();
					if(ai_msg.receiver == 0xFF && ai_msg.data[0] == 0xA1)
						answered = handleBroadcastMessage(&ai_msg) && typeid(*active_window) != typeid(IntroWindow);
					else if(ai_msg.sender == ID_GPS_ANTENNA && ai_msg.data[0] == 0x55) {
						answered = true;
						handleNavMessage(&ai_msg, window_handler->getNavParameters());

						NavWindow* active_window = window_handler->getActiveWindow();
						if(typeid(*active_window) == typeid(MapMainWindow)) {
							MapMainWindow* map_window = (MapMainWindow*)active_window;
							map_window->refreshWindow();
						}
					}

					if(!answered)
						answered = audio_window->handleAIBus(&ai_msg);
					if(!answered)
						answered = phone_window->handleAIBus(&ai_msg);
					if(!answered && this->window_handler->getActiveWindow() != NULL  && this->window_handler->getActiveWindow() != audio_window)
						answered = this->window_handler->getActiveWindow()->handleAIBus(&ai_msg);
					if(!answered) { //Handle the AIBus message directly.
						if(ai_msg.l >= 2 && ai_msg.data[0] == 0x22 && ai_msg.data[1] == 0x61 && (ai_msg.sender == ID_RADIO || ai_msg.sender == attribute_list->active_audio_device)) { //Headerbar.
							if(!audio_window->getActive()) {
								std::string header_text = "";

								for(int i=2;i<ai_msg.l;i+=1)
									header_text += char(ai_msg.data[i]);

								this->window_handler->setAudioText(header_text);

								this->header_timer_enabled = true;
								this->header_timer = elapsed_millis.time;

								if(attribute_list->mirror_connected && ai_msg.sender != ID_ANDROID_AUTO) {
									AIData header_fwd(ai_msg.l, ID_NAV_COMPUTER, ID_ANDROID_AUTO, ai_msg.data);
									aibus_handler->writeAIData(&header_fwd);
								}
							}
							answered = true;
						} else if(ai_msg.l >= 2 && ai_msg.data[0] == 0x2C && ai_msg.data[1] == 0xF0) { //Screen size request.
							const uint16_t w = window_handler->getWidth(), h = window_handler->getHeight();

							uint8_t res_resp_data[] = {0x2C, uint8_t(w>>8), uint8_t(w&0xFF), uint8_t(h>>8), uint8_t(h&0xFF)};
							AIData res_resp_msg(sizeof(res_resp_data), ID_NAV_COMPUTER, ai_msg.sender);
							res_resp_msg.refreshAIData(res_resp_data);

							aibus_handler->writeAIData(&res_resp_msg);
							answered = true;
						} else if(ai_msg.sender == ID_RADIO && ai_msg.l >= 3 && ai_msg.data[0] == 0x26) { //Volume bar.
							const uint8_t vol = ai_msg.data[1];
							std::string vol_text = "Vol: " + std::to_string(vol);
							
							this->window_handler->setText(vol_text, 1);
							
							this->vol_timer_enabled = true;
							this->vol_timer = elapsed_millis.time;
							
							answered = true;
						} else if(ai_msg.sender == ID_RADIO && ai_msg.l >= 3 && ai_msg.data[0] == 0x40 && ai_msg.data[1] == 0x1) { //Active audio device.
							attribute_list->active_audio_device = ai_msg[2];
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
						} else if(ai_msg.sender == ID_CANSLATOR) { 
							if(ai_msg.l >= 1 && ai_msg.data[0] == 0x11) { //Light info.
								InfoParameters* info_parameters = window_handler->getVehicleInfo();
								setLightState(&ai_msg, info_parameters);
								answered = true;
							} else if(ai_msg.l >= 3 && ai_msg.data[0] == 0x49) { //Supported parameters.
								InfoParameters* info_parameters = window_handler->getVehicleInfo();
								info_parameters->supported_a = ai_msg.data[1];
								info_parameters->supported_b = ai_msg.data[2];
								answered = true;
							}
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

								if(!attribute_list->monitor_on)
									setMonitorOn(true);
							} else if(ai_msg.l >= 2 && ai_msg.data[0] == 0x30) { //Phone type.
								const uint8_t type = ai_msg.data[1];
								attribute_list->phone_type = type;

								if(type == 0) {
									SDL_SetRenderTarget(renderer, amirror_texture);
									SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
									SDL_RenderClear(renderer);
									SDL_SetRenderTarget(renderer, NULL);
									amirror_video_socket_handler->clearVideoInit();
								}

								//TODO: Only if the window is the mirror window.
								window_handler->getActiveWindow()->refreshWindow();
							} else if(ai_msg.l >= 2 && ai_msg.data[0] == 0x23 && ai_msg.data[1] == 0x30) { //Phone name.
								std::string phone_name = "";
								
								for(int i=2;i<ai_msg.l;i+=1)
									phone_name += char(ai_msg.data[i]);
									
								attribute_list->phone_name = phone_name;

								//TODO: Only if the window is the mirror window.
								window_handler->getActiveWindow()->refreshWindow();
							} else if(ai_msg.l >= 2 && ai_msg.data[0] == 0x60 && ai_msg.data[1] == 0x20) { //Color request.
								this->setMirrorColors();
							} else if(ai_msg.l >= 3 && ai_msg[0] == 0x7F && ai_msg[1] == 0x6) { //Mute/unmute.
								attribute_list->nav_prompt_active = ai_msg[2] != 0;
							}
						} else if(ai_msg.sender == ID_CAMERA) {
							if(ai_msg.l >= 3 && ai_msg[0] == 0x48 && ai_msg[1] == 0x61) {
								attribute_list->camera_active = ai_msg[2] != 0;
								//TODO: Lots more.

								if(camera_video_socket_parameters.socket_handler != nullptr)  {
									if(attribute_list->phone_active) {
										uint8_t mirror_set_data[] = {0x48, 0x8E, uint8_t(attribute_list->camera_active ? 0 : 1)};
										AIData mirror_set_msg(sizeof(mirror_set_data), ID_NAV_COMPUTER, ID_ANDROID_AUTO, mirror_set_data);

										aibus_handler->writeAIData(&mirror_set_msg, attribute_list->mirror_connected);
									}

									if(!attribute_list->monitor_on)
										setMonitorOn(true);
								}

								if(!attribute_list->camera_active)
									camera_handler->resetCameraMessage();
							} else if(ai_msg.l >= 3 && ai_msg[0] == 0x25 && ai_msg[1] == 0x57) {
								if(ai_msg[2] == 0x61) { //Camera message.
									string new_camera_msg = "";
									for(int i=3;i<ai_msg.l;i+=1)
										new_camera_msg += char(ai_msg[i]);

									if(new_camera_msg.length() > 0)
										camera_handler->setCameraMessage(new_camera_msg.c_str());
									else
										camera_handler->resetCameraMessage();
								} else if(ai_msg[2] == 0x62) { //Camera path.
									string new_camera_path = "";
									for(int i=3;i<ai_msg.l;i+=1)
										new_camera_path += char(ai_msg[i]);

									const string camera_socket_path = new_camera_path.length() > 0 ? new_camera_path : CAMERA_VIDEO_SOCKET_PATH + string("0");
									camera_handler->setCameraPath(camera_socket_path.c_str());
								}
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

	 	if(amirror_video_socket_handler->getRefresh()) {
			VideoCache *video_cache = amirror_video_socket_handler->getVideoCache();
			
			SDL_LockTexture(amirror_texture, NULL, &video_cache->pixels, (int*)&video_cache->pitch);
			amirror_video_socket_handler->render();

			SDL_UnlockTexture(amirror_texture);

			if(attribute_list->phone_active && attribute_list->phone_type != 0 && amirror_video_socket_handler->getVideoInit()) {
				SDL_Rect dest = {0, 0, lw, lh};
				SDL_RenderCopy(renderer, amirror_texture, NULL, &dest);
				SDL_RenderPresent(renderer);
			}
		}
	} while(running && aibus_handler->getConnected() && (elapsed_millis.time - aibus_read_time) < AIBUS_WAIT);

	if(last_nav_prompt != attribute_list->nav_prompt_active) {
		#ifdef RPI_UART
		writePin(line_nav_mute, attribute_list->nav_prompt_active);
		#else
		cout<<"Nav mute "<<(attribute_list->nav_prompt_active ? "active" : "incative")<<endl;
		#endif
	}
}

//Handle a broadcast AIBus message.
bool AidF_Nav_Computer::handleBroadcastMessage(AIData* ai_d) {
	if(ai_d->sender == ID_CANSLATOR && ai_d->l >= 3 && ai_d->data[1] == 0x2) { //Key position message.
		this->key_position = ai_d->data[2]&0xF;
		
		if(this->key_position == 0x0 && (this->door_position&0xC) != 0) {
			if(this->key_switched_on) {
				this->sendPowerOffMessage();
				this->running = false;
			}
		} else
			this->running = true;

		if(this->key_position != 0)
			this->key_switched_on = true;

		return true;
	} else if(ai_d->sender == ID_CANSLATOR && ai_d->l >= 3 && ai_d->data[1] == 0x43) { //Door message.
		this->door_position = ai_d->data[2];

		if(this->key_position == 0x0 && (this->door_position&0xC) != 0) {
			if(this->key_switched_on || (this->door_position&0x80) != 0) {
				this->sendPowerOffMessage();
				this->running = false;
			}
		} else
			this->running = true;
		return true;
	} else if(ai_d->l >= 3 && (ai_d->sender == ID_GPS_ANTENNA || ai_d->sender == ID_CANSLATOR || ai_d->sender == ID_RADIO) && ai_d->data[1] == 0x1F) { //Time/day, speed, temp, etc.
		if(ai_d->data[2] == 0x1 && ai_d->l >= 6) { //Time.
			const uint8_t hour = ai_d->data[3]&0x1F, minute = ai_d->data[4];
			attribute_list->hour = hour;
			attribute_list->minute = minute&0x7F;

			const bool last_display_12h = attribute_list->display_12h, last_auto_clock = attribute_list->auto_clock;
			const uint8_t last_timekeeper = attribute_list->timekeeper;

			attribute_list->display_12h = (ai_d->data[3]&0x80) != 0;
			attribute_list->auto_clock = (ai_d->data[3]&0x40) != 0;
			attribute_list->timekeeper = ai_d->sender;

			if(hour < 24 && minute < 60) {
				std::string hour_str;
				if(!attribute_list->display_12h) //Use 24hr time.
					hour_str = std::to_string(int(hour));
				else {
					if(hour > 0 && hour < 13)
						hour_str = std::to_string(int(hour));
					else if(hour == 0)
						hour_str = "12";
					else
						hour_str = std::to_string(int(hour)-12);
				}
				
				std::string time_text = "";

				if(minute < 10)
					time_text = hour_str+":0"+std::to_string(int(minute));
				else
					time_text = hour_str+":"+std::to_string(int(minute));

				if(attribute_list->display_12h) {
					if(hour < 12)
						time_text += " AM";
					else
						time_text += " PM";
				}

				this->window_handler->setText(time_text, 0);
			} else {
				this->window_handler->setText("--:--", 0);
			}

			if(last_auto_clock != attribute_list->auto_clock || last_display_12h != attribute_list->display_12h || last_timekeeper != attribute_list->timekeeper)
				saveTimekeepingParams(attribute_list->display_12h, attribute_list->auto_clock, attribute_list->timekeeper);

			return true;
		} else if(ai_d->data[2] == 0x2 && ai_d->l >= 7) { //Date.
			const uint16_t year = ai_d->data[3] << 8 | ai_d->data[4];
			const uint8_t month = ai_d->data[5], date = ai_d->data[6];
			if(year != 0xFFFF && month != 0xFF && date != 0xFF) {
				//TODO: Date format.
				this->window_handler->setText(std::to_string(int(month)) + "/" + std::to_string(int(date)) + "/" + std::to_string(int(year)), 2);
			} else {
				this->window_handler->setText(" ", 2);
			}
			return true;
		} else if(ai_d->sender == ID_CANSLATOR) {
			InfoParameters* info_parameters = window_handler->getVehicleInfo();
			NavWindow* active_window = this->window_handler->getActiveWindow();

			if(ai_d->data[2] == 0x3 && ai_d->l >= 5) { //Outside temp.
				const uint8_t decimal_count = (ai_d->data[3]&0x70) >> 4;
				const bool fahrenheit = (ai_d->data[3]&0x80) != 0, neg = (ai_d->data[3]&0x8) != 0;

				unsigned int read_temp = 0;
				for(int i=4;i<ai_d->l;i+=1) {
					read_temp <<= 8;
					read_temp += ai_d->data[i];
				}

				if(decimal_count == 0)
					read_temp *= 10;
				else {
					for(int i=1;i<decimal_count;i+=1)
						read_temp /= 10;
				}

				int16_t norm_temp = read_temp&0xFFFF;
				if(neg)
					norm_temp *= -1;

				info_parameters->outside_temp = norm_temp;
				info_parameters->outside_temp_fahrenheit = fahrenheit;

				if(!info_parameters->outside_temp_sent)
					info_parameters->outside_temp_sent = true;
				
				if(typeid(*active_window) == typeid(VehicleInfoWindow))
					this->window_handler->refresh();

				return true;
			} else if (ai_d->data[2] == 0x4 && ai_d->l >= 5) { //Speed.
				const uint16_t last_speed = attribute_list->vehicle_speed;

				const uint8_t decimal_count = (ai_d->data[3]&0x70) >> 4;
				const bool mph = (ai_d->data[3]&0x80) != 0;

				unsigned int read_speed = 0;
				for(int i=4;i<ai_d->l;i+=1) {
					read_speed <<= 8;
					read_speed += ai_d->data[i];
				}

				if(decimal_count == 0)
					read_speed *= 10;
				else {
					for(int i=1;i<decimal_count;i+=1)
						read_speed /= 10;
				}

				if(mph)
					attribute_list->vehicle_speed = read_speed*1609/1000;
				else
					attribute_list->vehicle_speed = read_speed;

				if(last_speed == 0 && read_speed != 0)
					window_handler->refresh();
				else if(last_speed != 0 && read_speed == 0)
					window_handler->refresh();

				return true;
			} else if (ai_d->data[2] == 0x5 && ai_d->l >= 5) { //Coolant temp.
				const uint8_t decimal_count = (ai_d->data[3]&0x70) >> 4;
				const bool fahrenheit = (ai_d->data[3]&0x80) != 0, neg = (ai_d->data[3]&0x8) != 0;

				unsigned int read_temp = 0;
				for(int i=4;i<ai_d->l;i+=1) {
					read_temp <<= 8;
					read_temp += ai_d->data[i];
				}

				if(decimal_count == 0)
					read_temp *= 10;
				else {
					for(int i=1;i<decimal_count;i+=1)
						read_temp /= 10;
				}

				int16_t norm_temp = read_temp&0xFFFF;
				if(neg)
					norm_temp *= -1;

				info_parameters->coolant_temp = norm_temp;
				info_parameters->coolant_temp_fahrenheit = fahrenheit;

				if(!info_parameters->coolant_temp_sent)
					info_parameters->coolant_temp_sent = true;

				if(typeid(*active_window) == typeid(VehicleInfoWindow))
					this->window_handler->refresh();
				
				return true;
			} else if(ai_d->data[2] == 0x6 && ai_d->l >= 5) { //Battery voltage.
				const uint8_t decimal_count = (ai_d->data[3]&0xF0) >> 4;
				info_parameters->battery_voltage_precision = decimal_count <= 2 ? decimal_count : 2;

				unsigned long read_vbat = 0;
				for(int i=4;i<ai_d->l;i+=1) {
					read_vbat <<= 8;
					read_vbat += ai_d->data[i];
				}

				if(decimal_count == 0)
					read_vbat *= 100;
				else if(decimal_count == 1)
					read_vbat *= 10;
				else {
					for(int i=2;i<decimal_count;i+=1)
						read_vbat /= 10;
				}

				info_parameters->battery_voltage = read_vbat&0xFFFF;

				if(typeid(*active_window) == typeid(VehicleInfoWindow))
					this->window_handler->refresh();

				return true;
			} else if(ai_d->data[2] == 0x7 && ai_d->l >= 5) { //Range.
				const uint8_t decimal_count = (ai_d->data[3]&0x70) >> 4;
				const bool miles = (ai_d->data[3]&0x80) != 0;

				unsigned long read_range = 0;
				for(int i=4;i<ai_d->l;i+=1) {
					read_range <<= 8;
					read_range += ai_d->data[i];
				}

				for(int i=0;i<decimal_count;i+=1)
					read_range /= 10;

				info_parameters->range = read_range&0xFFFF;
				info_parameters->range_miles = miles;

				if(typeid(*active_window) == typeid(VehicleInfoWindow))
					this->window_handler->refresh();

				return true;
			} else if((ai_d->data[2] == 0x8 || ai_d->data[2] == 0x9) && ai_d->l >= 5) { //Instantenous or average economy.
				const uint8_t decimal_count = (ai_d->data[3]&0x1F);
				const uint8_t unit = (ai_d->data[3]&0xC0) >> 6;

				float norm_economy;

				if((ai_d->data[3]&0x20) == 0) {
					unsigned long read_economy = 0;
					for(int i=4;i<ai_d->l;i+=1) {
						read_economy <<= 8;
						read_economy += ai_d->data[i];
					}

					norm_economy = read_economy;
					for(int i=0;i<decimal_count;i+=1)
						norm_economy /= 10;
				} else //Undefined fuel economy.
					norm_economy = -1;

				if(ai_d->data[2] == 0x8) {
					info_parameters->inst_mpg = norm_economy;
					info_parameters->inst_units = (econ_unit)unit;
				} else if(ai_d->data[2] == 0x9) {
					info_parameters->avg_mpg = norm_economy;
					info_parameters->avg_units = (econ_unit)unit;
				}

				if(typeid(*active_window) == typeid(VehicleInfoWindow))
					this->window_handler->refresh();

				return true;
			} else if(ai_d->data[2] == 0xA && ai_d->l >= 5) { //Trip timer.
				info_parameters->trip_time_minutes = (ai_d->data[3]&0x80) != 0;
				unsigned long read_time = 0;
				for(int i=4;i<ai_d->l;i+=1) {
					read_time <<= 8;
					read_time += ai_d->data[i];
				}

				info_parameters->trip_time = read_time&0xFFFF;

				if(typeid(*active_window) == typeid(VehicleInfoWindow))
					this->window_handler->refresh();
			} else if(ai_d->data[2] == 0xB && ai_d->l >= 5) { //Trip distance.
				const uint8_t decimal_count = (ai_d->data[3]&0x70) >> 4;
				info_parameters->distance_miles = (ai_d->data[3]&0x80) != 0;

				unsigned long read_dist = 0;
				for(int i=4;i<ai_d->l;i+=1) {
					read_dist <<= 8;
					read_dist += ai_d->data[i];
				}

				if(decimal_count == 0)
					info_parameters->trip_distance = read_dist * 10;
				else {
					for(int i=1;i<decimal_count;i+=1)
						read_dist /= 10;

					info_parameters->trip_distance = read_dist;
				}

				if(typeid(*active_window) == typeid(VehicleInfoWindow))
					this->window_handler->refresh();
			} else if(ai_d->data[2] == 0xC && ai_d->l >= 5) { //Cruise speed.
				const uint8_t decimal_count = (ai_d->data[3]&0x70) >> 4;
				info_parameters->cruise_mph = (ai_d->data[3]&0x80) != 0;

				unsigned long read_speed = 0;
				for(int i=4;i<ai_d->l;i+=1) {
					read_speed <<= 8;
					read_speed += ai_d->data[i];
				}

				if(decimal_count == 0)
					info_parameters->cruise_speed = read_speed * 10;
				else {
					for(int i=1;i<decimal_count;i+=1)
						read_speed /= 10;

					info_parameters->cruise_speed = read_speed;
				}

				if(typeid(*active_window) == typeid(VehicleInfoWindow))
					this->window_handler->refresh();
			} else
				return false;
		}
	} else if(ai_d->sender == ID_CANSLATOR && ai_d->data[1] == 0x10) { //Night mode.
		if(this->attribute_list->day_night_settings == DAY_NIGHT_AUTO)
			this->setDayNight((ai_d->data[3]&0x80) != 0);
		return true;
	} else if(ai_d->sender == ID_CANSLATOR && ai_d->l >= 2 && ai_d->data[1] == 0x11) { //Light position.
		InfoParameters* info_parameters = window_handler->getVehicleInfo();
		setLightState(ai_d, info_parameters);
		return true;
	} else if(ai_d->sender == ID_CANSLATOR && ai_d->l >= 4 && ai_d->data[1] == 0x12) { //Gear/transmission type.
		InfoParameters* info_parameters = window_handler->getVehicleInfo();
		NavWindow* active_window = this->window_handler->getActiveWindow();

		info_parameters->transmission_type = (transmission_type_t)ai_d->data[2];
		info_parameters->selected_pos = (ai_d->data[3] >> 4)&0xF;
		info_parameters->gear = (ai_d->data[3]&0xF);

		if(typeid(*active_window) == typeid(VehicleInfoWindow))
			this->window_handler->refresh();

		return true;
	} else if(ai_d->sender == ID_CANSLATOR && ai_d->l >= 3 && ai_d->data[1] == 0x14) { //Auto stop.
		InfoParameters* info_parameters = window_handler->getVehicleInfo();
		if(ai_d->data[2] > 1)
			info_parameters->auto_stop = AUTO_STOP_ON;
		else
			info_parameters->auto_stop = (auto_stop_t)ai_d->data[2];
	} else if(ai_d->l >= 2 && ai_d->data[1] == 0x33) { //Hybrid system.
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
			info_parameters->charge_assist_pos = ai_d->data[5];
		}

		return true;
	}
	return false;
}

//Send the poweroff message.
void AidF_Nav_Computer::sendPowerOffMessage() {
	uint8_t poweroff_data[] = {0xA0};
	AIData poweroff_msg(sizeof(poweroff_data), ID_NAV_COMPUTER, 0xFF, poweroff_data);
	aibus_handler->writeAIData(&poweroff_msg, false);
}

//Set day/night mode.
void AidF_Nav_Computer::setDayNight(const bool night) {
	if(*this->night == night)
		return;
	
	*this->night = night;

	if(night)
		setColorProfile(&active_color_profile, night_profile);
	else
		setColorProfile(&active_color_profile, day_profile);
	
	if(this->br != NULL)
		delete this->br;

	if(active_color_profile.background == active_color_profile.background2)
		this->br = new BR_Solid(this->lw, this->lh, active_color_profile.background);
	else
		this->br = new BR_Gradient(this->lw, this->lh, active_color_profile.background, active_color_profile.background2, active_color_profile.vertical, active_color_profile.square);

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

	aibus_handler->writeAIData(&header_msg, attribute_list->mirror_connected);
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
	if(this->br != NULL)
		delete this->br;

	if(active_color_profile.background == active_color_profile.background2)
		this->br = new BR_Solid(this->lw, this->lh, active_color_profile.background);
	else
		this->br = new BR_Gradient(this->lw, this->lh, active_color_profile.background, active_color_profile.background2, active_color_profile.vertical, active_color_profile.square);
}

//Set the monitor on or off.
void AidF_Nav_Computer::setMonitorOn(const bool on) {
	uint8_t screen_off_data[] = {0x10, (uint8_t)(on ? 0x1 : 0x0)};
	AIData screen_off_msg(sizeof(screen_off_data), ID_NAV_COMPUTER, ID_NAV_SCREEN, screen_off_data);
	aibus_handler->writeAIData(&screen_off_msg);

	screen_off_data[0] = 0x71;
	AIData radio_screen_off(sizeof(screen_off_data), ID_NAV_COMPUTER, ID_RADIO, screen_off_data);
	aibus_handler->writeAIData(&radio_screen_off, attribute_list->radio_connected);

	const bool last_on = attribute_list->monitor_on;
	attribute_list->monitor_on = on;

	if(on && !last_on) {
		NavWindow* active_window = this->window_handler->getActiveWindow();
		if(typeid(*active_window) != typeid(main_window))
			attribute_list->next_window = NEXT_WINDOW_MAIN;
	}
}

//Thread setup.
void setup(AidF_Nav_Computer* nav_computer) {
}

//Thread loop.
void loop(AidF_Nav_Computer* nav_computer) {
	nav_computer->loop();
}

int main(int argc, char* args[]) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");

	TTF_Init();
	int screen_w = DEFAULT_W, screen_h = DEFAULT_H;
	getResolution(&screen_w, &screen_h);

	#ifdef RPI_UART
	SDL_Window* window = SDL_CreateWindow("AidF", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
	#else
	SDL_Window* window = SDL_CreateWindow("AidF", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	#endif
	SDL_ShowCursor(SDL_DISABLE);

	if(SDL_GetCurrentVideoDriver() == NULL) {
		system("echo \"Video driver not found.\n\"");
	} else {
		const std::string driver = std::string("echo \"Video Driver: ") + SDL_GetCurrentVideoDriver() + "\"\n";
		system(driver.c_str());
	}

	system("echo \"Available video drivers include:\n\"");
	const int driver_count = SDL_GetNumVideoDrivers();
	for(int i=0;i<driver_count;i+=1) {
		const std::string name_cmd = std::string("echo \"Driver " + std::to_string(i) + ": " + SDL_GetVideoDriver(i) + "\n\"");
		system(name_cmd.c_str());
	}

	string port = "";
	for(int i=1;i<argc;i+=1) {
		const string arg = args[i];

		if(arg.find("/dev") != string::npos) { //Serial port.
			port = arg;
			break;
		}
	}

	AidF_Nav_Computer nav_computer(window, port, screen_w, screen_h);
	setup(&nav_computer);

	#ifdef RPI_UART
	if(argc > 1) {
		for(int i=1;i<argc;i+=1) {
			std::string arg = args[i];
			if(arg.compare("-t") == 0) {
				nav_computer.test_mode = true;
				break;
			}
		}
	}
	#endif

	while(nav_computer.running)
		loop(&nav_computer);

	SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();
}

//Frame thread function.
void *frameThread(void* frame_v) {
	FrameParameters* frame_parameters = (FrameParameters*)frame_v;

	while(*frame_parameters->frame >= 0 && *frame_parameters->run) {
		if(*frame_parameters->frame < GRAD_W*3 - 1 && *frame_parameters->frame >= 0)
			*frame_parameters->frame += 1;
		else if(*frame_parameters->frame >= 0)
			*frame_parameters->frame = 0;
		
		usleep(1000000/75);

		if(!*frame_parameters->run)
			break;
	}

	void* result;
	return result;
}

//Timer thread function.
void *millisThread(void* millis_v) {
	ElapsedMillis* elapsed_millis = (ElapsedMillis*)millis_v;

	while(*elapsed_millis->run) {
		usleep(1000);

		elapsed_millis->time += 1;

		if(!*elapsed_millis->run)
			break;
	}

	void* result;
	return result;
}
