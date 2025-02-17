#include "Vehicle_Info_Window.h"

VehicleInfoWindow::VehicleInfoWindow(AttributeList *attribute_list, InfoParameters* info_parameters) : NavWindow(attribute_list) {
	this->info_parameters = info_parameters;

	title_box = new TextBox(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y, this->w-MAIN_TITLE_AREA_X, TITLE_HEIGHT, ALIGN_H_L, ALIGN_V_M, 40, &this->color_profile->text);
	if(info_parameters->hybrid_system_present)
		title_box->setText("Hybrid Power Flow");
	else
		title_box->setText("Vehicle Information");

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
}

VehicleInfoWindow::~VehicleInfoWindow() {
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
	this->title_box->renderText();
	
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
}

void VehicleInfoWindow::drawWindow() {
	if(!this->active)
		return;

	const int frame = attribute_list->frame;

	const int16_t silhouette_start_x = 100;

	int16_t silhouette_start_y = attribute_list->h/2 - 240/2;
	if(info_parameters->hybrid_system_present)
		silhouette_start_y -= 30;
	
	this->title_box->drawText();

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
	}
}
