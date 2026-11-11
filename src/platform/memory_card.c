#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>
#include "memory_card.h"
enum {MC_INFO=1,MC_ICON,MC_SAVE_OPEN,MC_SAVE_READ,MC_SAVE_CLOSE,MC_SAVE_RECORD,MC_SAVE_WRITE_OPEN,MC_SAVE_WRITE,MC_SLOT_DIRECTORY,MC_SLOT_CREATE,MC_SAVE_ICON_OPEN,MC_SAVE_ICON_WRITE};
struct gh_memory_card {wchar_t root[1024];unsigned command,slot,ended;HANDLE save;uint8_t *header;const uint8_t *write_record;unsigned writing,icon_kind;uint32_t write_bytes;};
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
gh_mc_error gh_memory_card_begin(gh_memory_card *c)
{
    if(!c)return GH_MC_ARGUMENT;
    if(c->command||c->save)return GH_MC_BUSY;
    c->ended=0;c->icon_kind=0;
    return GH_MC_OK;
}
gh_mc_error gh_memory_card_end(gh_memory_card *c)
{if(!c)return GH_MC_ARGUMENT;if(c->command)return GH_MC_BUSY;
 if(c->save&&!CloseHandle(c->save))return GH_MC_IO;c->save=0;c->ended=1;return GH_MC_OK;}
gh_mc_error gh_memory_card_abort(gh_memory_card *c)
{
    if(!c)return GH_MC_ARGUMENT;
    c->command=0;
    c->header=NULL;
    c->write_record=NULL;
    return gh_memory_card_end(c);
}
static gh_mc_error request(gh_memory_card *c,unsigned command)
{if(!c)return GH_MC_ARGUMENT;if(c->ended)return GH_MC_ENDED;if(c->command)return GH_MC_BUSY;
 c->command=command;return GH_MC_OK;}
gh_mc_error gh_memory_card_request_info(gh_memory_card *c){return request(c,MC_INFO);}
gh_mc_error gh_memory_card_request_icon(gh_memory_card *c,uint32_t slot)
{if(slot>=20)return GH_MC_ARGUMENT;gh_mc_error e=request(c,MC_ICON);if(!e)c->slot=slot;return e;}
gh_mc_error gh_memory_card_request_slot_directory(gh_memory_card *c,uint32_t slot)
{if(slot>=20)return GH_MC_ARGUMENT;gh_mc_error e=request(c,MC_SLOT_DIRECTORY);if(!e)c->slot=slot;return e;}
gh_mc_error gh_memory_card_request_save_open(gh_memory_card *c,uint32_t slot)
{if(!c||slot>=20)return GH_MC_ARGUMENT;if(c->save)return GH_MC_BUSY;
 gh_mc_error e=request(c,MC_SAVE_OPEN);if(!e)c->slot=slot;return e;}
gh_mc_error gh_memory_card_request_save_header(gh_memory_card *c,uint8_t out[GH_MC_SAVE_HEADER_BYTES])
{if(!c||!out)return GH_MC_ARGUMENT;if(!c->ended&&!c->command&&!c->save)return GH_MC_ARGUMENT;
 gh_mc_error e=request(c,MC_SAVE_READ);if(!e)c->header=out;return e;}
gh_mc_error gh_memory_card_request_save_record(gh_memory_card *c,uint8_t out[GH_MC_SAVE_RECORD_BYTES])
{if(!c||!out)return GH_MC_ARGUMENT;if(!c->ended&&!c->command&&!c->save)return GH_MC_ARGUMENT;
 gh_mc_error e=request(c,MC_SAVE_RECORD);if(!e)c->header=out;return e;}
