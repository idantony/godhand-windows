/* Independently written bounded codec/Windows PCM transport. Mathematical
 * codec model evidence: pinned PCSX2 SPU2/Mixer.cpp; no emulator runtime/code
 * imported. Does not claim SPU2 interpolation/ADSR/reverb or full game audio. */
#define COBJMACROS
#include "audio_events.h"
#include <windows.h>
#include <xaudio2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int32_t floor_pow2(int32_t n,unsigned shift)
{int32_t d=(int32_t)(UINT32_C(1)<<shift);return n>=0?n/d:-((-(int64_t)n+d-1)/d);}
static void decode_blocks(const uint8_t *p,size_t blocks,int16_t *samples,int32_t *last1,int32_t *last2)
{
 static const int coefficient[5][2]={{0,0},{60,0},{115,-52},{98,-55},{122,-60}};
 int32_t h1=*last1,h2=*last2;
 for(size_t i=0;i<blocks;i++){
  const unsigned shift=p[i*16]&15,filter=p[i*16]>>4;
  for(unsigned j=0;j<28;j++){
   const unsigned nibble=(p[i*16+2+j/2]>>((j&1)*4))&15;
   const int32_t signed_nibble=nibble<8?(int32_t)nibble:(int32_t)nibble-16;
   int32_t value=floor_pow2(signed_nibble*4096,shift)+
      floor_pow2(coefficient[filter][0]*h1+coefficient[filter][1]*h2+32,6);
   if(value>32767)value=32767;
   else if(value< -32768)value=-32768;
   samples[i*28+j]=(int16_t)value;
   h2=h1;
   h1=value;
  }
 }
 *last1=h1;
 *last2=h2;
}

static gh_voice_status decode(const void *data,size_t size,gh_adpcm_pcm *out,int repeat)
{
 const uint8_t *p=data;
 size_t blocks=0,loop_start=SIZE_MAX;
 if(!p||!out)return GH_VOICE_ARGUMENT;
 *out=(gh_adpcm_pcm){0};
 unsigned flags=0;
 for(size_t i=0;i<size/16;i++){
  const unsigned format=p[i*16];
  flags=p[i*16+1];
  if((format>>4)>4||(format&15)>12)return GH_VOICE_UNSUPPORTED;
  if(repeat){
   if(flags==6&&loop_start==SIZE_MAX)loop_start=i;
   else if(flags!=2&&flags!=3&&(flags!=0||loop_start!=SIZE_MAX))return GH_VOICE_UNSUPPORTED;
  }else if(flags&~1u)return GH_VOICE_UNSUPPORTED;
  ++blocks;
  if(flags&1)break;
 }
 if(!blocks||!(flags&1))return GH_VOICE_BOUNDS;
 if(repeat&&(flags!=3||loop_start==SIZE_MAX||loop_start>=blocks))return GH_VOICE_UNSUPPORTED;
 const size_t limit=SIZE_MAX/28/sizeof(int16_t);
 const size_t loop_blocks=repeat?blocks-loop_start:0;
 if(blocks>limit||loop_blocks>limit-blocks)return GH_VOICE_BOUNDS;
 const size_t frames=(blocks+loop_blocks)*28;
 int16_t *samples=malloc(frames*sizeof(*samples));
 if(!samples)return GH_VOICE_MEMORY;
 int32_t h1=0,h2=0;
 decode_blocks(p,blocks,samples,&h1,&h2);
 if(repeat){
  const int32_t repeat_h1=h1,repeat_h2=h2;
  decode_blocks(p+loop_start*16,loop_blocks,samples+blocks*28,&h1,&h2);
  if(h1!=repeat_h1||h2!=repeat_h2){
   free(samples);
   return GH_VOICE_UNSUPPORTED;
  }
 }
 *out=(gh_adpcm_pcm){.samples=samples,.frames=frames,.source_bytes=blocks*16,
     .loop_first=repeat?blocks*28:0,.loop_frames=loop_blocks*28};
 return GH_VOICE_OK;
}

gh_voice_status gh_audio_decode_ps_adpcm(const void *data,size_t size,gh_adpcm_pcm *out)
{return decode(data,size,out,0);}

gh_voice_status gh_audio_decode_ps_adpcm_loop(const void *data,size_t size,gh_adpcm_pcm *out)
{return decode(data,size,out,1);}

