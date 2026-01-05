#include "Vehicle_Info_Window.h"

VehicleInfoWindow::VehicleInfoWindow(AttributeList *attribute_list, InfoParameters* info_parameters) : NavWindow(attribute_list),
	title_box(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y, this->w-MAIN_TITLE_AREA_X, TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 40, &this->color_profile->text) {
	
	this->info_parameters = info_parameters;
	this->param_index = info_parameters->param_index;

	if(info_parameters->hybrid_system_present)
		title_box.setText(getString(LOCALE_STRING_HYBRID_POWER_FLOW, attribute_list->locale));
	else
		title_box.setText(getString(LOCALE_STRING_VEHICLE_INFORMATION, attribute_list->locale));

	for(int i=0;i<PARAM_COUNT;i+=1) {
		const int16_t tx = this->w/PARAM_COUNT*i, ty = this->h-PARAM_H;
		param_titles[i] = new TextBox(renderer, tx, ty, this->w/PARAM_COUNT, PARAM_H, ALIGN_H_C, ALIGN_V_T, 24, &this->color_profile->text);
		param_text[i] = new TextBox(renderer, tx, ty + 30, this->w/PARAM_COUNT, PARAM_H - 30, ALIGN_H_C, ALIGN_V_T, 26, &this->color_profile->text);
	}

	this->drl_texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 53, 24);
	SDL_SetTextureBlendMode(this->drl_texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(this->drl_texture, NULL, image_data_DRL, 4*53);

	this->side_texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 57, 22);
	SDL_SetTextureBlendMode(this->side_texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(this->side_texture, NULL, image_data_SideMarkers, 4*57);

	this->lowbeam_texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 55, 30);
	SDL_SetTextureBlendMode(this->lowbeam_texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(this->lowbeam_texture, NULL, image_data_LowBeam, 4*55);

	this->highbeam_texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 55, 24);
	SDL_SetTextureBlendMode(this->highbeam_texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(this->highbeam_texture, NULL, image_data_HighBeam, 4*55);

	this->frontfog_texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 42, 27);
	SDL_SetTextureBlendMode(this->frontfog_texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(this->frontfog_texture, NULL, image_data_FrontFog, 4*42);

	this->rearfog_texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 40, 24);
	SDL_SetTextureBlendMode(this->rearfog_texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(this->rearfog_texture, NULL, image_data_RearFog, 4*40);

	this->electric_motor_texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 48, 32);
	SDL_SetTextureBlendMode(this->electric_motor_texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(this->electric_motor_texture, NULL, image_data_elec, 4*48);

	this->engine_texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 61, 37);
	SDL_SetTextureBlendMode(this->engine_texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(this->engine_texture, NULL, image_data_eng, 61*4);

	uint8_t silhouette_data[sizeof(image_data_Silhouette)];
	for(int i=0;i<sizeof(image_data_Silhouette);i+=1)
		silhouette_data[i] = image_data_Silhouette[i];

	SDL_Surface* silhouette_surface = SDL_CreateRGBSurfaceWithFormatFrom(silhouette_data, 600, 240, 1, 607/8, SDL_PIXELFORMAT_INDEX1MSB);
	SDL_Color colors[] = {{0,0,0,0}, getSDLColor(attribute_list->color_profile->button)};
	SDL_SetPaletteColors(silhouette_surface->format->palette, colors, 0, 2);

	this->silhouette_texture = SDL_CreateTextureFromSurface(renderer, silhouette_surface);
	SDL_SetTextureBlendMode(this->silhouette_texture, SDL_BLENDMODE_BLEND);

	uint8_t silhouette_outline_data[sizeof(image_data_Silhouette_Outline)];
	for(int i=0;i<sizeof(image_data_Silhouette_Outline);i+=1)
		silhouette_outline_data[i] = image_data_Silhouette_Outline[i];

	silhouette_surface = SDL_CreateRGBSurfaceWithFormatFrom(silhouette_outline_data, 600, 240, 1, 607/8, SDL_PIXELFORMAT_INDEX1MSB);
	SDL_Color outline_colors[] = {{0,0,0,0}, getSDLColor(attribute_list->color_profile->outline)};
	SDL_SetPaletteColors(silhouette_surface->format->palette, outline_colors, 0, 2);

	this->silhouette_outline_texture = SDL_CreateTextureFromSurface(renderer, silhouette_surface);
	SDL_SetTextureBlendMode(this->silhouette_outline_texture, SDL_BLENDMODE_BLEND);

	SDL_FreeSurface(silhouette_surface);

	const int16_t text_start_x = attribute_list->w/2 - CHARGE_ASSIST_W/2 - 100, text_start_y = attribute_list->h/2 - 240/2 + 250 - 30;
	text_charge = new TextBox(renderer, text_start_x - 5, text_start_y, 100, CHARGE_ASSIST_H, ALIGN_H_R, ALIGN_V_M, 22, &attribute_list->color_profile->text);
	text_assist = new TextBox(renderer, text_start_x + 105 + CHARGE_ASSIST_W, text_start_y, 100, CHARGE_ASSIST_H, ALIGN_H_L, ALIGN_V_M, 22, &attribute_list->color_profile->text);

	text_charge->setText("Charge");
	text_assist->setText("Assist");

	text_charge->renderText();
	text_assist->renderText();

	for(int i=0;i<PARAM_COUNT;i+=1) {
		refreshParam(param_titles[i], param_text[i], param_index[i]);

		param_text[i]->renderText();
		param_titles[i]->renderText();
	}
}

