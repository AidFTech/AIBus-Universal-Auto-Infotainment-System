#include "AidF_Color_Profile.h"

uint8_t getRedComponent(const uint32_t color) {
	return uint8_t((color&0xFF000000)>>24);
}

uint8_t getGreenComponent(const uint32_t color) {
	return uint8_t((color&0x00FF0000)>>16);
}

uint8_t getBlueComponent(const uint32_t color) {
	return uint8_t((color&0x0000FF00)>>8);
}

uint8_t getAlphaComponent(const uint32_t color) {
	return uint8_t(color&0x000000FF);
}

SDL_Color getSDLColor(const uint32_t color) {
	SDL_Color the_return;

	the_return.r = getRedComponent(color);
	the_return.g = getGreenComponent(color);
	the_return.b = getBlueComponent(color);
	the_return.a = getAlphaComponent(color);

	return the_return;
}

void setColorProfile(AidFColorProfile* paste, AidFColorProfile copy) {
	paste->background = copy.background;
	paste->background2 = copy.background2;
	paste->text = copy.text;
	paste->button = copy.button;
	paste->selection = copy.selection;
	paste->headerbar = copy.headerbar;
	paste->outline = copy.outline;
}
