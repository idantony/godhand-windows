#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "video_player.h"
typedef struct {HANDLE process,read;} stream;
struct gh_video_player {stream video;HWAVEOUT audio;WAVEHDR header;unsigned char *pcm,*rgba;size_t pcm_bytes;uint64_t frames;LARGE_INTEGER start,frequency;DWORD owner;gh_video_present present;void*context;int prepared,started,eof,muted;uint32_t exit_code;gh_video_error error;};
static wchar_t *wide(const char*s){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,NULL,0);wchar_t*w;if(!n)return NULL;w=malloc((size_t)n*sizeof(*w));if(w&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,w,n)){free(w);return NULL;}return w;}
static int quote(wchar_t *out,size_t *used,const wchar_t *in){size_t i=0;if(*used+2>=32760)return 0;out[(*used)++]=L'"';while(in[i]){size_t slashes=0;while(in[i]==L'\\'){slashes++;i++;}if(in[i]==L'"'||!in[i])slashes*=2;if(*used+slashes+3>=32760)return 0;while(slashes--)out[(*used)++]=L'\\';if(in[i]==L'"')out[(*used)++]=L'\\';if(in[i])out[(*used)++]=in[i++];}out[(*used)++]=L'"';out[(*used)++]=L' ';out[*used]=0;return 1;}
static gh_video_error launch(const char*exe,const char*input,int audio,stream*out){
 const char *args_video[]={"-v","error","-nostdin","-protocol_whitelist","file,subfile,pipe","-i",input,"-map","0:v:0","-fps_mode","passthrough","-pix_fmt","rgba","-f","rawvideo","pipe:1"};
 const char *args_audio[]={"-v","error","-nostdin","-protocol_whitelist","file,subfile,pipe","-i",input,"-map","0:a:0","-ar","48000","-ac","2","-f","s16le","pipe:1"};
 const char**args=audio?args_audio:args_video;size_t count=audio?sizeof(args_audio)/sizeof(*args_audio):sizeof(args_video)/sizeof(*args_video),used=0,i;wchar_t*app=wide(exe),*cmd=calloc(32768,sizeof(wchar_t));SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};HANDLE write=NULL,nul=NULL;STARTUPINFOW startup;PROCESS_INFORMATION pi;
 if(!app||!cmd){free(app);free(cmd);return GH_VIDEO_MEMORY;}if(!quote(cmd,&used,app))goto fail;
 for(i=0;i<count;i++){wchar_t*a=wide(args[i]);int ok=a&&quote(cmd,&used,a);free(a);if(!ok)goto fail;}
 if(!CreatePipe(&out->read,&write,&sa,0)||!SetHandleInformation(out->read,HANDLE_FLAG_INHERIT,0))goto fail;
 nul=CreateFileW(L"NUL",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,NULL);if(nul==INVALID_HANDLE_VALUE){nul=NULL;goto fail;}
 memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdInput=nul;startup.hStdOutput=write;startup.hStdError=nul;memset(&pi,0,sizeof(pi));
 if(!CreateProcessW(app,cmd,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&startup,&pi))goto fail;CloseHandle(pi.hThread);CloseHandle(write);CloseHandle(nul);out->process=pi.hProcess;free(app);free(cmd);return GH_VIDEO_OK;
