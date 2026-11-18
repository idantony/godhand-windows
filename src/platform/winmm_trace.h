#ifndef GH_WINMM_TRACE_H
#define GH_WINMM_TRACE_H

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
 size_t nonzero;
 unsigned peak;
} gh_winmm_pcm_metrics;

/* IDs are paired with the source name because each translation unit owns its counter. */
static inline unsigned gh_winmm_trace_stream(void)
{
 static LONG next;
 wchar_t enabled[2]={0};
 if(GetEnvironmentVariableW(L"GODHAND_TRACE_WINMM",enabled,2)!=1 || enabled[0]!=L'1')return 0;
 return (unsigned)InterlockedIncrement(&next);
}

static inline gh_winmm_pcm_metrics gh_winmm_pcm16_stereo_metrics(const void *pcm,size_t frames)
{
 const unsigned char *bytes=pcm;
 gh_winmm_pcm_metrics result={0};
 for(size_t index=0;index<frames*2;index++){
  int16_t sample;
  memcpy(&sample,bytes+index*sizeof(sample),sizeof(sample));
  const int value=sample;
  const unsigned absolute=(unsigned)(value<0?-value:value);
  result.nonzero+=absolute!=0;
  if(absolute>result.peak)result.peak=absolute;
 }
 return result;
}

static inline void gh_winmm_trace_submit(const char *source,unsigned stream,uint64_t buffer,
 size_t frames,unsigned rate,int muted,gh_winmm_pcm_metrics decoded,gh_winmm_pcm_metrics submitted)
{
 fprintf(stderr,"NATIVE_WINMM {\"event\":\"submit\",\"source\":\"%s\",\"stream\":%u,\"buffer\":%llu,"
     "\"frames\":%zu,\"rate\":%u,\"muted\":%s,\"decoded_nonzero\":%zu,\"decoded_peak\":%u,"
     "\"submitted_nonzero\":%zu,\"submitted_peak\":%u}\n",
     source,stream,(unsigned long long)buffer,frames,rate,muted?"true":"false",
     decoded.nonzero,decoded.peak,submitted.nonzero,submitted.peak);
}

static inline void gh_winmm_trace_complete(const char *source,unsigned stream,uint64_t buffer,size_t frames)
{
 fprintf(stderr,"NATIVE_WINMM {\"event\":\"complete\",\"source\":\"%s\",\"stream\":%u,"
     "\"buffer\":%llu,\"frames\":%zu}\n",source,stream,(unsigned long long)buffer,frames);
}

#endif
