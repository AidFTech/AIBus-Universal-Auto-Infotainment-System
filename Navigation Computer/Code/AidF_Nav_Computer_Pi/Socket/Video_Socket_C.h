#include <mpv/client.h>
#include <mpv/render_gl.h>

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_stdinc.h>

#include <stdbool.h>

#ifndef video_socket_c_h
#define video_socket_c_h

static bool mpv_events_set = false;
static Uint32 wakeup_on_mpv_render_update, wakeup_on_mpv_events;

#ifdef __cplusplus
extern "C" {
#endif
void* getProcAddressMPV(void *fn_ctx, const char *name);
int createRenderContext(mpv_handle* mpv_handler, mpv_render_context** mpv_gl, mpv_opengl_init_params* mpv_ogl_init_params, int* mpv_advanced_control);

void onMPVEvents(void* ctx);
void onMPVRenderUpdate(void* ctx);

void setCallbacks(mpv_handle* mpv_handler, mpv_render_context* mpv_gl);
#ifdef __cplusplus
}
#endif

#endif