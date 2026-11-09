#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "adx_stream.h"
#define QUEUE_COUNT 4
#define BUFFER_FRAMES 4096
typedef struct {WAVEHDR header;int16_t pcm[BUFFER_FRAMES*2];uint32_t frames;int prepared,queued;} pcm_buffer;
struct gh_adx_stream {
 gh_adx_decoder *decoder;gh_adx_info info;HWAVEOUT device;DWORD owner;
 pcm_buffer buffers[QUEUE_COUNT];uint64_t submitted,completed,loops;
 int started,ever_started,paused,muted,loop,eof,complete;float gain;gh_adx_error error;
};
static int owned(const gh_adx_stream*p){return p&&p->owner==GetCurrentThreadId();}
void gh_adx_stream_close(gh_adx_stream*p){unsigned i;if(!p||!owned(p))return;
 if(p->device){waveOutReset(p->device);for(i=0;i<QUEUE_COUNT;i++)if(p->buffers[i].prepared)waveOutUnprepareHeader(p->device,&p->buffers[i].header,sizeof(WAVEHDR));waveOutClose(p->device);}
 gh_adx_close(p->decoder);free(p);
}
gh_adx_error gh_adx_stream_open(const void*b,size_t n,const gh_adx_key*k,gh_adx_stream**out){gh_adx_stream*p;gh_adx_error e;WAVEFORMATEX f;unsigned i;
 if(!out)return GH_ADX_ARGUMENT;*out=NULL;p=calloc(1,sizeof(*p));if(!p)return GH_ADX_MEMORY;p->owner=GetCurrentThreadId();p->gain=1;
 e=gh_adx_open(b,n,k,&p->decoder);if(e){free(p);return e;}gh_adx_inspect(p->decoder,&p->info);
 memset(&f,0,sizeof(f));f.wFormatTag=WAVE_FORMAT_PCM;f.nChannels=2;f.nSamplesPerSec=p->info.sample_rate;f.wBitsPerSample=16;f.nBlockAlign=4;f.nAvgBytesPerSec=f.nSamplesPerSec*4;
 if(waveOutOpen(&p->device,WAVE_MAPPER,&f,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR||waveOutPause(p->device)!=MMSYSERR_NOERROR){gh_adx_stream_close(p);return GH_ADX_DEVICE;}
 for(i=0;i<QUEUE_COUNT;i++){pcm_buffer*q=&p->buffers[i];q->header.lpData=(LPSTR)q->pcm;q->header.dwBufferLength=sizeof(q->pcm);
  if(waveOutPrepareHeader(p->device,&q->header,sizeof(q->header))!=MMSYSERR_NOERROR){gh_adx_stream_close(p);return GH_ADX_DEVICE;}q->prepared=1;
 }*out=p;return GH_ADX_OK;
}
gh_adx_error gh_adx_stream_set_muted(gh_adx_stream*p,int value){if(!owned(p)||(value!=0&&value!=1))return GH_ADX_ARGUMENT;if(p->ever_started)return GH_ADX_STATE;p->muted=value;return GH_ADX_OK;}
gh_adx_error gh_adx_stream_set_gain(gh_adx_stream*p,float gain){if(!owned(p)||!isfinite(gain)||gain<0||gain>1)return GH_ADX_ARGUMENT;p->gain=gain;return GH_ADX_OK;}
gh_adx_error gh_adx_stream_set_loop(gh_adx_stream*p,int loop){if(!owned(p)||(loop!=0&&loop!=1))return GH_ADX_ARGUMENT;if(p->started)return GH_ADX_STATE;if(loop&&!p->info.loop_enabled)return GH_ADX_UNSUPPORTED;p->loop=loop;return GH_ADX_OK;}
static gh_adx_error fill(gh_adx_stream*p,pcm_buffer*q){size_t total=0;
 while(total<BUFFER_FRAMES){uint32_t pos=gh_adx_tell(p->decoder),end=p->loop?p->info.loop_end:p->info.sample_count;size_t take,n;gh_adx_error e;
  if(pos>=end){if(!p->loop){p->eof=1;break;}e=gh_adx_seek(p->decoder,p->info.loop_start);if(e)return e;p->loops++;continue;}
  take=end-pos;if(take>BUFFER_FRAMES-total)take=BUFFER_FRAMES-total;e=gh_adx_decode(p->decoder,q->pcm+total*2,take,&n);if(e)return e;if(!n)return GH_ADX_FORMAT;total+=n;
 }
 if(!total)return GH_ADX_OK;
 {size_t i;for(i=0;i<total*2;i++){if(p->muted)q->pcm[i]=0;else q->pcm[i]=(int16_t)((float)q->pcm[i]*p->gain);}}
 q->frames=(uint32_t)total;q->header.dwBufferLength=(DWORD)(total*4);
 if(waveOutWrite(p->device,&q->header,sizeof(q->header))!=MMSYSERR_NOERROR)return GH_ADX_DEVICE;
 q->queued=1;p->submitted+=total;return GH_ADX_OK;
}
static gh_adx_error pump(gh_adx_stream*p){unsigned i;gh_adx_error e;
 for(i=0;i<QUEUE_COUNT;i++){pcm_buffer*q=&p->buffers[i];if(q->queued&&(q->header.dwFlags&WHDR_DONE)){p->completed+=q->frames;q->queued=0;}}
 for(i=0;i<QUEUE_COUNT&&!p->eof;i++)if(!p->buffers[i].queued){e=fill(p,&p->buffers[i]);if(e)return e;}
 p->complete=p->eof&&p->completed==p->submitted;return GH_ADX_OK;
}
gh_adx_error gh_adx_stream_start(gh_adx_stream*p){gh_adx_error e;if(!owned(p))return GH_ADX_ARGUMENT;if(p->error)return p->error;if(p->started)return GH_ADX_STATE;
 p->started=1;p->ever_started=1;p->complete=0;e=pump(p);if(e)return p->error=e;
 if(waveOutRestart(p->device)!=MMSYSERR_NOERROR)return p->error=GH_ADX_DEVICE;p->paused=0;return GH_ADX_OK;
}
gh_adx_error gh_adx_stream_pause(gh_adx_stream*p,int pause){if(!owned(p)||(pause!=0&&pause!=1))return GH_ADX_ARGUMENT;if(!p->started||p->error)return GH_ADX_STATE;
 if((pause?waveOutPause(p->device):waveOutRestart(p->device))!=MMSYSERR_NOERROR)return p->error=GH_ADX_DEVICE;p->paused=pause;return GH_ADX_OK;
}
gh_adx_error gh_adx_stream_poll(gh_adx_stream*p,gh_adx_stream_status*out){unsigned i;if(!owned(p)||!out)return GH_ADX_ARGUMENT;if(p->error)return p->error;
 if(p->started){gh_adx_error e=pump(p);if(e)return p->error=e;}
 memset(out,0,sizeof(*out));out->format=p->info;out->submitted_frames=p->submitted;out->completed_frames=p->completed;out->loop_count=p->loops;out->prepared=1;out->started=p->started;out->paused=p->paused;out->complete=p->complete;out->muted=p->muted;out->gain=p->gain;
 for(i=0;i<QUEUE_COUNT;i++)if(p->buffers[i].queued)out->queued_buffers++;return GH_ADX_OK;
}
gh_adx_error gh_adx_stream_stop(gh_adx_stream*p){unsigned i;if(!owned(p))return GH_ADX_ARGUMENT;
 if(waveOutReset(p->device)!=MMSYSERR_NOERROR||waveOutPause(p->device)!=MMSYSERR_NOERROR)return p->error=GH_ADX_DEVICE;
 for(i=0;i<QUEUE_COUNT;i++)p->buffers[i].queued=0;p->started=0;p->paused=0;p->complete=0;p->submitted=0;p->completed=0;p->loops=0;p->error=GH_ADX_OK;return GH_ADX_OK;
}
gh_adx_error gh_adx_stream_seek(gh_adx_stream*p,uint32_t sample){gh_adx_error e;if(!owned(p))return GH_ADX_ARGUMENT;if(p->started)return GH_ADX_STATE;if(p->loop&&sample>p->info.loop_end)return GH_ADX_BOUNDS;e=gh_adx_seek(p->decoder,sample);if(!e){p->eof=0;p->complete=0;p->submitted=0;p->completed=0;p->loops=0;}return e;}
