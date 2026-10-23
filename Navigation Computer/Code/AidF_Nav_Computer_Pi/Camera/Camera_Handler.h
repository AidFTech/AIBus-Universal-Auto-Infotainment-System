#include <filesystem>

#include <stdint.h>
#include <SDL2/SDL.h>

#include <pthread.h>

#include "../Text_Box.h"

#include "../Locale/Locale.h"

#include "../Window/Attribute_List.h"
#include "../Window_Handler.h"

#include "../Socket/Video_Socket.h"

using namespace std;

class CameraHandler {
public:
	CameraHandler(SDL_Renderer* renderer, AttributeList* attribute_list, VideoSocketParameters* socket_handler, Window_Handler* window_handler);
	~CameraHandler();

	void setCameraActive(const bool camera_active);

	void overlayCameraInfo(SDL_Texture* video_texture);

	void setCameraPath(const char* path);
	void setCameraMessage(const char* message);
	void resetCameraMessage();
private:
	uint32_t text_color = 0xFFB12AFF;

	AttributeList* attribute_list;
	VideoSocketParameters* socket_handler;
	Window_Handler* window_handler;

	SDL_Renderer* renderer;

	TextBox message_box;

	bool camera_active = false;
	pthread_t camera_thread;
};