#include "Video_Socket.h"

ClientVideoSocketHandler::ClientVideoSocketHandler(const char* socket_path, SDL_Window* window, const int w, const int h) {
	this->w = w;
	this->h = h;

	mpv_handler = mpv_create();
	mpv_set_option_string(mpv_handler, "vo", "libmpv");
	
	string geometry_str = to_string(w) + "x" + to_string(h) + "+0+0";
	mpv_set_option_string(mpv_handler, "geometry", geometry_str.c_str());

	#ifdef RPI_MPV
	mpv_set_option_string(mpv_handler, "hwdec", "rpi");
	#endif

	mpv_set_option_string(mpv_handler, "hwdec", "no");
	mpv_set_option_string(mpv_handler, "demuxer-rawvideo-fps", "60");
	mpv_set_option_string(mpv_handler, "untimed", "yes");

	#ifdef RPI_MPV
	mpv_set_option_string(mpv_handler, "fs", "yes");
	#else
	mpv_set_option_string(mpv_handler, "cmd", "no");
	#endif

	mpv_set_option_string(mpv_handler, "fps", "60");
	mpv_set_option_string(mpv_handler, "profile", "low-latency");
	mpv_set_option_string(mpv_handler, "no-correct-pts", "yes");
	mpv_set_option_string(mpv_handler, "input-ipc-server", "/tmp/mka_cmd");
	mpv_set_option_string(mpv_handler, "keep-open", "yes");
	mpv_set_option_string(mpv_handler, "idle", "yes");

	mpv_initialize(mpv_handler);
	sdl_gl = SDL_GL_CreateContext(window);
	createRenderContext(mpv_handler, &mpv_gl, &mpv_ogl_init_params, &mpv_advanced_control);

	if(!mpv_events_set) {
		mpv_events_set = true;
		wakeup_on_mpv_events = SDL_RegisterEvents(1);
		wakeup_on_mpv_render_update = SDL_RegisterEvents(1);
	}

	setCallbacks(mpv_handler, mpv_gl);

	const char* load_command[] = {"loadfile", VIDEO_SOCKET_PATH, NULL};
	mpv_command_async(mpv_handler, 0, load_command);
}

ClientVideoSocketHandler::~ClientVideoSocketHandler() {
	if(this->ipc_fifo >= 0)
		close(ipc_fifo);

	mpv_render_context_free(mpv_gl);
	mpv_destroy(mpv_handler);
}

//Refresh the socket connection.
void ClientVideoSocketHandler::refreshSocket(const char* socket_path) {
	if(this->ipc_fifo >= 0)
		return;

	ipc_fifo = open(socket_path, O_RDONLY);
}

//Read a video message.
int ClientVideoSocketHandler::readVideoMessage() {
	uint8_t data[0x8000];

	const int message_size = read(ipc_fifo, data, sizeof(data));

	if(message_size < 0)
		return -1;
	else if(message_size == 0)
		return 0;

	char s_data[message_size];
	for(int i=0;i<message_size;i+=1)
		s_data[i] = (char)data[i];

	const char* video_command[] = {s_data, NULL};
	cout<<"Command run "<<mpv_command_async(mpv_handler, 0, video_command)<<endl;

	return message_size;
}

//Handle an SDL event.
void ClientVideoSocketHandler::handleEvent(SDL_Event* event) {
	cout<<"Event received: "<<event->type<<endl;
}

//Clear the socket address.
void ClientVideoSocketHandler::clearSocket() {
	close(ipc_fifo);
	ipc_fifo = -1;
}

//Get the client socket.
int ClientVideoSocketHandler::getClient() {
	return this->ipc_fifo;
}

//Refresh the video socket.
void ClientVideoSocketHandler::refreshVideo() {
	const char* load_command[] = {"loadfile", VIDEO_SOCKET_PATH, "replace", NULL};
	mpv_command_async(mpv_handler, 0, load_command);
}

//Socket thread function.
void *videoSocketThread(void* parameters_v) {
	VideoSocketParameters* parameters = (VideoSocketParameters*)parameters_v;
	ClientVideoSocketHandler* socket_ptr = parameters->socket_handler;

	while(*parameters->running) {
		if(socket_ptr->getClient() < 0) {
			socket_ptr->refreshSocket(parameters->socket_path.c_str());
			continue;
		}

		const int socket_byte_count = socket_ptr->readVideoMessage();

		if(socket_byte_count == 0)
			socket_ptr->clearSocket();
	}

	void* result;
	return result;
}

//Video thread function.
void *videoPlayThread(void* parameters_v) {
	VideoSocketParameters* parameters = (VideoSocketParameters*)parameters_v;
	ClientVideoSocketHandler* socket_ptr = parameters->socket_handler;

	while(*parameters->running) {
		//if(socket_ptr->getClient() < 0)
		//	continue;
		socket_ptr->refreshVideo();

		SDL_Event event;
		if(SDL_WaitEventTimeout(&event, 100))
			socket_ptr->handleEvent(&event);
	}

	void* result;
	return result;
}
