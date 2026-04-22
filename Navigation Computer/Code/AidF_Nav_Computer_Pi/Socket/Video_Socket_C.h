#include <mpv/client.h>
#include <mpv/render_gl.h>

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_stdinc.h>

#include <stdbool.h>

#include <stdio.h>

#include "Video_Socket_Params.h"

#ifndef video_socket_c_h
#define video_socket_c_h

#ifdef __cplusplus
extern "C" {
#endif
void* getProcAddressMPV(void *fn_ctx, const char *name);
int createRenderContext(mpv_handle* mpv_handler, mpv_render_context** mpv_gl, int* mpv_advanced_control);

void onMPVEvents(void* ctx);
void onMPVRenderUpdate(void* ctx);

void setCallbacks(mpv_handle* mpv_handler, mpv_render_context* mpv_gl);
int mpvPixelRender(mpv_render_context* renderer, const int w, const int h, size_t* pitch, void* pixels);
#ifdef __cplusplus
}
#endif

#endif