VehicleInfoWindow::~VehicleInfoWindow() {
	if(settings_menu != NULL)
		delete settings_menu;

	for(int i=0;i<PARAM_COUNT;i+=1) {
		delete param_text[i];
		delete param_titles[i];
	}

	delete text_charge;
	delete text_assist;

	if(this->drl_texture != NULL)
		SDL_DestroyTexture(this->drl_texture);

	if(this->side_texture != NULL)
		SDL_DestroyTexture(this->side_texture);

	if(this->lowbeam_texture != NULL)
		SDL_DestroyTexture(this->lowbeam_texture);

	if(this->highbeam_texture != NULL)
		SDL_DestroyTexture(this->highbeam_texture);

	if(this->frontfog_texture != NULL)
		SDL_DestroyTexture(this->frontfog_texture);

	if(this->rearfog_texture != NULL)
		SDL_DestroyTexture(this->rearfog_texture);

	if(this->electric_motor_texture != NULL)
		SDL_DestroyTexture(this->electric_motor_texture);

	if(this->engine_texture != NULL)
		SDL_DestroyTexture(this->engine_texture);

	if(this->silhouette_texture != NULL)
		SDL_DestroyTexture(this->silhouette_texture);

	if(this->silhouette_outline_texture != NULL)
		SDL_DestroyTexture(this->silhouette_outline_texture);
}

void VehicleInfoWindow::refreshWindow() {
	this->title_box.renderText();
	
	if(this->silhouette_texture != NULL)
		SDL_DestroyTexture(this->silhouette_texture);
	
	uint8_t silhouette_data[sizeof(image_data_Silhouette)];
	for(int i=0;i<sizeof(image_data_Silhouette);i+=1)
		silhouette_data[i] = image_data_Silhouette[i];

	SDL_Surface* silhouette_surface = SDL_CreateRGBSurfaceWithFormatFrom(silhouette_data, 600, 240, 1, 607/8, SDL_PIXELFORMAT_INDEX1MSB);
	SDL_Color colors[] = {{0,0,0,0}, getSDLColor(attribute_list->color_profile->button)};
	SDL_SetPaletteColors(silhouette_surface->format->palette, colors, 0, 2);

	this->silhouette_texture = SDL_CreateTextureFromSurface(renderer, silhouette_surface);
	SDL_SetTextureBlendMode(this->silhouette_texture, SDL_BLENDMODE_BLEND);

	uint8_t silhouette_outline_data[sizeof(image_data_Silhouette_Outline)];
	for(int i=0;i<sizeof(image_data_Silhouette_Outline);i+=1)
		silhouette_outline_data[i] = image_data_Silhouette_Outline[i];

	silhouette_surface = SDL_CreateRGBSurfaceWithFormatFrom(silhouette_outline_data, 600, 240, 1, 607/8, SDL_PIXELFORMAT_INDEX1MSB);
	SDL_Color outline_colors[] = {{0,0,0,0}, getSDLColor(attribute_list->color_profile->outline)};
	SDL_SetPaletteColors(silhouette_surface->format->palette, outline_colors, 0, 2);

	this->silhouette_outline_texture = SDL_CreateTextureFromSurface(renderer, silhouette_surface);
	SDL_SetTextureBlendMode(this->silhouette_outline_texture, SDL_BLENDMODE_BLEND);

	SDL_FreeSurface(silhouette_surface);

	text_charge->renderText();
	text_assist->renderText();

	for(int i=0;i<PARAM_COUNT;i+=1) {
		refreshParam(param_titles[i], param_text[i], param_index[i]);

		param_text[i]->renderText();
		param_titles[i]->renderText();
	}
}

