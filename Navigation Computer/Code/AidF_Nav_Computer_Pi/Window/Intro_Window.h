#include <stdint.h>

#include "Nav_Window.h"
#include "../AIBus/AIBus.h"
#include "../Text_Box.h"
#include "../AidF_Color_Profile.h"

#ifndef intro_window_h
#define intro_window_h

class IntroWindow : public NavWindow {
public:
	IntroWindow(AttributeList *attribute_list);
	~IntroWindow();

	void drawWindow();

	bool handleAIBus(AIData* ai_d);
private:
	TextBox* logo_box;
};

#endif
