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

	int16_t silhouette_start_y = attribute_list->h/2 - 240/2;
	if(info_parameters->hybrid_system_present)
		silhouette_start_y -= 30;
	
	this->title_box->drawText();

	if(this->silhouette_texture != NULL) {
		SDL_Rect silhouette_rect = {100, silhouette_start_y, 600, 240};
		SDL_RenderCopy(this->renderer, this->silhouette_texture, NULL, &silhouette_rect);
	}

	if(this->silhouette_outline_texture != NULL) {
		SDL_Rect silhouette_rect = {100, silhouette_start_y, 600, 240};
		SDL_RenderCopy(this->renderer, this->silhouette_outline_texture, NULL, &silhouette_rect);
	}

	if((this->info_parameters->light_state_a & INFO_LIGHTS_A_DRL) != 0 && (this->info_parameters->light_state_a & INFO_LIGHTS_A_FRONT_FOG) == 0) {
		if(this->drl_texture != NULL) {
			SDL_Rect drl_rect = {85,silhouette_start_y + 160,53,24};
			SDL_RenderCopy(this->renderer, this->drl_texture, NULL, &drl_rect);
		}
	}

	if((this->info_parameters->light_state_a & INFO_LIGHTS_A_PARKING) != 0) {
		if(this->side_texture != NULL) {
			SDL_Rect side_rect = {150,silhouette_start_y + 75,57,22};
			SDL_RenderCopy(this->renderer, this->side_texture, NULL, &side_rect);
		}
	}

	if((this->info_parameters->light_state_a & INFO_LIGHTS_A_LOW_BEAM) != 0) {
		if(this->lowbeam_texture != NULL) {
			SDL_Rect lowbeam_rect = {85, silhouette_start_y + 130, 55, 30};
			SDL_RenderCopy(this->renderer, this->lowbeam_texture, NULL, &lowbeam_rect);
		}
	}

	if((this->info_parameters->light_state_a & INFO_LIGHTS_A_HIGH_BEAM) != 0) {
		if(this->highbeam_texture != NULL) {
			SDL_Rect highbeam_rect = {85, silhouette_start_y + 100, 55, 24};
			SDL_RenderCopy(this->renderer, this->highbeam_texture, NULL, &highbeam_rect);
		}
	}

	if((this->info_parameters->light_state_a & INFO_LIGHTS_A_FRONT_FOG) != 0) {
		if(this->frontfog_texture != NULL) {
			SDL_Rect frontfog_rect = {95, silhouette_start_y + 160, 42, 27};
			SDL_RenderCopy(this->renderer, this->frontfog_texture, NULL, &frontfog_rect);
		}
	}

	if((this->info_parameters->light_state_b & INFO_LIGHTS_B_REAR_FOG) != 0) {
		if(this->rearfog_texture != NULL) {
			SDL_Rect rearfog_rect = {680, silhouette_start_y + 150, 40, 24};
			SDL_RenderCopy(this->renderer, this->rearfog_texture, NULL, &rearfog_rect);
		}
	}
}