void VehicleInfoWindow::drawWindow() {
	if(!this->active)
		return;

	if(this->settings_menu == NULL) {
		const int frame = attribute_list->frame;

		const int16_t silhouette_start_x = attribute_list->w/2 - 600/2;

		int16_t silhouette_start_y = attribute_list->h/2 - 240/2;
		if(info_parameters->charge_assist_meter && info_parameters->draw_charge_assist)
			silhouette_start_y -= 30;
		
		this->title_box.drawText();

		//Draw the lower info parameters.
		for(int i=0;i<PARAM_COUNT;i+=1) {
			param_titles[i]->drawText();
			param_text[i]->drawText();
		}

		if(this->silhouette_texture != NULL) {
			SDL_Rect silhouette_rect = {silhouette_start_x, silhouette_start_y, 600, 240};
			SDL_RenderCopy(this->renderer, this->silhouette_texture, NULL, &silhouette_rect);
		}

		//Draw the silhouette.
		if(this->silhouette_outline_texture != NULL) {
			SDL_Rect silhouette_rect = {silhouette_start_x, silhouette_start_y, 600, 240};
			SDL_RenderCopy(this->renderer, this->silhouette_outline_texture, NULL, &silhouette_rect);
		}

		//Draw the DRLs.
		if((this->info_parameters->light_state_a & INFO_LIGHTS_A_DRL) != 0 && (this->info_parameters->light_state_a & INFO_LIGHTS_A_FRONT_FOG) == 0) {
			if(this->drl_texture != NULL) {
				SDL_Rect drl_rect = {silhouette_start_x - 15,silhouette_start_y + 160,53,24};
				SDL_RenderCopy(this->renderer, this->drl_texture, NULL, &drl_rect);
			}
		}

		//Draw the side markers.
		if((this->info_parameters->light_state_a & INFO_LIGHTS_A_PARKING) != 0) {
			if(this->side_texture != NULL) {
				SDL_Rect side_rect = {silhouette_start_x + 50,silhouette_start_y + 75,57,22};
				SDL_RenderCopy(this->renderer, this->side_texture, NULL, &side_rect);
			}
		}

		//Draw the low beams.
		if((this->info_parameters->light_state_a & INFO_LIGHTS_A_LOW_BEAM) != 0) {
			if(this->lowbeam_texture != NULL) {
				SDL_Rect lowbeam_rect = {silhouette_start_x - 15, silhouette_start_y + 130, 55, 30};
				SDL_RenderCopy(this->renderer, this->lowbeam_texture, NULL, &lowbeam_rect);
			}
		}

		//Draw the high beams.
		if((this->info_parameters->light_state_a & INFO_LIGHTS_A_HIGH_BEAM) != 0) {
			if(this->highbeam_texture != NULL) {
				SDL_Rect highbeam_rect = {silhouette_start_x - 15, silhouette_start_y + 100, 55, 24};
				SDL_RenderCopy(this->renderer, this->highbeam_texture, NULL, &highbeam_rect);
			}
		}

		//Draw the front foglights.
		if((this->info_parameters->light_state_a & INFO_LIGHTS_A_FRONT_FOG) != 0) {
			if(this->frontfog_texture != NULL) {
				SDL_Rect frontfog_rect = {silhouette_start_x - 5, silhouette_start_y + 160, 42, 27};
				SDL_RenderCopy(this->renderer, this->frontfog_texture, NULL, &frontfog_rect);
			}
		}

		//Draw the rear foglights.
		if((this->info_parameters->light_state_b & INFO_LIGHTS_B_REAR_FOG) != 0) {
			if(this->rearfog_texture != NULL) {
				SDL_Rect rearfog_rect = {silhouette_start_x + 580, silhouette_start_y + 150, 40, 24};
				SDL_RenderCopy(this->renderer, this->rearfog_texture, NULL, &rearfog_rect);
			}
		}

		//Draw the hybrid components.
		if(this->info_parameters->hybrid_system_present) {
			uint8_t last_r, last_g, last_b, last_a;
			SDL_GetRenderDrawColor(renderer, &last_r, &last_g, &last_b, &last_a);

			//Hybrid battery:
			const int inner_battery_height = 64, inner_battery_width = 32;

			//Outer battery body:
			SDL_Rect battery_body_outer = {silhouette_start_x + 400, silhouette_start_y + 70, inner_battery_width + 4, inner_battery_height + 4};
			SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
			SDL_RenderFillRect(renderer, &battery_body_outer);
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderDrawRect(renderer, &battery_body_outer);

			//Battery posts:
			SDL_Rect battery_post_left = {silhouette_start_x + 400 + 5, silhouette_start_y + 64, (inner_battery_width + 4)/3, 7};
			SDL_Rect battery_post_right = {silhouette_start_x + 400 + 2*(inner_battery_width + 4)/3 - 5, silhouette_start_y + 64, (inner_battery_width + 4)/3, 7};
			SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
			SDL_RenderFillRect(renderer, &battery_post_left);
			SDL_RenderFillRect(renderer, &battery_post_right);
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderDrawRect(renderer, &battery_post_left);
			SDL_RenderDrawRect(renderer, &battery_post_right);

			//Inner battery body:
			SDL_Rect battery_body_inner = {silhouette_start_x + 400 + 2, silhouette_start_y + 72, inner_battery_width, inner_battery_height};
			SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
			SDL_RenderFillRect(renderer, &battery_body_inner);

			//Battery gauge:
			const int gauge_height = info_parameters->hybrid_battery_state*inner_battery_height/255;

			SDL_Rect battery_gauge = {silhouette_start_x + 400 + 2, silhouette_start_y + 72 + (inner_battery_height - gauge_height), inner_battery_width, gauge_height};
			if(info_parameters->hybrid_battery_state > 96)
				SDL_SetRenderDrawColor(renderer, 119, 255, 62, 255);
			else
				SDL_SetRenderDrawColor(renderer, 255, 72, 62, 255);
			SDL_RenderFillRect(renderer, &battery_gauge);

			//Electric motor:
			if(this->electric_motor_texture != NULL) {
				SDL_Rect electric_motor_rect = {silhouette_start_x + 250, silhouette_start_y + 150, 48, 32};
				SDL_RenderCopy(this->renderer, this->electric_motor_texture, NULL, &electric_motor_rect);
			}

			//Engine:
			if(this->engine_texture != NULL) {
				SDL_Rect engine_rect = {silhouette_start_x + 230, silhouette_start_y + 60, 61, 37};
				SDL_RenderCopy(this->renderer, this->engine_texture, NULL, &engine_rect);
			}

			SDL_SetRenderDrawColor(renderer, last_r, last_g, last_b, last_a);

			const uint8_t button_red = getRedComponent(color_profile->button), button_green = getGreenComponent(color_profile->button), button_blue = getBlueComponent(color_profile->button), button_alpha = getAlphaComponent(color_profile->button);
			const uint32_t dimmed_button = ((button_red*7/10)<<24) | ((button_green*7/10)<<16) | ((button_blue*7/10)<<8) | button_alpha;
			const uint32_t outline_color = color_profile->outline;

			const uint8_t hybrid_status = info_parameters->hybrid_status_main;

			//Arrows:
			PowerFlowArrow arrow_eb1(renderer, silhouette_start_x + 360, silhouette_start_y + 100, 23 + (((hybrid_status&0x6) == 0) ? 7 : 0), 15);
			PowerFlowArrow arrow_eb2(renderer, silhouette_start_x + 345, silhouette_start_y + 115, 15, 45);
			PowerFlowArrow arrow_eb3(renderer, silhouette_start_x + 305 + (((hybrid_status&0x1) == 0) ? 0 : 10), silhouette_start_y + 160, 30 + (((hybrid_status&0x1) == 0) ? 10 : 0), 15);

			PowerFlowCorner corner_eb12(renderer, silhouette_start_x + 345, silhouette_start_y + 100, 15);
			PowerFlowCorner corner_eb23(renderer, silhouette_start_x + 344, silhouette_start_y + 159, 15);

			PowerFlowArrow arrow_ge(renderer, silhouette_start_x + 267, silhouette_start_y + 104, 15, 28 + (((hybrid_status&0x40) == 0) ? 10 : 0));

			PowerFlowArrow arrow_ew(renderer, silhouette_start_x + 205 + ((((hybrid_status&0x8) == 0) ? 0 : 10)), silhouette_start_y + 165, 30 + ((((hybrid_status&0x10) == 0 && (hybrid_status&0x8) == 0) ? 10 : 0)), 15);

			if((hybrid_status&0x7) == 0) {
				arrow_eb1.drawOutline(dimmed_button, outline_color);
				arrow_eb2.drawOutline(dimmed_button, outline_color);
				arrow_eb3.drawOutline(dimmed_button, outline_color);
				corner_eb12.drawOutline(dimmed_button, outline_color, CORNER_ANGLE_DR);
				corner_eb23.drawOutline(dimmed_button, outline_color, CORNER_ANGLE_UL);
			} else {
				if((hybrid_status&0x1) != 0) { //Battery to motor.
					arrow_eb1.drawFilled(COLOR_EV1, COLOR_EV2, outline_color, frame, ARROW_DIR_LEFT, false);
					arrow_eb2.drawFilled(COLOR_EV1, COLOR_EV2, outline_color, frame, ARROW_DIR_DOWN, false);
					arrow_eb3.drawFilled(COLOR_EV1, COLOR_EV2, outline_color, frame, ARROW_DIR_LEFT, true);
					
					corner_eb12.drawFilled(COLOR_EV1, COLOR_EV2, outline_color, frame, CORNER_ANGLE_DR, false);
					corner_eb23.drawFilled(COLOR_EV1, COLOR_EV2, outline_color, frame, CORNER_ANGLE_UL, true);
				} else if((hybrid_status&0x2) != 0) { //Motor to battery.
					arrow_eb1.drawFilled(COLOR_EV1, COLOR_EV2, outline_color, frame, ARROW_DIR_RIGHT, true);
					arrow_eb2.drawFilled(COLOR_EV1, COLOR_EV2, outline_color, frame, ARROW_DIR_UP, false);
					arrow_eb3.drawFilled(COLOR_EV1, COLOR_EV2, outline_color, frame, ARROW_DIR_RIGHT, false);

					corner_eb12.drawFilled(COLOR_EV1, COLOR_EV2, outline_color, frame, CORNER_ANGLE_DR, true);
					corner_eb23.drawFilled(COLOR_EV1, COLOR_EV2, outline_color, frame, CORNER_ANGLE_UL, false);
				} else if((hybrid_status&0x4) != 0) { //Motor to battery, regen.
					arrow_eb1.drawFilled(COLOR_REG1, COLOR_REG2, outline_color, frame, ARROW_DIR_RIGHT, true);
					arrow_eb2.drawFilled(COLOR_REG1, COLOR_REG2, outline_color, frame, ARROW_DIR_UP, false);
					arrow_eb3.drawFilled(COLOR_REG1, COLOR_REG2, outline_color, frame, ARROW_DIR_RIGHT, false);

					corner_eb12.drawFilled(COLOR_REG1, COLOR_REG2, outline_color, frame, CORNER_ANGLE_DR, true);
					corner_eb23.drawFilled(COLOR_REG1, COLOR_REG2, outline_color, frame, CORNER_ANGLE_UL, false);
				}
			}

			if((hybrid_status&0x40) == 0)
				arrow_ge.drawOutline(dimmed_button, outline_color);
			else
				arrow_ge.drawFilled(COLOR_ENG1, COLOR_ENG2, outline_color, frame, ARROW_DIR_DOWN, true);

			if((hybrid_status&0x18) == 0)
				arrow_ew.drawOutline(dimmed_button, outline_color);
			else {
				if((hybrid_status&0x8) != 0) //Motor to wheels.
					arrow_ew.drawFilled(COLOR_EV1, COLOR_EV2, outline_color, frame, ARROW_DIR_LEFT, true);
				else if((hybrid_status&0x10) != 0) //Wheels to motor.
					arrow_ew.drawFilled(COLOR_REG1, COLOR_REG2, outline_color, frame, ARROW_DIR_RIGHT, true);
			}

			if(info_parameters->hybrid_system_type != 1) { //Not present in a series hybrid.
				PowerFlowArrow arrow_gw1(renderer, silhouette_start_x + 230, silhouette_start_y + 100, 15, 38);
				PowerFlowArrow arrow_gw2(renderer, silhouette_start_x + 200, silhouette_start_y + 139, 30, 15);

				PowerFlowCorner corner_gw12(renderer, silhouette_start_x + 229, silhouette_start_y + 137, 15);

				if((hybrid_status&0x20) == 0) {
					arrow_gw1.drawOutline(dimmed_button, outline_color);
					arrow_gw2.drawOutline(dimmed_button, outline_color);
					corner_gw12.drawOutline(dimmed_button, outline_color, CORNER_ANGLE_UL);
				} else {
					arrow_gw1.drawFilled(COLOR_ENG1, COLOR_ENG2, outline_color, frame, ARROW_DIR_DOWN, false);
					arrow_gw2.drawFilled(COLOR_ENG1, COLOR_ENG2, outline_color, frame, ARROW_DIR_LEFT, true);
					corner_gw12.drawFilled(COLOR_ENG1, COLOR_ENG2, outline_color, frame, CORNER_ANGLE_UL, true);
				}
			}

			if(info_parameters->charge_assist_meter && info_parameters->draw_charge_assist) {
				PowerFlowArrow arrow_charge(renderer, w/2 - CHARGE_ASSIST_W/2, silhouette_start_y + 250, CHARGE_ASSIST_W/2, CHARGE_ASSIST_H);
				PowerFlowArrow arrow_assist(renderer, w/2, silhouette_start_y + 250, CHARGE_ASSIST_W/2, CHARGE_ASSIST_H);
				if(info_parameters->charge_assist_pos > 0x7F) { //Assist.
					arrow_charge.drawOutline(dimmed_button, outline_color);
					arrow_assist.drawPartialFilled(COLOR_EV1, COLOR_EV2, outline_color, dimmed_button, frame, ARROW_DIR_RIGHT, uint8_t((int(info_parameters->charge_assist_pos)-0x7F)*2), false);
				} else if(info_parameters->charge_assist_pos < 0x7F) { //Charge.
					arrow_charge.drawPartialFilled(((hybrid_status&0x4) != 0) ? COLOR_REG1 : COLOR_EV1, ((hybrid_status&0x4) != 0) ? COLOR_REG2 : COLOR_EV2, outline_color, dimmed_button, frame, ARROW_DIR_LEFT, uint8_t((0x7F-int(info_parameters->charge_assist_pos))*2), false);
					arrow_assist.drawOutline(dimmed_button, outline_color);
				} else {
					arrow_charge.drawOutline(dimmed_button, outline_color);
					arrow_assist.drawOutline(dimmed_button, outline_color);
				}

				text_assist->drawText();
				text_charge->drawText();
			}
		}
	} else
		this->settings_menu->drawMenu();
}

