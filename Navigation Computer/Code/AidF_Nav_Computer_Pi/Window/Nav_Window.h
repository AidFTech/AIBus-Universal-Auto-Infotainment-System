#include <stdint.h>

#include "Attribute_List.h"

#include "../Background/Nav_Background.h"
#include "../AidF_Color_Profile.h"
#include "../Text_Box.h"
#include "../AIBus/AIBus.h"

#ifndef nav_window_h
#define nav_window_h

#define MAIN_TITLE_AREA_X 25
#define MAIN_TITLE_AREA_Y 50
#define TITLE_HEIGHT 40

class NavWindow {
public:
	NavWindow(AttributeList *attribute_list);
	~NavWindow();

	virtual void drawWindow();
	virtual void clearWindow();
	virtual void refreshWindow();
	virtual void exitWindow();
	
	virtual bool handleAIBus(AIData* msg);
	
	virtual void setActive(bool active);
	virtual bool getActive();

protected:
	AttributeList* attribute_list;
	SDL_Renderer* renderer;

	AidFColorProfile* color_profile;
	uint16_t w, h;

	bool active = false;
};

#endif