void gh_audio_pcm_release(gh_adpcm_pcm *p){if(p){free(p->samples);memset(p,0,sizeof(*p));}}
typedef HRESULT (WINAPI *create_engine)(IXAudio2 **,UINT32,XAUDIO2_PROCESSOR);
typedef HRESULT (WINAPI *com_init)(LPVOID,DWORD);
typedef void (WINAPI *com_end)(void);
#define GH_AUDIO_SOURCE_MAX_RATIO 8.0f
typedef struct output_voice {
 IXAudio2SourceVoice *source;
 int16_t *samples;
 uint64_t frames_observed;
 uint32_t id;
 uint8_t channels;
} output_voice;
struct gh_audio_output {
 HMODULE dll,ole;
 IXAudio2 *engine;
 IXAudio2MasteringVoice *master;
 com_end uninit;
 int owns_com,trace;
 uint32_t next;
 output_voice voices[64];
};

static void observe_voice_progress(const gh_audio_output *s,output_voice *voice,
 const XAUDIO2_VOICE_STATE *state)
{
 if(s->trace && state->SamplesPlayed>voice->frames_observed)
  voice->frames_observed=state->SamplesPlayed;
}

static void trace_voice_completion(const gh_audio_output *s,output_voice *voice,
 const XAUDIO2_VOICE_STATE *state)
{
 if(!s->trace || state->BuffersQueued)return;
 /* SamplesPlayed can reset at end of stream. Preserve the observed lower bound. */
 observe_voice_progress(s,voice,state);
 fprintf(stderr,"NATIVE_XAUDIO2 {\"event\":\"complete\",\"voice\":%u,\"frames_observed\":%llu}\n",
     voice->id,(unsigned long long)voice->frames_observed);
}
void gh_audio_output_destroy(gh_audio_output *s)
{
 if(!s)return;
 for(unsigned i=0;i<64;++i)if(s->voices[i].source){IXAudio2SourceVoice_DestroyVoice(s->voices[i].source);free(s->voices[i].samples);}
 if(s->master)IXAudio2MasteringVoice_DestroyVoice(s->master);
 if(s->engine)IXAudio2_Release(s->engine);
 if(s->dll)FreeLibrary(s->dll);if(s->owns_com && s->uninit)s->uninit();if(s->ole)FreeLibrary(s->ole);free(s);
}
gh_voice_status gh_audio_output_create(gh_audio_output **out,int32_t *native)
{
 gh_audio_output *s;create_engine create;com_init init;HRESULT hr;
 if(!out || !native)return GH_VOICE_ARGUMENT;*out=0;*native=0;s=calloc(1,sizeof(*s));if(!s)return GH_VOICE_MEMORY;
 wchar_t trace[2]={0};
 s->trace=GetEnvironmentVariableW(L"GODHAND_TRACE_XAUDIO2",trace,2)==1 && trace[0]==L'1';
 s->ole=LoadLibraryExW(L"ole32.dll",0,LOAD_LIBRARY_SEARCH_SYSTEM32);s->dll=LoadLibraryExW(L"xaudio2_9.dll",0,LOAD_LIBRARY_SEARCH_SYSTEM32);
 if(!s->ole || !s->dll){*native=(int32_t)GetLastError();gh_audio_output_destroy(s);return GH_VOICE_PLATFORM;}
 init=(com_init)(void *)GetProcAddress(s->ole,"CoInitializeEx");s->uninit=(com_end)(void *)GetProcAddress(s->ole,"CoUninitialize");create=(create_engine)(void *)GetProcAddress(s->dll,"XAudio2Create");
 if(!init || !s->uninit || !create){*native=(int32_t)GetLastError();gh_audio_output_destroy(s);return GH_VOICE_PLATFORM;}
 hr=init(0,COINIT_MULTITHREADED);if(SUCCEEDED(hr))s->owns_com=1;else if(hr!=RPC_E_CHANGED_MODE){*native=hr;gh_audio_output_destroy(s);return GH_VOICE_PLATFORM;}
 hr=create(&s->engine,0,XAUDIO2_DEFAULT_PROCESSOR);
 if(SUCCEEDED(hr))hr=IXAudio2_CreateMasteringVoice(s->engine,&s->master,2,48000,0,0,0,AudioCategory_GameEffects);
 if(FAILED(hr)){*native=hr;gh_audio_output_destroy(s);return GH_VOICE_PLATFORM;}
 *out=s;return GH_VOICE_OK;
}
gh_voice_status gh_audio_output_set_gain(gh_audio_output *s,float gain)
{
 if(!s || !(gain>=0 && gain<=1))return GH_VOICE_ARGUMENT;
 if(FAILED(IXAudio2MasteringVoice_SetVolume(s->master,gain,0)))return GH_VOICE_PLATFORM;
 if(s->trace)fprintf(stderr,"NATIVE_XAUDIO2 {\"event\":\"gain\",\"value\":%.8g}\n",(double)gain);
 return GH_VOICE_OK;
}
void gh_audio_output_reap(gh_audio_output *s)
{
 if(!s)return;
 for(unsigned i=0;i<64;++i){
  output_voice *voice=s->voices+i;
  if(!voice->source)continue;
  XAUDIO2_VOICE_STATE state;
  IXAudio2SourceVoice_GetState(voice->source,&state,0);
  if(state.BuffersQueued){
   observe_voice_progress(s,voice,&state);
   continue;
  }
  trace_voice_completion(s,voice,&state);
  IXAudio2SourceVoice_DestroyVoice(voice->source);
  free(voice->samples);
  memset(voice,0,sizeof(*voice));
 }
}
static HRESULT set_voice_gain(gh_audio_output *s,output_voice *v,float left,float right)
{
 const float mono[2]={left,right};
 const float stereo[4]={left,0,0,right};
 const float *matrix=v->channels==2?stereo:mono;
 return IXAudio2SourceVoice_SetOutputMatrix(v->source,(IXAudio2Voice *)s->master,
     v->channels,2,matrix,0);
}

