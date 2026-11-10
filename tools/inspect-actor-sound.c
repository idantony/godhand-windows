/* Read-only descriptor/sample association audit. No device, raw data or PCM output.
 * Usage: inspect-actor-sound ISO OUTPUT.json [hex-program ...]
 * Keep generated reports in the caller's private analysis/build directory. */
#include "audio_startup.h"
#include "audio_events.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
static unsigned h(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static unsigned w(const uint8_t*p){return h(p)|h(p+2)<<16;}
int main(int argc,char**argv){
 if(argc<3){fprintf(stderr,"usage: inspect-actor-sound ISO OUTPUT.json [hex-program ...]\n");return 2;}
 gh_disc*d=NULL;gh_audio_registry*r=NULL;FILE*out=NULL;int result=1;
 if(gh_disc_open(argv[1],&d)||gh_audio_registry_create(&r)||gh_audio_startup_load(r,d))goto done;
 out=fopen(argv[2],"wb");if(!out)goto done;
 static const int defaults[]={0x1e,0x1f,0x21,0x48,0x52,0x57,0x161};
 fprintf(out,"{\"schema\":1,\"scope\":\"real core bank descriptor and decoded sample association; no playback\",\"programs\":[");
 int count=argc==3?(int)(sizeof(defaults)/sizeof(defaults[0])):argc-3;
 for(int i=0;i<count;i++){
  long program=defaults[0];if(argc==3)program=defaults[i];else{char*end;errno=0;program=strtol(argv[i+3],&end,16);if(errno||*end||program<0||program>SHRT_MAX)goto done;}
  uint8_t p[48];gh_audio_status e=gh_audio_get_program(r,0,(int16_t)program,0,p);if(i)fputc(',',out);
  fprintf(out,"{\"bank\":0,\"program\":%ld,\"status\":%d",program,e);
  if(!e){
   fprintf(out,",\"sample\":%u,\"first_channel\":%u,\"channel_count\":%u,\"priority\":%u,\"position_selector\":%u,\"driver_flags\":%u,\"level_lr\":[%u,%u],\"pan\":%u,\"alternate_pan\":%u,\"pitch_range\":[%d,%d],\"pitch_offset\":%d,\"variants\":%u,\"duplicate_age\":%u,\"recent_depth\":%u,\"ee_flags\":%u,\"option_flags\":%u,\"actor_rate_flags\":%u,\"reverb_index\":%d",h(p+4),p[6],p[7]+1,p[8],p[9],h(p+14),h(p+16),h(p+18),h(p+20),h(p+22),(int)(int16_t)h(p+24),(int)(int16_t)h(p+26),(int)(int16_t)h(p+28),p[33],p[34],p[35],p[32],p[36],h(p+37),(int)(int16_t)h(p+40));
   gh_sound_span descriptor,tail;gh_audio_status sample=gh_audio_sample_at(r,0,h(p+4),&descriptor,&tail);fprintf(out,",\"sample_status\":%d",sample);
   if(!sample&&descriptor.size==16){gh_adpcm_pcm pcm={0};gh_voice_status decoded=gh_audio_decode_ps_adpcm(tail.data,tail.size,&pcm);fprintf(out,",\"rate\":%u,\"adsr\":[%u,%u],\"extra\":%u,\"decode_status\":%d,\"decoded_frames\":%zu,\"consumed_bytes\":%zu",w(descriptor.data+4),h(descriptor.data+8),h(descriptor.data+10),w(descriptor.data+12),decoded,pcm.frames,pcm.source_bytes);gh_audio_pcm_release(&pcm);}
  }fputc('}',out);
 }
 fputs("]}\n",out);result=ferror(out)?1:0;
done:
 if(out&&fclose(out))result=1;if(r)gh_audio_registry_destroy(r);if(d)gh_disc_close(d);return result;
}
