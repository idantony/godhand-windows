#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../../src/platform/adx_stream.h"
#include "../../src/platform/disc.h"
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static DWORD WINAPI foreign(LPVOID p){return (DWORD)gh_adx_stream_set_gain(p,0.5f);}
int main(int argc,char**argv){gh_disc*d=NULL;size_t index,elfn;unsigned char *encoded,*elf;gh_adx_key key;gh_adx_stream*p=NULL;gh_adx_stream_status s,before;DWORD code;HANDLE thread;ULONGLONG start;int full;
 C(argc==2||(argc==3&&!strcmp(argv[2],"--full")));full=argc==3;
 C(gh_disc_open(argv[1],&d)==GH_DISC_OK&&gh_disc_find(d,"bgm/bgm00.afs",&index)==GH_DISC_OK);elfn=(size_t)gh_disc_boot_elf_size(d);elf=malloc(elfn);encoded=malloc(7168000);C(elf&&encoded);
 C(gh_disc_read(d,index,2048,encoded,7168000)==GH_DISC_OK&&gh_disc_read_boot_elf(d,0,elf,elfn)==GH_DISC_OK);gh_disc_close(d);
 C(gh_adx_key_from_elf(elf,elfn,&key)==GH_ADX_OK);C(gh_adx_stream_open(encoded,7168000,&key,&p)==GH_ADX_OK);
 C(gh_adx_stream_poll(p,&s)==GH_ADX_OK&&s.prepared&&!s.started&&!s.submitted_frames&&!s.complete);
 C(gh_adx_stream_set_muted(p,1)==GH_ADX_OK);C(gh_adx_stream_set_gain(p,NAN)==GH_ADX_ARGUMENT&&gh_adx_stream_set_gain(p,1.01f)==GH_ADX_ARGUMENT);
 thread=CreateThread(NULL,0,foreign,p,0,NULL);C(thread&&WaitForSingleObject(thread,2000)==WAIT_OBJECT_0&&GetExitCodeThread(thread,&code)&&code==GH_ADX_ARGUMENT);CloseHandle(thread);
 C(gh_adx_stream_set_loop(p,1)==GH_ADX_OK);C(gh_adx_stream_seek(p,6163980-4096)==GH_ADX_OK);C(gh_adx_stream_start(p)==GH_ADX_OK);
 C(gh_adx_stream_poll(p,&s)==GH_ADX_OK&&s.started&&s.muted&&s.loop_count>=1&&s.queued_buffers==4&&!s.complete);
 C(gh_adx_stream_seek(p,0)==GH_ADX_STATE&&gh_adx_stream_set_muted(p,0)==GH_ADX_STATE);
 C(gh_adx_stream_pause(p,1)==GH_ADX_OK);C(gh_adx_stream_poll(p,&before)==GH_ADX_OK&&before.paused);Sleep(40);C(gh_adx_stream_poll(p,&s)==GH_ADX_OK&&s.completed_frames==before.completed_frames);
 C(gh_adx_stream_pause(p,0)==GH_ADX_OK);start=GetTickCount64();do{Sleep(5);C(gh_adx_stream_poll(p,&s)==GH_ADX_OK);C(GetTickCount64()-start<2000);}while(!s.completed_frames);C(!s.complete&&s.loop_count>=1);
 C(gh_adx_stream_stop(p)==GH_ADX_OK);C(gh_adx_stream_poll(p,&s)==GH_ADX_OK&&!s.started&&!s.queued_buffers&&!s.complete);
 C(gh_adx_stream_set_loop(p,0)==GH_ADX_OK);C(gh_adx_stream_set_gain(p,0.5f)==GH_ADX_OK);C(gh_adx_stream_seek(p,full?0:6369200-4800)==GH_ADX_OK);C(gh_adx_stream_start(p)==GH_ADX_OK);
 start=GetTickCount64();do{Sleep(5);C(gh_adx_stream_poll(p,&s)==GH_ADX_OK);C(GetTickCount64()-start<(full?150000u:3000u));}while(!s.complete);
 C(s.submitted_frames==(uint64_t)(full?6369200:4800)&&s.completed_frames==s.submitted_frames&&!s.queued_buffers&&!s.loop_count);
 printf("PASS real muted waveOut ADX stream: %llu frames consumed / %llu ms; prepare/start/pause/loop/seek/stop/EOF ownership and gain validation\n",(unsigned long long)s.completed_frames,(unsigned long long)(GetTickCount64()-start));
 gh_adx_stream_close(p);free(encoded);free(elf);return 0;
}
