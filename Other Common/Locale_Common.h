#include <stdint.h>

#ifndef locale_common_h
#define locale_common_h

typedef uint8_t menu_index_t;

struct MenuList {
	menu_index_t start, end;
	const char** menu_str;
	const char* title;

	//The menu length.
	unsigned int size() const {
		return end - start;
	}

	//Get the local menu index of the option.
	int getLocalIndex(const menu_index_t index) const {
		if(index < start || index >= end)
			return -1;
		else
			return index - start;
	}

	//Get a local menu entry at index.
	const char* getLocalEntry(const menu_index_t index) const {
		const int new_index = getLocalIndex(index);
		if(new_index >= 0)
			return (*this)[new_index];
		else
			return nullptr;
	}

	//Get the global menu index of int index.
	menu_index_t getGlobalIndex(const int index) const {
		if(index < 0 || index >= size())
			return (menu_index_t)0;
		else
			return (menu_index_t)(index + start);
	}

	const char* operator[] (int index) const {
		return menu_str[index];
	}
};

#endif
