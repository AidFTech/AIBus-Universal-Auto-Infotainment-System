#include <stdint.h>

#include <SDL2/SDL_video.h>

#include "../Window/Nav_Window.h"
#include "../AidF_Color_Profile.h"

#include "../Locale/Locale.h"

#ifndef mirror_window_h
#define mirror_window_h

class MirrorWindow : public NavWindow {
public:
	MirrorWindow(AttributeList *attribute_list);
	~MirrorWindow();

	void refreshWindow();
	void drawWindow();

	void exitWindow();

	void writeConnectDisconnectMessage(const bool connect);
private:
	TextBox title_box, message_box;
};

#endif