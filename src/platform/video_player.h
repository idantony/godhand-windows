#ifndef GH_VIDEO_PLAYER_H
#define GH_VIDEO_PLAYER_H
#include "disc.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_VIDEO_OK,GH_VIDEO_ARGUMENT,GH_VIDEO_MEMORY,GH_VIDEO_DISC,GH_VIDEO_PROCESS,GH_VIDEO_CODEC,GH_VIDEO_AUDIO,GH_VIDEO_PRESENT} gh_video_error;
typedef struct gh_video_player gh_video_player;
typedef int (*gh_video_present)(void *,const unsigned char *rgba,size_t bytes,unsigned width,unsigned height,uint64_t pts_ns);
typedef struct {uint64_t frames_presented,audio_sample_frames;int video_eof,audio_done,complete;uint32_t process_exit;} gh_video_status;
/* Restricted verified Movie0: original clover.sfd MPEG2 640x448 30000/1001,
 * ADX48k stereo. FFmpeg is an explicit native codec executable, not a guest
 * interpreter. Uses bounded subfile reads directly from caller's ISO. Buffers
 * contain one RGBA frame and <=10s decoded PCM. No extraction or fake finish.
 * Presenter must synchronously commit real native output; zero fails the task.
 * PCM is really queued through Windows waveOut. Open/poll/close same thread. */
gh_video_error gh_video_open_movie0(const char *iso,const char *ffmpeg,gh_video_present,void *,gh_video_player **);
/* Explicitper-playerhostmastermute, onlybeforefirstpoll. DoesnotchangeOS
 * devicevolume: decodedownedPCM ismultipliedbyzero justbeforewaveOutqueue.
 * Decoder/realqueue/completionstillrun, neverfakeaudioDONE. */
gh_video_error gh_video_set_muted(gh_video_player *,int muted);
gh_video_error gh_video_poll(gh_video_player *,gh_video_status *);
void gh_video_close(gh_video_player *);
#ifdef __cplusplus
}
#endif
#endif
