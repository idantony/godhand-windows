#include "se_voice.h"
#include "bgm_requests.h"
#include "game_movie_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while(0)

struct gh_adx_stream {
 gh_adx_decoder *decoder;
 gh_adx_stream_status status;
};
static unsigned commands;
gh_adx_error gh_adx_stream_open(const void *bytes,size_t size,const gh_adx_key *key,
                                gh_adx_stream **out)
{
 ++commands;
 gh_adx_stream *stream=calloc(1,sizeof(*stream));
 if(!stream)return GH_ADX_MEMORY;
 gh_adx_error e=gh_adx_open(bytes,size,key,&stream->decoder);
 if(!e)e=gh_adx_inspect(stream->decoder,&stream->status.format);
 if(e){gh_adx_close(stream->decoder);free(stream);return e;}
 stream->status.prepared=1;
 *out=stream;
 return GH_ADX_OK;
}
gh_adx_error gh_adx_stream_set_muted(gh_adx_stream *s,int mute)
{++commands;s->status.muted=mute;return GH_ADX_OK;}
gh_adx_error gh_adx_stream_set_gain(gh_adx_stream *s,float gain)
{++commands;s->status.gain=gain;return GH_ADX_OK;}
gh_adx_error gh_adx_stream_set_loop(gh_adx_stream *s,int loop)
{(void)s;(void)loop;++commands;return GH_ADX_OK;}
gh_adx_error gh_adx_stream_start(gh_adx_stream *s)
{
 ++commands;
 int16_t pcm[512];
 size_t frames=0;
 gh_adx_error e=gh_adx_decode(s->decoder,pcm,256,&frames);
 if(e)return e;
 s->status.started=1;
 s->status.submitted_frames=frames;
 return GH_ADX_OK;
}
gh_adx_error gh_adx_stream_pause(gh_adx_stream *s,int pause)
{++commands;s->status.paused=pause;return GH_ADX_OK;}
gh_adx_error gh_adx_stream_poll(gh_adx_stream *s,gh_adx_stream_status *out)
{++commands;*out=s->status;return GH_ADX_OK;}
gh_adx_error gh_adx_stream_stop(gh_adx_stream *s)
{++commands;s->status.started=0;return GH_ADX_OK;}
void gh_adx_stream_close(gh_adx_stream *s)
{if(s){++commands;gh_adx_close(s->decoder);free(s);}}

