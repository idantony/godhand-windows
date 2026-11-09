#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>
#include "memory_card.h"
struct gh_memory_card {wchar_t root[1024];unsigned command,slot,ended;};
gh_mc_error gh_memory_card_open(const char *root,gh_memory_card **out)
{
    gh_memory_card *c;
    if(!root || !*root || !out)return GH_MC_ARGUMENT;
    *out=0;c=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*c));
    if(!c)return GH_MC_MEMORY;
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,root,-1,c->root,1024)) {
        HeapFree(GetProcessHeap(),0,c);return GH_MC_ARGUMENT;
    }
    *out=c;return GH_MC_OK;
}
void gh_memory_card_close(gh_memory_card *c) {if(c)HeapFree(GetProcessHeap(),0,c);}
gh_mc_error gh_memory_card_end(gh_memory_card *c)
{if(!c)return GH_MC_ARGUMENT;if(c->command)return GH_MC_BUSY;c->ended=1;return GH_MC_OK;}
gh_mc_error gh_memory_card_request_info(gh_memory_card *c)
{if(!c)return GH_MC_ARGUMENT;if(c->ended)return GH_MC_ENDED;if(c->command)return GH_MC_BUSY;c->command=1;return GH_MC_OK;}
gh_mc_error gh_memory_card_request_icon(gh_memory_card *c,uint32_t slot)
{if(!c || slot>=20)return GH_MC_ARGUMENT;if(c->ended)return GH_MC_ENDED;if(c->command)return GH_MC_BUSY;c->command=2;c->slot=slot;return GH_MC_OK;}
gh_mc_error gh_memory_card_poll(gh_memory_card *c,int *done,gh_mc_result *r)
{
    DWORD attributes,error;
    if(!c || !done || !r)return GH_MC_ARGUMENT;
    *done=0;ZeroMemory(r,sizeof(*r));
    if(c->ended)return GH_MC_ENDED;
    if(!c->command)return GH_MC_OK;
    if(c->command==1) {
        ULARGE_INTEGER available;
        attributes=GetFileAttributesW(c->root);
        if(attributes==INVALID_FILE_ATTRIBUTES) {
            error=GetLastError();
            if(error!=ERROR_FILE_NOT_FOUND && error!=ERROR_PATH_NOT_FOUND) {c->command=0;return GH_MC_IO;}
            r->result=-1; /* actual absent profile, not a fabricated success */
        } else {
            if(!(attributes&FILE_ATTRIBUTE_DIRECTORY) || (attributes&FILE_ATTRIBUTE_REPARSE_POINT)) {c->command=0;return GH_MC_UNSUPPORTED;}
            if(!GetDiskFreeSpaceExW(c->root,&available,0,0)) {c->command=0;return GH_MC_IO;}
            r->card_type=2;r->formatted=1;
            r->free_clusters=available.QuadPart/1024>INT32_MAX?INT32_MAX:(int32_t)(available.QuadPart/1024);
        }
    } else {
        wchar_t path[1100];
        int length=swprintf(path,1100,L"%ls\\BASLUS-21503GODHD0%02u",c->root,c->slot);
        if(length<0) {c->command=0;return GH_MC_ARGUMENT;}
        attributes=GetFileAttributesW(path);
        if(attributes==INVALID_FILE_ATTRIBUTES) {
            error=GetLastError();c->command=0;
            if(error!=ERROR_FILE_NOT_FOUND && error!=ERROR_PATH_NOT_FOUND)return GH_MC_IO;
            r->result=0;*done=1;return GH_MC_OK;
        }
        if(!(attributes&FILE_ATTRIBUTE_DIRECTORY) || (attributes&FILE_ATTRIBUTE_REPARSE_POINT)) {c->command=0;return GH_MC_UNSUPPORTED;}
        if((size_t)length+10>=1100) {c->command=0;return GH_MC_ARGUMENT;}
        wcscat(path,L"\\icon.sys");
        attributes=GetFileAttributesW(path);
        if(attributes==INVALID_FILE_ATTRIBUTES) {
            error=GetLastError();
            if(error!=ERROR_FILE_NOT_FOUND && error!=ERROR_PATH_NOT_FOUND) {c->command=0;return GH_MC_IO;}
            r->result=0;
        } else {
            if(attributes&FILE_ATTRIBUTE_REPARSE_POINT) {c->command=0;return GH_MC_UNSUPPORTED;}
            r->result=(attributes&FILE_ATTRIBUTE_DIRECTORY)?0:1;
        }
    }
    c->command=0;*done=1;return GH_MC_OK;
}
