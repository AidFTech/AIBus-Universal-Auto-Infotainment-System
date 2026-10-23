#include "Camera_Handler.h"

CameraHandler::CameraHandler(SDL_Renderer* renderer, AttributeList* attribute_list, VideoSocketParameters* socket_handler, Window_Handler* window_handler) :
	message_box(renderer, 0, attribute_list->h *9/10, attribute_list->w, 35, ALIGN_H_C, ALIGN_V_M, 30, &text_color) {
	this->attribute_list = attribute_list;
	this->socket_handler = socket_handler;
	this->window_handler = window_handler;

	const char* camera_message = getString(LOCALE_STRING_REAR_CAMERA_MESSAGE, attribute_list->locale);
	message_box.setText(camera_message);
	message_box.renderText();

	setCameraPath(socket_handler->socket_path.c_str());
}

CameraHandler::~CameraHandler() {
	if(socket_handler->thread_run)
		pthread_cancel(camera_thread);
}

//Set whether the camera is on or off.
void CameraHandler::setCameraActive(const bool camera_active) {
	const bool last_active = this->camera_active;
	this->camera_active = camera_active;

	if(this->camera_active && !last_active) { //Camera turned on.
		if(filesystem::exists(filesystem::path(socket_handler->socket_path))) {
			socket_handler->socket_handler = new ClientVideoSocketHandler(renderer, CAMERA_VIDEO_IPC_PATH, socket_handler->socket_path, attribute_list->w, attribute_list->h);
			pthread_create(&camera_thread, NULL, videoPlayThread, (void*)socket_handler);
		}
	} else if(!this->camera_active && last_active) { //Camera turned off.
		bool run = false;
		bool* last_run = socket_handler->running;

		socket_handler->running = &run;

		if(socket_handler->thread_run)
			pthread_join(camera_thread, NULL);

		socket_handler->running = last_run;

		if(socket_handler->socket_handler != nullptr) {
			delete socket_handler->socket_handler;
			socket_handler->socket_handler = nullptr;
		}
	}
}

//Overlay the camera message and other info.
void CameraHandler::overlayCameraInfo(SDL_Texture* video_texture) {
	SDL_SetRenderTarget(renderer, video_texture);
	message_box.drawText();
	SDL_SetRenderTarget(renderer, NULL);

	window_handler->drawClockHeader();
}

//Set the camera path.
void CameraHandler::setCameraPath(const char* path) {
	if(camera_active) {
		if(socket_handler->socket_handler != nullptr) {
			bool run = false;
			bool* last_run = socket_handler->running;

			socket_handler->running = &run;

			if(socket_handler->thread_run)
				pthread_join(camera_thread, NULL);

			socket_handler->running = last_run;

			delete socket_handler->socket_handler;
			socket_handler->socket_handler = nullptr;
		}

		if(filesystem::exists(filesystem::path(path))) {
			socket_handler->socket_handler = new ClientVideoSocketHandler(renderer, CAMERA_VIDEO_IPC_PATH, path, attribute_list->w, attribute_list->h);
			socket_handler->socket_path = path;

			pthread_create(&camera_thread, NULL, videoPlayThread, (void*)socket_handler);
		}
	} else
		socket_handler->socket_path = path;
}

//Set the message displayed at the bottom of the camera screen.
void CameraHandler::setCameraMessage(const char* message) {
	message_box.setText(message);
	message_box.renderText();
}

//Reset the camera message to the default.
void CameraHandler::resetCameraMessage() {
	const char* camera_message = getString(LOCALE_STRING_REAR_CAMERA_MESSAGE, attribute_list->locale);
	message_box.setText(camera_message);
	message_box.renderText();
}