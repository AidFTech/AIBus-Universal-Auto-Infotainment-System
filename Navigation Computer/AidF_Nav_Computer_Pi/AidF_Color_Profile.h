#include <stdint.h>
#include <SDL2/SDL.h>

#ifndef aidf_color_profile_h
#define aidf_color_profile_h

#define DEFAULT_BACKGROUND 0xFFFF3AFF
#define DEFAULT_TEXT_COLOR 0x000000FF
#define DEFAULT_BUTTON_COLOR 0xFFFF00FF
#define DEFAULT_SELECTION_COLOR 0xFF0000FF
#define DEFAULT_HEADERBAR_COLOR 0xFFFF00FF
#define DEFAULT_OUTLINE_COLOR 0x000000FF

#define DEFAULT_BACKGROUND_NIGHT 0x000084FF
#define DEFAULT_TEXT_NIGHT 0xFFAA00FF
#define DEFAULT_BUTTON_NIGHT 0x525DFFFF
#define DEFAULT_SELECTION_NIGHT 0xF72000FF
#define DEFAULT_HEADERBAR_NIGHT 0x0000FFFF
#define DEFAULT_OUTLINE_NIGHT 0x000000FF

#define DEFAULT_SQUARE 100

struct AidFColorProfile {
	uint32_t background = DEFAULT_BACKGROUND, background2 = DEFAULT_BACKGROUND, text = DEFAULT_TEXT_COLOR, button = DEFAULT_BUTTON_COLOR,
			selection = DEFAULT_SELECTION_COLOR, headerbar = DEFAULT_HEADERBAR_COLOR, outline = DEFAULT_OUTLINE_COLOR;
	uint16_t square = DEFAULT_SQUARE;
	bool vertical = false;
};

uint8_t getRedComponent(const uint32_t color);
uint8_t getGreenComponent(const uint32_t color);
uint8_t getBlueComponent(const uint32_t color);
uint8_t getAlphaComponent(const uint32_t color);

SDL_Color getSDLColor(const uint32_t color);

void setColorProfile(AidFColorProfile* paste, AidFColorProfile copy);

#endif
