#include "mc_save_control.h"
/* Actual mcport Control/Move/Sleep boot-mode2 slice, not a replacement menu.
 * Binary import/source evidence: analysis/mc-control-semantics.json. */
#include "mc_title_load.h"
#include <string.h>
static gh_id_error get(gh_mc_task_state *s,const gh_mc_task_bindings *b,int id,gh_id_entry **entry)
{return gh_id_base_get_work_002aa410(b->id_runtime,&s->id_base,id,entry,0);}
static gh_id_error display(gh_mc_task_state *s,const gh_mc_task_bindings *b,int id,int show)
{
    gh_id_entry *entry;gh_id_error error=get(s,b,id,&entry);if(error)return error;
    if(show==1)entry->flags&=~UINT32_C(0x20000000);else entry->flags|=UINT32_C(0x20000000);
    return gh_id_base_set_family_display(b->id_runtime,&s->id_base,id,show);
}
static gh_id_error message(gh_mc_task_state *s,const gh_mc_task_bindings *b,int id,uint16_t code)
{gh_id_entry *entry;gh_id_error error=get(s,b,id,&entry);if(error)return error;entry->message=code;return display(s,b,id,1);}
static gh_id_error shift_half(gh_id_entry *e,int sign)
{
    float twice=e->pos_y*2.0f;int32_t n;
    if(!(twice>=-16777214.0f && twice<=16777214.0f))return GH_ID_UNSUPPORTED;
    n=(int32_t)twice;if((float)n!=twice)return GH_ID_UNSUPPORTED;
    e->pos_y=(float)(n+sign)*0.5f;return GH_ID_OK;
}
static gh_id_error ui_move(gh_mc_control_state *c,gh_mc_task_state *s,const gh_mc_task_bindings *b)
{
    static const int ids[]={0x55,0x56,0x57,0x58,0x74,0x75};
    static const int hide[]={0x40,0x41,0x42,0x43,0x71,0x50,5,0,1};
    gh_id_entry *entry;gh_id_error error;unsigned i,alpha;
    if(!(s->flags&UINT32_C(0x40000000)))return GH_ID_OK; /* actual original gate */
    if(s->checks.mode!=2&&!((s->load_mode||(s->flags&0x00800000u))&&s->checks.mode==1)){c->first_unported=0x72c8;return GH_ID_UNSUPPORTED;}
#define DO(expr)do{error=(expr);if(error)return error;}while(0)
    s->flags&=~UINT32_C(0x02000000);
    for(i=0;i<sizeof(ids)/sizeof(ids[0]);++i)DO(message(s,b,ids[i],0x2005));
    for(i=0;i<sizeof(hide)/sizeof(hide[0]);++i)DO(display(s,b,hide[i],0));
    if(!s->load_mode&&!(s->flags&0x00800000u)){
        memset(c->progress,0,sizeof(c->progress));c->zero_b0=0;c->zero_b4=0;c->zero_b8=0;
    }else if(s->checks.file_index||s->checks.load_index){
        const gh_mc_boot_checks *checks=&s->checks;
        unsigned numerator=(unsigned)checks->file_index;
        for(int slot=0;slot<checks->file_index;++slot)
            if(!(checks->present_mask&(UINT32_C(1)<<slot)))numerator+=2;
        for(int slot=0;slot<checks->load_index;++slot)
            if(checks->present_mask&(UINT32_C(1)<<slot))numerator+=2;
        volatile float progress=(float)numerator/60.0f;
        float value=progress;
        memcpy(&c->zero_b0,&value,4);
    }
    DO(display(s,b,0x40,1));DO(message(s,b,0x41,0x2007));s->flags|=UINT32_C(0x02000000);
    DO(get(s,b,0,&entry));DO(shift_half(entry,-1));if(entry->pos_y<75.0f)entry->pos_y=81.0f;
    DO(get(s,b,1,&entry));DO(shift_half(entry,1));if(entry->pos_y>342.0f)entry->pos_y=336.0f;
    DO(get(s,b,0xd,&entry));alpha=entry->color_base>>24;
    if(s->flags&0x02000000){alpha+=3;if(alpha>18)alpha=18;}
    else alpha=alpha>3?alpha-3:0;
    entry->color_base=(entry->color_base&0x00ffffff)|(uint32_t)alpha<<24;
#undef DO
    return GH_ID_OK;
}
static gh_mc_boot_error update_ui(gh_mc_control_state *c,gh_mc_task_state *s,const gh_mc_task_bindings *b)
{
    gh_id_error error;uint32_t after=s->checks.frame;
    /* The shared check primitive records the final Control increment. Restore
     * preincrement while running UI SetDisp, then commit it in original order. */
    --s->checks.frame;error=ui_move(c,s,b);s->checks.frame=after;
    if(error){if(!c->first_unported)c->first_unported=0x72c8;return GH_MC_BOOT_UNSUPPORTED;}
    return GH_MC_BOOT_OK;
}
gh_id_error gh_mc_control_set_boot_display_000072c8(gh_mc_control_state*c,gh_mc_task_state*s,const gh_mc_task_bindings*b)
{if(!c||!s||!b||!b->id_runtime||!s->ui_initialized)return GH_ID_ARGUMENT;return ui_move(c,s,b);}
gh_mc_boot_error gh_mc_control_advance(gh_mc_control_state *c,gh_mc_task_state *s,const gh_mc_task_bindings *b)
{
    gh_mc_boot_error error;
    if(!c || !s || !b || !b->card || !b->work || !b->id_runtime || !s->ui_initialized)return GH_MC_BOOT_ARGUMENT;
    c->first_unported=0;
    error=gh_mc_boot_check_control(&s->checks,b->card,&b->work->flags);
    if(error==GH_MC_BOOT_UNSUPPORTED){c->first_unported=s->checks.phase==2?0x730:0xbd8;return error;}
    if(error)return error;
    return update_ui(c,s,b);
}
gh_mc_boot_error gh_mc_control_finish_delay(gh_mc_control_state *c,gh_mc_task_state *s,const gh_mc_task_bindings *b)
{
    gh_mc_boot_error error;
    if(!c || !s || !b || !s->ui_initialized)return GH_MC_BOOT_ARGUMENT;
    error=gh_mc_boot_delay_completed(&s->checks);if(error)return error;
    return update_ui(c,s,b);
}
gh_id_error gh_mc_control_sleep_frame(gh_mc_control_state *c,gh_mc_task_state *s,const gh_mc_task_bindings *b,
    gh_id_post_message post,void *post_context,gh_mc_draw_consumer draw,void *draw_context)
{
    gh_id_draw_job jobs[640];size_t count=0,i;gh_id_error error;uint32_t system_word;
    if(!c || !s || !b || !b->work || !b->threads || !s->ui_initialized || !draw)return GH_ID_ARGUMENT;
    memcpy(&system_word,(const unsigned char*)b->work+0x5e8,4);
    if(!(system_word&UINT32_C(0x80000000)))return GH_ID_OK; /* actual Sleep gate */
    c->first_unported=0;
    error=gh_id_jobs_move(&s->id_base,post,post_context,&c->id_frontier);
    if(error){c->first_unported=c->id_frontier.address;return error;}
    if(s->flags&0x10000000){c->first_unported=0x8b38;return GH_ID_UNSUPPORTED;}
    if(!s->id_base.hide && s->id_base.entries) {
        int original_return;
        /* Original Trans repeats packed/font/cursor/environment setup every
         * frame, even though initialization already did it. The real packed
         * consumer supplies that prelude; default sprite draw state belongs
         * to each actual draw consumer, never an empty setup callback. */
        if(!s->id_base.packed.bytes){c->first_unported=0x2aac28;return GH_ID_UNSUPPORTED;}
        error=gh_packed_message_bind(&s->id_base,b->id_manager,b->fonts,b->packed,b->language,
            b->work->language,0xb,1,&original_return);
        if(error || original_return!=1){c->first_unported=0x2aac28;return error?error:GH_ID_NOT_FOUND;}
    }
    error=gh_id_jobs_collect_visible(&s->id_base,jobs,640,&count,&c->id_frontier);
    if(error){c->first_unported=c->id_frontier.address;return error;}
    for(i=0;i<count;++i) {
        error=draw(draw_context,jobs+i);
        if(error){c->first_unported=jobs[i].consumer_address;return error;}
    }
    if(gh_task_threads_sleep(b->threads,1)!=GH_THREAD_OK)return GH_ID_ARGUMENT;
    if(s->flags&0x20000000)s->flags|=0x10000000;
    return GH_ID_OK;
}

