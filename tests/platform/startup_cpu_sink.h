#ifndef GH_STARTUP_CPU_SINK_H
#define GH_STARTUP_CPU_SINK_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include "../../src/platform/startup_host.h"
#include <stdio.h>
#include <stdlib.h>

#define GH_CPU_SINK_CHECK(expression) do { \
    if(!(expression)){ \
        fprintf(stderr,"CPU sink line %d: %s\n",__LINE__,#expression); \
        abort(); \
    } \
} while(0)

/* Record and complete Movie0 and BGM waveOut buffers without a waveOut device.
 * The host still uses its real muted XAudio2 sound-effect transport. */
static MMRESULT WINAPI sink_open(LPHWAVEOUT out,UINT device,LPCWAVEFORMATEX format,
 DWORD_PTR callback,DWORD_PTR user,DWORD flags)
{
 (void)device;(void)callback;(void)user;(void)flags;
 GH_CPU_SINK_CHECK(out&&format&&format->nSamplesPerSec);
 *out=(HWAVEOUT)(uintptr_t)1;
 return MMSYSERR_NOERROR;
}
static MMRESULT WINAPI sink_prepare(HWAVEOUT device,LPWAVEHDR header,UINT size)
{
 GH_CPU_SINK_CHECK(device&&header&&size==sizeof(*header));
 header->dwFlags|=WHDR_PREPARED;
 return MMSYSERR_NOERROR;
}
static MMRESULT WINAPI sink_unprepare(HWAVEOUT device,LPWAVEHDR header,UINT size)
{
 GH_CPU_SINK_CHECK(device&&header&&size==sizeof(*header));
 header->dwFlags&=~WHDR_PREPARED;
 return MMSYSERR_NOERROR;
}
static MMRESULT WINAPI sink_write(HWAVEOUT device,LPWAVEHDR header,UINT size)
{
 GH_CPU_SINK_CHECK(device&&header&&size==sizeof(*header)&&header->lpData&&header->dwBufferLength);
 header->dwFlags|=WHDR_DONE;
 return MMSYSERR_NOERROR;
}
static MMRESULT WINAPI sink_device(HWAVEOUT device){GH_CPU_SINK_CHECK(device);return MMSYSERR_NOERROR;}
#ifdef __cplusplus
extern "C" {
#endif
__typeof__(&waveOutOpen) __imp_waveOutOpen=sink_open;
__typeof__(&waveOutPrepareHeader) __imp_waveOutPrepareHeader=sink_prepare;
__typeof__(&waveOutUnprepareHeader) __imp_waveOutUnprepareHeader=sink_unprepare;
__typeof__(&waveOutWrite) __imp_waveOutWrite=sink_write;
__typeof__(&waveOutClose) __imp_waveOutClose=sink_device;
__typeof__(&waveOutReset) __imp_waveOutReset=sink_device;
__typeof__(&waveOutPause) __imp_waveOutPause=sink_device;
__typeof__(&waveOutRestart) __imp_waveOutRestart=sink_device;
#ifdef __cplusplus
}
#endif