static gh_voice_status play(gh_audio_output *s,const gh_adpcm_pcm *p,uint32_t rate,float ratio,
 uint8_t channels,float left,float right,uint32_t *id)
{
 if(!s || !p || !p->samples || !p->frames || !id || rate<1000 || rate>200000 ||
    !(left>=0 && left<=1 && right>=0 && right<=1))return GH_VOICE_ARGUMENT;
 const uint16_t block_align=(uint16_t)(channels*sizeof(int16_t));
 if(p->frames>UINT32_MAX/block_align)return GH_VOICE_BOUNDS;
 if((!p->loop_frames&&p->loop_first)||p->loop_first>p->frames||
    p->loop_frames>p->frames-p->loop_first)
     return GH_VOICE_ARGUMENT;
 const size_t bytes=p->frames*block_align;
 gh_audio_output_reap(s);
 unsigned i;
 for(i=0;i<64 && s->voices[i].source;++i){}
 if(i==64)return GH_VOICE_CAPACITY;
 output_voice *v=s->voices+i;
 v->samples=malloc(bytes);
 if(!v->samples)return GH_VOICE_MEMORY;
 memcpy(v->samples,p->samples,bytes);
 v->channels=channels;
 WAVEFORMATEX format={0};
 format.wFormatTag=WAVE_FORMAT_PCM;
 format.nChannels=channels;
 format.nSamplesPerSec=rate;
 format.nAvgBytesPerSec=rate*block_align;
 format.nBlockAlign=block_align;
 format.wBitsPerSample=16;
 HRESULT hr=IXAudio2_CreateSourceVoice(s->engine,&v->source,&format,0,GH_AUDIO_SOURCE_MAX_RATIO,0,0,0);
 if(SUCCEEDED(hr))hr=IXAudio2SourceVoice_SetFrequencyRatio(v->source,ratio,0);
 if(SUCCEEDED(hr))hr=set_voice_gain(s,v,left,right);
 XAUDIO2_BUFFER buffer={0};
 buffer.Flags=XAUDIO2_END_OF_STREAM;
 buffer.AudioBytes=(UINT32)bytes;
 buffer.pAudioData=(const BYTE *)v->samples;
 if(p->loop_frames){
  buffer.LoopBegin=(UINT32)p->loop_first;
  buffer.LoopLength=(UINT32)p->loop_frames;
  buffer.LoopCount=XAUDIO2_LOOP_INFINITE;
 }
 if(SUCCEEDED(hr))hr=IXAudio2SourceVoice_SubmitSourceBuffer(v->source,&buffer,0);
 if(SUCCEEDED(hr))hr=IXAudio2SourceVoice_Start(v->source,0,0);
 if(FAILED(hr)){
  if(v->source)IXAudio2SourceVoice_DestroyVoice(v->source);
  free(v->samples);
  memset(v,0,sizeof(*v));
  return GH_VOICE_PLATFORM;
 }
 if(!++s->next)++s->next;
 v->id=s->next;
 *id=v->id;
 if(s->trace){
  size_t nonzero=0;
  unsigned peak=0;
  for(size_t sample=0;sample<bytes/sizeof(*v->samples);++sample){
   const int value=v->samples[sample];
   const unsigned magnitude=(unsigned)(value<0?-value:value);
   nonzero+=magnitude!=0;
   if(magnitude>peak)peak=magnitude;
  }
  fprintf(stderr,"NATIVE_XAUDIO2 {\"event\":\"start\",\"voice\":%u,\"frames\":%zu,"
      "\"channels\":%u,\"nonzero_samples\":%zu,\"peak\":%u,\"looped\":%s}\n",
      v->id,p->frames,(unsigned)channels,nonzero,peak,p->loop_frames?"true":"false");
 }
 return GH_VOICE_OK;
}
gh_voice_status gh_audio_output_play(gh_audio_output *s,const gh_adpcm_pcm *p,uint32_t rate,float left,float right,uint32_t *id)
{return play(s,p,rate,1,1,left,right,id);}
gh_voice_status gh_audio_output_play_stereo(gh_audio_output *s,const gh_adpcm_pcm *p,uint32_t rate,float left,float right,uint32_t *id)
{return play(s,p,rate,1,2,left,right,id);}
gh_voice_status gh_audio_output_play_q12(gh_audio_output *s,const gh_adpcm_pcm *p,uint16_t pitch,float left,float right,uint32_t *id)
{
 if(pitch<1 || pitch>0x3fff)return GH_VOICE_ARGUMENT;
 if(pitch<4)return GH_VOICE_UNSUPPORTED; /* XAudio2 minimum ratio1/1024 */
 return play(s,p,48000,(float)pitch/4096.0f,1,left,right,id);
}
gh_voice_status gh_audio_output_active(gh_audio_output *s,uint32_t id,int *active,uint64_t *played)
{
 if(!s || !id || !active || !played)return GH_VOICE_ARGUMENT;
 for(unsigned i=0;i<64;++i)if(s->voices[i].id==id){
  output_voice *voice=s->voices+i;
  XAUDIO2_VOICE_STATE state;
  IXAudio2SourceVoice_GetState(voice->source,&state,0);
  observe_voice_progress(s,voice,&state);
  *active=state.BuffersQueued!=0;
  *played=state.SamplesPlayed;
  return GH_VOICE_OK;
 }
 return GH_VOICE_BOUNDS;
}
gh_voice_status gh_audio_output_stop(gh_audio_output *s,uint32_t id)
{
 if(!s || !id)return GH_VOICE_ARGUMENT;
 for(unsigned i=0;i<64;++i)if(s->voices[i].id==id){
  if(s->trace){
   XAUDIO2_VOICE_STATE state;
   IXAudio2SourceVoice_GetState(s->voices[i].source,&state,0);
   trace_voice_completion(s,s->voices+i,&state);
  }
  IXAudio2SourceVoice_DestroyVoice(s->voices[i].source);free(s->voices[i].samples);
  memset(s->voices+i,0,sizeof(s->voices[i]));return GH_VOICE_OK;
 }
 return GH_VOICE_BOUNDS;
}

