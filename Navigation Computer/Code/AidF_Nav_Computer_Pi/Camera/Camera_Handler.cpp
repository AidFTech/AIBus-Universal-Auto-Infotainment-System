#include "Camera_Handler.h"

CameraHandler::CameraHandler(SDL_Renderer* renderer, AttributeList* attribute_list, VideoSocketParameters* socket_handler, Window_Handler* window_handler) :
	message_box(renderer, 0, attribute_list->h *9/10, attribute_list->w, 35, ALIGN_H_C, ALIGN_V_M, 30, &text_color) {
	this->attribute_list = attribute_list;
	this->socket_handler = socket_handler;
	this->window_handler = window_handler;

	const char* camera_message = getString(LOCALE_STRING_REAR_CAMERA_MESSAGE, attribute_list->locale);
	message_box.setText(camera_message);
	message_box.renderText();
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
	if(socket_handler->socket_handler != nullptr) {
		delete socket_handler->socket_handler;
		socket_handler->socket_handler = nullptr;
	}

	if(filesystem::exists(filesystem::path(path))) {
		socket_handler->socket_handler = new ClientVideoSocketHandler(renderer, CAMERA_VIDEO_IPC_PATH, path, attribute_list->w, attribute_list->h);
		socket_handler->socket_path = path;
	}
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