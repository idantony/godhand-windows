/* Direct source recovery of actual mcport ErrorProc@BD8 and its initial
 * SetDisp error cases. Evidence: analysis/mc-error-semantics.json. */
#include "mc_error.h"
#include <string.h>
static uint32_t buttons(const gh_boot_work *work,size_t offset)
{uint32_t n;memcpy(&n,(const unsigned char*)work+offset,4);return n;}
static void clear_progress(gh_mc_control_state *c)
{memset(c->progress,0,sizeof(c->progress));c->zero_b0=0;c->zero_b4=0;c->zero_b8=0;}
static int sound(gh_mc_error_state *e,gh_mc_sound_dispatch play,void *context,int id)
{
    const gh_mc_sound_request request={0,id,-1,-1,0,0};
    if(!play){e->first_unported=0x2cb8a0;return 0;}
    e->sound_error=play(context,&request);
    if(e->sound_error){e->first_unported=0x2cb8a0;return 0;}
    return 1;
}
gh_mc_boot_error gh_mc_error_process_00000bd8(gh_mc_error_state *e,gh_mc_control_state *c,
    gh_mc_task_state *s,const gh_mc_task_bindings *b,gh_mc_sound_dispatch play,void *context)
{
    uint32_t press,repeat;
    if(!e || !c || !s || !b || !b->work || s->checks.phase!=3)return GH_MC_BOOT_ARGUMENT;
    e->first_unported=0;e->sound_error=0;
    press=buttons(b->work,0x1a0);repeat=buttons(b->work,0x1b0);
    if(s->checks.subphase==0){s->checks.subphase=1;e->choice=0;}
    if(s->checks.subphase==1) {
        if(s->checks.error==1 || s->checks.error==2 || s->checks.error==10) {
            clear_progress(c);s->checks.mode=s->checks.error==1?0x12:s->checks.error==2?0x14:0x19;
        }
        if(press&0x30000000) {
            if(!sound(e,play,context,0x160))return GH_MC_BOOT_UNSUPPORTED;
            s->checks.subphase=2;
        }
        return GH_MC_BOOT_OK;
    }
    if(s->checks.subphase!=2)return GH_MC_BOOT_OK;
    clear_progress(c);s->checks.mode=0x17;
    if(repeat&0x0c000000) {
        if(!sound(e,play,context,0x161))return GH_MC_BOOT_UNSUPPORTED;
        e->choice=(uint8_t)(e->choice!=1);return GH_MC_BOOT_OK;
    }
    if(press&0x10000000) {
        if(e->choice==1) {
            if(!sound(e,play,context,0x160))return GH_MC_BOOT_UNSUPPORTED;
            s->checks.done=1;return GH_MC_BOOT_OK;
        }
        if(!sound(e,play,context,0x15e))return GH_MC_BOOT_UNSUPPORTED;
        s->checks.frame=0;
    } else if(press&0x20000000) {
        if(!sound(e,play,context,0x15e))return GH_MC_BOOT_UNSUPPORTED;
        s->checks.frame=0;e->choice=0;
    } else return GH_MC_BOOT_OK;
    s->checks.phase=0;s->checks.subphase=0;return GH_MC_BOOT_OK;
}
static gh_id_error get(gh_mc_task_state *s,const gh_mc_task_bindings *b,int id,gh_id_entry **out)
{return gh_id_base_get_work_002aa410(b->id_runtime,&s->id_base,id,out,0);}
static gh_id_error show(gh_mc_task_state *s,const gh_mc_task_bindings *b,int id,int display)
{
    gh_id_entry *entry;gh_id_error error=get(s,b,id,&entry);if(error)return error;
    if(display)entry->flags&=~UINT32_C(0x20000000);else entry->flags|=UINT32_C(0x20000000);
    return gh_id_base_set_family_display(b->id_runtime,&s->id_base,id,display);
}
static gh_id_error message(gh_mc_task_state *s,const gh_mc_task_bindings *b,int id,uint16_t code)
{gh_id_entry *entry;gh_id_error error=get(s,b,id,&entry);if(error)return error;entry->message=code;return show(s,b,id,1);}
static gh_id_error half_y(gh_id_entry *entry,int change)
{
    float x=entry->pos_y*2.0f;int32_t n;
    if(!(x>=-16777214 && x<=16777214))return GH_ID_UNSUPPORTED;
    n=(int32_t)x;if((float)n!=x)return GH_ID_UNSUPPORTED;
    entry->pos_y=(float)(n+change)*0.5f;return GH_ID_OK;
}
gh_id_error gh_mc_error_set_display(gh_mc_error_state *e,gh_mc_control_state *c,
    gh_mc_task_state *s,const gh_mc_task_bindings *b)
{
    static const int text[]={0x55,0x56,0x57,0x58,0x74,0x75};
    static const int hide[]={0x40,0x41,0x42,0x43,0x71,0x50,5,0,1};
    gh_id_error error;gh_id_entry *entry;unsigned i,alpha;uint16_t code;
    if(!e || !c || !s || !b || !b->id_runtime || !s->ui_initialized)return GH_ID_ARGUMENT;
    e->first_unported=0;
    if(!(s->flags&0x40000000))return GH_ID_OK;
    switch(s->checks.mode){case 0x12:code=0x201f;break;case 0x14:code=0x2020;break;case 0x19:code=0x2021;break;case 0x17:code=0x201e;break;default:e->first_unported=0x72c8;return GH_ID_UNSUPPORTED;}
#define DO(expr)do{error=(expr);if(error)return error;}while(0)
    s->flags&=~UINT32_C(0x02000000);
    for(i=0;i<sizeof(text)/sizeof(text[0]);++i)DO(message(s,b,text[i],0x2005));
    for(i=0;i<sizeof(hide)/sizeof(hide[0]);++i)DO(show(s,b,hide[i],0));
    DO(show(s,b,0x40,1));DO(message(s,b,0x41,code));s->flags|=0x02000000;
    if(s->checks.mode==0x17) {
        gh_id_entry *cursor,*target;
        DO(message(s,b,0x55,0x2003));DO(message(s,b,0x56,0x2004));DO(show(s,b,0x50,1));
        DO(get(s,b,0x50,&cursor));DO(get(s,b,e->choice==0?0x52:0x51,&target));
        cursor->pos_x=target->pos_x;cursor->pos_y=target->pos_y;
    }
    DO(get(s,b,0,&entry));DO(half_y(entry,-1));if(entry->pos_y<75)entry->pos_y=81;
    DO(get(s,b,1,&entry));DO(half_y(entry,1));if(entry->pos_y>342)entry->pos_y=336;
    DO(get(s,b,0xd,&entry));alpha=(entry->color_base>>24)+3;if(alpha>18)alpha=18;
    entry->color_base=(entry->color_base&0x00ffffff)|(uint32_t)alpha<<24;
#undef DO
    return GH_ID_OK;
}