int main(int argc,char **argv)
{
 gh_disc *disc;
 gh_audio_registry *registry;
 gh_sound_control control={0};
 gh_heap *heap;
 gh_bgm_requests *bgm;
 gh_se_voice_engine *voices;
 gh_movie_pool *movies;
 gh_boot_work work={0};
 gh_se_mix mix={.se_level_q8=0x5a00,.output_mode=1,.dsp_policy=GH_SE_DSP_NATIVE_LINEAR_REFERENCE};
 /* No voice is dispatched. Creation only borrows the opaque endpoint. */
 CHECK(argc==2&&gh_disc_open(argv[1],&disc)==GH_DISC_OK);
 CHECK(gh_audio_registry_create(&registry)==GH_AUDIO_OK);
 CHECK(gh_audio_startup_load(registry,disc)==GH_AUDIO_OK);
 CHECK(gh_sound_control_from_startup(&control,disc,registry)==GH_VOICE_OK);
 CHECK(control.output_mode==1);
 CHECK(gh_heap_create(64*1024*1024,&heap)==GH_MEM_OK);
 CHECK(gh_bgm_requests_create(registry,&control,disc,heap,&bgm)==GH_BGM_OK);
 CHECK(gh_se_voice_create(registry,(gh_audio_output *)(void *)&work,&mix,&voices)==GH_VOICE_OK);
 CHECK(gh_movie_pool_create_002b4960(&movies)==GH_MOVIE_OK);
 CHECK(gh_bgm_requests_restore_title_output_mode_00375700(bgm,1,voices,movies)==GH_BGM_OK);
 CHECK(!commands);
 size_t elf_size=(size_t)gh_disc_boot_elf_size(disc);
 uint8_t *elf=malloc(elf_size);
 gh_adx_key key;
 CHECK(elf&&gh_disc_read_boot_elf(disc,0,elf,elf_size)==GH_DISC_OK);
 CHECK(gh_adx_key_from_elf(elf,elf_size,&key)==GH_ADX_OK);
 CHECK(gh_se_voice_bind_origin_seh(voices,NULL,0)==GH_VOICE_OK);
 CHECK(gh_se_voice_bind_external_adx(voices,disc,&control,elf,elf_size)==GH_VOICE_OK);
 free(elf);
 CHECK(gh_bgm_requests_enable_native(bgm,&key,1,GH_BGM_GAIN_NATIVE_DB_REFERENCE)==GH_BGM_OK);
 CHECK(gh_bgm_event_start_002d1610(bgm,0,0,0)==GH_BGM_OK);
 gh_bgm_request_view before,after;
 CHECK(gh_bgm_requests_inspect(bgm,0,&before)==GH_BGM_OK);
 CHECK(before.linked&&before.transport.started&&before.transport.submitted_frames==256);
 CHECK(before.member==0&&before.member_offset==2048&&before.member_bytes==7168000);
 CHECK(before.rate==48000&&before.adx_channels==2&&before.adx_version==4&&before.adx_flags==8);
 uint8_t scalar[0xdc];
 memcpy(scalar,before.original_scalars,sizeof(scalar));
 unsigned writes=commands;
 CHECK(gh_bgm_requests_restore_idle_output_mode_00375700(bgm,1)==GH_BGM_UNSUPPORTED);
 CHECK(gh_bgm_requests_restore_title_output_mode_00375700(bgm,0,voices,movies)==GH_BGM_UNSUPPORTED);
 CHECK(gh_bgm_requests_frontier(bgm)==0x32ba30&&control.output_mode==1);
 control.output_mode=0;
 CHECK(gh_bgm_requests_restore_title_output_mode_00375700(bgm,1,voices,movies)==GH_BGM_UNSUPPORTED);
 CHECK(control.output_mode==0);
 control.output_mode=1;
 uint32_t handle;
 CHECK(gh_movie_pool_commit_started(movies,0,&handle,0x20,&work,&handle)==GH_MOVIE_OK);
 CHECK(gh_bgm_requests_restore_title_output_mode_00375700(bgm,1,voices,movies)==GH_BGM_UNSUPPORTED);
 CHECK(gh_movie_pool_track_closed(movies,0,&handle)==GH_MOVIE_OK);
 CHECK(gh_bgm_requests_restore_title_output_mode_00375700(bgm,1,voices,movies)==GH_BGM_OK);
 CHECK(!gh_bgm_requests_frontier(bgm)&&control.output_mode==1&&commands==writes);
 uint32_t menu_voice;
 CHECK(gh_se_voice_request_origin(voices,0,0x2d,-1,-1,0,0,&menu_voice)==GH_VOICE_OK&&menu_voice);
 CHECK(gh_se_voice_active_nodes(voices)==1&&!gh_se_voice_external_adx_nodes(voices));
 CHECK(gh_bgm_requests_restore_title_output_mode_00375700(bgm,1,voices,movies)==GH_BGM_OK);
 size_t ses_index;
 gh_disc_entry ses_entry;
 CHECK(gh_disc_find(disc,"se/r100.ses",&ses_index)==GH_DISC_OK);
 CHECK(gh_disc_entry_at(disc,ses_index,&ses_entry)==GH_DISC_OK);
 size_t ses_size=(size_t)ses_entry.size;
 uint8_t *ses=malloc(ses_size);
 CHECK(ses&&gh_disc_read(disc,ses_index,0,ses,ses_size)==GH_DISC_OK);
 gh_audio_token token;
 CHECK(gh_audio_commit_se_with_external_index(registry,2,ses,ses_size,disc,&token)==GH_AUDIO_OK);
 free(ses);
 uint32_t external_voice;
 CHECK(gh_se_voice_request_origin(voices,2,0x32,-1,-1,0,0,&external_voice)==GH_VOICE_OK&&external_voice);
 CHECK(gh_se_voice_external_adx_nodes(voices)==1);
 CHECK(gh_bgm_requests_restore_title_output_mode_00375700(bgm,1,voices,movies)==GH_BGM_UNSUPPORTED);
 CHECK(gh_bgm_requests_frontier(bgm)==0x32ba30&&control.output_mode==1&&commands==writes);
 CHECK(gh_se_voice_stop_bank_002cbc58(voices,2,0)==GH_VOICE_OK);
 CHECK(!gh_se_voice_external_adx_nodes(voices));
 CHECK(gh_bgm_requests_restore_title_output_mode_00375700(bgm,1,voices,movies)==GH_BGM_OK);
 CHECK(gh_bgm_requests_inspect(bgm,0,&after)==GH_BGM_OK);
 CHECK(!memcmp(scalar,after.original_scalars,sizeof(scalar)));
 CHECK(before.encoded.data==after.encoded.data&&before.transport.gain==after.transport.gain);
 CHECK(before.transport.submitted_frames==after.transport.submitted_frames);
 CHECK(gh_bgm_event_set_002d15a8(bgm,0,0,0)==GH_BGM_OK&&gh_bgm_requests_active(bgm)==2);
 CHECK(gh_bgm_requests_restore_title_output_mode_00375700(bgm,1,voices,movies)==GH_BGM_UNSUPPORTED);
 CHECK(gh_movie_pool_destroy(movies)==GH_MOVIE_OK);
 gh_se_voice_destroy(voices);
 gh_bgm_requests_destroy(bgm);
 CHECK(gh_heap_destroy(heap)==GH_MEM_OK);
 gh_audio_registry_destroy(registry);
 gh_disc_close(disc);
 puts("PASS R020 title event0 real stereo ADX decoded; active mode1 restore preserves routing and transport; other owners stay strict");
 return 0;
}