bool VehicleInfoWindow::handleAIBus(AIData* ai_d) {
	if(this->settings_menu != NULL) {
		if(this->settings_menu->handleAIBus(ai_d)) {
			attribute_list->aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
			return true;
		}
	}

	if(ai_d->sender == ID_NAV_SCREEN) {
		if(ai_d->l >= 3 && ai_d->data[0] == 0x30) { //Button press.
			const uint8_t control = ai_d->data[1], state = ai_d->data[2]>>6;

			if(control == 0x7 && state == 0x2) {
				attribute_list->aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
				handleEnterButton();
				return true;
			} else if(control == 0x27 && state == 0x2) {
				attribute_list->aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
				if(active_menu == INFO_ACTIVE_MENU_MAIN) {
					this->settings_menu = NULL;
					this->active_menu = INFO_ACTIVE_MENU_NONE;
				} else if(active_menu == INFO_ACTIVE_MENU_PARAM)
					createDefaultSettingsMenu();
				return true;
			} else if(control == 0x51 && state == 0x2) { //Menu button.
				attribute_list->aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, ai_d->sender);
				
				if(this->settings_menu == NULL)
					createDefaultSettingsMenu();
				else {
					this->settings_menu = NULL;
					this->active_menu = INFO_ACTIVE_MENU_NONE;
				}

				return true;
			}
		}
	}

	return false;
}

