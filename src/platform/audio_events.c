/* Independently written bounded codec/Windows PCM transport. Mathematical
 * codec model evidence: pinned PCSX2 SPU2/Mixer.cpp; no emulator runtime/code
 * imported. Does not claim SPU2 interpolation/ADSR/reverb or full game audio. */
#define COBJMACROS
#include "audio_events.h"
#include <windows.h>
#include <xaudio2.h>
#include <stdlib.h>
#include <string.h>
static int32_t floor_pow2(int32_t n,unsigned shift)
{int32_t d=(int32_t)(UINT32_C(1)<<shift);return n>=0?n/d:-((-(int64_t)n+d-1)/d);}
gh_voice_status gh_audio_decode_ps_adpcm(const void *data,size_t size,gh_adpcm_pcm *out)
{
 const uint8_t *p=data;size_t blocks=0,i,j;int32_t h1=0,h2=0;int16_t *samples;
 static const int coefficient[5][2]={{0,0},{60,0},{115,-52},{98,-55},{122,-60}};
 if(!p || !out)return GH_VOICE_ARGUMENT;memset(out,0,sizeof(*out));
 for(i=0;i+16<=size;i+=16){
  if((p[i]>>4)>4 || (p[i]&15)>12 || (p[i+1]&~1))return GH_VOICE_UNSUPPORTED;
  ++blocks;if(p[i+1]&1)break;
 }
 if(!blocks || !(p[(blocks-1)*16+1]&1))return GH_VOICE_BOUNDS;
 if(blocks>SIZE_MAX/28/sizeof(*samples))return GH_VOICE_BOUNDS;
 samples=malloc(blocks*28*sizeof(*samples));if(!samples)return GH_VOICE_MEMORY;
 for(i=0;i<blocks;++i){unsigned shift=p[i*16]&15,filter=p[i*16]>>4;
  for(j=0;j<28;++j){unsigned nibble=(p[i*16+2+j/2]>>((j&1)*4))&15;int32_t signed_nibble=nibble<8?(int32_t)nibble:(int32_t)nibble-16;
   int32_t value=floor_pow2(signed_nibble*4096,shift)+floor_pow2(coefficient[filter][0]*h1+coefficient[filter][1]*h2+32,6);
   if(value>32767)value=32767;else if(value< -32768)value=-32768;
   samples[i*28+j]=(int16_t)value;h2=h1;h1=value;
  }
 }
 out->samples=samples;out->frames=blocks*28;out->source_bytes=blocks*16;return GH_VOICE_OK;
}
void gh_audio_pcm_release(gh_adpcm_pcm *p){if(p){free(p->samples);memset(p,0,sizeof(*p));}}
typedef HRESULT (WINAPI *create_engine)(IXAudio2 **,UINT32,XAUDIO2_PROCESSOR);
typedef HRESULT (WINAPI *com_init)(LPVOID,DWORD);
typedef void (WINAPI *com_end)(void);
typedef struct output_voice {IXAudio2SourceVoice *source;int16_t *samples;uint32_t id;} output_voice;
struct gh_audio_output {HMODULE dll,ole;IXAudio2 *engine;IXAudio2MasteringVoice *master;com_end uninit;int owns_com;uint32_t next;output_voice voices[64];};
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
 return SUCCEEDED(IXAudio2MasteringVoice_SetVolume(s->master,gain,0))?GH_VOICE_OK:GH_VOICE_PLATFORM;
}
void gh_audio_output_reap(gh_audio_output *s)
{
 if(!s)return;for(unsigned i=0;i<64;++i)if(s->voices[i].source){XAUDIO2_VOICE_STATE v;IXAudio2SourceVoice_GetState(s->voices[i].source,&v,0);if(!v.BuffersQueued){IXAudio2SourceVoice_DestroyVoice(s->voices[i].source);free(s->voices[i].samples);memset(s->voices+i,0,sizeof(s->voices[i]));}}
}
static gh_voice_status play(gh_audio_output *s,const gh_adpcm_pcm *p,uint32_t rate,float ratio,float left,float right,uint32_t *id)
{
 unsigned i;WAVEFORMATEX format={0};XAUDIO2_BUFFER buffer={0};output_voice *v;HRESULT hr;float matrix[2]={left,right};
 if(!s || !p || !p->samples || !p->frames || !id || rate<1000 || rate>200000 || !(left>=0 && left<=1 && right>=0 && right<=1))return GH_VOICE_ARGUMENT;
 if(p->frames>UINT32_MAX/2)return GH_VOICE_BOUNDS;gh_audio_output_reap(s);
 for(i=0;i<64 && s->voices[i].source;++i){}if(i==64)return GH_VOICE_CAPACITY;v=s->voices+i;
 v->samples=malloc(p->frames*2);if(!v->samples)return GH_VOICE_MEMORY;memcpy(v->samples,p->samples,p->frames*2);
 format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=1;format.nSamplesPerSec=rate;format.nAvgBytesPerSec=rate*2;format.nBlockAlign=2;format.wBitsPerSample=16;
 hr=IXAudio2_CreateSourceVoice(s->engine,&v->source,&format,0,4,0,0,0);
 if(SUCCEEDED(hr))hr=IXAudio2SourceVoice_SetFrequencyRatio(v->source,ratio,0);
 if(SUCCEEDED(hr))hr=IXAudio2SourceVoice_SetOutputMatrix(v->source,(IXAudio2Voice *)s->master,1,2,matrix,0);
 buffer.Flags=XAUDIO2_END_OF_STREAM;buffer.AudioBytes=(UINT32)(p->frames*2);buffer.pAudioData=(const BYTE *)v->samples;
 if(SUCCEEDED(hr))hr=IXAudio2SourceVoice_SubmitSourceBuffer(v->source,&buffer,0);
 if(SUCCEEDED(hr))hr=IXAudio2SourceVoice_Start(v->source,0,0);
 if(FAILED(hr)){if(v->source)IXAudio2SourceVoice_DestroyVoice(v->source);free(v->samples);memset(v,0,sizeof(*v));return GH_VOICE_PLATFORM;}
 if(!++s->next)++s->next;v->id=s->next;*id=v->id;return GH_VOICE_OK;
}
gh_voice_status gh_audio_output_play(gh_audio_output *s,const gh_adpcm_pcm *p,uint32_t rate,float left,float right,uint32_t *id)
{return play(s,p,rate,1,left,right,id);}
gh_voice_status gh_audio_output_play_q12(gh_audio_output *s,const gh_adpcm_pcm *p,uint16_t pitch,float left,float right,uint32_t *id)
{
 if(pitch<1 || pitch>0x3fff)return GH_VOICE_ARGUMENT;
 if(pitch<4)return GH_VOICE_UNSUPPORTED; /* XAudio2 minimum ratio1/1024 */
 return play(s,p,48000,(float)pitch/4096.0f,left,right,id);
}
gh_voice_status gh_audio_output_active(gh_audio_output *s,uint32_t id,int *active,uint64_t *played)
{
 if(!s || !id || !active || !played)return GH_VOICE_ARGUMENT;
 for(unsigned i=0;i<64;++i)if(s->voices[i].id==id){XAUDIO2_VOICE_STATE v;IXAudio2SourceVoice_GetState(s->voices[i].source,&v,0);*active=v.BuffersQueued!=0;*played=v.SamplesPlayed;return GH_VOICE_OK;}
 return GH_VOICE_BOUNDS;
}
gh_voice_status gh_audio_output_stop(gh_audio_output *s,uint32_t id)
{
 if(!s || !id)return GH_VOICE_ARGUMENT;
 for(unsigned i=0;i<64;++i)if(s->voices[i].id==id){
  IXAudio2SourceVoice_DestroyVoice(s->voices[i].source);free(s->voices[i].samples);
  memset(s->voices+i,0,sizeof(s->voices[i]));return GH_VOICE_OK;
 }
 return GH_VOICE_BOUNDS;
}
