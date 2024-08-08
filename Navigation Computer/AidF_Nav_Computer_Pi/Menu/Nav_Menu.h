#include <stdint.h>
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <time.h>

#include "../Text_Box.h"
#include "../Window/Attribute_List.h"

#ifndef nav_menu_h
#define nav_menu_h

#define SQ_WIDTH 25
#define OUTLINE_THICKNESS 5

#define NAV_UP 0
#define NAV_DOWN 1
#define NAV_LEFT 2
#define NAV_RIGHT 3

class NavMenu {
public:
	NavMenu(AttributeList* attribute_list,
							const int16_t x,
							const int16_t y,
							const uint16_t text_w,
							const uint16_t text_h,
							const uint16_t newl,
							const int8_t indent,
							const uint8_t size,
							const uint8_t rows,
							const bool loop,
							std::string title);
	~NavMenu();

	void setItem(std::string text, const uint16_t item);
	void setTextItems();
	void drawMenu();

	void setSelected(const uint16_t selected);
	uint16_t getSelected();

	void setTitle(std::string title);

	int16_t getX();
	int16_t getY();
	uint16_t getLength();

	void incrementSelected();
	void decrementSelected();
	
	void navigateSelected(const int8_t direction);

	uint16_t getFilledTextItems();
	void refreshItems();
private:
	AttributeList* attribute_list;
	SDL_Renderer* renderer;

	std::string title;
	std::string* items;
	TextBox* title_box;
	std::vector<TextBox*> item_box;

	uint16_t length = 0, selected = 0;
	AidFColorProfile* color_profile;

	int16_t x, y;
	int8_t indent;
	uint16_t w, h, text_w, text_h;
	uint8_t rows;
	bool loop;

	//Item text change timer.
	bool item_text_changed = false;
	clock_t text_change_time;

	void getIndexPosition(uint16_t index, int16_t* x_pos, int16_t* y_pos);
};

#endif