//Handle the Enter button.
void VehicleInfoWindow::handleEnterButton() {
	if(this->settings_menu == NULL)
		return;

	const int selected = this->settings_menu->getSelected() - 1;

	if(selected < 0)
		return;

	if(active_menu == INFO_ACTIVE_MENU_MAIN) {
		const MenuList settings_menu_list = getMenu(MENU_INDEX_INFORMATION_MAIN, attribute_list->locale);

		switch(settings_menu_list.getGlobalIndex(selected)) {
		case MENU_INDEX_INFORMATION_MAIN_DISP_1:
		case MENU_INDEX_INFORMATION_MAIN_DISP_2:
		case MENU_INDEX_INFORMATION_MAIN_DISP_3:
		case MENU_INDEX_INFORMATION_MAIN_DISP_4:
			createParamSettingsMenu(uint8_t(selected));
			break;
		case MENU_INDEX_INFORMATION_MAIN_CHARGE_ASSIST:
			if(info_parameters->charge_assist_meter) {
				info_parameters->draw_charge_assist = !info_parameters->draw_charge_assist;
				if(info_parameters->draw_charge_assist)
					this->settings_menu->setItem(std::string("#RON ") + settings_menu_list.getLocalEntry(MENU_INDEX_INFORMATION_MAIN_CHARGE_ASSIST), settings_menu_list.getLocalIndex(MENU_INDEX_INFORMATION_MAIN_CHARGE_ASSIST));
				else
					this->settings_menu->setItem(std::string("#ROF ") + settings_menu_list.getLocalEntry(MENU_INDEX_INFORMATION_MAIN_CHARGE_ASSIST), settings_menu_list.getLocalIndex(MENU_INDEX_INFORMATION_MAIN_CHARGE_ASSIST));
				saveParamSettings();
			}
			break;
		case MENU_INDEX_INFORMATION_MAIN_CRUISE:
			info_parameters->display_cruise = !info_parameters->display_cruise;
			if(info_parameters->display_cruise)
				this->settings_menu->setItem(std::string("#RON ") + settings_menu_list.getLocalEntry(MENU_INDEX_INFORMATION_MAIN_CRUISE), settings_menu_list.getLocalIndex(MENU_INDEX_INFORMATION_MAIN_CRUISE));
			else
				this->settings_menu->setItem(std::string("#ROF ") + settings_menu_list.getLocalEntry(MENU_INDEX_INFORMATION_MAIN_CRUISE), settings_menu_list.getLocalIndex(MENU_INDEX_INFORMATION_MAIN_CRUISE));
			saveParamSettings();
			break;
		default:
			break;
		}
	} else if(active_menu == INFO_ACTIVE_MENU_PARAM) {
		param_index[active_param] = (info_param)selected;
		saveParamSettings();
		createDefaultSettingsMenu();
		refreshWindow();
	}
}

