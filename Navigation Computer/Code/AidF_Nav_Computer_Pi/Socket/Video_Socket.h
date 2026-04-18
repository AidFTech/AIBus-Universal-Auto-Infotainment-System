#if __has_include(<gpiod.h>)
#define RPI_MPV
#endif

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_stdinc.h>

#include <unistd.h>
#include <stdint.h>

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

#ifndef video_socket_h
#define video_socket_h

#define VIDEO_SOCKET_PATH "/tmp/amirror_mpv"

using namespace std;

class ClientVideoSocketHandler {
public:
	ClientVideoSocketHandler(const char* socket_path, SDL_Window* window, const int w, const int h);
	~ClientVideoSocketHandler();

	void refreshSocket(const char* socket_path);
	void clearSocket();

	int readVideoMessage();
	void handleEvent(SDL_Event* event);

	int getClient();
	void refreshVideo();
private:
	int ipc_fifo = -1;
	int w = 800, h = 480;

	mpv_handle* mpv_handler;
	SDL_GLContext sdl_gl;
	mpv_render_context* mpv_gl;

	mpv_opengl_init_params mpv_ogl_init_params = {.get_proc_address = getProcAddressMPV};
	int mpv_advanced_control = 1;
};

struct VideoSocketParameters {
	ClientVideoSocketHandler* socket_handler;
	bool* running;
	string socket_path;
};

void *videoSocketThread(void* parameters_v);
void *videoPlayThread(void* parameters_v);

#endif