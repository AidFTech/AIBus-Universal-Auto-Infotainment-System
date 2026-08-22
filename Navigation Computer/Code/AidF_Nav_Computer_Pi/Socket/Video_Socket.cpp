#include "Video_Socket.h"

ClientVideoSocketHandler::ClientVideoSocketHandler(SDL_Renderer* renderer, string ipc_path, string socket_path, const int w, const int h) {
	this->w = w;
	this->h = h;

	this->video_socket_path = socket_path;

	mpv_handler = mpv_create();
	mpv_set_option_string(mpv_handler, "vo", "libmpv");

	#ifdef RPI_MPV
	//mpv_set_option_string(mpv_handler, "hwdec", "no");
	#else
	mpv_set_option_string(mpv_handler, "hwdec", "no");
	#endif

	mpv_set_option_string(mpv_handler, "untimed", "yes");

	#ifdef RPI_MPV
	mpv_set_option_string(mpv_handler, "fs", "yes");
	#else
	mpv_set_option_string(mpv_handler, "cmd", "no");
	#endif

	mpv_set_option_string(mpv_handler, "override-display-fps", "60");

	mpv_set_option_string(mpv_handler, "profile", "low-latency");
	mpv_set_option_string(mpv_handler, "keep-open", "yes");
	mpv_set_option_string(mpv_handler, "idle", "yes");

	mpv_set_option_string(mpv_handler, "input-ipc-server", ipc_path.c_str());

	mpv_initialize(mpv_handler);
	mpv_request_log_messages(mpv_handler, "debug");

	createRenderContext(mpv_handler, &mpv_gl, &mpv_advanced_control);

	if(!mpv_events_set) {
		mpv_events_set = true;
		wakeup_on_mpv_events = SDL_RegisterEvents(1);
		wakeup_on_mpv_render_update = SDL_RegisterEvents(1);
	}

	setCallbacks(mpv_handler, mpv_gl);

	system(("chmod 666 " + ipc_path).c_str());
}

ClientVideoSocketHandler::~ClientVideoSocketHandler() {
	mpv_render_context_free(mpv_gl);
	mpv_destroy(mpv_handler);
}

//Object loop function.
void ClientVideoSocketHandler::loop() {
	if(!video_socket_path_set) {
		if(access(video_socket_path.c_str(), F_OK) == 0) {
			const char* load_command[] = {"loadfile", video_socket_path.c_str(), NULL};
			mpv_command_async(mpv_handler, 0, load_command);
			video_socket_path_set = true;
		} else {
			return;
		}
	} else {
		if(access(video_socket_path.c_str(), F_OK) != 0) {
			video_socket_path_set = false;
			return;
		}
	}

	SDL_Event event;
	if(SDL_WaitEvent(&event) > 0)
		this->handleEvent(&event);
}

//Handle an SDL event. Return whether to rerender.
bool ClientVideoSocketHandler::handleEvent(SDL_Event* event) {
	if(event->type == wakeup_on_mpv_render_update) {
		const uint64_t flags = mpv_render_context_update(mpv_gl);
		if((flags&MPV_RENDER_UPDATE_FRAME) == MPV_RENDER_UPDATE_FRAME) {
			refresh = true;
			video_init = true;
			return true;
		}
	} else if(event->type == wakeup_on_mpv_events) {
		while(true) {
			mpv_event* mp_event = mpv_wait_event(mpv_handler, 0);
			if(mp_event->event_id == MPV_EVENT_NONE)
				break;
		}
	}

	return false;
}

//Render a frame.
void ClientVideoSocketHandler::render() {
	size_t* pitch_s = (size_t*)&video_cache.pitch;
	refresh = false;
	mpvPixelRender(mpv_gl, w, h, pitch_s, video_cache.pixels);
}

//Get the video texture.
VideoCache* ClientVideoSocketHandler::getVideoCache() {
	return &this->video_cache;
}

//Return whether the frame was refreshed.
bool ClientVideoSocketHandler::getRefresh() {
	return refresh;
}

//Return whether a video frame has been sent.
bool ClientVideoSocketHandler::getVideoInit() {
	return this->video_init;
}

//Clear video initialization.
void ClientVideoSocketHandler::clearVideoInit() {
	this->video_init = false;
}

//Video thread function.
void *videoPlayThread(void* parameters_v) {
	VideoSocketParameters* parameters = (VideoSocketParameters*)parameters_v;
	ClientVideoSocketHandler* socket_ptr = parameters->socket_handler;

	while(*parameters->running)
		socket_ptr->loop();

	void* result;
	return result;
}