gh_mc_boot_error gh_mc_control_scan_title_00001590(gh_mc_control_state *control,
    gh_mc_load_scan *scan,gh_mc_task_state *task,const gh_mc_task_bindings *bindings,
    gh_mc_load_menu_memory *memory,uint32_t *frontier)
{
    if(!control||!task||!bindings||!bindings->id_runtime||!task->ui_initialized||
       !task->load_mode||task->load_phase!=2||!frontier)return GH_MC_BOOT_ARGUMENT;
    const int reset_progress=task->checks.phase==0&&!task->checks.subphase;
    if(reset_progress)task->checks.mode=1; /* 1658 slotCheck entry */
    gh_mc_boot_error status=gh_mc_title_load_scan_00001590(scan,&task->checks,
        bindings->card,memory,frontier);
    if(status)return status;
    if(reset_progress){
        memset(control->progress,0,sizeof(control->progress));
        control->zero_b0=control->zero_b4=control->zero_b8=0;
    }
    if(ui_move(control,task,bindings)!=GH_ID_OK){
        *frontier=control->first_unported?control->first_unported:0x72c8;
        return GH_MC_BOOT_UNSUPPORTED;
    }
    return GH_MC_BOOT_OK;
}

static uint32_t record_word(const uint8_t *bytes)
{
    return (uint32_t)bytes[0]|(uint32_t)bytes[1]<<8|(uint32_t)bytes[2]<<16|(uint32_t)bytes[3]<<24;
}

static gh_id_error list_number(gh_mc_task_state *task,const gh_mc_task_bindings *bindings,int id,uint32_t n)
{
    gh_id_entry *entry;
    gh_id_error error=get(task,bindings,id,&entry);
    if(error)return error;
    entry->seed=n;
    return gh_id_base_set_family_display(bindings->id_runtime,&task->id_base,id,1);
}

static gh_id_error title_list_rows(gh_mc_control_state *control,
    const gh_mc_load_scan *scan,gh_mc_task_state *task,const gh_mc_task_bindings *bindings)
{
    control->first_unported=0;
    const uint8_t *records=task->checks.icon_work.pointer;
    for(unsigned row=0;row<3;++row){
        const uint8_t *record=records+(scan->first_slot+row)*GH_MC_SAVE_HEADER_BYTES;
        if(record_word(record+4)!=UINT32_MAX&&record[0x10]>2){
            control->first_unported=0x8058;
            return GH_ID_UNSUPPORTED;
        }
    }
#define ROW_CALL(call) do{gh_id_error error=(call);if(error){control->first_unported=0x8058;return error;}}while(0)
    for(int row=0;row<3;++row){
        unsigned slot=(unsigned)(scan->first_slot+row);
        const uint8_t *record=records+slot*GH_MC_SAVE_HEADER_BYTES;
        gh_id_entry *entry;
        ROW_CALL(display(task,bindings,0x14+row,1));
        ROW_CALL(list_number(task,bindings,0x14+row,slot+1));
        const int empty=record_word(record+4)==UINT32_MAX;
        ROW_CALL(display(task,bindings,0x10+row,!empty));
        ROW_CALL(display(task,bindings,0x60+row,empty));
        if(empty)ROW_CALL(message(task,bindings,0x60+row,0x2006));
        ROW_CALL(get(task,bindings,0x14+row,&entry));
        entry->color_base=empty?0x801e1c1e:0x80808080;
        if(empty)continue;

        uint32_t chapter=record[0x0d]?
            (record[0x0d]-1u)*16u+record[0x0c]+0x2023u:0x20a3u;
        if(chapter>0x20a3)chapter=0x20a3;
        ROW_CALL(message(task,bindings,0x18+row,(uint16_t)chapter));
        ROW_CALL(get(task,bindings,0x18+row,&entry));
        entry->scale_x=entry->scale_y=0.75f;
        const uint32_t ticks=record_word(record+0x20);
        int64_t signed_ticks=(ticks&UINT32_C(0x80000000))?
            (int64_t)ticks-INT64_C(0x100000000):ticks;
        int32_t seconds=(int32_t)(signed_ticks/30);
        int32_t hours=seconds/3600;
        if(hours>99)hours=99;
        ROW_CALL(list_number(task,bindings,0x24+row,(uint32_t)hours));
        ROW_CALL(list_number(task,bindings,0x28+row,(uint32_t)((seconds/60)%60)));
        ROW_CALL(list_number(task,bindings,0x2c+row,(uint32_t)(seconds%60)));

        static const float english_scale[]={1.1f,1.0f,1.1f};
        unsigned difficulty=record[0x10];
        ROW_CALL(message(task,bindings,0x30+row,(uint16_t)(0x2000+difficulty)));
        ROW_CALL(get(task,bindings,0x30+row,&entry));
        entry->scale_x=bindings->work->language==1?english_scale[difficulty]:0.75f;
        entry->scale_y=1.0f;
        unsigned marker=(unsigned)record[0x0e]|(unsigned)record[0x0f]<<8;
        if(marker)ROW_CALL(list_number(task,bindings,0x34+row,marker));
        else ROW_CALL(display(task,bindings,0x34+row,0));
        ROW_CALL(message(task,bindings,0x76+row,marker?0x20a6:0x2005));
    }
#undef ROW_CALL
    return GH_ID_OK;
}
gh_id_error gh_mc_control_init_title_list_00007f30(gh_mc_control_state *control,
    const gh_mc_load_scan *scan,gh_mc_task_state *task,const gh_mc_task_bindings *bindings)
{
    if(!control||!scan||!task||!bindings||!bindings->id_runtime||!bindings->work||
       !task->ui_initialized||!task->load_mode||!scan->menu_ready||
       scan->first_slot<0||scan->first_slot>17||task->checks.phase!=3||
       !task->checks.icon_work.pointer)return GH_ID_ARGUMENT;
    if(task->checks.subphase==1)return GH_ID_OK;
    if(task->checks.subphase)return GH_ID_ARGUMENT;
    gh_id_error error=title_list_rows(control,scan,task,bindings);
    if(error)return error;
    memset(control->progress,0,sizeof(control->progress));
    control->zero_b0=control->zero_b4=control->zero_b8=0;
    task->checks.mode=0;
    task->checks.subphase=1;
    return GH_ID_OK;
}

