#include "Video_Socket_C.h"

void* getProcAddressMPV(void *fn_ctx, const char *name) {
	return SDL_GL_GetProcAddress(name);
}

//Create the render context.
int createRenderContext(mpv_handle* mpv_handler, mpv_render_context** mpv_gl, mpv_opengl_init_params* mpv_ogl_init_params, int* mpv_advanced_control) {
	mpv_render_param params[] = {
		{MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
		{MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, mpv_ogl_init_params},
		{MPV_RENDER_PARAM_ADVANCED_CONTROL, mpv_advanced_control},
		{0},
	};

	return mpv_render_context_create(mpv_gl, mpv_handler, params);
}

//On an MPV event.
void onMPVEvents(void* ctx) {
	SDL_Event event = {.type = wakeup_on_mpv_events};
	SDL_PushEvent(&event);
}

//On a render update.
void onMPVRenderUpdate(void* ctx) {
	SDL_Event event = {.type = wakeup_on_mpv_render_update};
	SDL_PushEvent(&event);
}

//Set the MPV/SDL callbacks.
void setCallbacks(mpv_handle* mpv_handler, mpv_render_context* mpv_gl) {
	mpv_set_wakeup_callback(mpv_handler, onMPVEvents, NULL);
	mpv_render_context_set_update_callback(mpv_gl, onMPVRenderUpdate, NULL);
}