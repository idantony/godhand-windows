#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../../src/platform/video_player.h"
#include <stdio.h>
#include <stdlib.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {unsigned count;uint64_t last;int fail;} sink;
static int consume(void*c,const unsigned char*rgba,size_t n,unsigned w,unsigned h,uint64_t pts){sink*s=c;C(rgba&&n==640*448*4&&w==640&&h==448);if(s->count)C(pts>s->last);else C(pts==0);s->last=pts;s->count++;return !s->fail;}
static DWORD WINAPI foreign(void*p){gh_video_status s;return (DWORD)gh_video_poll(p,&s);}
int main(int argc,char**argv){gh_video_player*p=NULL;gh_video_status s={0};sink output={0};gh_video_error e;ULONGLONG start;HANDLE thread;DWORD code;C(argc==3);
 C(gh_video_open_movie0(argv[1],argv[2],NULL,&output,&p)==GH_VIDEO_ARGUMENT);C(gh_video_open_movie0(argv[1],"G:\\middle_thing\\godhand_windows\\missing-codec.exe",consume,&output,&p)==GH_VIDEO_PROCESS&&p==NULL);
 C(gh_video_open_movie0(argv[1],argv[2],consume,&output,&p)==GH_VIDEO_OK);C(gh_video_set_muted(p,1)==GH_VIDEO_OK);thread=CreateThread(NULL,0,foreign,p,0,NULL);C(thread);C(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0&&GetExitCodeThread(thread,&code)&&code==GH_VIDEO_ARGUMENT);CloseHandle(thread);
 start=GetTickCount64();do{e=gh_video_poll(p,&s);C(e==GH_VIDEO_OK);if(!s.complete)Sleep(1);C(GetTickCount64()-start<15000);}while(!s.complete);
 C(s.video_eof&&s.audio_done&&s.frames_presented==90&&output.count==90&&s.audio_sample_frames==123328&&GetTickCount64()-start>=3000);gh_video_close(p);p=NULL;
 output.count=0;output.fail=1;C(gh_video_open_movie0(argv[1],argv[2],consume,&output,&p)==GH_VIDEO_OK);C(gh_video_set_muted(p,1)==GH_VIDEO_OK);C(gh_video_poll(p,&s)==GH_VIDEO_PRESENT&&!s.complete);C(gh_video_poll(p,&s)==GH_VIDEO_PRESENT&&output.count==1);gh_video_close(p);
 puts("PASS ISO Movie0: 90 realdecodedframes/123328 PCM frames, actualwaveOut DONE/EOF after3s, foreignthread reject, missingcodec error and presenterfailure latched/cancel cleanup");return 0;}
