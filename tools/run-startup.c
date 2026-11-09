/* Developer CLI for the real startup-task integration slice, not game.exe. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../src/platform/startup_host.h"
#include <stdio.h>
#include <stdlib.h>
int wmain(int argc,wchar_t **argv)
{
    char *iso;int bytes;unsigned i;gh_startup_host *host;gh_startup_snapshot s;
    gh_host_error e;
    if(argc!=2) {fputs("Usage: run-startup.exe <own-game.iso>\n",stderr);return 1;}
    bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,argv[1],-1,0,0,0,0);
    if(!bytes)return 1;
    iso=malloc((size_t)bytes);if(!iso)return 1;
    if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,argv[1],-1,iso,bytes,0,0)) {free(iso);return 1;}
    e=gh_startup_host_create(iso,1,0,64*1024*1024,&host);free(iso);
    if(e) {fprintf(stderr,"startup host create error%d\n",e);return 1;}
    for(i=0;i<10000 && gh_startup_host_tick(host)==GH_HOST_RUNNING;++i) {}
    (void)gh_startup_host_snapshot(host,&s);
    printf("{\"purpose\":\"original-startup-task-execution-slice\",\"playable\":false,"
           "\"phase\":%d,\"error\":%d,\"subsystem_error\":%d,\"ticks\":%llu,"
           "\"failed_operation\":%u,\"unavailable_entry\":\"0x%08x\","
           "\"loaded_bundles\":%u,\"font_bound\":%s,\"menu_bound\":%s,\"id_initialized\":%s}\n",
           (int)s.phase,(int)s.error,s.subsystem_error,(unsigned long long)s.ticks,
           s.failed_operation,(unsigned)s.unavailable_entry,s.loaded_bundles,
           s.font->slots[0].bound?"true":"false",s.menu->tables_bound?"true":"false",
           s.id && s.id->initialized?"true":"false");
    e=gh_startup_host_destroy(host);if(e)return 1;
    /*2 is an expected, explicitly incomplete execution frontier, not success. */
    return s.phase==GH_HOST_FRONTIER?2:(s.phase==GH_HOST_COMPLETE?0:1);
}