static void select_phase(gh_mc_boot_checks *checks,uint32_t phase)
{
    checks->phase=phase;
    checks->subphase=0;
    checks->frame=0;
}

static void clear_progress(gh_mc_control_state *control)
{
    memset(control->progress,0,sizeof(control->progress));
    control->zero_b0=control->zero_b4=control->zero_b8=0;
}

static gh_mc_load_status select_sound(const gh_mc_title_load_providers *providers,int program,uint32_t *frontier)
{
    const gh_mc_sound_request request={0,program,-1,-1,0,0};
    if(providers->sound&&!providers->sound(providers->sound_context,&request))
        return GH_MC_LOAD_PENDING;
    *frontier=0x2cb8a0;
    return GH_MC_LOAD_UNSUPPORTED;
}

static gh_mc_load_status select_info(gh_mc_title_load *load,gh_mc_boot_checks *checks,
    gh_memory_card *card,gh_mc_load_menu_memory *memory,int *input_ready,uint32_t *frontier)
{
    *input_ready=0;
    const uint32_t phase=checks->subphase;
    if(phase==1){
        gh_mc_error request=gh_memory_card_request_info(card); /* REL3850 */
        checks->error=-1;
        if(request){
            checks->error=9;
            memory->first_slot=memory->selected_row=-1;
            select_phase(checks,7);
            *frontier=0x3850;
            return GH_MC_LOAD_IO;
        }
        load->info_pending=1;
        checks->subphase=2;
    }
    if(load->info_pending){
        int done;
        gh_mc_result result;
        if(gh_memory_card_poll(card,&done,&result)){
            load->info_pending=0;
            *frontier=0x31c8;
            return GH_MC_LOAD_IO;
        }
        if(!done){
            *input_ready=checks->subphase==2;
            return GH_MC_LOAD_PENDING;
        }
        load->info_pending=0;
        if(checks->subphase==2){
            if(result.result){
                checks->subphase=3;
                return GH_MC_LOAD_PENDING;
            }
            checks->subphase=1;
        }
    }
    if(phase!=3&&phase!=4)*input_ready=1;
    return GH_MC_LOAD_PENDING;
}

static gh_id_error copy_position(gh_mc_task_state *task,const gh_mc_task_bindings *bindings,
    int id,int source)
{
    gh_id_entry *entry,*target;
    gh_id_error error=get(task,bindings,id,&entry);
    if(error)return error;
    error=get(task,bindings,source,&target);
    if(error)return error;
    entry->pos_x=target->pos_x;
    entry->pos_y=target->pos_y;
    return GH_ID_OK;
}

