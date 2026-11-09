/* Developer CLI for the real startup-task integration slice, not game.exe. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../src/platform/startup_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
static char *utf8(const wchar_t *s)
{
    int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s,-1,0,0,0,0);char *out;
    if(!n)return 0;out=malloc((size_t)n);if(!out)return 0;
    if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s,-1,out,n,0,0)){free(out);return 0;}return out;
}
int wmain(int argc,wchar_t **argv)
{
    char *iso,*profile=0,*codec=0;unsigned i;gh_startup_host *host;gh_startup_snapshot s;
    uint32_t flags598=0;
    int flags_set=0;gh_startup_continuation continuation={0};
    gh_host_error e;
    if(argc<2) {fputs("Usage: run-startup.exe <own-game.iso> [original-flags598] [--continue] [--muted] [--profile path] [--ffmpeg path]\n",stderr);return 1;}
    for(int a=2;a<argc;++a){
        if(!wcscmp(argv[a],L"--continue"))continuation.enabled=1;
        else if(!wcscmp(argv[a],L"--muted"))continuation.muted=1;
        else if(!wcscmp(argv[a],L"--profile") && a+1<argc){free(profile);profile=utf8(argv[++a]);if(!profile)return 1;}
        else if(!wcscmp(argv[a],L"--ffmpeg") && a+1<argc){free(codec);codec=utf8(argv[++a]);if(!codec)return 1;}
        else {
            wchar_t *end;unsigned long long value=wcstoull(argv[a],&end,0);
            if(flags_set || !*argv[a] || *end || value>UINT32_MAX){free(profile);free(codec);return 1;}
            flags598=(uint32_t)value;flags_set=1;
        }
    }
    iso=utf8(argv[1]);if(!iso){free(profile);free(codec);return 1;}
    e=profile?gh_startup_host_create_with_profile(iso,profile,1,flags598,64*1024*1024,&host):
        gh_startup_host_create(iso,1,flags598,64*1024*1024,&host);free(iso);free(profile);
    if(e) {fprintf(stderr,"startup host create error%d\n",e);return 1;}
    continuation.audio_dsp_policy=GH_SE_DSP_NATIVE_LINEAR_REFERENCE;continuation.ffmpeg_path=codec;
    e=gh_startup_host_set_continuation(host,&continuation);free(codec);
    if(e){(void)gh_startup_host_destroy(host);return 1;}
    for(i=0;i<10000 && gh_startup_host_tick(host)==GH_HOST_RUNNING;++i)if(continuation.enabled)Sleep(16);
    (void)gh_startup_host_snapshot(host,&s);
    printf("{\"purpose\":\"original-startup-task-execution-slice\",\"playable\":false,"
           "\"phase\":%d,\"error\":%d,\"subsystem_error\":%d,\"ticks\":%llu,"
           "\"failed_operation\":%u,\"unavailable_entry\":\"0x%08x\","
           "\"frontier_module\":\"%s\",\"mc_ui_entries\":%u,"
           "\"loaded_bundles\":%u,\"font_bound\":%s,\"menu_bound\":%s,\"id_initialized\":%s,"
           "\"game_state\":%u,\"game_assets_loaded\":%s,\"game_rate\":%.9g,"
           "\"normal_continuation\":%s,\"mc_frame_serial\":%llu,\"mc_ui_epoch\":%llu,"
           "\"mc_phase\":%u,\"mc_subphase\":%u,\"mc_done\":%s,\"movie_frames\":%llu}\n",
           (int)s.phase,(int)s.error,s.subsystem_error,(unsigned long long)s.ticks,
           s.failed_operation,(unsigned)s.unavailable_entry,s.frontier_module?s.frontier_module:"main ELF",
           s.mc_ui?s.mc_ui->entry_count:0,s.loaded_bundles,
           s.font->slots[0].bound?"true":"false",s.menu->tables_bound?"true":"false",
           s.id && s.id->initialized?"true":"false",(unsigned)s.game_state,
           s.game_assets_loaded?"true":"false",(double)s.game_rate,s.normal_continuation?"true":"false",
           (unsigned long long)s.mc_frame_serial,(unsigned long long)s.mc_ui_epoch,
           (unsigned)s.mc_phase,(unsigned)s.mc_subphase,s.mc_done?"true":"false",
           (unsigned long long)s.movie_video.frames_presented);
    e=gh_startup_host_destroy(host);if(e)return 1;
    /*2 is an expected, explicitly incomplete execution frontier, not success. */
    return s.phase==GH_HOST_FRONTIER?2:(s.phase==GH_HOST_COMPLETE?0:1);
}
