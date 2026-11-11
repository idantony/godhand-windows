#include "game_event_audio.h"
#include "bgm_requests.h"

#include <stdio.h>
#include <string.h>

static uint32_t word(const uint8_t *p)
{
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}

static gh_game_event_status fail(gh_event_audio_channel1 *s,uint32_t pc,int detail,gh_game_event_status status)
{
    s->frontier=pc;
    s->detail=detail;
    return status;
}

gh_game_event_status gh_event_audio_channel1_bind(gh_event_audio_channel1 *s,
    gh_sound_control *control,gh_dvd_jobs *dvd,gh_heap *heap)
{
    if(!s||s->control||!control||!control->initialized||!dvd||!dvd->disc||!heap)return GH_EVENT_ARGUMENT;
    *s=(gh_event_audio_channel1){.control=control,.dvd=dvd,.heap=heap,.mode=-1,.event_no=-1};
    return GH_EVENT_OK;
}

static gh_game_event_status reset_channel(gh_event_audio_channel1 *s)
{
    if(s->flags&4){
        gh_audio_status status=gh_audio_event_afs_release(s->registry,s);
        if(status)return fail(s,0x322f58,status,GH_EVENT_CALLEE);
        s->flags&=~12u;
    }
    if(s->job && gh_dvd_check(s->dvd,s->job) && !gh_dvd_cancel(s->dvd,s->job))
        return fail(s,0x200f50,s->dvd->last_error,GH_EVENT_CALLEE);
    if(s->afd.pointer){
        gh_mem_error status=gh_heap_release(s->heap,s->afd);
        if(status)return fail(s,0x2a9680,status,GH_EVENT_CALLEE);
        s->afd=(gh_r207_allocation){0};
    }
    if(s->work.pointer){
        gh_mem_error status=gh_heap_release(s->heap,s->work);
        if(status)return fail(s,0x2a9680,status,GH_EVENT_CALLEE);
    }
    *s=(gh_event_audio_channel1){.control=s->control,.dvd=s->dvd,.heap=s->heap,
        .mode=-1,.event_no=-1,.registry=s->registry,
        .prefetch=s->prefetch,.prefetch_context=s->prefetch_context,.requests=s->requests};
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_audio_channel1_bind_registry(gh_event_audio_channel1 *s,gh_audio_registry *registry)
{
    if(!s||!s->control||!registry||s->registry||s->control->room_heaps[0].registry!=registry)
        return GH_EVENT_ARGUMENT;
    s->registry=registry;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_audio_channel1_bind_prefetch(gh_event_audio_channel1 *s,
    gh_event_audio_prefetch_provider provider,void *context)
{
    if(!s||!s->registry||s->prefetch||!provider)return GH_EVENT_ARGUMENT;
    s->prefetch=provider;
    s->prefetch_context=context;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_audio_channel1_bind_lifecycle(gh_event_audio_channel1 *s,gh_bgm_requests *requests)
{
    if(!s||s->requests||!gh_bgm_requests_uses_owners(requests,s->registry,s->control))return GH_EVENT_ARGUMENT;
    s->requests=requests;
    return GH_EVENT_OK;
}

static gh_game_event_status prefetch_ev01(gh_event_audio_channel1 *s,
    uint32_t rows,uint32_t row_index,const uint8_t *row,uint32_t key,uint32_t *frontier)
{
    /* Only the verified single-row call can currently resume without a
     * per-row cursor when a provider reaches a later boundary. */
    if(s->event_no!=1 || rows!=1 || row_index || key!=UINT32_MAX || row[0x10]){
        *frontier=0x2d1c78;
        return GH_EVENT_UNSUPPORTED;
    }
    if(!s->prefetch){*frontier=0x2d22b0;return GH_EVENT_UNSUPPORTED;}
    gh_event_audio_prefetch_request request={.registry=s->registry,.archive_owner=s,.disc=s->dvd->disc,
        .archive_index=s->afs_index,.archive_id=0x81,.row_index=row_index};
    uint32_t member=(uint32_t)row[2]|(uint32_t)row[3]<<8;
    gh_audio_status status=gh_audio_event_afs_member(s->registry,s,member,
        &request.member_offset,&request.member_size);
    if(status){*frontier=0x32bb48;return GH_EVENT_CALLEE;}
    memcpy(request.afd_row,row,sizeof(request.afd_row));
    gh_game_event_status result=s->prefetch(s->prefetch_context,&request,frontier);
    if(result && !*frontier)*frontier=0x2d1c78;
    return result;
}

static gh_game_event_status prepare(gh_event_audio_channel1 *s,int32_t event_no)
{
    gh_game_event_status status=reset_channel(s);
    if(status)return status;
    s->mode=0x81;
    s->event_no=event_no;
    char path[32];
    snprintf(path,sizeof(path),"se/ev/ev%02x.afd",(unsigned)event_no);
    size_t index;
    gh_disc_entry entry;
    gh_disc_error disc=gh_disc_find(s->dvd->disc,path,&index);
    if(disc==GH_DISC_NOT_FOUND)return GH_EVENT_OK;
    if(disc || gh_disc_entry_at(s->dvd->disc,index,&entry) || entry.size>SIZE_MAX)
        return fail(s,0x2011f8,disc,GH_EVENT_BOUNDS);
    s->afd_bytes=(size_t)entry.size;
    s->job=gh_dvd_read_alloc_sized(s->dvd,path,&s->afd,s->heap,0,NULL,0,0);
    if(!s->job || !s->afd.pointer){
        int detail=s->dvd->last_error;
        status=reset_channel(s);
        return status?status:fail(s,0x200cb0,detail,GH_EVENT_CALLEE);
    }
    s->flags=1;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_audio_channel1_service(void *context,gh_event_audio_operation op,
    int32_t event_no,int32_t argument,int *ready,uint32_t *frontier)
{
    gh_event_audio_channel1 *s=context;
    if(!s||!s->control||!ready||!frontier)return GH_EVENT_ARGUMENT;
    *frontier=0;
    *ready=0;
    if(op==GH_EVENT_AUDIO_PREPARE_002D1900){
        gh_game_event_status status=prepare(s,event_no);
        if(status)*frontier=s->frontier;
        return status;
    }
    if(s->event_no!=event_no)return GH_EVENT_ARGUMENT;
    if(op==GH_EVENT_AUDIO_READY_002D19D8){
        *ready=s->flags==0||s->flags==15;
        return GH_EVENT_OK;
    }
    if(op==GH_EVENT_AUDIO_START_002D1A30){
        if(s->flags!=15)return GH_EVENT_OK;
        const uint8_t *afd=s->afd.pointer;
        uint32_t rows=word(afd),key=(uint32_t)argument;
        for(uint32_t i=0;i<rows;++i){
            const uint8_t *row=afd+s->row_offset+(size_t)i*32;
            if(row[0xb]==0x80)continue;
            if(row[0x11]==key){
                if(!s->requests){*frontier=0x2d1d00;return GH_EVENT_UNSUPPORTED;}
                int original;
                gh_bgm_status status=gh_bgm_event_start_002d1d00(s->requests,1,i,0,&original);
                if(status){
                    *frontier=gh_bgm_requests_frontier(s->requests);
                    if(!*frontier)*frontier=0x2d1d00;
                    return status==GH_BGM_UNSUPPORTED?GH_EVENT_UNSUPPORTED:GH_EVENT_CALLEE;
                }
                continue; /* CutSet ignores the original return from Start. */
            }
            if(row[0x12]?row[0x12]==key:row[0x11]==key+1u)
                return prefetch_ev01(s,rows,i,row,key,frontier);
        }
        return GH_EVENT_OK;
    }
    if(op==GH_EVENT_AUDIO_WAIT_002D1B70){
        if(s->flags==15){
            const uint8_t *afd=s->afd.pointer;
            uint32_t rows=word(afd);
            for(uint32_t i=0;i<rows;++i){
                const uint8_t *row=afd+s->row_offset+(size_t)i*32;
                if(row[0xb]!=0x80 && row[0x11]==(uint32_t)argument){
                    if(!s->requests){*frontier=0x2d1dd0;return GH_EVENT_UNSUPPORTED;}
                    int row_ready;
                    if(gh_bgm_event_wait_002d1dd0(s->requests,1,i,&row_ready)){
                        *frontier=0x2d1dd0;
                        return GH_EVENT_CALLEE;
                    }
                    if(!row_ready)return GH_EVENT_OK;
                }
            }
        }
        *ready=1;
        return GH_EVENT_OK;
    }
    return GH_EVENT_ARGUMENT;
}

gh_game_event_status gh_event_audio_channel1_tick_002d0068(gh_event_audio_channel1 *s)
{
    if(!s||!s->control)return GH_EVENT_ARGUMENT;
    s->frontier=0;
    if(!(s->flags&1)||s->flags==15)return GH_EVENT_OK;
    if(s->flags&4){
        uint64_t offset,size;
        gh_audio_status status=gh_audio_event_afs_member(s->registry,s,0,&offset,&size);
        if(status)return fail(s,0x323000,status,GH_EVENT_CALLEE);
        s->flags|=8;
        return GH_EVENT_OK;
    }
    if(!(s->flags&2)){
        if(s->dvd->failed_id==s->job)return fail(s,0x201098,s->dvd->last_error,GH_EVENT_CALLEE);
        if(gh_dvd_check(s->dvd,s->job))return GH_EVENT_OK;
        if(!s->afd.pointer||s->afd_bytes<16)return fail(s,0x2d0360,0,GH_EVENT_FORMAT);
        const uint8_t *afd=s->afd.pointer;
        uint32_t rows=word(afd),offset=word(afd+4);
        if(afd[12]||word(afd+8)||offset<16||offset>s->afd_bytes||rows>(s->afd_bytes-offset)/32)
            return fail(s,0x2d0360,0,GH_EVENT_FORMAT);
        s->row_offset=offset;
        s->flags|=2;
        s->register_pending=1;
    }
    if(!s->register_pending)return fail(s,0x2d03e8,0,GH_EVENT_UNSUPPORTED);
    if(!s->work.pointer){
        const uint8_t *afd=s->afd.pointer;
        size_t bytes=((size_t)word(afd)+0x8e)/2*4;
        gh_mem_error status=gh_heap_allocate(s->heap,bytes,16,&s->work);
        if(status)return fail(s,0x30ee08,status,GH_EVENT_CALLEE);
    }
    char path[32];
    snprintf(path,sizeof(path),"se/ev/ev%02x.afs",(unsigned)s->event_no);
    gh_disc_entry entry;
    gh_disc_error disc=gh_disc_find(s->dvd->disc,path,&s->afs_index);
    if(disc||gh_disc_entry_at(s->dvd->disc,s->afs_index,&entry)||entry.size>SIZE_MAX)
        return fail(s,0x1ff9c0,disc,GH_EVENT_BOUNDS);
    s->afs_bytes=(size_t)entry.size;
    if(!s->registry)return fail(s,0x322a68,0,GH_EVENT_UNSUPPORTED);
    gh_sound_span afd={s->afd.pointer,s->afd_bytes};
    gh_audio_status status=gh_audio_event_afs_register(s->registry,s,s->dvd->disc,path,afd);
    if(status)return fail(s,0x322a68,status,GH_EVENT_CALLEE);
    /* Native file I/O completes the directory in this tick. Preserve the
     * original separate 323000 polling transition on the next sound tick. */
    s->flags|=4;
    s->register_pending=0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_audio_channel1_shutdown(gh_event_audio_channel1 *s)
{
    if(!s)return GH_EVENT_ARGUMENT;
    if(!s->control)return GH_EVENT_OK;
    gh_game_event_status status=reset_channel(s);
    if(!status)*s=(gh_event_audio_channel1){0};
    return status;
}

gh_game_event_status gh_event_audio_channel1_data_end_002d1958(gh_event_audio_channel1 *s)
{
    if(!s||!s->control)return GH_EVENT_ARGUMENT;
    if(!s->requests)return fail(s,0x2d1958,0,GH_EVENT_UNSUPPORTED);
    if(!s->data_end_faded){
        gh_bgm_status status=gh_bgm_event_data_end_fade_002d1958(s->requests);
        if(status){
            uint32_t frontier=gh_bgm_requests_frontier(s->requests);
            return fail(s,frontier?frontier:0x2d1958,status,GH_EVENT_CALLEE);
        }
        s->data_end_faded=1;
    }
    return reset_channel(s);
}