static gh_id_error title_select_display(uint8_t choice,gh_mc_control_state *control,
    const gh_mc_load_scan *scan,gh_mc_task_state *task,const gh_mc_task_bindings *bindings)
{
    static const int text[]={0x55,0x56,0x57,0x58,0x74,0x75};
    static const int hide[]={0x40,0x41,0x42,0x43,0x71,0x50,5,0,1};
    gh_id_entry *entry;
#define UI(call) do{gh_id_error error=(call);if(error)return error;}while(0)
    if(!(task->flags&UINT32_C(0x40000000)))return GH_ID_OK;
    if(task->checks.mode==1)return ui_move(control,task,bindings);
    task->flags&=~UINT32_C(0x02000000);
    for(unsigned i=0;i<sizeof(text)/sizeof(*text);++i)
        UI(message(task,bindings,text[i],0x2005));
    for(unsigned i=0;i<sizeof(hide)/sizeof(*hide);++i)
        UI(display(task,bindings,hide[i],0));
    switch(task->checks.mode){
    case 0:
        UI(display(task,bindings,5,1));
        UI(copy_position(task,bindings,5,scan->selected_row+6));
        UI(display(task,bindings,0,1));
        UI(display(task,bindings,1,1));
        break;
    case 7:
    case 8:
    case 9: {
        const uint16_t code=task->checks.mode==7?0x2010:task->checks.mode==8?0x200e:0x200f;
        UI(display(task,bindings,5,1));
        UI(copy_position(task,bindings,5,scan->selected_row+6));
        UI(display(task,bindings,0x42,1));
        UI(message(task,bindings,0x43,code));
        UI(message(task,bindings,0x71,code));
        UI(message(task,bindings,0x57,0x2003));
        UI(message(task,bindings,0x58,0x2004));
        UI(message(task,bindings,0x74,0x2003));
        UI(message(task,bindings,0x75,0x2004));
        UI(display(task,bindings,0x50,1));
        UI(copy_position(task,bindings,0x50,choice?0x53:0x54));
        break;
    }
    case 3: {
        UI(display(task,bindings,0x40,1));
        UI(message(task,bindings,0x41,0x2015));
        task->flags|=UINT32_C(0x02000000);
        static const float weights[]={80,100,100,100,30,80};
        float total=0,value,increment;
        unsigned count=0;
        while(count<6&&record_word(control->progress+count*4)){
            total+=weights[count];
            ++count;
        }
        if(!count)break;
        memcpy(&value,&control->zero_b0,4);
        memcpy(&increment,&control->zero_b4,4);
        if(count!=control->zero_b8){
            control->zero_b8=count;
            increment=(total/490.0f-value)/weights[count-1];
            memcpy(&control->zero_b4,&increment,4);
        }
        volatile float sum=value+increment;
        value=sum>1.0f?1.0f:sum;
        memcpy(&control->zero_b0,&value,4);
        break;
    }
    case 4: {
        UI(display(task,bindings,0x40,1));
        UI(message(task,bindings,0x41,0x2016));
        task->flags|=UINT32_C(0x02000000);
        float value;
        memcpy(&value,&control->zero_b0,4);
        volatile float incremented=value+1.0f/20.0f;
        value=incremented>1.0f?1.0f:incremented;
        memcpy(&control->zero_b0,&value,4);
        break;
    }
    case 0xe:
        UI(display(task,bindings,0x42,1));
        UI(message(task,bindings,0x43,0x2018));
        UI(message(task,bindings,0x71,0x2018));
        break;
    case 0x11:
    case 0x18:
        UI(display(task,bindings,0x40,1));
        UI(message(task,bindings,0x41,task->checks.mode==0x11?0x200c:0x2008));
        task->flags|=UINT32_C(0x02000000);
        if(task->checks.mode==0x11){
            UI(message(task,bindings,0x55,0x2003));
            UI(message(task,bindings,0x56,0x2004));
            UI(display(task,bindings,0x50,1));
            UI(copy_position(task,bindings,0x50,choice?0x51:0x52));
        }
        break;
    case 0x16: {
        const uint32_t system=record_word((const uint8_t*)bindings->work+0x5e4);
        const uint16_t code=system&UINT32_C(0x4000)?0x201c:0x201d;
        UI(display(task,bindings,5,1));
        UI(copy_position(task,bindings,5,scan->selected_row+6));
        UI(display(task,bindings,0x42,1));
        UI(message(task,bindings,0x43,code));
        UI(message(task,bindings,0x71,code));
        UI(message(task,bindings,0x57,0x2003));
        UI(message(task,bindings,0x58,0x2004));
        UI(message(task,bindings,0x74,0x2003));
        UI(message(task,bindings,0x75,0x2004));
        UI(display(task,bindings,0x50,1));
        UI(copy_position(task,bindings,0x50,choice?0x53:0x54));
        break;
    }
    case 0xa:
    case 0xb: {
        const uint16_t code=task->checks.mode==0xa?0x2019:0x201a;
        UI(display(task,bindings,0x42,1));
        UI(message(task,bindings,0x43,code));
        UI(message(task,bindings,0x71,code));
        break;
    }
    default:
        return GH_ID_UNSUPPORTED;
    }
    UI(get(task,bindings,0,&entry));UI(shift_half(entry,-1));if(entry->pos_y<75)entry->pos_y=81;
    UI(get(task,bindings,1,&entry));UI(shift_half(entry,1));if(entry->pos_y>342)entry->pos_y=336;
    UI(get(task,bindings,0xd,&entry));
    unsigned alpha=entry->color_base>>24;
    if(task->flags&UINT32_C(0x02000000)){alpha+=3;if(alpha>18)alpha=18;}
    else alpha=alpha>3?alpha-3:0;
    entry->color_base=(entry->color_base&0x00ffffffu)|(uint32_t)alpha<<24;
#undef UI
    return GH_ID_OK;
}

static uint32_t work_button(const gh_boot_work *work,unsigned offset)
{
    return record_word((const uint8_t*)work+offset);
}

static gh_mc_load_status title_menu(gh_mc_title_load *load,gh_mc_control_state *control,
    gh_mc_load_scan *scan,gh_mc_task_state *task,const gh_mc_task_bindings *bindings,
    gh_mc_load_menu_memory *memory,const gh_mc_title_load_providers *providers,uint32_t *frontier)
{
    gh_mc_boot_checks *checks=&task->checks;
    const int confirm=checks->phase==4;
    if(!checks->subphase){
        if(confirm){
            clear_progress(control);
            checks->mode=9;
            load->choice=1;
            checks->subphase=1;
        }else if(gh_mc_control_init_title_list_00007f30(control,scan,task,bindings)!=GH_ID_OK){
            *frontier=control->first_unported?control->first_unported:0x7f30;
            return GH_MC_LOAD_UNSUPPORTED;
        }
    }
    const uint32_t entering_subphase=checks->subphase;
    int input_ready;
    gh_mc_load_status status=select_info(load,checks,bindings->card,memory,&input_ready,frontier);
    if(status!=GH_MC_LOAD_PENDING)return status;
    if(!input_ready){
        if(!load->info_pending&&entering_subphase==3&&checks->subphase==3){
            select_phase(checks,0);
            checks->mode=1;
        }else if(!load->info_pending&&entering_subphase==4&&checks->subphase==4){
            if(!confirm){
                load->selected_slot=(uint8_t)(scan->first_slot+scan->selected_row);
                select_phase(checks,4);
            }else if(load->choice){
                clear_progress(control);
                checks->mode=4;
                select_phase(checks,5);
            }else{
                select_phase(checks,3);
            }
        }
        return GH_MC_LOAD_PENDING;
    }

    const uint32_t pressed=work_button(bindings->work,0x1a0);
    const uint32_t repeat=work_button(bindings->work,0x1b0);
    if(confirm){
        if(repeat&UINT32_C(0x0c000000)){
            status=select_sound(providers,0x161,frontier);
            if(status!=GH_MC_LOAD_PENDING)return status;
            load->choice=(uint8_t)!load->choice;
        }else if(pressed&UINT32_C(0x30000000)){
            const int yes=(pressed&UINT32_C(0x10000000))&&load->choice;
            status=select_sound(providers,yes?0x160:0x15e,frontier);
            if(status!=GH_MC_LOAD_PENDING)return status;
            load->choice=(uint8_t)yes;
            checks->subphase=4;
        }
        return GH_MC_LOAD_PENDING;
    }

    if(pressed&UINT32_C(0x10000000)){
        unsigned slot=(unsigned)(scan->first_slot+scan->selected_row);
        const int valid=(checks->present_mask&(UINT32_C(1)<<slot))!=0;
        status=select_sound(providers,valid?0x160:0x122,frontier);
        if(status!=GH_MC_LOAD_PENDING)return status;
        if(valid)checks->subphase=4;
    }else if(pressed&UINT32_C(0x20000000)){
        status=select_sound(providers,0x15e,frontier);
        if(status!=GH_MC_LOAD_PENDING)return status;
        checks->error=-1;
        select_phase(checks,7);
    }else if(repeat&UINT32_C(0x03000000)){
        status=select_sound(providers,0x161,frontier);
        if(status!=GH_MC_LOAD_PENDING)return status;
        if(repeat&UINT32_C(0x01000000)){
            if(scan->selected_row>0)--scan->selected_row;
            else if(scan->first_slot>0)--scan->first_slot;
            else {scan->first_slot=17;scan->selected_row=2;}
        }else{
            if(scan->selected_row<2)++scan->selected_row;
            else if(scan->first_slot<17)++scan->first_slot;
            else {scan->first_slot=0;scan->selected_row=0;}
        }
    }
    if(title_list_rows(control,scan,task,bindings)!=GH_ID_OK){
        *frontier=control->first_unported?control->first_unported:0x7f30;
        return GH_MC_LOAD_UNSUPPORTED;
    }
    return GH_MC_LOAD_PENDING;
}