//Refresh the given parameter.
void VehicleInfoWindow::refreshParam(TextBox* title, TextBox* text, const info_param param) {
	if(param == INFO_PARAM_BATTERY_VOLTAGE) { //Battery voltage.
		title->setText("Vbat");

		if(info_parameters->battery_voltage > 100) {
			std::string batt_text = "";
			if(info_parameters->battery_voltage_precision >= 1) {
				batt_text = std::to_string(info_parameters->battery_voltage/100) + ".";
				
				if(info_parameters->battery_voltage_precision >= 2) {
					if(info_parameters->battery_voltage%100 >= 10)
						batt_text += std::to_string(info_parameters->battery_voltage%100);
					else
						batt_text += "0" + std::to_string(info_parameters->battery_voltage%100);
				} else { //One decimal point.
					batt_text += std::to_string((info_parameters->battery_voltage%100)/10);
				}
			} else
				batt_text = std::to_string(info_parameters->battery_voltage/100);

			batt_text += "V";

			text->setText(batt_text);
		} else
			text->setText("-.--V");
	} else if(param == INFO_PARAM_OUTSIDE_TEMP && info_parameters->outside_temp_sent) { //Outside temp.
		title->setText(getString(LOCALE_STRING_OUTSIDE_TEMP, attribute_list->locale));

		std::string temp_text = std::to_string(info_parameters->outside_temp/10) + '\xb0';
		if(info_parameters->outside_temp_fahrenheit)
			temp_text += "F";
		else
			temp_text += "C";

		text->setText(temp_text);
	} else if(param == INFO_PARAM_COOLANT_TEMP && info_parameters->coolant_temp_sent) { //Coolant temp.
		title->setText(getString(LOCALE_STRING_COOLANT_TEMP, attribute_list->locale));

		string temp_text = std::to_string(info_parameters->coolant_temp/10) + '\xb0';
		if(info_parameters->coolant_temp_fahrenheit)
			temp_text += "F";
		else
			temp_text += "C";

		text->setText(temp_text);
	} else if(param == INFO_PARAM_RANGE) {
		title->setText(getString(LOCALE_STRING_RANGE, attribute_list->locale));

		string range_text = to_string(info_parameters->range);
		if(info_parameters->range_miles)
			range_text += "mi";
		else
			range_text += "km";

		text->setText(range_text);
	} else if(param == INFO_PARAM_INST_ECONOMY || param == INFO_PARAM_TRIP_AVERAGE_ECONOMY) {
		float econ = -1;
		econ_unit unit = ECON_L_100KM;

		if(param == INFO_PARAM_INST_ECONOMY) {
			title->setText(getString(LOCALE_STRING_INST_ECONOMY, attribute_list->locale));
			econ = info_parameters->inst_mpg;
			unit = info_parameters->inst_units;
		} else if(param == INFO_PARAM_TRIP_AVERAGE_ECONOMY) {
			title->setText(getString(LOCALE_STRING_AVG_ECONOMY, attribute_list->locale));
			econ = info_parameters->avg_mpg;
			unit = info_parameters->avg_units;
		}

		string econ_text;
		if(econ >= 0) {
			econ_text = to_string(econ);
			if(econ_text.length() > 4)
				econ_text = econ_text.substr(0, 4);
			if(econ_text.length() > 0 && econ_text[econ_text.length()-1] == '.')
				econ_text.pop_back();
		} else {
			if(unit == ECON_L_100KM)
				econ_text = "-.--";
			else
				econ_text = "--.-";
		}

		switch(unit) {
		case ECON_L_100KM:
			econ_text += "L/100km";
			break;
		case ECON_KM_L:
			econ_text += "km/L";
			break;
		case ECON_MPG_US:
		case ECON_MPG_IMP:
			econ_text += "mpg";
			break;
		}

		text->setText(econ_text);
	} else if(param == INFO_PARAM_TRIP_TIMER) {
		title->setText(getString(LOCALE_STRING_TRIP_TIMER, attribute_list->locale));

		const int msd = info_parameters->trip_time/60, lsd = info_parameters->trip_time%60;
		
		string timer_str = to_string(msd) + ":";
		if(lsd >= 10)
			timer_str += to_string(lsd);
		else
			timer_str += "0" + to_string(lsd);

		if(info_parameters->trip_time_minutes)
			timer_str += "m";

		text->setText(timer_str);
	} else if(param == INFO_PARAM_TRIP_DISTANCE) {
		title->setText(getString(LOCALE_STRING_TRIP_DISTANCE, attribute_list->locale));
		
		string dist_str = to_string(info_parameters->trip_distance/10) + "." + to_string(info_parameters->trip_distance%10);
		if(info_parameters->distance_miles)
			dist_str += "mi";
		else
			dist_str += "km";

		text->setText(dist_str);
	} else if(param == INFO_PARAM_CRUISE_SPEED) {
		title->setText(getString(LOCALE_STRING_CRUISE_SPEED, attribute_list->locale));

		if(info_parameters->cruise_speed >= 0) {
			string speed_str = to_string(info_parameters->cruise_speed/10);
			if(info_parameters->cruise_mph)
				speed_str += "mph";
			else
				speed_str += "km/h";

			text->setText(speed_str);
		} else {
			text->setText("--");
		}
	} else if(param == INFO_PARAM_GEAR) {
		title->setText(getString(LOCALE_STRING_GEAR, attribute_list->locale));

		string gear_str = "";
		switch(info_parameters->transmission_type) {
		case TRANSMISSION_MANUAL:
		case TRANSMISSION_SMG:
			if(info_parameters->gear <= 0) {
				if(info_parameters->selected_pos == TRANSMISSION_POS_NEUTRAL)
					gear_str = "N";
				else if(info_parameters->selected_pos == TRANSMISSION_POS_REVERSE)
					gear_str = "R";
			} else {
				if(info_parameters->transmission_type == TRANSMISSION_SMG) {
					if(info_parameters->selected_pos == TRANSMISSION_POS_MANUAL)
						gear_str = "M";
					else
						gear_str = "A";
				} else
					gear_str = "";
				gear_str += to_string(int(info_parameters->gear));
			}
			break;
		case TRANSMISSION_OTHER:
			break;
		default:
			if(info_parameters->selected_pos == TRANSMISSION_POS_NEUTRAL)
				gear_str = "N";
			else if(info_parameters->selected_pos == TRANSMISSION_POS_REVERSE)
				gear_str = "R";
			else if(info_parameters->selected_pos == TRANSMISSION_POS_PARK)
				gear_str = "P";
			else if(info_parameters->selected_pos == TRANSMISSION_POS_DRIVE)
				gear_str = "D";
			else if(info_parameters->selected_pos == TRANSMISSION_POS_MANUAL)
				gear_str = "M";
			else if(info_parameters->selected_pos == TRANSMISSION_POS_LOW)
				gear_str += "L";

			switch(info_parameters->transmission_type) {
			case TRANSMISSION_AUTOMATIC:
			case TRANSMISSION_SEMI_AUTO:
			case TRANSMISSION_DCT:
				if(info_parameters->gear > 0)
					gear_str += to_string(int(info_parameters->gear));
				break;
			case TRANSMISSION_IVT:
			case TRANSMISSION_CVT:
				if(info_parameters->selected_pos == TRANSMISSION_POS_MANUAL && info_parameters->gear > 0)
					gear_str += to_string(int(info_parameters->gear));
			default:
				break;
			}
			break;
		}

		text->setText(gear_str);
 	} else {
		title->setText("");
		text->setText("");
	}
}

