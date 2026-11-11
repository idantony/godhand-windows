#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>
#include "memory_card.h"
enum {MC_INFO=1,MC_ICON,MC_SAVE_OPEN,MC_SAVE_READ,MC_SAVE_CLOSE};
struct gh_memory_card {wchar_t root[1024];unsigned command,slot,ended;HANDLE save;uint8_t *header;};
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
void gh_memory_card_close(gh_memory_card *c)
{if(c){if(c->save)CloseHandle(c->save);HeapFree(GetProcessHeap(),0,c);}}
gh_mc_error gh_memory_card_end(gh_memory_card *c)
{if(!c)return GH_MC_ARGUMENT;if(c->command)return GH_MC_BUSY;
 if(c->save&&!CloseHandle(c->save))return GH_MC_IO;c->save=0;c->ended=1;return GH_MC_OK;}
static gh_mc_error request(gh_memory_card *c,unsigned command)
{if(!c)return GH_MC_ARGUMENT;if(c->ended)return GH_MC_ENDED;if(c->command)return GH_MC_BUSY;
 c->command=command;return GH_MC_OK;}
gh_mc_error gh_memory_card_request_info(gh_memory_card *c){return request(c,MC_INFO);}
gh_mc_error gh_memory_card_request_icon(gh_memory_card *c,uint32_t slot)
{if(slot>=20)return GH_MC_ARGUMENT;gh_mc_error e=request(c,MC_ICON);if(!e)c->slot=slot;return e;}
gh_mc_error gh_memory_card_request_save_open(gh_memory_card *c,uint32_t slot)
{if(!c||slot>=20)return GH_MC_ARGUMENT;if(c->save)return GH_MC_BUSY;
 gh_mc_error e=request(c,MC_SAVE_OPEN);if(!e)c->slot=slot;return e;}
gh_mc_error gh_memory_card_request_save_header(gh_memory_card *c,uint8_t out[GH_MC_SAVE_HEADER_BYTES])
{if(!c||!out)return GH_MC_ARGUMENT;if(!c->ended&&!c->command&&!c->save)return GH_MC_ARGUMENT;
 gh_mc_error e=request(c,MC_SAVE_READ);if(!e)c->header=out;return e;}
gh_mc_error gh_memory_card_request_save_close(gh_memory_card *c)
{if(!c)return GH_MC_ARGUMENT;if(!c->ended&&!c->command&&!c->save)return GH_MC_ARGUMENT;
 return request(c,MC_SAVE_CLOSE);}
static int missing(DWORD error){return error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND;}
static gh_mc_error slot_file(gh_memory_card *c,unsigned command,gh_mc_result *r)
{
    wchar_t folder[24],path[1100];
    if(swprintf(folder,24,L"BASLUS-21503GODHD0%02u",c->slot)<0)return GH_MC_ARGUMENT;
    int length=swprintf(path,1100,L"%ls\\%ls",c->root,folder);
    if(length<0 || length>=1100)return GH_MC_ARGUMENT;
    DWORD attributes=GetFileAttributesW(path);
    if(attributes==INVALID_FILE_ATTRIBUTES){
        if(!missing(GetLastError()))return GH_MC_IO;
        r->result=command==MC_ICON?0:GH_MC_SAVE_NOT_FOUND;
        return GH_MC_OK;
    }
    if(!(attributes&FILE_ATTRIBUTE_DIRECTORY)||(attributes&FILE_ATTRIBUTE_REPARSE_POINT))return GH_MC_UNSUPPORTED;
    const wchar_t *basename=command==MC_ICON?L"icon.sys":folder;
    if((size_t)length+1+wcslen(basename)>=1100)return GH_MC_ARGUMENT;
    wcscat(path,L"\\");wcscat(path,basename);
    if(command==MC_ICON){
        attributes=GetFileAttributesW(path);
        if(attributes==INVALID_FILE_ATTRIBUTES)return missing(GetLastError())?GH_MC_OK:GH_MC_IO;
        if(attributes&FILE_ATTRIBUTE_REPARSE_POINT)return GH_MC_UNSUPPORTED;
        r->result=!(attributes&FILE_ATTRIBUTE_DIRECTORY);
        return GH_MC_OK;
    }
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,0);
    if(file==INVALID_HANDLE_VALUE){
        if(!missing(GetLastError()))return GH_MC_IO;
        r->result=GH_MC_SAVE_NOT_FOUND;
        return GH_MC_OK;
    }
    BY_HANDLE_FILE_INFORMATION info;
    if(!GetFileInformationByHandle(file,&info)){CloseHandle(file);return GH_MC_IO;}
    if(info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)){
        CloseHandle(file);return GH_MC_UNSUPPORTED;
    }
    c->save=file;
    return GH_MC_OK;
}
gh_mc_error gh_memory_card_poll(gh_memory_card *c,int *done,gh_mc_result *r)
{
    DWORD attributes,error;
    if(!c || !done || !r)return GH_MC_ARGUMENT;
    *done=0;ZeroMemory(r,sizeof(*r));
    if(c->ended)return GH_MC_ENDED;
    if(!c->command)return GH_MC_OK;
    unsigned command=c->command;
    c->command=0;
    if(command==MC_INFO) {
        ULARGE_INTEGER available;
        attributes=GetFileAttributesW(c->root);
        if(attributes==INVALID_FILE_ATTRIBUTES) {
            error=GetLastError();
            if(!missing(error))return GH_MC_IO;
            r->result=-1; /* actual absent profile, not a fabricated success */
        } else {
            if(!(attributes&FILE_ATTRIBUTE_DIRECTORY) || (attributes&FILE_ATTRIBUTE_REPARSE_POINT))return GH_MC_UNSUPPORTED;
            if(!GetDiskFreeSpaceExW(c->root,&available,0,0))return GH_MC_IO;
            r->card_type=2;r->formatted=1;
            r->free_clusters=available.QuadPart/1024>INT32_MAX?INT32_MAX:(int32_t)(available.QuadPart/1024);
        }
    } else if(command==MC_ICON||command==MC_SAVE_OPEN) {
        gh_mc_error e=slot_file(c,command,r);
        if(e)return e;
    } else if(command==MC_SAVE_READ) {
        uint8_t data[GH_MC_SAVE_HEADER_BYTES],*destination=c->header;
        DWORD bytes;
        c->header=0;
        if(!ReadFile(c->save,data,sizeof(data),&bytes,0))return GH_MC_IO;
        CopyMemory(destination,data,bytes);
        r->result=(int32_t)bytes;
    } else if(command==MC_SAVE_CLOSE) {
        if(!CloseHandle(c->save))return GH_MC_IO;
        c->save=0;
    }
    *done=1;return GH_MC_OK;
}