/* REL2A58 error acknowledgement and retry/cancel confirmation. */
static gh_mc_load_status title_error(gh_mc_title_load *load,gh_mc_control_state *control,
    gh_mc_task_state *task,const gh_mc_task_bindings *bindings,
    const gh_mc_title_load_providers *providers,uint32_t *frontier)
{
    gh_mc_boot_checks *checks=&task->checks;
    const uint32_t pressed=work_button(bindings->work,0x1a0);
    switch(checks->subphase){
    case 0:
        load->choice=0;
        if(checks->error!=-1)for(int id=0x10;id<=0x12;++id){
            if(display(task,bindings,id,0)!=GH_ID_OK){
                *frontier=0x88d0;
                return GH_MC_LOAD_UNSUPPORTED;
            }
        }
        if(checks->error==5||checks->error==8){
            clear_progress(control);
            checks->mode=checks->error==5?0xe:0x18;
            checks->subphase=1;
        }else checks->subphase=2;
        /* Original phase0 reaches acknowledgement in the same visit. */
        /* fallthrough */
    case 1:
        if(pressed&UINT32_C(0x30000000)){
            load->choice=0;
            if(checks->error==5){
                gh_mc_error reset=gh_mc_selected_load_abort_io(&load->selected);
                if(reset){*frontier=0x31c8;return GH_MC_LOAD_IO;}
                select_phase(checks,0);
            }else checks->subphase=2;
        }
        return GH_MC_LOAD_PENDING;
    case 2: {
        clear_progress(control);
        checks->mode=checks->error==1?0x11:0x16;
        if(work_button(bindings->work,0x1b0)&UINT32_C(0x0c000000)){
            gh_mc_load_status status=select_sound(providers,0x161,frontier);
            if(status!=GH_MC_LOAD_PENDING)return status;
            load->choice=(uint8_t)!load->choice;
        }else if(pressed&UINT32_C(0x10000000)){
            gh_mc_load_status status=select_sound(providers,load->choice?0x160:0x15e,frontier);
            if(status!=GH_MC_LOAD_PENDING)return status;
            if(load->choice){
                checks->done=1;
                load->completion_display_pending=1;
            }else checks->subphase=3;
        }
        return GH_MC_LOAD_PENDING;
    }
    case 3:
        checks->subphase=4;
        /* REL31C8 treats both a completed command and an idle card as ready. */
        /* fallthrough */
    case 4: {
        gh_mc_error status=gh_mc_control_title_abort_io(load,bindings->card);
        if(status==GH_MC_BUSY)return GH_MC_LOAD_PENDING;
        if(status){*frontier=0x31c8;return GH_MC_LOAD_IO;}
        select_phase(checks,0);
        return GH_MC_LOAD_PENDING;
    }
    default:
        *frontier=0x2a58;
        return GH_MC_LOAD_UNSUPPORTED;
    }
}

gh_mc_load_status gh_mc_control_title_select_00001590(gh_mc_title_load *load,gh_mc_control_state *control,
    gh_mc_load_scan *scan,gh_mc_task_state *task,const gh_mc_task_bindings *bindings,
    gh_mc_load_menu_memory *memory,const gh_mc_title_load_providers *providers,uint32_t *frontier)
{
    if(!load||!control||!scan||!task||!bindings||!bindings->card||!bindings->work||
       !memory||!providers||!frontier||!task->ui_initialized||!task->load_mode||
       task->load_phase!=2||scan->selected_row<0||scan->selected_row>2||
       (task->checks.phase!=7&&(!scan->menu_ready||scan->first_slot<0||
                               scan->first_slot>17)))return GH_MC_LOAD_ARGUMENT;
    *frontier=0;
    gh_mc_boot_checks *checks=&task->checks;
    if(checks->done&&!load->completion_display_pending)
        return checks->phase==7?GH_MC_LOAD_CANCELED:GH_MC_LOAD_RESTORED;
    gh_mc_load_status status=GH_MC_LOAD_PENDING;
    if(!checks->done)switch(checks->phase){
    case 3:case 4:
        status=title_menu(load,control,scan,task,bindings,memory,providers,frontier);
        if(status!=GH_MC_LOAD_PENDING)return status;
        break;
    case 5:
        ++checks->frame;
        if(!checks->subphase){
            if(!task->load_payload.pointer)return GH_MC_LOAD_ARGUMENT;
            status=gh_mc_selected_load_begin_using(&load->selected,bindings->card,load->selected_slot,
                task->load_payload.pointer,GH_MC_SAVE_RECORD_BYTES);
            if(status!=GH_MC_LOAD_PENDING){*frontier=0x3350;return status;}
            checks->subphase=1;
        }
        status=gh_mc_selected_load_step_wait_close(&load->selected,checks->frame>=20,frontier);
        if(status==GH_MC_LOAD_VERIFIED)select_phase(checks,6);
        else if(status!=GH_MC_LOAD_PENDING){
            if(status==GH_MC_LOAD_NOT_FOUND||status==GH_MC_LOAD_CHECKSUM||
               status==GH_MC_LOAD_VERSION||status==GH_MC_LOAD_SHORT){
                checks->error=5;
                select_phase(checks,7);
                status=GH_MC_LOAD_PENDING;
                *frontier=0;
            }else return status;
        }
        break;
    case 6: {
        if(!checks->subphase){
            memory->first_slot=scan->first_slot;memory->selected_row=scan->selected_row;
            checks->frame=90;
            clear_progress(control);
            checks->mode=0xb;
            bindings->work->flags=(bindings->work->flags&~UINT32_C(0x08000000))|4;
            checks->subphase=1;
        }
        uint64_t pressed;
        memcpy(&pressed,(const uint8_t*)bindings->work+0x1a0,sizeof(pressed));
        if(checks->frame&&!(checks->frame<80&&(pressed&UINT64_C(0x8030000000)))){
            --checks->frame;
            break;
        }
        bindings->work->flags|=0x40;
        status=gh_mc_selected_load_restore_00002c88(&load->selected,providers->checkpoint,
            providers->checkpoint_bindings,providers->door,providers->output_mode,
            providers->audio_context,frontier);
        if(status!=GH_MC_LOAD_RESTORED)return status;
        checks->done=1;
        load->completion_display_pending=1;
        break;
    }
    case 7:
        status=title_error(load,control,task,bindings,providers,frontier);
        if(status!=GH_MC_LOAD_PENDING)return status;
        break;
    default:
        *frontier=0x1658;
        return GH_MC_LOAD_UNSUPPORTED;
    }
    if(title_select_display(load->choice,control,scan,task,bindings)!=GH_ID_OK){
        *frontier=0x72c8;
        return GH_MC_LOAD_UNSUPPORTED;
    }
    load->completion_display_pending=0;
    if(!checks->done)return GH_MC_LOAD_PENDING;
    return checks->phase==7?GH_MC_LOAD_CANCELED:GH_MC_LOAD_RESTORED;
}


