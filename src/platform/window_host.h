#ifndef GH_WINDOW_HOST_H
#define GH_WINDOW_HOST_H
#include "native_input.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_WINDOW_OK,GH_WINDOW_ARGUMENT,GH_WINDOW_SYSTEM,GH_WINDOW_GPU,GH_WINDOW_CLOSED,GH_WINDOW_MINIMIZED} gh_window_error;
typedef struct gh_window_host gh_window_host;
typedef struct {unsigned width,height;int show;const char*title;gh_native_input_config input;gh_native_input_map map;int xinput_index;} gh_window_config;
/* HWND/D3D11 hardware swapchain; hidden unless show explicitly1. Ownthread.
 * Poll pumps only thiswindow's messages; no global keyhooks or inputinjection. */
gh_window_error gh_window_create(const gh_window_config*,gh_window_host**);
void gh_window_destroy(gh_window_host*);
gh_window_error gh_window_poll(gh_window_host*,gh_native_input_snapshot*);
/* RGBbytepreserving nearest presentation; alpha is alreadycomposited and
 * displayedopaque. Explicitdisplayaspect e.g4/3 independentofrawpixelaspect.
 * Aspectfit letterboxblack; no inferredGS/fade/gameframe rendering. */
gh_window_error gh_window_present(gh_window_host*,const unsigned char*rgba,size_t bytes,unsigned source_width,unsigned source_height,unsigned aspect_numerator,unsigned aspect_denominator);
gh_window_error gh_window_readback(gh_window_host*,unsigned char*,size_t,unsigned*width,unsigned*height);
uintptr_t gh_window_native_handle(const gh_window_host*);
/* QPCNTSC startup clock: rationalrate suppliedexplicitly (30000/1001 default
 * chosenbycaller). Wait advances one deadline, pumpsmessages and sleeps,
 * dropslateness after >4periods insteadofbursting busy-loopfakeframes. */
typedef struct {int64_t frequency,epoch;uint64_t ordinal;uint32_t numerator,denominator;} gh_frame_clock;
int gh_frame_clock_init(gh_frame_clock*,uint32_t rate_numerator,uint32_t rate_denominator);
gh_window_error gh_window_wait_frame(gh_window_host*,gh_frame_clock*);
#ifdef __cplusplus
}
#endif
#endif
