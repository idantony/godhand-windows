#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <string.h>
#include "game_room_module.h"
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static gh_room_module_status version(const uint8_t*b,size_t size,uint16_t room){
 typedef NTSTATUS(WINAPI *open_fn)(BCRYPT_ALG_HANDLE*,LPCWSTR,LPCWSTR,ULONG);
 typedef NTSTATUS(WINAPI *hash_fn)(BCRYPT_ALG_HANDLE,PUCHAR,ULONG,PUCHAR,ULONG,PUCHAR,ULONG);
 typedef NTSTATUS(WINAPI *close_fn)(BCRYPT_ALG_HANDLE,ULONG);
 static const uint8_t hashes[2][32]={
  {0x2a,0x37,0x9d,0x50,0x3a,0x95,0xdf,0x9c,0xce,0xa4,0xa6,0x3b,0xba,0xa6,0x6f,0xe5,0x67,0x8c,0xae,0x12,0xeb,0x25,0x66,0xf5,0x91,0x47,0x8f,0x26,0xae,0xbb,0x8c,0x42},
  {0x7a,0x33,0x5d,0x22,0xc3,0x1f,0x4c,0x66,0x7e,0x94,0x86,0x81,0xa7,0x37,0x9d,0xde,0xb7,0x35,0x6e,0x3c,0x65,0x5b,0xba,0xeb,0x4e,0xe5,0x21,0x5d,0x84,0xb1,0xbc,0x57}};
 const uint8_t *expected=hashes[room==0x100];size_t expected_size=room==0x100?25663:24129;
 HMODULE dll;BCRYPT_ALG_HANDLE alg=0;uint8_t digest[32];NTSTATUS st;
 open_fn open;hash_fn hash;close_fn close;
 if(!b||size!=expected_size||memcmp(b,"SNR2",4)||u32(b+44)!=size)return GH_ROOM_MODULE_VERSION;
 dll=LoadLibraryExW(L"bcrypt.dll",0,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!dll)return GH_ROOM_MODULE_PLATFORM;
 open=(open_fn)(void*)GetProcAddress(dll,"BCryptOpenAlgorithmProvider");hash=(hash_fn)(void*)GetProcAddress(dll,"BCryptHash");close=(close_fn)(void*)GetProcAddress(dll,"BCryptCloseAlgorithmProvider");
 if(!open||!hash||!close){FreeLibrary(dll);return GH_ROOM_MODULE_PLATFORM;}
 st=open(&alg,BCRYPT_SHA256_ALGORITHM,0,0);if(st>=0)st=hash(alg,0,0,(PUCHAR)(void*)b,(ULONG)size,digest,32);
 if(alg)(void)close(alg,0);FreeLibrary(dll);if(st<0)return GH_ROOM_MODULE_PLATFORM;
 return memcmp(digest,expected,32)?GH_ROOM_MODULE_VERSION:GH_ROOM_MODULE_OK;
}
gh_room_module_status gh_room_module_load_002be708(gh_room_module*s,uint16_t room){
 const char*path=room==0x100?"dll/debugoptimize/r100.rel":"dll/debugoptimize/r020.rel";size_t index;gh_disc_entry entry;gh_dvd_error de;gh_room_module_status e;
 if(!s||!s->input.dvd||!s->input.dvd->disc||!s->input.heap||!s->input.threads)return GH_ROOM_MODULE_ARGUMENT;
 if(s->input.allocation.pointer)return GH_ROOM_MODULE_BUSY;s->frontier=0;s->detail=0;
 if(room!=0x20&&room!=0x100){s->frontier=0x2be708;return GH_ROOM_MODULE_UNSUPPORTED;}
 if(gh_disc_find(s->input.dvd->disc,path,&index)||gh_disc_entry_at(s->input.dvd->disc,index,&entry))return GH_ROOM_MODULE_DVD;
 s->input.file_bytes=(size_t)entry.size;s->room=room;
 s->input.job=gh_dvd_read_alloc_sized(s->input.dvd,path,&s->input.allocation,s->input.heap,0,&s->input.allocated_bytes,0,0);
 if(!s->input.job){s->detail=s->input.dvd->last_error;s->frontier=0x200cb0;return GH_ROOM_MODULE_DVD;}
 de=gh_dvd_check_wait_frame(s->input.dvd,s->input.job,s->input.threads);
 if(de){s->detail=de;s->frontier=0x201108;return GH_ROOM_MODULE_DVD;}
 e=version(s->input.allocation.pointer,s->input.file_bytes,room);if(e){s->frontier=0x394dd8;return e;}
 /* Actual main SNR2 reloc300/301 patch record003C2774+8/+C using
  * u16 symbols1185/1186, not the symbol byte assumed by old inventory code.
  * +4 is unpatched zero. Empty constructor list at3D80 is proven by hash. */
 /* R100 has a real PreInit export at1A0. Its constructor list at3400 is
  * zero-count/NULL; no static constructors are silently acknowledged. Native
  * providers attach later, before the original Scenario callsites. */
 s->hook4=(gh_scene_hook){.kind=room==0x100?GH_SCENE_HOOK_UNRESOLVED:GH_SCENE_HOOK_ABSENT};
 s->hook8=(gh_scene_hook){.kind=GH_SCENE_HOOK_UNRESOLVED};
 s->hook12=(gh_scene_hook){.kind=GH_SCENE_HOOK_UNRESOLVED};
 s->init_offset=room==0x100?0x208:0x1a0;s->move_offset=room==0x100?0x740:0x220;
 s->end_offset=room==0x100?0:0x248;s->bound=1;return GH_ROOM_MODULE_OK;
}
gh_room_module_status gh_room_module_release(gh_room_module*s){
 if(!s)return GH_ROOM_MODULE_ARGUMENT;
 gh_room_pack_status e=gh_game_room_pack_release(&s->input);if(e)return e==GH_PACK_BUSY?GH_ROOM_MODULE_BUSY:GH_ROOM_MODULE_DVD;
 s->bound=0;s->hook4=(gh_scene_hook){0};s->hook8=(gh_scene_hook){0};s->hook12=(gh_scene_hook){0};return GH_ROOM_MODULE_OK;
}