gh_mc_error gh_mc_control_title_abort_io(gh_mc_title_load *load,gh_memory_card *card)
{
    if(!load||!card||(load->selected.card&&load->selected.card!=card))return GH_MC_ARGUMENT;
    if(load->info_pending){
        int done;
        gh_mc_result result;
        gh_mc_error error=gh_memory_card_poll(card,&done,&result);
        if(error)return error;
        if(!done)return GH_MC_BUSY;
        load->info_pending=0;
    }
    return gh_mc_selected_load_abort_io(&load->selected);
}

static gh_mc_save_status save_fail(uint32_t *frontier,uint32_t address,gh_mc_save_status status)
{
    *frontier=address;
    return status;
}

static gh_mc_save_status save_sound(const gh_mc_save_providers *b,int program,uint32_t *frontier)
{
    const gh_mc_sound_request request={0,program,-1,-1,0,0};
    if(!b->sound||b->sound(b->sound_context,&request))
        return save_fail(frontier,0x2cb8a0,GH_MC_SAVE_UNSUPPORTED);
    return GH_MC_SAVE_PENDING;
}

/* This path uses the actual native card queue and keeps the selected slot
 * and record stable from open through CloseCheck and readback. */
static gh_mc_save_status save_queue(gh_mc_error error,uint32_t address,uint32_t *frontier)
{
    return error?save_fail(frontier,address,GH_MC_SAVE_IO):GH_MC_SAVE_PENDING;
}

static gh_mc_save_status save_poll(gh_memory_card *card,int32_t *result,int *done,uint32_t *frontier)
{
    gh_mc_result reply;
    gh_mc_error error=gh_memory_card_poll(card,done,&reply);
    if(error)return save_fail(frontier,0x31c8,GH_MC_SAVE_IO);
    if(*done)*result=reply.result;
    return GH_MC_SAVE_PENDING;
}

static void save_move_slot(gh_mc_load_scan *scan,int up)
{
    if(up){
        if(scan->selected_row>0)--scan->selected_row;
        else if(scan->first_slot>0)--scan->first_slot;
        else {scan->first_slot=17;scan->selected_row=2;}
    }else{
        if(scan->selected_row<2)++scan->selected_row;
        else if(scan->first_slot<17)++scan->first_slot;
        else {scan->first_slot=0;scan->selected_row=0;}
    }
}

static gh_mc_save_status save_select(gh_mc_save_control *save,gh_mc_control_state *control,
    gh_mc_task_state *task,const gh_mc_task_bindings *bindings,
    const gh_mc_save_providers *providers,uint32_t *frontier)
{
    gh_mc_boot_checks *checks=&task->checks;
    const int list=checks->phase==6;
    if(!save->scan.menu_ready)return GH_MC_SAVE_ARGUMENT;
    if(!checks->subphase){
        clear_progress(control);
        checks->mode=list?0:checks->phase==7?8:7;
        if(list&&title_list_rows(control,&save->scan,task,bindings)!=GH_ID_OK)
            return save_fail(frontier,0x7f30,GH_MC_SAVE_UNSUPPORTED);
        if(!list)save->choice=0;
        checks->subphase=1;
    }
    const uint32_t entering_subphase=checks->subphase;
    if(entering_subphase==1){
        gh_mc_save_status status=save_queue(gh_memory_card_request_info(bindings->card),0x3850,frontier);
        if(status!=GH_MC_SAVE_PENDING)return status;
        save->info_pending=1;
        checks->subphase=2;
    }
    if(save->info_pending){
        int done;
        gh_mc_result reply;
        gh_mc_error error=gh_memory_card_poll(bindings->card,&done,&reply);
        if(error)return save_fail(frontier,0x31c8,GH_MC_SAVE_IO);
        if(done){
            save->info_pending=0;
            if(reply.result||reply.card_type!=2||!reply.formatted)
                return save_fail(frontier,0x6db8,GH_MC_SAVE_UNSUPPORTED);
            save->free_clusters=reply.free_clusters;
            if(checks->subphase==2)checks->subphase=1;
        }
    }
    if(entering_subphase==4){
        if(save->info_pending)return GH_MC_SAVE_PENDING;
        if(list){
            unsigned slot=(unsigned)(save->scan.first_slot+save->scan.selected_row);
            save->selected_slot=(uint8_t)slot;
            if(checks->present_mask&(UINT32_C(1)<<slot))select_phase(checks,8);
            else if(save->free_clusters<150)return save_fail(frontier,0x5120,GH_MC_SAVE_UNSUPPORTED);
            else select_phase(checks,7);
        }else if(!save->choice)select_phase(checks,6);
        else if(checks->phase==8)return save_fail(frontier,0x5a20,GH_MC_SAVE_UNSUPPORTED);
        else{
            clear_progress(control);
            checks->mode=3;
            select_phase(checks,10);
        }
        return GH_MC_SAVE_PENDING;
    }

    const uint32_t pressed=work_button(bindings->work,0x1a0);
    const uint32_t repeat=work_button(bindings->work,0x1b0);
    gh_mc_save_status status=GH_MC_SAVE_PENDING;
    if(list){
        if(pressed&UINT32_C(0x10000000)){
            status=save_sound(providers,0x160,frontier);
            if(status!=GH_MC_SAVE_PENDING)return status;
            checks->subphase=4;
        }else if(pressed&UINT32_C(0x20000000)){
            status=save_sound(providers,0x15e,frontier);
            if(status!=GH_MC_SAVE_PENDING)return status;
            checks->error=-1;
            select_phase(checks,15);
        }else if(repeat&UINT32_C(0x03000000)){
            status=save_sound(providers,0x161,frontier);
            if(status!=GH_MC_SAVE_PENDING)return status;
            save_move_slot(&save->scan,(repeat&UINT32_C(0x01000000))!=0);
        }
        if(title_list_rows(control,&save->scan,task,bindings)!=GH_ID_OK)
            return save_fail(frontier,0x7f30,GH_MC_SAVE_UNSUPPORTED);
    }else if(repeat&UINT32_C(0x0c000000)){
        status=save_sound(providers,0x161,frontier);
        if(status!=GH_MC_SAVE_PENDING)return status;
        save->choice=(uint8_t)!save->choice;
    }else if(pressed&UINT32_C(0x30000000)){
        const int yes=(pressed&UINT32_C(0x10000000))&&save->choice;
        status=save_sound(providers,yes?0x160:0x15e,frontier);
        if(status!=GH_MC_SAVE_PENDING)return status;
        save->choice=(uint8_t)yes;
        checks->subphase=4;
    }
    return GH_MC_SAVE_PENDING;
}