fail:if(out->read){CloseHandle(out->read);out->read=NULL;}if(write)CloseHandle(write);if(nul)CloseHandle(nul);free(app);free(cmd);return GH_VIDEO_PROCESS;
}
static void stop(stream*s){if(s->read)CloseHandle(s->read);if(s->process){if(WaitForSingleObject(s->process,0)==WAIT_TIMEOUT){TerminateProcess(s->process,1);WaitForSingleObject(s->process,1000);}CloseHandle(s->process);}memset(s,0,sizeof(*s));}
void gh_video_close(gh_video_player*p){if(!p)return;if(p->audio){waveOutReset(p->audio);if(p->prepared)waveOutUnprepareHeader(p->audio,&p->header,sizeof(p->header));waveOutClose(p->audio);}stop(&p->video);free(p->pcm);free(p->rgba);free(p);}
gh_video_error gh_video_open_movie0(const char*iso,const char*ffmpeg,gh_video_present present,void*context,gh_video_player**out){
 gh_disc*d=NULL;size_t index;gh_disc_entry e;gh_video_player*p=NULL;stream audio={0};char*input=NULL;size_t input_size;gh_video_error error=GH_VIDEO_MEMORY;DWORD read=0,code=0;WAVEFORMATEX format;unsigned char prefix[16384];size_t k;int sequence=0;
 if(!iso||!ffmpeg||!present||!out)return GH_VIDEO_ARGUMENT;*out=NULL;
 if(gh_disc_open(iso,&d)!=GH_DISC_OK)return GH_VIDEO_DISC;
 if(gh_disc_find(d,"movie/clover.sfd",&index)!=GH_DISC_OK||gh_disc_entry_at(d,index,&e)!=GH_DISC_OK||gh_disc_read(d,index,0,prefix,sizeof(prefix))!=GH_DISC_OK){gh_disc_close(d);return GH_VIDEO_DISC;}gh_disc_close(d);
 if(memcmp(prefix,"\0\0\1\272",4))return GH_VIDEO_CODEC;
 for(k=0;k+8<=sizeof(prefix);k++)if(!memcmp(prefix+k,"\0\0\1\263",4)){unsigned w=((unsigned)prefix[k+4]<<4)|(prefix[k+5]>>4),h=((unsigned)(prefix[k+5]&15)<<8)|prefix[k+6];if(w!=640||h!=448||(prefix[k+7]&15)!=4)return GH_VIDEO_CODEC;sequence=1;break;}
 if(!sequence)return GH_VIDEO_CODEC;
 if(e.size!=1935360||e.iso_offset>UINT64_MAX-e.size)return GH_VIDEO_CODEC;input_size=strlen(iso)+160;input=malloc(input_size);p=calloc(1,sizeof(*p));if(!input||!p)goto fail;
 snprintf(input,input_size,"subfile,,start,%llu,end,%llu,,:%s",(unsigned long long)e.iso_offset,(unsigned long long)(e.iso_offset+e.size),iso);p->owner=GetCurrentThreadId();p->present=present;p->context=context;p->rgba=malloc(640*448*4);p->pcm=malloc(48000*4*10);if(!p->rgba||!p->pcm)goto fail;
 error=launch(ffmpeg,input,1,&audio);if(error)goto fail;
 for(;;){unsigned char chunk[16384];BOOL ok=ReadFile(audio.read,chunk,sizeof(chunk),&read,NULL);if(!ok&&GetLastError()!=ERROR_BROKEN_PIPE){error=GH_VIDEO_PROCESS;goto fail;}if(!read)break;if(read>48000*4*10-p->pcm_bytes){error=GH_VIDEO_CODEC;goto fail;}memcpy(p->pcm+p->pcm_bytes,chunk,read);p->pcm_bytes+=read;}
 if(WaitForSingleObject(audio.process,10000)!=WAIT_OBJECT_0||!GetExitCodeProcess(audio.process,&code)||code||!p->pcm_bytes||p->pcm_bytes%4){error=GH_VIDEO_CODEC;goto fail;}stop(&audio);
 memset(&format,0,sizeof(format));format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=2;format.nSamplesPerSec=48000;format.wBitsPerSample=16;format.nBlockAlign=4;format.nAvgBytesPerSec=192000;
 if(waveOutOpen(&p->audio,WAVE_MAPPER,&format,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR){error=GH_VIDEO_AUDIO;goto fail;}p->header.lpData=(LPSTR)p->pcm;p->header.dwBufferLength=(DWORD)p->pcm_bytes;
 if(waveOutPrepareHeader(p->audio,&p->header,sizeof(p->header))!=MMSYSERR_NOERROR){error=GH_VIDEO_AUDIO;goto fail;}p->prepared=1;
 error=launch(ffmpeg,input,0,&p->video);if(error)goto fail;free(input);*out=p;return GH_VIDEO_OK;
fail:stop(&audio);free(input);gh_video_close(p);return error;
}
gh_video_error gh_video_set_muted(gh_video_player*p,int muted){if(!p||GetCurrentThreadId()!=p->owner||p->started||(muted!=0&&muted!=1))return GH_VIDEO_ARGUMENT;p->muted=muted;return GH_VIDEO_OK;}
gh_video_error gh_video_poll(gh_video_player*p,gh_video_status*out){
 LARGE_INTEGER now;uint64_t ns,due;size_t used=0;DWORD count=0;if(!p||!out||GetCurrentThreadId()!=p->owner)return GH_VIDEO_ARGUMENT;memset(out,0,sizeof(*out));if(p->error)return p->error;
 if(!p->started){QueryPerformanceFrequency(&p->frequency);QueryPerformanceCounter(&p->start);if(p->muted)memset(p->pcm,0,p->pcm_bytes);if(waveOutWrite(p->audio,&p->header,sizeof(p->header))!=MMSYSERR_NOERROR)return p->error=GH_VIDEO_AUDIO;p->started=1;}
 QueryPerformanceCounter(&now);ns=(uint64_t)(now.QuadPart-p->start.QuadPart)*1000000000/(uint64_t)p->frequency.QuadPart;due=p->frames*UINT64_C(1001000000000)/30000;
 if(!p->eof&&ns>=due){
  while(used<640*448*4){BOOL ok=ReadFile(p->video.read,p->rgba+used,(DWORD)(640*448*4-used),&count,NULL);if(!ok&&GetLastError()!=ERROR_BROKEN_PIPE)return p->error=GH_VIDEO_PROCESS;if(!count)break;used+=count;}
  if(!used){DWORD code;if(WaitForSingleObject(p->video.process,10000)!=WAIT_OBJECT_0||!GetExitCodeProcess(p->video.process,&code))return p->error=GH_VIDEO_PROCESS;p->exit_code=code;if(code||!p->frames)return p->error=GH_VIDEO_CODEC;p->eof=1;}
  else{if(used!=640*448*4)return p->error=GH_VIDEO_CODEC;if(!p->present(p->context,p->rgba,used,640,448,due))return p->error=GH_VIDEO_PRESENT;p->frames++;}
 }
 out->frames_presented=p->frames;out->audio_sample_frames=p->pcm_bytes/4;out->video_eof=p->eof;out->audio_done=(p->header.dwFlags&WHDR_DONE)!=0;out->complete=p->eof&&out->audio_done;out->process_exit=p->exit_code;return GH_VIDEO_OK;
}
