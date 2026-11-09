#ifndef GH_ADX_STREAM_H
#define GH_ADX_STREAM_H
#include "../assets/adx.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_adx_stream gh_adx_stream;
typedef struct {
 gh_adx_info format;
 /* Counts are for this start/seek session. loop_count counts decoder loop
  * crossings in queued audio, not a claim that the listener heard them. */
 uint64_t submitted_frames,completed_frames,loop_count;
 uint32_t queued_buffers;
 int prepared,started,paused,complete,muted;
 float gain;
} gh_adx_stream_status;
/* Native replacement of the platform stream, not original request/pool policy.
 * Borrows immutable encoded until close. Open validates and opens a stereo
 * PCM endpoint but never starts playback. All calls belong to opening thread. */
gh_adx_error gh_adx_stream_open(const void*,size_t,const gh_adx_key*,gh_adx_stream**);
gh_adx_error gh_adx_stream_set_muted(gh_adx_stream*,int); /* before first start */
/* Finite 0..1 native PCM gain, applied to future queue fills. This is not the
 * original SDK dB/volume command; caller performs its evidenced conversion. */
gh_adx_error gh_adx_stream_set_gain(gh_adx_stream*,float);
gh_adx_error gh_adx_stream_set_loop(gh_adx_stream*,int); /* before start; header loop only */
gh_adx_error gh_adx_stream_start(gh_adx_stream*);
gh_adx_error gh_adx_stream_pause(gh_adx_stream*,int);
/* Poll retires actual WHDR_DONE buffers and refills a bounded queue. Complete
 * requires EOF plus every submitted PCM buffer consumed by waveOut. */
gh_adx_error gh_adx_stream_poll(gh_adx_stream*,gh_adx_stream_status*);
/* Stops/resets the actual endpoint; keeps exact decoder position. Seek only
 * while stopped. Start then queues from that position. */
gh_adx_error gh_adx_stream_stop(gh_adx_stream*);
gh_adx_error gh_adx_stream_seek(gh_adx_stream*,uint32_t);
void gh_adx_stream_close(gh_adx_stream*);
#ifdef __cplusplus
}
#endif
#endif