gh_mc_error gh_memory_card_request_save_write_open(gh_memory_card *c,uint32_t slot)
{
    if(!c||slot>=20)return GH_MC_ARGUMENT;
    if(c->save)return GH_MC_BUSY;
    gh_mc_error e=request(c,MC_SAVE_WRITE_OPEN);
    if(!e){c->slot=slot;c->icon_kind=0;}
    return e;
}
gh_mc_error gh_memory_card_request_save_write(gh_memory_card *c,const uint8_t *record)
{
    if(!c||!record)return GH_MC_ARGUMENT;
    if(!c->ended&&!c->command&&(!c->save||!c->writing||c->icon_kind))return GH_MC_ARGUMENT;
    gh_mc_error e=request(c,MC_SAVE_WRITE);
    if(!e){c->write_record=record;c->write_bytes=GH_MC_SAVE_RECORD_BYTES;}
    return e;
}
gh_mc_error gh_memory_card_request_slot_create(gh_memory_card *c,uint32_t slot)
{
    if(!c||slot>=20)return GH_MC_ARGUMENT;
    if(c->save)return GH_MC_BUSY;
    gh_mc_error e=request(c,MC_SLOT_CREATE);
    if(!e)c->slot=slot;
    return e;
}
static uint32_t icon_write_bytes(gh_mc_save_icon_kind kind)
{
    if(kind==GH_MC_SAVE_ICON_MODEL)return 0x8618;
    if(kind==GH_MC_SAVE_ICON_SYSTEM)return 0x3c4;
    return 0;
}
gh_mc_error gh_memory_card_request_save_icon_write_open(gh_memory_card *c,uint32_t slot,gh_mc_save_icon_kind kind)
{
    if(!c||slot>=20||!icon_write_bytes(kind))return GH_MC_ARGUMENT;
    if(c->save)return GH_MC_BUSY;
    gh_mc_error e=request(c,MC_SAVE_ICON_OPEN);
    if(!e){c->slot=slot;c->icon_kind=(unsigned)kind;}
    return e;
}
gh_mc_error gh_memory_card_request_save_icon_write(gh_memory_card *c,gh_mc_save_icon_kind kind,const uint8_t *bytes,uint32_t count)
{
    if(!c||!bytes||!count||count!=icon_write_bytes(kind))return GH_MC_ARGUMENT;
    if(!c->ended&&!c->command&&(!c->save||!c->writing||c->icon_kind!=(unsigned)kind))
        return GH_MC_ARGUMENT;
    gh_mc_error e=request(c,MC_SAVE_ICON_WRITE);
    if(!e){c->write_record=bytes;c->write_bytes=count;}
    return e;
}
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
    const int writing=command==MC_SAVE_WRITE_OPEN||command==MC_SAVE_ICON_OPEN;
    if(writing||command==MC_SLOT_CREATE){
        DWORD root=GetFileAttributesW(c->root);
        if(root==INVALID_FILE_ATTRIBUTES)return GH_MC_IO;
        if(!(root&FILE_ATTRIBUTE_DIRECTORY)||(root&FILE_ATTRIBUTE_REPARSE_POINT))
            return GH_MC_UNSUPPORTED;
    }
    DWORD attributes=GetFileAttributesW(path);
    if((command==MC_SAVE_WRITE_OPEN||command==MC_SLOT_CREATE)&&attributes==INVALID_FILE_ATTRIBUTES&&missing(GetLastError())){
        if(!CreateDirectoryW(path,0)&&GetLastError()!=ERROR_ALREADY_EXISTS)return GH_MC_IO;
        attributes=GetFileAttributesW(path);
    }
    if(attributes==INVALID_FILE_ATTRIBUTES){
        if(!missing(GetLastError()))return GH_MC_IO;
        r->result=command==MC_ICON||command==MC_SLOT_DIRECTORY?0:GH_MC_SAVE_NOT_FOUND;
        return GH_MC_OK;
    }
    if(!(attributes&FILE_ATTRIBUTE_DIRECTORY)||(attributes&FILE_ATTRIBUTE_REPARSE_POINT))return GH_MC_UNSUPPORTED;
    if(command==MC_SLOT_CREATE)return GH_MC_OK;
    if(command==MC_SLOT_DIRECTORY){r->result=1;return GH_MC_OK;}
    const wchar_t *basename=folder;
    if(command==MC_ICON||(command==MC_SAVE_ICON_OPEN&&c->icon_kind==GH_MC_SAVE_ICON_SYSTEM))
        basename=L"icon.sys";
    else if(command==MC_SAVE_ICON_OPEN)basename=L"godh0.ico";
    if((size_t)length+1+wcslen(basename)>=1100)return GH_MC_ARGUMENT;
    wcscat(path,L"\\");wcscat(path,basename);
    if(command==MC_ICON){
        WIN32_FILE_ATTRIBUTE_DATA icon;
        if(!GetFileAttributesExW(path,GetFileExInfoStandard,&icon))
            return missing(GetLastError())?GH_MC_OK:GH_MC_IO;
        if(icon.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)return GH_MC_UNSUPPORTED;
        r->result=!(icon.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY);
        if(r->result)r->file_bytes=((uint64_t)icon.nFileSizeHigh<<32)|icon.nFileSizeLow;
        return GH_MC_OK;
    }
    HANDLE file=CreateFileW(path,writing?GENERIC_WRITE:GENERIC_READ,FILE_SHARE_READ,0,
                            writing?OPEN_ALWAYS:OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,0);
    if(file==INVALID_HANDLE_VALUE){
        if(!missing(GetLastError()))return GH_MC_IO;
        r->result=GH_MC_SAVE_NOT_FOUND;
        return GH_MC_OK;
    }
    BY_HANDLE_FILE_INFORMATION info;
    if(!GetFileInformationByHandle(file,&info)){CloseHandle(file);return GH_MC_IO;}
    if((info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))||
       (writing&&info.nNumberOfLinks!=1)){
        CloseHandle(file);return GH_MC_UNSUPPORTED;
    }
    c->save=file;c->writing=(unsigned)writing;
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
    } else if(command==MC_ICON||command==MC_SAVE_OPEN||command==MC_SAVE_WRITE_OPEN||command==MC_SLOT_DIRECTORY||
              command==MC_SLOT_CREATE||command==MC_SAVE_ICON_OPEN) {
        gh_mc_error e=slot_file(c,command,r);
        if(e)return e;
    } else if(command==MC_SAVE_READ) {
        uint8_t data[GH_MC_SAVE_HEADER_BYTES],*destination=c->header;
        DWORD bytes;
        c->header=0;
        if(!ReadFile(c->save,data,sizeof(data),&bytes,0))return GH_MC_IO;
        CopyMemory(destination,data,bytes);
        r->result=(int32_t)bytes;
    } else if(command==MC_SAVE_RECORD) {
        uint8_t *destination=c->header;
        DWORD bytes;
        c->header=0;
        if(!ReadFile(c->save,destination,GH_MC_SAVE_RECORD_BYTES,&bytes,0))return GH_MC_IO;
        r->result=(int32_t)bytes;
    } else if(command==MC_SAVE_WRITE||command==MC_SAVE_ICON_WRITE) {
        const uint8_t *record=c->write_record;
        DWORD bytes;
        c->write_record=0;
        if(!WriteFile(c->save,record,c->write_bytes,&bytes,0))return GH_MC_IO;
        r->result=(int32_t)bytes;
    } else if(command==MC_SAVE_CLOSE) {
        if(!CloseHandle(c->save))return GH_MC_IO;
        c->save=0;c->icon_kind=0;
    }
    *done=1;return GH_MC_OK;
}