typedef struct {
 unsigned world,actor,shadow,effects,ui,movie;
} fixture_sink;
static gh_id_error begin(void *p){GH_CPU_SINK_CHECK(p);return GH_ID_OK;}
static gh_id_error mc_draw(void *p,const gh_id_draw_job *job)
{fixture_sink *f=(fixture_sink *)p;GH_CPU_SINK_CHECK(job&&job->entry);f->ui++;return GH_ID_OK;}
static gh_id_error post(void *p,uint16_t message){GH_CPU_SINK_CHECK(p&&message);return GH_ID_OK;}
static gh_id_error fade_draw(void *p,const gh_fade_draw *draw)
{GH_CPU_SINK_CHECK(p&&draw&&draw->visible);return GH_ID_OK;}
static gh_id_error scene_begin(void *p,uint32_t *frontier){(void)frontier;return begin(p);}
static gh_id_error scene_draw(void *p,const gh_id_base *base,const gh_id_draw_job *job,uint32_t *frontier)
{(void)frontier;GH_CPU_SINK_CHECK(base&&job&&job->entry_index<base->entry_count);return mc_draw(p,job);}
static gh_id_error scene_fan(void *p,const gh_cockpit_fan *fan,uint32_t *frontier)
{(void)frontier;GH_CPU_SINK_CHECK(p&&fan&&fan->ready);return GH_ID_OK;}
static gh_id_error scene_cover(void *p,const gh_child_cover_quad *quad,uint32_t *frontier)
{(void)frontier;GH_CPU_SINK_CHECK(p&&quad);return GH_ID_OK;}
static gh_id_error scene_radar(void *p,const gh_cockpit_radar_quad *quad,uint32_t *frontier)
{(void)frontier;GH_CPU_SINK_CHECK(p&&quad);return GH_ID_OK;}
static gh_scene_ui_status scene_message(void *p,const gh_scene_message_glyph_pair *pair,uint32_t *frontier)
{(void)frontier;GH_CPU_SINK_CHECK(p&&pair);return GH_SCENE_UI_OK;}
static gh_object_step_status world(void *p,const gh_object_static_command *command,uint32_t *frontier)
{(void)frontier;fixture_sink *f=(fixture_sink *)p;GH_CPU_SINK_CHECK(command);f->world++;return GH_OBJSTEP_OK;}
static gh_actor_model_status actor(void *p,const gh_actor_draw_command *command,uint32_t *frontier)
{(void)frontier;fixture_sink *f=(fixture_sink *)p;GH_CPU_SINK_CHECK(command);f->actor++;return GH_ACTOR_MODEL_OK;}
static gh_actor_model_status shadow(void *p,const gh_shadow_group *command,uint32_t *frontier)
{(void)frontier;fixture_sink *f=(fixture_sink *)p;GH_CPU_SINK_CHECK(command);f->shadow++;return GH_ACTOR_MODEL_OK;}
static gh_effect_status effect(void *p,const gh_effect_trans_packet *packet)
{fixture_sink *f=(fixture_sink *)p;GH_CPU_SINK_CHECK(packet);f->effects++;return GH_EFFECT_OK;}
static gh_id_error save_map_draw(void *p,const gh_id_base *base,const gh_id_draw_job *job,
 const gh_packed_message_state *packed,uint16_t chapter,uint32_t *frontier)
{
 GH_CPU_SINK_CHECK(packed&&chapter);
 return scene_draw(p,base,job,frontier);
}
static int movie(void *p,const unsigned char *rgba,size_t bytes,unsigned width,unsigned height,uint64_t pts)
{(void)pts;fixture_sink *f=(fixture_sink *)p;GH_CPU_SINK_CHECK(rgba&&width&&height&&bytes==(size_t)width*height*4);f->movie++;return 1;}

static int configure_cpu_sink(gh_startup_host *host,fixture_sink *sink,const char *ffmpeg)
{
 const gh_startup_continuation continuation={1,1,GH_SE_DSP_NATIVE_LINEAR_REFERENCE,ffmpeg,movie,sink};
 const gh_host_scene_ui_callbacks callbacks={sink,scene_begin,scene_draw,scene_fan,scene_cover,scene_message,scene_radar,save_map_draw};
 return gh_startup_host_set_continuation(host,&continuation)==GH_HOST_OK&&
        gh_startup_host_set_mc_consumers(host,mc_draw,sink,post,sink)==GH_HOST_OK&&
        gh_startup_host_set_mc_frame_begin(host,begin,sink)==GH_HOST_OK&&
        gh_startup_host_set_fade_consumer(host,fade_draw,sink)==GH_HOST_OK&&
        gh_startup_host_set_scene_ui_consumers(host,&callbacks)==GH_HOST_OK&&
        gh_startup_host_set_world_consumer(host,world,sink)==GH_HOST_OK&&
        gh_startup_host_set_actor_consumer(host,actor,sink)==GH_HOST_OK&&
        gh_startup_host_set_shadow_consumer(host,shadow,sink)==GH_HOST_OK&&
        gh_startup_host_set_effect_consumer(host,effect,sink)==GH_HOST_OK;
}

#undef GH_CPU_SINK_CHECK
#endif
