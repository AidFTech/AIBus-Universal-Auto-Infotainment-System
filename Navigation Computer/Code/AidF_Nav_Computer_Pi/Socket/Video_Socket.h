#if __has_include(<gpiod.h>)
#define RPI_MPV
#endif

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_stdinc.h>

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <mpv/client.h>
#include <mpv/render_gl.h>

#include "AIBus_Socket.h"
#include "Video_Socket_C.h"
#include "Video_Socket_Params.h"

#ifndef video_socket_h
#define video_socket_h

#define AMIRROR_VIDEO_SOCKET_PATH "/tmp/amirror_mpv"
#define AMIRROR_VIDEO_IPC_PATH "/tmp/mka_cmd"

#define CAMERA_VIDEO_SOCKET_PATH "/dev/video"
#define CAMERA_VIDEO_IPC_PATH "/tmp/camera_cmd"

using namespace std;

struct VideoCache {
	void* pixels;
	size_t pitch;
};

class ClientVideoSocketHandler {
public:
	ClientVideoSocketHandler(SDL_Renderer* renderer, string ipc_path, string socket_path, const int w, const int h);
	~ClientVideoSocketHandler();

	void loop();

	bool handleEvent(SDL_Event* event);
	void render();

	VideoCache* getVideoCache();
	bool getRefresh();

	bool getVideoInit();
	void clearVideoInit();
private:
	VideoCache video_cache;
	int w = 800, h = 480;

	string video_socket_path = "";
	bool video_socket_path_set = false;

	mpv_handle* mpv_handler;
	mpv_render_context* mpv_gl;

	int mpv_advanced_control = 1, mpv_flip_y = 1;

	bool refresh = false, video_init = false;
};

struct VideoSocketParameters {
	ClientVideoSocketHandler* socket_handler;
	bool* running;
	string socket_path;
};

void *videoPlayThread(void* parameters_v);

#endif