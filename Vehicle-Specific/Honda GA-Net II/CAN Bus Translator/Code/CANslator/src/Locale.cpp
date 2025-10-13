#include "Locale.h"

//Get the menu with defined locale.
MenuList getMenu(const menu_index_t index, const uint8_t locale) {
	int menu_index = -1;
	for(int i=0;i<sizeof(MENU_START_INDEX)/sizeof(menu_index_t);i+=1) {
		if(MENU_START_INDEX[i] == index) {
			menu_index = i;
			break;
		}
	}

	if(menu_index < 0) {
		MenuList menu_list;
		menu_list.start = (menu_index_t)0;
		menu_list.end = (menu_index_t)0;
		menu_list.menu_str = nullptr;
		menu_list.title = nullptr;

		return menu_list;
	}

	MenuList return_list;
	return_list.start = (menu_index_t)(MENU_START_INDEX[menu_index] + 1);
	if(menu_index < sizeof(MENU_START_INDEX)/sizeof(menu_index_t) - 1)
		return_list.end = MENU_START_INDEX[menu_index + 1];
	else
		return_list.end = MENU_INDEX_LEN;

	return_list.title = MENUS_ENG[index];
	return_list.menu_str = &MENUS_ENG[index + 1];
	return return_list;
}