gh_voice_status gh_audio_output_voice_gain(gh_audio_output *s,uint32_t id,float left,float right)
{
 if(!s||!id||!(left>=0&&left<=1&&right>=0&&right<=1))return GH_VOICE_ARGUMENT;
 for(unsigned i=0;i<64;i++)if(s->voices[i].id==id)
  return SUCCEEDED(set_voice_gain(s,s->voices+i,left,right))?GH_VOICE_OK:GH_VOICE_PLATFORM;
 return GH_VOICE_BOUNDS;
}
gh_voice_status gh_audio_output_voice_pitch_ratio(gh_audio_output *s,uint32_t id,
 uint16_t target,uint16_t submitted)
{
 if(!s||!id||!target||target>0x3fff||!submitted||submitted>0x3fff)return GH_VOICE_ARGUMENT;
 float ratio=(float)target/(float)submitted;
 if(ratio<XAUDIO2_MIN_FREQ_RATIO||ratio>GH_AUDIO_SOURCE_MAX_RATIO)return GH_VOICE_UNSUPPORTED;
 for(unsigned i=0;i<64;++i)if(s->voices[i].id==id)
  return SUCCEEDED(IXAudio2SourceVoice_SetFrequencyRatio(s->voices[i].source,ratio,0))?
         GH_VOICE_OK:GH_VOICE_PLATFORM;
 return GH_VOICE_BOUNDS;
}
gh_voice_status gh_audio_output_voice_gain_query(gh_audio_output *s,uint32_t id,float *left,float *right)
{
 if(!s||!id||!left||!right)return GH_VOICE_ARGUMENT;
 for(unsigned i=0;i<64;i++)if(s->voices[i].id==id){
  output_voice *v=s->voices+i;
  float values[4];
  IXAudio2SourceVoice_GetOutputMatrix(v->source,(IXAudio2Voice *)s->master,v->channels,2,values);
  *left=values[0];
  *right=values[v->channels==2?3:1];
  return GH_VOICE_OK;
 }
 return GH_VOICE_BOUNDS;
}