//Create the default settings menu.
void VehicleInfoWindow::createDefaultSettingsMenu() {
	this->active_menu = INFO_ACTIVE_MENU_MAIN;

	if(settings_menu != NULL)
		delete settings_menu;

	MenuList settings_menu_list = getMenu(MENU_INDEX_INFORMATION_MAIN, attribute_list->locale);

	this->settings_menu = new NavMenu(attribute_list, 0, 40, this->w, 35, INFO_SETTING_COUNT, ALIGN_H_L, 30, INFO_SETTING_COUNT, false, settings_menu_list.title);

	for(int i=0;i<settings_menu_list.size();i+=1) {
		if(i == settings_menu_list.getLocalIndex(MENU_INDEX_INFORMATION_MAIN_CRUISE)) {
			const std::string cruise_option = info_parameters->display_cruise ? "#RON " : "#ROF ";
			this->settings_menu->setItem(cruise_option + settings_menu_list[i], i);
		} else if(i == settings_menu_list.getLocalIndex(MENU_INDEX_INFORMATION_MAIN_CHARGE_ASSIST)) {
			if(info_parameters->charge_assist_meter) {
				const std::string charge_assist_option = info_parameters->draw_charge_assist ? "#RON " : "#ROF ";
				this->settings_menu->setItem(charge_assist_option + settings_menu_list[i], i);
			}
		} else //TODO: Print the Units option only if a Units menu exists.
			this->settings_menu->setItem(settings_menu_list[i], i);
	}

	this->settings_menu->setSelected(1);
}

