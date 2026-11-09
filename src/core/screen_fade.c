/* Actual307208/307388/3076E0/307718/307760; MITupstreamclassFADEtype
 * attribution LICENSE.upstream. Evidence analysis/screen-fade-evidence.json. */
#include "screen_fade.h"
#include <string.h>
_Static_assert(sizeof(gh_screen_fade)==0x28,"classFADEsize");
_Static_assert(offsetof(gh_screen_fade,time)==0x18,"timeoffset");
_Static_assert(offsetof(gh_screen_fade,state)==0x1c,"stateoffset");
_Static_assert(offsetof(gh_screen_fade,ot_min)==0x20,"OToffset");
gh_fade_error gh_screen_fade_start_00307208(gh_screen_fade*s,const gh_screen_fade_context*c,uint8_t kind,uint8_t state,uint32_t type,uint32_t from,uint32_t to,uint16_t frames){
 static const uint64_t alpha[]={UINT64_C(0x8000000044),UINT64_C(0x8000000029),UINT64_C(0x80000000a1)};
 if(!s||!c||c->screen_width<1||c->screen_width>4096||c->gs_y_origin<0||c->gs_y_origin>4095)return GH_FADE_ARGUMENT;if(type>2)return GH_FADE_UNSUPPORTED;
 s->color_from=from;s->color_to=to;s->kind=kind;s->state=state|1;s->time=(uint16_t)((uint32_t)frames<<(c->frame_shift&31));s->count=0;s->x=(uint16_t)((0x800-c->screen_width/2)*16);s->y=(uint16_t)((uint32_t)c->gs_y_origin*16);s->w=0x2000;s->h=0x1c00;s->alpha=alpha[type];s->ot_min=7;s->ot_max=1;return GH_FADE_OK;
}
void gh_screen_fade_kill_003076e0(gh_screen_fade*s){if(s)s->state=0;}
uint8_t gh_screen_fade_status_00307718(const gh_screen_fade*s){return s?s->state:0;}
void gh_screen_fade_set_ot_00307760(gh_screen_fade*s,uint32_t a,uint32_t b){if(s){s->ot_min=a;s->ot_max=b;}}
gh_fade_error gh_screen_fade_step_00307388(gh_screen_fade*s,const gh_screen_fade_context*c,gh_fade_draw*out){
 uint32_t color=0;unsigned i;if(!s||!c||!out)return GH_FADE_ARGUMENT;memset(out,0,sizeof(*out));if((c->input_mask_5e0&0x20000)||!(s->state&1))return GH_FADE_OK;
 if(s->count==s->time){if(s->state&2)s->state|=4;else s->state=0;color=s->color_to;}
 else{if(!s->time)return GH_FADE_UNSUPPORTED;for(i=0;i<4;i++){int from=(int)((s->color_from>>(i*8))&255),to=(int)((s->color_to>>(i*8))&255);uint8_t v=(uint8_t)(from+((int)s->count*(to-from))/(int)s->time);color|=(uint32_t)v<<(i*8);}s->count=(uint16_t)(s->count+1);}
 out->visible=1;out->rgba=color;out->alpha=s->alpha;out->test=(s->state&0x18)?0x30000:0x70000;out->x=s->x;out->y=s->y;out->w=s->w;out->h=s->h;out->kind=s->kind;out->ot_min=s->ot_min;out->ot_max=s->ot_max;return GH_FADE_OK;
}
