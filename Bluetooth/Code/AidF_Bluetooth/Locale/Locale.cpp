#include "Locale.h"

//Get a string with the defined locale.
const char* getString(const bta_text_index index, const uint8_t locale) {
	switch(locale) {
	case 0: //English
		return TEXT_ENG[index];
	default:
		return TEXT_ENG[index];
	}
}

//Get the menu with defined locale.
MenuList getMenu(const bta_menu_index index, const uint8_t locale) {
	int menu_index = -1;
	for(int i=0;i<sizeof(MENU_START_INDEX)/sizeof(bta_menu_index);i+=1) {
		if(MENU_START_INDEX[i] == index) {
			menu_index = i;
			break;
		}
	}

	if(menu_index < 0) {
		MenuList menu_list;
		menu_list.start = (bta_menu_index)0;
		menu_list.end = (bta_menu_index)0;
		menu_list.menu_str = nullptr;
		menu_list.title = nullptr;

		return menu_list;
	}

	MenuList return_list;
	return_list.start = (bta_menu_index)(MENU_START_INDEX[menu_index] + 1);
	if(menu_index < sizeof(MENU_START_INDEX)/sizeof(bta_menu_index) - 1)
		return_list.end = MENU_START_INDEX[menu_index + 1];
	else
		return_list.end = MENU_INDEX_LEN;

	return_list.title = MENUS_ENG[index];
	return_list.menu_str = &MENUS_ENG[index + 1];
	return return_list;
}