//Create the parameter settings menu.
void VehicleInfoWindow::createParamSettingsMenu(const uint8_t active_param) {
	MenuList param_menu = getMenu(MENU_INDEX_INFORMATION_PARAM, attribute_list->locale);
	const int param_count = param_menu.size();

	this->active_menu = INFO_ACTIVE_MENU_PARAM;
	this->active_param = active_param;

	if(settings_menu != NULL)
		delete settings_menu;

	this->settings_menu = new NavMenu(attribute_list, 0, 40, this->w, 35, param_count, ALIGN_H_L, 30, param_count, false, param_menu.title + std::to_string(active_param + 1));

	std::string params[param_count];
	for(int i=0;i<param_count;i+=1) {
		if(param_index[active_param] == i)
			params[i] = "#CON ";
		else
			params[i] = "#COF ";
	}

	for(int i=0;i<param_count;i+=1)
		params[i] += param_menu[i];

	for(int i=0;i<param_count;i+=1) {
		bool sel = true;
		if(i != 0) {
			for(int j=0;j<PARAM_COUNT;j+=1) {
				if(param_index[j] == i && j != active_param)
					sel = false;	
				if(!sel)
					break;
			}

			if(i < 8) {
				if(((info_parameters->supported_b)&(1<<(i-1))) == 0)
					sel = false;
			} else if(i < 16) {
				if(((info_parameters->supported_a)&(1<<(i-8))) == 0)
					sel = false;
			}
		}

		if(sel)
			settings_menu->setItem(params[i], i);
	}

	this->settings_menu->setSelected(1);
}

//Save parameter settings to a file.
void VehicleInfoWindow::saveParamSettings() {
	uint8_t params[PARAM_COUNT];
	for(int i=0;i<PARAM_COUNT;i+=1)
		params[i] = (uint8_t)this->info_parameters->param_index[i];

	saveVehicleInfoParams(this->info_parameters->display_cruise, this->info_parameters->draw_charge_assist, params, PARAM_COUNT);
}