/* Actual mcport Control/Move/Sleep boot-mode2 slice, not a replacement menu.
 * Binary import/source evidence: analysis/mc-control-semantics.json. */
#include "mc_control.h"
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
    if(s->checks.mode!=2){c->first_unported=0x72c8;return GH_ID_UNSUPPORTED;}
#define DO(expr)do{error=(expr);if(error)return error;}while(0)
    s->flags&=~UINT32_C(0x02000000);
    for(i=0;i<sizeof(ids)/sizeof(ids[0]);++i)DO(message(s,b,ids[i],0x2005));
    for(i=0;i<sizeof(hide)/sizeof(hide[0]);++i)DO(display(s,b,hide[i],0));
    memset(c->progress,0,sizeof(c->progress));c->zero_b0=0;c->zero_b4=0;c->zero_b8=0;
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