static void save_progress(gh_mc_control_state *control,unsigned index)
{
    uint32_t value=record_word(control->progress+index*4)+1;
    memcpy(control->progress+index*4,&value,4);
}

static gh_mc_save_status save_write_step(gh_mc_save_control *control,gh_mc_save_task_state *save,
    gh_memory_card *card,const gh_mc_save_providers *b,uint32_t *frontier)
{
    gh_mc_boot_checks *checks=&save->common.checks;
    const unsigned slot=control->selected_slot;
    int done;
    int32_t result=0;
    gh_mc_error error;
    gh_mc_save_status status;
    switch(checks->subphase){
    case 0:
        error=gh_memory_card_request_slot_directory(card,slot);
        status=save_queue(error,0x34e8,frontier);
        if(status!=GH_MC_SAVE_PENDING)return status;
        checks->subphase=1;
        /* Original 05AF8 polls the directory immediately after request. */
        /* fall through */
    case 1:
        status=save_poll(card,&result,&done,frontier);
        if(status!=GH_MC_SAVE_PENDING||!done)return status;
        if(result!=0&&result!=1)return save_fail(frontier,0x6e18,GH_MC_SAVE_IO);
        if(!result){
            error=gh_memory_card_request_slot_create(card,slot);
            status=save_queue(error,0x3588,frontier);
            if(status==GH_MC_SAVE_PENDING)checks->subphase=2;
            return status;
        }
        break;
    case 2:
        status=save_poll(card,&result,&done,frontier);
        if(status!=GH_MC_SAVE_PENDING||!done)return status;
        if(result)return save_fail(frontier,0x7058,GH_MC_SAVE_IO);
        break;
    case 3: {
        status=save_poll(card,&result,&done,frontier);
        if(status!=GH_MC_SAVE_PENDING||!done)return status;
        if(result)return save_fail(frontier,0x6ee0,GH_MC_SAVE_IO);
        checks->subphase=6;
        /* The open completed. A blocked checkpoint provider resumes here. */
        /* fall through */
    }
    case 6: {
        if(!b->checkpoint||!b->checkpoint_bindings||!b->door||!b->metadata)
            return save_fail(frontier,0x6758,GH_MC_SAVE_UNSUPPORTED);
        gh_mc_save_metadata metadata=*b->metadata;
        metadata.mode=0;
        metadata.history_mask=checks->present_mask;
        metadata.history=checks->icon_work.pointer;
        metadata.history_bytes=20*GH_MC_SAVE_HEADER_BYTES;
        gh_mc_load_status capture=gh_mc_save_serialize_00006758(b->checkpoint,
            b->checkpoint_bindings,b->door,&metadata,save->current_record.pointer,frontier);
        if(capture!=GH_MC_LOAD_VERIFIED){
            if(!*frontier)*frontier=0x6758;
            return GH_MC_SAVE_UNSUPPORTED;
        }
        checks->subphase=7;
        /* fall through */
    }
    case 7:
        error=gh_memory_card_request_save_write(card,save->current_record.pointer);
        status=save_queue(error,0x3698,frontier);
        if(status==GH_MC_SAVE_PENDING)checks->subphase=4;
        return status;
    case 4:
        status=save_poll(card,&result,&done,frontier);
        if(status!=GH_MC_SAVE_PENDING||!done)return status;
        if(result!=GH_MC_SAVE_RECORD_BYTES)return save_fail(frontier,0x70a8,GH_MC_SAVE_IO);
        error=gh_memory_card_request_save_close(card);
        status=save_queue(error,0x3300,frontier);
        if(status==GH_MC_SAVE_PENDING)checks->subphase=5;
        return status;
    case 5:
        status=save_poll(card,&result,&done,frontier);
        if(status!=GH_MC_SAVE_PENDING||!done)return status;
        if(result)return save_fail(frontier,0x6f88,GH_MC_SAVE_IO);
        select_phase(checks,11);
        return GH_MC_SAVE_PENDING;
    default:
        return GH_MC_SAVE_ARGUMENT;
    }
    error=gh_memory_card_request_save_write_open(card,slot);
    status=save_queue(error,0x3370,frontier);
    if(status==GH_MC_SAVE_PENDING)checks->subphase=3;
    return status;
}

static gh_mc_save_status save_icon_step(gh_mc_save_control *control,gh_mc_save_task_state *save,
    gh_memory_card *card,uint32_t *frontier)
{
    gh_mc_boot_checks *checks=&save->common.checks;
    const int model=checks->phase==11;
    const gh_mc_save_icon_kind kind=model?GH_MC_SAVE_ICON_MODEL:GH_MC_SAVE_ICON_SYSTEM;
    const gh_id_view image=model?save->icon_model:save->icon_template;
    const uint32_t count=model?0x8618:0x3c4;
    if(!image.bytes||image.size<count)return save_fail(frontier,0x3bc0,GH_MC_SAVE_UNSUPPORTED);
    int done;
    int32_t result;
    gh_mc_save_status status;
    switch(checks->subphase){
    case 0:
        status=save_queue(gh_memory_card_request_save_icon_write_open(card,control->selected_slot,kind),0x3370,frontier);
        if(status==GH_MC_SAVE_PENDING)checks->subphase=1;
        return status;
    case 1:
        status=save_poll(card,&result,&done,frontier);
        if(status!=GH_MC_SAVE_PENDING||!done)return status;
        if(result)return save_fail(frontier,0x6ee0,GH_MC_SAVE_IO);
        status=save_queue(gh_memory_card_request_save_icon_write(card,kind,image.bytes,count),0x3698,frontier);
        if(status==GH_MC_SAVE_PENDING)checks->subphase=2;
        return status;
    case 2:
        status=save_poll(card,&result,&done,frontier);
        if(status!=GH_MC_SAVE_PENDING||!done)return status;
        if(result!=(int32_t)count)return save_fail(frontier,0x70a8,GH_MC_SAVE_IO);
        status=save_queue(gh_memory_card_request_save_close(card),0x3300,frontier);
        if(status==GH_MC_SAVE_PENDING)checks->subphase=3;
        return status;
    case 3:
        status=save_poll(card,&result,&done,frontier);
        if(status!=GH_MC_SAVE_PENDING||!done)return status;
        if(result)return save_fail(frontier,0x6f88,GH_MC_SAVE_IO);
        select_phase(checks,model?12:14);
        return GH_MC_SAVE_PENDING;
    default:
        return GH_MC_SAVE_ARGUMENT;
    }
}

