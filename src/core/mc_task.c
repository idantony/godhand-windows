/* Native source-port prefix of actual2AE8A8 and mcport Main/DispInit(mode2).
 * Full source ordering/import evidence: analysis/mc-task-semantics.json.
 * No binary translation/emulation and no successful empty game callback. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <string.h>
#include "mc_task.h"

static uint32_t work_word(const gh_boot_work *w,size_t offset)
{uint32_t n;memcpy(&n,(const unsigned char*)w+offset,4);return n;}
static void put_work_word(gh_boot_work *w,size_t offset,uint32_t n)
{memcpy((unsigned char*)w+offset,&n,4);}
static gh_mc_task_status frontier(gh_mc_task_state *s,gh_mc_task_status error,uint32_t address,const char *name)
{s->frontier_offset=address;s->frontier_name=name;return error;}
static int bindings_valid(const gh_mc_task_bindings *b)
{return b && b->dvd && b->module_heap && b->subscreen_heap && b->threads && b->card &&
    b->id_manager && b->id_runtime && b->fonts && b->packed && b->language && b->work && b->game_suspend_word;}
static gh_mc_task_status module_version(const unsigned char *bytes,size_t size)
{
    typedef NTSTATUS(WINAPI *open_fn)(BCRYPT_ALG_HANDLE*,LPCWSTR,LPCWSTR,ULONG);
    typedef NTSTATUS(WINAPI *hash_fn)(BCRYPT_ALG_HANDLE,PUCHAR,ULONG,PUCHAR,ULONG,PUCHAR,ULONG);
    typedef NTSTATUS(WINAPI *close_fn)(BCRYPT_ALG_HANDLE,ULONG);
    static const unsigned char expected[32]={0x52,0xb9,0x1c,0x32,0xa6,0xae,0x5c,0x9f,0x50,0xef,0x1a,0xb8,0x0b,0xd1,0x7a,0x9e,
        0x26,0x68,0xed,0xc5,0x73,0x70,0xbe,0xdf,0x8d,0x33,0x28,0x41,0x27,0xa8,0xa8,0x99};
    HMODULE dll;BCRYPT_ALG_HANDLE algorithm=0;unsigned char digest[32];NTSTATUS status;
    open_fn open;hash_fn hash;close_fn close;
    if(!bytes || size!=114375 || memcmp(bytes,"SNR2",4))return GH_MC_TASK_MODULE_VERSION;
    dll=LoadLibraryExW(L"bcrypt.dll",0,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!dll)return GH_MC_TASK_PLATFORM_ERROR;
    open=(open_fn)(void*)GetProcAddress(dll,"BCryptOpenAlgorithmProvider");
    hash=(hash_fn)(void*)GetProcAddress(dll,"BCryptHash");
    close=(close_fn)(void*)GetProcAddress(dll,"BCryptCloseAlgorithmProvider");
    if(!open || !hash || !close){FreeLibrary(dll);return GH_MC_TASK_PLATFORM_ERROR;}
    status=open(&algorithm,BCRYPT_SHA256_ALGORITHM,0,0);
    if(status>=0)status=hash(algorithm,0,0,(PUCHAR)(void*)bytes,(ULONG)size,digest,sizeof(digest));
    if(algorithm)(void)close(algorithm,0);FreeLibrary(dll);
    if(status<0)return GH_MC_TASK_PLATFORM_ERROR;
    return !memcmp(digest,expected,32)?GH_MC_TASK_OK:GH_MC_TASK_MODULE_VERSION;
}
static gh_id_error get(gh_mc_task_state *s,const gh_mc_task_bindings *b,int id,gh_id_entry **e)
{return gh_id_base_get_work_002aa410(b->id_runtime,&s->id_base,id,e,0);}
static gh_id_error display(gh_mc_task_state *s,const gh_mc_task_bindings *b,int id,int show)
{
    gh_id_entry *e;gh_id_error error=get(s,b,id,&e);if(error)return error;
    if(show==1)e->flags&=~UINT32_C(0x20000000);else e->flags|=UINT32_C(0x20000000);
    return gh_id_base_set_family_display(b->id_runtime,&s->id_base,id,show);
}
static gh_id_error number(gh_mc_task_state *s,const gh_mc_task_bindings *b,int id,int n)
{
    gh_id_entry *e;gh_id_error error=get(s,b,id,&e);if(error)return error;
    e->seed=(uint32_t)n;return gh_id_base_set_family_display(b->id_runtime,&s->id_base,id,1);
}
static gh_id_error add_y(gh_mc_task_state *s,const gh_mc_task_bindings *b,int id,int delta)
{
    gh_id_entry *e;int32_t old,sum;gh_id_error error=get(s,b,id,&e);if(error)return error;
    /* Actual current MC records use small integer positions. These additions
     * are exact on EE and host; reject an unverified fractional/inexact branch. */
    if(!(e->pos_y>=-16777216.0f && e->pos_y<=16777216.0f))return GH_ID_UNSUPPORTED;
    old=(int32_t)e->pos_y;if((float)old!=e->pos_y)return GH_ID_UNSUPPORTED;
    sum=old+delta;if(sum>16777216 || sum< -16777216)return GH_ID_UNSUPPORTED;
    e->pos_y=(float)sum;return GH_ID_OK;
}
static gh_id_error disp_init_boot(gh_mc_task_state *s,const gh_mc_task_bindings *b)
{
    gh_id_entry *e;gh_id_error error;int i;
#define ID_CALL(expr) do{error=(expr);if(error)return error;}while(0)
    ID_CALL(display(s,b,0xc,1));
    for(i=0;i<3;++i) {ID_CALL(number(s,b,0x14+i,i));ID_CALL(display(s,b,0x10+i,0));ID_CALL(display(s,b,0x60+i,0));}
    ID_CALL(display(s,b,0xd,1));ID_CALL(display(s,b,5,0));
    ID_CALL(gh_id_base_restart_anim(&s->id_base,0,0));
    ID_CALL(display(s,b,0xc,0));ID_CALL(display(s,b,0xd,0));ID_CALL(display(s,b,0xe,0));
    ID_CALL(display(s,b,0xa,0));ID_CALL(display(s,b,0xb,0));
    ID_CALL(get(s,b,0x40,&e));e->scale_x=1.0f;e->scale_y=1.4f;
    ID_CALL(add_y(s,b,0x40,100));ID_CALL(add_y(s,b,0x41,80));
    ID_CALL(add_y(s,b,0x51,125));ID_CALL(add_y(s,b,0x52,125));
    ID_CALL(add_y(s,b,0x55,125));ID_CALL(add_y(s,b,0x56,125));
#undef ID_CALL
    return GH_ID_OK;
}
gh_mc_task_status gh_mc_task_prepare_002ae8a8(gh_mc_task_state *s,const gh_mc_task_bindings *b)
{
    gh_disc_entry entry;size_t index;gh_mc_task_status version;int original_return;
    if(!s || !bindings_valid(b))return GH_MC_TASK_ARGUMENT;
    if(s->stage || s->module.pointer || s->checks.icon_work.pointer)return GH_MC_TASK_BUSY;
    s->stage=1;put_work_word(b->work,0x5e8,work_word(b->work,0x5e8)|UINT32_C(0x80000000));
    if(work_word(b->work,0x590)&0x400)return frontier(s,GH_MC_TASK_UNSUPPORTED,0x2ae4a8,"debug host0 module path");
    /* Original logical dll/Release/McPort.rel resolves to this sole actual FST
     * payload on the audited disc. Exact SHA256 prevents other versions running
     * a mismatched native reconstruction. No unknown binary imports are called. */
    s->module_job=gh_dvd_read_alloc(b->dvd,"dll/debugoptimize/mcport.rel",&s->module,b->module_heap,0,0,0,0);
    if(!s->module_job)return frontier(s,GH_MC_TASK_DISC_ERROR,0x2ae4a8,"mcport module allocation/read");
    while(gh_dvd_check(b->dvd,s->module_job))if(gh_task_threads_sleep(b->threads,1)!=GH_THREAD_OK)
        return frontier(s,GH_MC_TASK_PLATFORM_ERROR,0x2d5aa0,"module wait task sleep");
    if(b->dvd->failed_id==s->module_job && b->dvd->last_error==GH_DVD_IO)
        return frontier(s,GH_MC_TASK_DISC_ERROR,0x2ae4a8,"mcport module I/O");
    if(gh_disc_find(b->dvd->disc,"dll/debugoptimize/mcport.rel",&index)!=GH_DISC_OK ||
       gh_disc_entry_at(b->dvd->disc,index,&entry)!=GH_DISC_OK)return GH_MC_TASK_DISC_ERROR;
    version=module_version(s->module.pointer,(size_t)entry.size);
    if(version!=GH_MC_TASK_OK)return frontier(s,version,0x394dd8,"native mcport version binding");
    /* Required McPort service was truly created by native card backend. This
     * source module does not pretend SIF initialization returned success. */
    s->module_loaded=1;s->stage=2;
    s->flags=0;s->zero_bc=0;s->zero_c0=0;s->zero_c8=0;s->zero_cc=0;s->zero_d0=0;
    s->system_phase_dc=b->work->phase569;s->progress_e0=0;s->progress_e4=0;
    b->work->flags|=0x4000;s->mc_sys_initialized=1;
    if(*b->game_suspend_word==0){*b->game_suspend_word=1;s->flags|=0x08000000;}
    s->flags|=UINT32_C(0x81000000); /* cMcSys.Initialize + cMcBoot.Initialize */
    if(gh_mc_boot_checks_create(&s->checks,b->subscreen_heap)!=GH_MC_BOOT_OK)
        return frontier(s,GH_MC_TASK_MEMORY_ERROR,0x180,"McBoot workInit icon allocation");
    s->stage=3;
    if(gh_mc_boot_load_id_resource(&s->id_resource,b->dvd,b->subscreen_heap,b->threads,
        b->id_manager,-1,(int8_t)b->work->language)!=GH_MC_BOOT_OK)
        return frontier(s,GH_MC_TASK_ID_ERROR,0x82f0,"SetIdData idsavecore resource");
    s->id_error=gh_id_base_initialize_002a9ea0(b->id_runtime,&s->id_base,0xb,0,0,0,&original_return);
    if(s->id_error!=GH_ID_OK || original_return!=1)
        return frontier(s,GH_MC_TASK_ID_ERROR,s->id_base.first_unported?s->id_base.first_unported:0x2a9ea0,"cIDBase initialize");
    s->id_error=gh_packed_message_bind(&s->id_base,b->id_manager,b->fonts,b->packed,b->language,
        b->work->language,0xb,1,&original_return);
    if(s->id_error!=GH_ID_OK || original_return!=1)return frontier(s,GH_MC_TASK_ID_ERROR,0x2a9f30,"packed message initialize");
    s->flags|=UINT32_C(0x40000000);
    s->id_error=disp_init_boot(s,b);
    if(s->id_error!=GH_ID_OK)return frontier(s,GH_MC_TASK_ID_ERROR,0x8468,"DispInit boot UI fields");
    s->checks.frame=0;s->checks.phase=0;s->checks.subphase=0;s->ui_initialized=1;s->stage=4;
    return frontier(s,GH_MC_TASK_CONTROL_FRONTIER,0x2e8,"cMcBoot Control");
}
gh_mc_task_status gh_mc_task_abort(gh_mc_task_state *s,const gh_mc_task_bindings *b)
{
    int original,reclaimed;
    if(!s || !bindings_valid(b))return GH_MC_TASK_ARGUMENT;
    if(s->module_job && gh_dvd_check(b->dvd,s->module_job))(void)gh_dvd_cancel(b->dvd,s->module_job);
    if(s->id_resource.job_id && gh_dvd_check(b->dvd,s->id_resource.job_id))(void)gh_dvd_cancel(b->dvd,s->id_resource.job_id);
    if(s->id_base.entries && gh_id_base_release_002aa078(b->id_runtime,&s->id_base,&original,&reclaimed)!=GH_ID_OK)return GH_MC_TASK_ID_ERROR;
    if(gh_mc_boot_release_id_resource(&s->id_resource,b->subscreen_heap,b->id_manager)!=GH_MC_BOOT_OK)return GH_MC_TASK_MEMORY_ERROR;
    if(gh_mc_boot_checks_release(&s->checks,b->subscreen_heap)!=GH_MC_BOOT_OK)return GH_MC_TASK_MEMORY_ERROR;
    if(s->module.pointer && gh_heap_release(b->module_heap,s->module)!=GH_MEM_OK)return GH_MC_TASK_MEMORY_ERROR;
    memset(&s->module,0,sizeof(s->module));s->module_loaded=0;s->ui_initialized=0;
    if(s->mc_sys_initialized){if(s->flags&0x08000000)*b->game_suspend_word=0;b->work->flags&=~UINT32_C(0x4000);}
    put_work_word(b->work,0x5e8,work_word(b->work,0x5e8)&~UINT32_C(0x80000000));
    s->stage=5;return GH_MC_TASK_OK;
}
