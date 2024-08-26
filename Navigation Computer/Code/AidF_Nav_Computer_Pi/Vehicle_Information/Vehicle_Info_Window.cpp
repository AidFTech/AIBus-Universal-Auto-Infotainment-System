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
}

void VehicleInfoWindow::refreshWindow() {
	this->title_box->renderText();
}

void VehicleInfoWindow::drawWindow() {
	if(!this->active)
		return;
	
	this->title_box->drawText();
	if(this->drl_texture != NULL) {
		SDL_Rect drl_rect = {90,120,53,24};
		SDL_RenderCopy(this->renderer, this->drl_texture, NULL, &drl_rect);
	}

	if(this->side_texture != NULL) {
		SDL_Rect side_rect = {150,120,57,22};
		SDL_RenderCopy(this->renderer, this->side_texture, NULL, &side_rect);
	}

	if(this->lowbeam_texture != NULL) {
		SDL_Rect lowbeam_rect = {90, 150, 55, 30};
		SDL_RenderCopy(this->renderer, this->lowbeam_texture, NULL, &lowbeam_rect);
	}

	if(this->highbeam_texture != NULL) {
		SDL_Rect highbeam_rect = {150, 150, 55, 24};
		SDL_RenderCopy(this->renderer, this->highbeam_texture, NULL, &highbeam_rect);
	}

	if(this->frontfog_texture != NULL) {
		SDL_Rect frontfog_rect = {90, 180, 42, 27};
		SDL_RenderCopy(this->renderer, this->frontfog_texture, NULL, &frontfog_rect);
	}

	if(this->rearfog_texture != NULL) {
		SDL_Rect rearfog_rect = {150, 180, 40, 24};
		SDL_RenderCopy(this->renderer, this->rearfog_texture, NULL, &rearfog_rect);
	}
}