static gh_mc_save_status save_verify_step(gh_mc_save_control *control,gh_mc_save_task_state *save,
    gh_memory_card *card,uint32_t *frontier)
{
    gh_mc_boot_checks *checks=&save->common.checks;
    int done;
    int32_t result;
    gh_mc_save_status status;
    if(!checks->subphase){
        status=save_queue(gh_memory_card_request_save_open(card,control->selected_slot),0x3350,frontier);
        if(status!=GH_MC_SAVE_PENDING)return status;
        checks->subphase=1;
    }
    status=save_poll(card,&result,&done,frontier);
    if(status!=GH_MC_SAVE_PENDING||!done)return status;
    switch(checks->subphase){
    case 1:
        if(result)return save_fail(frontier,0x6e78,GH_MC_SAVE_IO);
        status=save_queue(gh_memory_card_request_save_record(card,save->previous_record.pointer),0x35e0,frontier);
        break;
    case 2:
        if(result!=GH_MC_SAVE_RECORD_BYTES)return save_fail(frontier,0x6fc8,GH_MC_SAVE_IO);
        status=save_queue(gh_memory_card_request_save_close(card),0x3300,frontier);
        break;
    case 3:
        if(result)return save_fail(frontier,0x6f38,GH_MC_SAVE_IO);
        if(memcmp(save->current_record.pointer,save->previous_record.pointer,GH_MC_SAVE_RECORD_BYTES))
            return save_fail(frontier,0x6ce0,GH_MC_SAVE_IO);
        select_phase(checks,13);
        return GH_MC_SAVE_PENDING;
    default:
        return GH_MC_SAVE_ARGUMENT;
    }
    if(status==GH_MC_SAVE_PENDING)++checks->subphase;
    return status;
}

gh_mc_save_status gh_mc_control_save_00003f78(gh_mc_save_control *save,gh_mc_control_state *control,
    gh_mc_save_task_state *task,const gh_mc_task_bindings *bindings,gh_mc_load_menu_memory *memory,
    const gh_mc_save_providers *providers,uint32_t *frontier)
{
    if(!save||!control||!task||!bindings||!bindings->card||!bindings->work||!memory||!providers||!frontier||
       task->phase!=2||!task->common.ui_initialized||!task->current_record.pointer||!task->previous_record.pointer)
        return GH_MC_SAVE_ARGUMENT;
    *frontier=0;
    gh_mc_task_state *common=&task->common;
    gh_mc_boot_checks *checks=&common->checks;
    if(checks->done)return GH_MC_SAVE_COMPLETE;
    const uint32_t phase=checks->phase;
    gh_mc_save_status status=GH_MC_SAVE_PENDING;
    switch(phase){
    case 0:case 1:case 2: {
        if(!phase&&!checks->subphase)clear_progress(control);
        gh_mc_boot_error scan=gh_mc_save_scan_00003f78(&save->scan,checks,bindings->card,memory,
            &save->free_clusters,frontier);
        if(scan){
            if(!*frontier)*frontier=0x3f78;
            return scan==GH_MC_BOOT_IO?GH_MC_SAVE_IO:GH_MC_SAVE_UNSUPPORTED;
        }
        break;
    }
    case 6:case 7:case 8:
        status=save_select(save,control,common,bindings,providers,frontier);
        break;
    case 10:
        status=save_write_step(save,task,bindings->card,providers,frontier);
        if(status==GH_MC_SAVE_PENDING)save_progress(control,0);
        break;
    case 11:case 13:
        status=save_icon_step(save,task,bindings->card,frontier);
        if(status==GH_MC_SAVE_PENDING&&checks->phase==phase)save_progress(control,phase==11?1:5);
        break;
    case 12:
        status=save_verify_step(save,task,bindings->card,frontier);
        if(status==GH_MC_SAVE_PENDING&&checks->phase==phase)save_progress(control,4);
        break;
    case 14: {
        if(!checks->subphase){
            uint8_t *record=(uint8_t*)checks->icon_work.pointer+save->selected_slot*GH_MC_SAVE_HEADER_BYTES;
            memcpy(record,task->current_record.pointer,GH_MC_SAVE_HEADER_BYTES);
            checks->present_mask|=UINT32_C(1)<<save->selected_slot;
            if(title_list_rows(control,&save->scan,common,bindings)!=GH_ID_OK)
                return save_fail(frontier,0x8058,GH_MC_SAVE_UNSUPPORTED);
            memory->first_slot=memory->selected_row=-1;
            checks->frame=90;
            clear_progress(control);
            checks->mode=10;
            bindings->work->flags|=4;
            checks->subphase=1;
        }
        uint64_t pressed;
        memcpy(&pressed,(const uint8_t*)bindings->work+0x1a0,sizeof(pressed));
        if(checks->frame&&!(checks->frame<80&&(pressed&UINT64_C(0x8030000000))))--checks->frame;
        else {checks->done=1;save->complete=1;}
        break;
    }
    case 15:
        return save_fail(frontier,0x64f8,GH_MC_SAVE_UNSUPPORTED);
    default:
        return save_fail(frontier,0x3f78,GH_MC_SAVE_UNSUPPORTED);
    }
    if(status!=GH_MC_SAVE_PENDING)return status;
    if(title_select_display(save->choice,control,&save->scan,common,bindings)!=GH_ID_OK)
        return save_fail(frontier,0x72c8,GH_MC_SAVE_UNSUPPORTED);
    return checks->done?GH_MC_SAVE_COMPLETE:GH_MC_SAVE_PENDING;
}
