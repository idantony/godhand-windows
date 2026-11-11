/* Actual mcport.rel fileLoad730 / GetAllSystemFlags8FF0 / GetSystemFlags9088.
 * Native slice scope and function hashes: analysis/mc-file-load-semantics.json. */
#include "mc_file_load.h"
#include <string.h>
#include <math.h>
static uint32_t word(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t half(const unsigned char *p){return (uint16_t)((unsigned)p[0]|(unsigned)p[1]<<8);}
static uint32_t unlock_bits(uint32_t flags,uint32_t f,uint16_t n)
{
    if(n || (f&1))flags|=0x80000000;
    if(f&2)flags|=0x40000000;if(f&4)flags|=0x20000000;
    if(f&0x200)flags|=0x200000;
    if(f&8)flags|=0x10000000;if(f&0x10)flags|=0x08000000;
    if(f&0x20)flags|=0x04000000;if(f&0x40)flags|=0x02000000;
    if(f&0x80)flags|=0x01000000;if(f&0x100)flags|=0x00400000;
    if(n>98)flags|=0xbf600000;
    return flags;
}
static gh_mc_boot_error platform(gh_mc_error e,uint32_t pc,uint32_t *first)
{
    if(!e)return GH_MC_BOOT_OK;
    *first=pc;
    return e==GH_MC_UNSUPPORTED?GH_MC_BOOT_UNSUPPORTED:GH_MC_BOOT_IO;
}
static void file_error(gh_mc_boot_checks *s,int32_t error)
{
    s->error=error;s->phase=3;s->frame=0;s->subphase=0;
}
/* Shared 0x400-byte open/read/close transport. Policy for original
 * negative card results belongs to the invoking boot or title mode. */
static gh_mc_boot_error poll_header(gh_memory_card *card,uint8_t *record,uint32_t *phase,
    gh_mc_result *result,int *done,uint32_t *first)
{
    gh_mc_boot_error status=platform(gh_memory_card_poll(card,done,result),0x31c8,first);
    if(status||!*done||result->result<0)return status;
    if(*phase==1){
        status=platform(gh_memory_card_request_save_header(card,record),0x35e0,first);
        if(!status)*phase=2;
    }else if(*phase==2){
        status=platform(gh_memory_card_request_save_close(card),0x3300,first);
        if(!status)*phase=3;
    }
    return status;
}

gh_mc_boot_error gh_mc_file_load_00000730(gh_mc_boot_checks *s,gh_memory_card *card,gh_boot_work *work,
    uint8_t original_f8,uint32_t *first)
{
    uint32_t flags;unsigned i;unsigned char *records;
    if(!s || !work || !first || s->phase!=2 || !s->icon_work.pointer)return GH_MC_BOOT_ARGUMENT;
    *first=0;
    if(s->load_index<0){*first=0x730;return GH_MC_BOOT_UNSUPPORTED;}
    records=s->icon_work.pointer;
    if(s->subphase){
        if(!card||s->subphase>3||s->load_index>=20){*first=0x31c8;return GH_MC_BOOT_UNSUPPORTED;}
        gh_mc_result result;
        int completed;
        uint32_t phase=s->subphase;
        uint8_t *record=records+(unsigned)s->load_index*GH_MC_SAVE_HEADER_BYTES;
        gh_mc_boot_error e=poll_header(card,record,&s->subphase,&result,&completed,first);
        if(e||!completed)return e;
        if(phase<3){
            if(result.result<0)
                file_error(s,phase==1&&result.result==GH_MC_SAVE_NOT_FOUND?2:1);
            return GH_MC_BOOT_OK;
        }
        if(result.result){file_error(s,result.result==GH_MC_SAVE_NOT_FOUND?2:1);return GH_MC_BOOT_OK;}
        /* After CloseCheck@FC8 the original compares the binary32 version to .1f. */
        if(word(record)!=UINT32_C(0x3dcccccd))s->present_mask&=~(UINT32_C(1)<<(unsigned)s->load_index);
        ++s->load_index;
        s->subphase=0;
    }
    while(s->load_index<20){
        if(s->present_mask&(UINT32_C(1)<<(unsigned)s->load_index)){
            if(!card){*first=0x3350;return GH_MC_BOOT_UNSUPPORTED;}
            gh_mc_boot_error e=platform(gh_memory_card_request_save_open(card,(unsigned)s->load_index),0x3350,first);
            if(!e)s->subphase=1;
            return e;
        }
        ++s->load_index;
    }
    memcpy(&flags,work->pad588,4);
    for(i=0;i<20;++i){
        const unsigned char *p=records+i*GH_MC_SAVE_HEADER_BYTES;uint32_t f=word(p+8);uint16_t n=half(p+0x30);
        flags=unlock_bits(flags,f,n);
    }
    memcpy(work->pad588,&flags,4);
    if(s->present_mask)work->flags|=4;
    if(original_f8){s->done=1;return GH_MC_BOOT_OK;}
    s->error=s->present_mask?10:2;s->phase=3;s->frame=0;s->subphase=0;
    return GH_MC_BOOT_OK;
}

static uint64_t dword(const uint8_t *p)
{
    return (uint64_t)word(p)|(uint64_t)word(p+4)<<32;
}

uint64_t gh_mc_save_checksum_00008c08(const uint8_t *p)
{
    uint64_t sum=UINT64_C(0x9be6fa3b72afda1d);
    for(size_t i=0;i<0xb108;i+=8)sum^=dword(p+i);
    return sum;
}

enum { LOAD_OPEN=1, LOAD_READ_REQUEST, LOAD_READ, LOAD_CLOSE_REQUEST,
       LOAD_CLOSE, LOAD_TERMINAL };

static gh_mc_load_status finish(gh_mc_selected_load *s,gh_mc_load_status result,uint32_t frontier)
{
    s->phase=LOAD_TERMINAL;s->result=result;s->frontier=frontier;
    return result;
}

static uint8_t *load_record(gh_mc_selected_load *s)
{return s->borrowed_record?s->borrowed_record:s->record;}

gh_mc_load_status gh_mc_selected_load_begin_using(gh_mc_selected_load *s,gh_memory_card *card,
    unsigned slot,uint8_t *record,size_t bytes)
{
    if(!s||!card||(record&&bytes<GH_MC_SAVE_RECORD_BYTES)||(s->phase&&s->phase!=LOAD_TERMINAL))return GH_MC_LOAD_ARGUMENT;
    gh_mc_error e=gh_memory_card_request_save_open(card,slot);
    if(e)return e==GH_MC_ARGUMENT?GH_MC_LOAD_ARGUMENT:
        e==GH_MC_BUSY?GH_MC_LOAD_BUSY:GH_MC_LOAD_IO;
    s->borrowed_record=record;s->card=card;s->phase=LOAD_OPEN;s->applied=0;s->frontier=0;s->result=GH_MC_LOAD_PENDING;
    return GH_MC_LOAD_PENDING;
}

gh_mc_load_status gh_mc_selected_load_begin(gh_mc_selected_load *s,gh_memory_card *card,unsigned slot)
{return gh_mc_selected_load_begin_using(s,card,slot,NULL,0);}

gh_mc_load_status gh_mc_selected_load_step_wait_close(gh_mc_selected_load *s,int allow_close,uint32_t *frontier)
{
    if(!s||!frontier||!s->card||!s->phase)return GH_MC_LOAD_ARGUMENT;
    *frontier=0;
    if(s->phase==LOAD_TERMINAL){*frontier=s->frontier;return s->result;}
    gh_mc_result r;
    int done;
    gh_mc_error e;
    switch(s->phase){
    case LOAD_OPEN:
        e=gh_memory_card_poll(s->card,&done,&r);
        if(e){*frontier=0x2e80;return finish(s,GH_MC_LOAD_IO,*frontier);}
        if(!done)return GH_MC_LOAD_PENDING;
        if(r.result<0){*frontier=0x2e80;return finish(s,r.result==GH_MC_SAVE_NOT_FOUND?
            GH_MC_LOAD_NOT_FOUND:GH_MC_LOAD_IO,*frontier);}
        s->phase=LOAD_READ_REQUEST;
        /* fall through */
    case LOAD_READ_REQUEST:
        e=gh_memory_card_request_save_record(s->card,load_record(s));
        if(e){*frontier=0x35e0;return GH_MC_LOAD_IO;}
        s->phase=LOAD_READ;
        return GH_MC_LOAD_PENDING;
    case LOAD_READ:
        e=gh_memory_card_poll(s->card,&done,&r);
        if(!e&&!done)return GH_MC_LOAD_PENDING;
        if(e||r.result<0)s->result=GH_MC_LOAD_IO;
        else if(r.result!=GH_MC_SAVE_RECORD_BYTES)s->result=GH_MC_LOAD_SHORT;
        s->phase=LOAD_CLOSE_REQUEST;
        /* fall through */
    case LOAD_CLOSE_REQUEST:
        e=gh_memory_card_request_save_close(s->card);
        if(e){*frontier=0x3300;return GH_MC_LOAD_IO;}
        s->phase=LOAD_CLOSE;
        return GH_MC_LOAD_PENDING;
    case LOAD_CLOSE:
        if(!allow_close)return GH_MC_LOAD_PENDING;
        e=gh_memory_card_poll(s->card,&done,&r);
        if(e){*frontier=0x2ef8;s->phase=LOAD_CLOSE_REQUEST;return GH_MC_LOAD_IO;}
        if(!done)return GH_MC_LOAD_PENDING;
        if(r.result){*frontier=0x2ef8;return finish(s,GH_MC_LOAD_IO,*frontier);}
        if(s->result!=GH_MC_LOAD_PENDING){*frontier=0x2f50;return finish(s,s->result,*frontier);}
        if(gh_mc_save_checksum_00008c08(load_record(s))!=dword(load_record(s)+0xb110)){
            *frontier=0x8c08;return finish(s,GH_MC_LOAD_CHECKSUM,*frontier);
        }
        if(word(load_record(s))!=UINT32_C(0x3dcccccd)){
            *frontier=0x2918;return finish(s,GH_MC_LOAD_VERSION,*frontier);
        }
        return finish(s,GH_MC_LOAD_VERIFIED,*frontier);
    default:
        return GH_MC_LOAD_ARGUMENT;
    }
}

gh_mc_load_status gh_mc_selected_load_step(gh_mc_selected_load *s,uint32_t *frontier)
{return gh_mc_selected_load_step_wait_close(s,1,frontier);}

gh_mc_error gh_mc_selected_load_abort_io(gh_mc_selected_load *s)
{
    if(!s)return GH_MC_ARGUMENT;
    if(!s->phase||s->phase==LOAD_TERMINAL){
        s->phase=0;s->card=NULL;s->borrowed_record=NULL;
        return GH_MC_OK;
    }
    if(!s->card)return GH_MC_ARGUMENT;
    int done;
    gh_mc_result result;
    gh_mc_error error;
    if(s->phase==LOAD_OPEN||s->phase==LOAD_READ||s->phase==LOAD_CLOSE){
        error=gh_memory_card_poll(s->card,&done,&result);
        if(!error&&!done)return GH_MC_BUSY;
        if(s->phase==LOAD_OPEN&&(error||result.result<0))goto clear;
        if(s->phase==LOAD_CLOSE&&!error&&!result.result)goto clear;
        s->phase=LOAD_CLOSE_REQUEST;
    }
    error=gh_memory_card_request_save_close(s->card);
    if(error)return error;
    s->phase=LOAD_CLOSE;
    error=gh_memory_card_poll(s->card,&done,&result);
    if(!error&&!done)return GH_MC_BUSY;
    if(error||result.result){
        s->phase=LOAD_CLOSE_REQUEST;
        return error?error:GH_MC_IO;
    }
clear:
    s->phase=0;
    s->card=NULL;
    s->borrowed_record=NULL;
    return GH_MC_OK;
}

gh_mc_load_status gh_mc_selected_load_restore_00002c88(gh_mc_selected_load *s,
    gh_game_checkpoint *checkpoint,const gh_checkpoint_bindings *b,gh_game_door *door,
    gh_mc_output_mode_apply output_mode,void *audio_context,uint32_t *frontier)
{
    if(!frontier)return GH_MC_LOAD_ARGUMENT;
    *frontier=0;
    if(!s||s->phase!=LOAD_TERMINAL||s->result!=GH_MC_LOAD_VERIFIED||!checkpoint||
       !b||!b->work||!b->save.bytes||b->save.size<GH_CORE_SAVE_BYTES||
       !b->prepare||!b->destination||b->destination_size<GH_CHECKPOINT_BYTES)
        return GH_MC_LOAD_ARGUMENT;
    if(!b->player||!b->player->bytes||b->player->size<0x49c||
       b->player->alias_f0!=b->player->bytes+0xb0){
        *frontier=checkpoint->frontier=0x31c438;return GH_MC_LOAD_UNSUPPORTED;
    }
    _Static_assert(GH_CORE_SAVE_BYTES+0x9c30+sizeof(gh_checkpoint_tail32)==GH_CHECKPOINT_BYTES,
                   "original checkpoint payload");
    if(!s->applied){
        memcpy(b->destination,load_record(s)+0x20,GH_CHECKPOINT_BYTES);
        b->work->flags|=2;
        uint32_t flags;
        memcpy(&flags,b->work->pad588,4);
        flags=unlock_bits(flags,word(load_record(s)+8),half(load_record(s)+0x30));
        memcpy(b->work->pad588,&flags,4);

        /* getCheckPoint(mode1) imports these into the same owners used by capture. */
        memcpy(b->save.bytes,b->destination,GH_CORE_SAVE_BYTES);
        memcpy(b->prepare->fields+0x1660,b->destination+0x14a0,0x9c30);
        memcpy(&checkpoint->tail,b->destination+0xb0d0,sizeof(checkpoint->tail));
        memcpy(b->player->alias_f0,checkpoint->tail.position,12);
        memcpy(b->player->bytes+0x490,checkpoint->tail.position,12);
        memcpy(b->player->bytes+0x100,checkpoint->tail.rotation,12);
        s->applied=1;
    }

    if(s->applied==1){
        if(!b->init){
            *frontier=checkpoint->frontier=0x2a9bf0;
            return GH_MC_LOAD_UNSUPPORTED;
        }
        float current,saved;
        memcpy(&current,b->init->heat+4,4);
        memcpy(&saved,&checkpoint->tail.heat,4);
        if(!isfinite(current)||!isfinite(saved)){
            *frontier=checkpoint->frontier=0x2a9bf0;
            return GH_MC_LOAD_UNSUPPORTED;
        }
        /* 2A9BF0 computes max from CoreSave, ignoring the saved max word. */
        uint8_t *core=b->save.bytes;
        if(b->work->flags598&UINT32_C(0x08000000))core[0x155]=5;
        float maximum=(float)(120u+30u*core[0x155]);
        memcpy(b->init->heat,&maximum,4);
        if(maximum<current){
            current=maximum;
            memcpy(b->init->heat+4,&current,4);
        }
        volatile float ratio=current/maximum;
        uint32_t rank=ratio<0.5f?0u:ratio<=0.75f?1u:2u;
        memcpy(b->init->heat+0x14,&rank,4);

        /* 2A9C58 applies the saved current and refreshes the rank again. */
        if(maximum<saved)saved=maximum;
        memcpy(b->init->heat+4,&saved,4);
        ratio=saved/maximum;
        rank=ratio<0.5f?0u:ratio<=0.75f?1u:2u;
        memcpy(b->init->heat+0x14,&rank,4);

        /* Main2C06D0 pose mirrors, then the first writes of 1F9CB0(mode1). */
        uint8_t *work=(uint8_t*)b->work;
        memcpy(work+0x5c0,b->player->alias_f0,12);
        memcpy(work+0x5d0,b->player->bytes+0x104,4);
        memcpy(work+0x5b4,core+0xc,4);
        s->applied=2;
    }
    if(s->applied==2){
        if(!door||!b->init||b->player->size<0x54c){
            *frontier=checkpoint->frontier=0x1f9f50;
            return GH_MC_LOAD_UNSUPPORTED;
        }
        const uint8_t *core=b->save.bytes;
        /* 1F9F50 builds the 24-byte stack record copied by 2009B0. */
        memcpy(door->backup,core+0x15c,16);
        memcpy(door->backup+16,core+0x15a,2);
        memcpy(door->backup+18,core+0x158,2);
        memcpy(door->backup+20,core+0x16c,4);

        b->init->world_time=word(core);
        b->prepare->global_0061a994=word(core+4);

        /* The original stores max as a halfword before signed comparison. */
        uint16_t maximum_bits=(uint16_t)word(core+0x80);
        int32_t maximum=(maximum_bits&0x8000u)?(int32_t)maximum_bits-0x10000:maximum_bits;
        uint32_t current_bits=word(core+0x84);
        int64_t current=(current_bits&UINT32_C(0x80000000))?
            (int64_t)current_bits-INT64_C(0x100000000):current_bits;
        if(current<1)current=1;
        if(current>maximum)current=maximum;
        uint16_t vitality=(uint16_t)current;
        memcpy(b->player->bytes+0x548,&maximum_bits,2);
        memcpy(b->player->bytes+0x54a,&vitality,2);
        b->init->world_active_005e85f8=(uint8_t)(word(core+0x174)!=0);
        s->applied=3;
    }
    if(s->applied==3){
        if(!output_mode||!b->init){
            *frontier=checkpoint->frontier=0x375700;
            return GH_MC_LOAD_UNSUPPORTED;
        }
        uint32_t audio_frontier=0;
        gh_mc_load_status status=output_mode(audio_context,word(b->save.bytes+0x170),
                                             &audio_frontier);
        if(status!=GH_MC_LOAD_RESTORED){
            *frontier=checkpoint->frontier=audio_frontier?audio_frontier:0x375700;
            return status;
        }
        uint8_t *work=(uint8_t*)b->work;
        memcpy(work+0x56c,b->save.bytes+0x178,4); /* 1F9EB0 */
        gh_game_cheat_state *cheat=&b->init->cheat_00568240;
        cheat->flags00=0;
        cheat->count08=0;
        memset(cheat->values0c,0,sizeof(cheat->values0c)); /* 1F6CF8 */
        s->applied=4;
    }
    checkpoint->frontier=0;
    return GH_MC_LOAD_RESTORED;
}

gh_mc_load_status gh_mc_selected_load_apply_with_door_00002c88(gh_mc_selected_load *s,
    gh_game_checkpoint *checkpoint,const gh_checkpoint_bindings *b,gh_game_door *door,
    uint32_t *frontier)
{
    return gh_mc_selected_load_restore_00002c88(s,checkpoint,b,door,NULL,NULL,frontier);
}

gh_mc_load_status gh_mc_selected_load_apply_00002c88(gh_mc_selected_load *s,
    gh_game_checkpoint *checkpoint,const gh_checkpoint_bindings *b,uint32_t *frontier)
{
    return gh_mc_selected_load_apply_with_door_00002c88(s,checkpoint,b,NULL,frontier);
}
static void title_phase(gh_mc_boot_checks *checks,uint32_t phase)
{
    checks->phase=phase;
    checks->subphase=0;
    checks->frame=0;
}

static void title_empty(gh_mc_boot_checks *checks,gh_mc_load_menu_memory *memory)
{
    checks->error=8;
    memory->first_slot=memory->selected_row=-1;
    title_phase(checks,7);
}

static gh_mc_boot_error title_request(gh_mc_boot_checks *checks,gh_mc_load_menu_memory *memory,
    gh_mc_error error,uint32_t *frontier,uint32_t address)
{
    checks->error=-1;
    if(!error)return GH_MC_BOOT_OK;
    checks->error=9;
    memory->first_slot=memory->selected_row=-1;
    title_phase(checks,7);
    return platform(error,address,frontier);
}

static gh_mc_boot_error title_slot(gh_mc_boot_checks *checks,gh_memory_card *card,
    gh_mc_load_menu_memory *memory,int32_t *free_clusters,uint32_t *frontier)
{
    if(checks->subphase<=1){
        if(!checks->subphase){
            gh_mc_boot_reset_slot_records(checks);
            checks->retry=0;
        }
        gh_mc_boot_error error=title_request(checks,memory,gh_memory_card_request_info(card),
                                           frontier,0x37f0);
        if(!error)checks->subphase=2;
        return error;
    }
    if(checks->subphase!=2)return GH_MC_BOOT_ARGUMENT;
    gh_mc_result result;
    int done;
    gh_mc_boot_error error=platform(gh_memory_card_poll(card,&done,&result),0x31c8,frontier);
    if(error||!done)return error;
    checks->error=-1;
    if(free_clusters){
        if(result.result||result.card_type!=2||!result.formatted){
            *frontier=0x40c0; /* Original no-card, unformatted and retry prompts are distinct. */
            return GH_MC_BOOT_UNSUPPORTED;
        }
        *free_clusters=result.free_clusters;
    }
    if(result.result==-2){
        if(++checks->retry<=2)checks->subphase=1;
        else checks->error=8; /* original waits in subphase 2 */
    }else if(result.result!=0&&result.result!=-1){
        checks->error=1;
        memory->first_slot=memory->selected_row=-1;
        title_phase(checks,7);
    }else if(result.card_type!=2){
        checks->error=1;
        title_phase(checks,7);
    }else if(!result.formatted){
        checks->error=8;
        title_phase(checks,7);
    }else{
        title_phase(checks,1);
    }
    return GH_MC_BOOT_OK;
}

static gh_mc_boot_error title_files(gh_mc_boot_checks *checks,gh_memory_card *card,
    gh_mc_load_menu_memory *memory,const int32_t *free_clusters,uint32_t *frontier)
{
    while(checks->file_index<20){
        unsigned slot=(unsigned)checks->file_index;
        if(!checks->subphase){
            gh_mc_boot_error status=title_request(checks,memory,
                gh_memory_card_request_slot_directory(card,slot),frontier,0x3488);
            if(status)return status;
            checks->subphase=1;
        }
        gh_mc_result result;
        int done;
        gh_mc_boot_error error=platform(gh_memory_card_poll(card,&done,&result),0x31c8,frontier);
        if(error||!done)return error;
        checks->error=-1;
        if(result.result<0&&result.result!=-4&&result.result!=-6){
            if(free_clusters){*frontier=0x42e8;return GH_MC_BOOT_UNSUPPORTED;}
            checks->error=result.result==-2?8:1;
            title_phase(checks,7);
            return GH_MC_BOOT_OK;
        }
        if(checks->subphase==1&&result.result>0){
            error=title_request(checks,memory,gh_memory_card_request_icon(card,slot),
                                frontier,0x3528);
            if(!error)checks->subphase=2;
            return error;
        }
        if(checks->subphase==2&&result.result>0&&result.file_bytes==0x3c4)
            checks->present_mask|=UINT32_C(1)<<slot;
        ++checks->file_index;
        checks->subphase=0;
    }
    if(!checks->present_mask&&!free_clusters)title_empty(checks,memory);
    else if(!checks->present_mask&&*free_clusters<150){
        *frontier=0x42e8;
        return GH_MC_BOOT_UNSUPPORTED;
    }else title_phase(checks,2);
    return GH_MC_BOOT_OK;
}

static unsigned newest_title_slot(const uint8_t *records,uint32_t mask)
{
    static const unsigned rtc_offsets[]={0x13,0x14,0x15,0x17,0x18,0x19};
    unsigned newest=0;
    uint64_t best=0;
    for(unsigned slot=0;slot<20;++slot){
        if(!(mask&(UINT32_C(1)<<slot)))continue;
        const uint8_t *record=records+slot*GH_MC_SAVE_HEADER_BYTES;
        uint64_t time=0;
        for(unsigned i=0;i<6;++i)time|=(uint64_t)record[rtc_offsets[i]]<<(i*8);
        if(time>best){best=time;newest=slot;}
    }
    return newest;
}

static gh_mc_boot_error title_headers(gh_mc_load_scan *scan,gh_mc_boot_checks *checks,
    gh_memory_card *card,gh_mc_load_menu_memory *memory,int save,uint32_t *frontier)
{
    uint8_t *records=checks->icon_work.pointer;
    if(checks->subphase){
        unsigned slot=(unsigned)checks->load_index;
        uint8_t *record=records+slot*GH_MC_SAVE_HEADER_BYTES;
        uint32_t phase=checks->subphase;
        gh_mc_result result;
        int done;
        gh_mc_boot_error status=poll_header(card,record,&checks->subphase,&result,&done,frontier);
        if(status||!done)return status;
        checks->error=-1;
        if(phase==1&&(result.result==-4||result.result==-5||result.result==-7)){
            checks->error=5;
            checks->present_mask&=~(UINT32_C(1)<<slot);
        }else if(result.result==-2){
            checks->error=8;
            title_phase(checks,0);
            return GH_MC_BOOT_OK;
        }else if(result.result<0||(phase==3&&result.result)){
            if(save){*frontier=0x46d0;return GH_MC_BOOT_UNSUPPORTED;}
            if(phase==1)checks->error=result.result==-3?2:1;
            if(phase==2)checks->error=result.result>=-5&&result.result<=-3?5:1;
            if(phase==3)checks->error=result.result<=-4?5:1;
            title_phase(checks,7);
            return GH_MC_BOOT_OK;
        }else if(phase<3){
            return GH_MC_BOOT_OK;
        }else if(!save&&word(record)!=UINT32_C(0x3dcccccd)){
            memset(record,0,GH_MC_SAVE_HEADER_BYTES);
            memset(record+4,0xff,4);
            checks->present_mask&=~(UINT32_C(1)<<slot);
        }
        ++checks->load_index;
        checks->subphase=0;
    }
    while(checks->load_index<20){
        unsigned slot=(unsigned)checks->load_index;
        if(checks->present_mask&(UINT32_C(1)<<slot)){
            gh_mc_boot_error error=title_request(checks,memory,
                gh_memory_card_request_save_open(card,slot),frontier,0x3350);
            if(!error)checks->subphase=1;
            return error;
        }
        ++checks->load_index;
    }
    if(!checks->present_mask&&!save){
        title_empty(checks,memory);
        return GH_MC_BOOT_OK;
    }
    int32_t first=memory->first_slot,row=memory->selected_row;
    if(first==-1){
        first=(int32_t)((newest_title_slot(records,checks->present_mask)+19u)%20u);
        row=1;
    }
    if(first==19){first=0;row=0;}
    else if(first>=18){row=first-16;first=17;}
    scan->first_slot=first;
    scan->selected_row=row;
    scan->menu_ready=1;
    title_phase(checks,save?6:3);
    return GH_MC_BOOT_OK;
}

gh_mc_boot_error gh_mc_title_load_scan_00001590(gh_mc_load_scan *scan,gh_mc_boot_checks *checks,
    gh_memory_card *card,gh_mc_load_menu_memory *memory,uint32_t *frontier)
{
    if(!scan||!checks||!card||!memory||!frontier||!checks->icon_work.pointer||
       checks->mode!=1||checks->file_index<0||checks->file_index>20||
       checks->load_index<0||checks->load_index>20||checks->subphase>3||
       memory->first_slot < -1||memory->first_slot>19||
       (memory->first_slot>=0&&(memory->selected_row<0||memory->selected_row>2)))
        return GH_MC_BOOT_ARGUMENT;
    *frontier=0;
    if(checks->phase==0){scan->menu_ready=0;return title_slot(checks,card,memory,NULL,frontier);}
    if(checks->phase==1)return title_files(checks,card,memory,NULL,frontier);
    if(checks->phase==2){
        if(checks->subphase&&checks->load_index>=20)return GH_MC_BOOT_ARGUMENT;
        return title_headers(scan,checks,card,memory,0,frontier);
    }
    *frontier=checks->phase==3?0x20b0:0x2a58;
    return GH_MC_BOOT_UNSUPPORTED;
}

gh_mc_boot_error gh_mc_save_scan_00003f78(gh_mc_load_scan *scan,gh_mc_boot_checks *checks,
    gh_memory_card *card,gh_mc_load_menu_memory *memory,int32_t *free_clusters,uint32_t *frontier)
{
    if(!scan||!checks||!card||!memory||!free_clusters||!frontier||!checks->icon_work.pointer||
       checks->file_index<0||checks->file_index>20||checks->load_index<0||checks->load_index>20||
       checks->subphase>3||memory->first_slot< -1||memory->first_slot>19||
       (memory->first_slot>=0&&(memory->selected_row<0||memory->selected_row>2)))
        return GH_MC_BOOT_ARGUMENT;
    *frontier=0;
    if(!checks->phase){
        ++checks->frame;
        if(!checks->subphase){checks->mode=1;checks->frame=0;scan->menu_ready=0;}
        return title_slot(checks,card,memory,free_clusters,frontier);
    }
    if(checks->phase==1)return title_files(checks,card,memory,free_clusters,frontier);
    if(checks->phase==2){
        if(checks->subphase&&checks->load_index>=20)return GH_MC_BOOT_ARGUMENT;
        return title_headers(scan,checks,card,memory,1,frontier);
    }
    *frontier=0x3f78;
    return GH_MC_BOOT_UNSUPPORTED;
}
