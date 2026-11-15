/* Read-only descriptor/sample association audit. No device, raw data or PCM output.
 * Usage: inspect-actor-sound ISO OUTPUT.json [--ses ISO-path] [--external] [hex-program ...]
 * Keep generated reports in the caller's private analysis/build directory. */
#include "audio_startup.h"
#include "audio_events.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
static unsigned h(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static unsigned w(const uint8_t*p){return h(p)|h(p+2)<<16;}
static unsigned be32(const uint8_t *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int load_ses(gh_disc *disc,gh_audio_registry *registry,const char *path,int16_t bank,int external)
{
 size_t index;
 gh_disc_entry entry;
 if(gh_disc_find(disc,path,&index)||gh_disc_entry_at(disc,index,&entry)||
    entry.size<0x40||entry.size>64u*1024u*1024u)return 0;
 const size_t bytes=(size_t)entry.size;
 uint8_t *image=malloc(bytes);
 if(!image)return 0;
 gh_audio_token token;
 int valid=0;
 if(!gh_disc_read(disc,index,0,image,bytes)){
  gh_audio_status status=external?
      gh_audio_commit_se_with_external_index(registry,bank,image,bytes,disc,&token):
      gh_audio_commit_se(registry,bank,image,bytes,&token);
  valid=!status&&gh_audio_token_complete(registry,token);
 }
 free(image);
 return valid;
}

static int write_external_clip(FILE *out,gh_disc *disc,const gh_audio_registry *registry,int16_t bank,unsigned row)
{
 const char *path=NULL;
 gh_sound_span afd;
 size_t index;
 uint64_t offset,bytes;
 uint8_t header[20];
 if(gh_audio_external_path(registry,bank,&path)||gh_audio_external_sample(registry,bank,row,&afd,&offset,&bytes)||
    afd.size!=32||bytes<sizeof(header)||gh_disc_find(disc,path,&index)||
    gh_disc_read(disc,index,offset,header,sizeof(header)))return 0;
 fprintf(out,",\"external\":{\"row\":%u,\"member\":%u,\"offset\":%llu,\"bytes\":%llu,"
  "\"format_word\":%u,\"level_word\":%u",row,h(afd.data+2),(unsigned long long)offset,
  (unsigned long long)bytes,w(afd.data+8),w(afd.data+12));
 if(header[0]==0x80&&!header[1]){
  fprintf(out,",\"adx\":{\"encoding\":%u,\"block_bytes\":%u,\"bits\":%u,\"channels\":%u,"
   "\"rate\":%u,\"frames\":%u,\"version_flags\":%u,\"data_offset\":%u}",header[4],header[5],
   header[6],header[7],be32(header+8),be32(header+12),(unsigned)header[18]*256+header[19],
   (unsigned)header[2]*256+header[3]+4);
 }
 return fputc('}',out)!=EOF;
}

int main(int argc,char**argv){
 if(argc<3){fprintf(stderr,"usage: inspect-actor-sound ISO OUTPUT.json [--ses ISO-path] [--external] [hex-program ...]\n");return 2;}
 const int separate=argc>3&&!strcmp(argv[3],"--ses");
 if(separate&&argc<5){fprintf(stderr,"--ses requires an ISO path\n");return 2;}
 const int option=separate?5:3;
 const int external=argc>option&&!strcmp(argv[option],"--external");
 const int first=option+external;
 const int16_t bank=separate?1:0;
 gh_disc*d=NULL;gh_audio_registry*r=NULL;FILE*out=NULL;int result=1;
 if(gh_disc_open(argv[1],&d)||gh_audio_registry_create(&r))goto done;
 if(separate){if(!load_ses(d,r,argv[4],bank,external))goto done;}
 else if(gh_audio_startup_load(r,d))goto done;
 out=fopen(argv[2],"wb");if(!out)goto done;
 static const int defaults[]={0x1e,0x1f,0x21,0x48,0x52,0x57,0x161};
 fprintf(out,"{\"schema\":1,\"scope\":\"%s; no playback\",\"programs\":[",
    separate?"selected ISO SES in isolated audit bank 1":"real core bank descriptor and decoded sample association");
 int count=argc==first?(separate?1:(int)(sizeof(defaults)/sizeof(defaults[0]))):argc-first;
 for(int i=0;i<count;i++){
  long program=0;if(argc==first){if(!separate)program=defaults[i];}
  else{const char *argument=argv[i+first];char*end;errno=0;program=strtol(argument,&end,16);if(errno||end==argument||*end||program<0||program>SHRT_MAX)goto done;}
  uint8_t p[48];gh_audio_status e=gh_audio_get_program(r,bank,(int16_t)program,0,p);if(i)fputc(',',out);
  fprintf(out,"{\"bank\":%d,\"program\":%ld,\"status\":%d",bank,program,e);
  if(!e){
   fprintf(out,",\"sample\":%u,\"first_channel\":%u,\"channel_count\":%u,\"priority\":%u,\"position_selector\":%u,\"driver_flags\":%u,\"level_lr\":[%u,%u],\"pan\":%u,\"alternate_pan\":%u,\"pitch_range\":[%d,%d],\"pitch_offset\":%d,\"variants\":%u,\"duplicate_age\":%u,\"recent_depth\":%u,\"ee_flags\":%u,\"option_flags\":%u,\"actor_rate_flags\":%u,\"reverb_index\":%d",h(p+4),p[6],p[7]+1,p[8],p[9],h(p+14),h(p+16),h(p+18),h(p+20),h(p+22),(int)(int16_t)h(p+24),(int)(int16_t)h(p+26),(int)(int16_t)h(p+28),p[33],p[34],p[35],p[32],p[36],h(p+37),(int)(int16_t)h(p+40));
   if(external&&p[6]>=48&&h(p+40)!=0xffff){
    if(!write_external_clip(out,d,r,bank,h(p+40)))goto done;
   }else{
    gh_sound_span descriptor,tail;gh_audio_status sample=gh_audio_sample_at(r,bank,h(p+4),&descriptor,&tail);fprintf(out,",\"sample_status\":%d",sample);
    if(!sample&&descriptor.size==16){gh_adpcm_pcm pcm={0};gh_voice_status decoded=gh_audio_decode_ps_adpcm(tail.data,tail.size,&pcm);fprintf(out,",\"sample_offset\":%u,\"rate\":%u,\"adsr\":[%u,%u],\"extra\":%u,\"decode_status\":%d,\"decoded_frames\":%zu,\"consumed_bytes\":%zu",w(descriptor.data),w(descriptor.data+4),h(descriptor.data+8),h(descriptor.data+10),w(descriptor.data+12),decoded,pcm.frames,pcm.source_bytes);gh_audio_pcm_release(&pcm);}
   }
  }fputc('}',out);
 }
 fputs("]}\n",out);result=ferror(out)?1:0;
done:
 if(out&&fclose(out))result=1;if(r)gh_audio_registry_destroy(r);if(d)gh_disc_close(d);return result;
}
