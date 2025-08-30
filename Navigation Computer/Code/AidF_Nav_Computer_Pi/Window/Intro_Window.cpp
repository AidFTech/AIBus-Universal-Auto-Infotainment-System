#include "Intro_Window.h"

IntroWindow::IntroWindow(AttributeList *attribute_list) : NavWindow(attribute_list) {
	uint32_t default_background = DEFAULT_BACKGROUND;
	this->logo_box = new TextBox(this->renderer, 0, 0, this->w, this->h, ALIGN_H_C, ALIGN_V_M, int(this->h)*1/3 < 255 ? uint8_t(int(this->h)*1/3) : 255, &default_background);
	this->logo_box->setText("AidF");
}

IntroWindow::~IntroWindow() {
	delete this->logo_box;
}

void IntroWindow::drawWindow() {
	SDL_Rect background_rect = {0,0,this->w,this->h};
	SDL_RenderFillRect(renderer, &background_rect);

	this->logo_box->drawText();
}

bool IntroWindow::handleAIBus(AIData* ai_d) {
	if(ai_d->sender == ID_CANSLATOR && ai_d->receiver == 0xFF) {
		if(ai_d->l >= 3 && ai_d->data[0] == 0xA1 && ai_d->data[1] == 0x2 && (ai_d->data[2]&0xF) != 0) //Key on.
			attribute_list->next_window = NEXT_WINDOW_MAIN;
	}

	return false;
}
