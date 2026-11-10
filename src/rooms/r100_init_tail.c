#include "r100_init_tail.h"
#include <string.h>
static gh_r100_init_status fail(gh_r100_init_tail*s,uint32_t pc,int rel,gh_r100_init_status e){s->frontier=pc;s->frontier_relative=(uint8_t)rel;return e;}
gh_r100_init_status gh_r100_init_tail_step(gh_r100_init_tail*s,const gh_r100_init_tail_bindings*b){
 static const uint32_t position[4][4]={{0xc058f5c3,0x3fb33333,0x418dd70a,0x3f800000},{0xc170f5c3,0x3fb33333,0xc0f28f5c,0x3f800000},{0xbf947ae1,0x4007ae14,0xc210b852,0x3f800000},{0x410cf5c3,0x3fb33333,0xc1cf999a,0x3f800000}};
 static const uint32_t init_call[]={0x7a4,0x828,0x8b0,0x938},position_call[]={0x7f8,0x87c,0x904,0x98c};
 if(!s||!b||!b->work)return GH_R100_INIT_ARGUMENT;if(s->complete)return GH_R100_INIT_OK;
 if(!b->save)return fail(s,0x1fa6c0,0,GH_R100_INIT_UNSUPPORTED);
 uint32_t f=0;gh_r100_init_status e;
 if(s->stage==0){if(b->save->bytes){if(b->save->size<0x12)return fail(s,0x1fa6c0,0,GH_R100_INIT_BOUNDS);unsigned x=b->save->bytes[0x10]|(unsigned)b->save->bytes[0x11]<<8;int n=x<32768?(int)x:(int)x-65536;if(!n&&(b->work->flags59c&0x40000))n=1;s->clear_num=(int16_t)n;}else s->clear_num=0;s->stage=1;}
 if(s->stage==1){if(s->clear_num){s->complete=1;s->stage=15;return GH_R100_INIT_OK;}if(b->save->bytes&&b->save->size<0x20)return fail(s,0x1fa678,0,GH_R100_INIT_BOUNDS);s->difficulty=b->save->bytes?b->save->bytes[0x1f]:1;s->tutorial_required=s->difficulty!=2;if(!s->tutorial_required){s->complete=1;s->stage=15;return GH_R100_INIT_OK;}s->stage=2;}
 if(s->stage==2){if(!b->tutorial_room_init_00147028)return fail(s,0x147028,0,GH_R100_INIT_UNSUPPORTED);e=b->tutorial_room_init_00147028(b->context,&f);if(e)return fail(s,f?f:0x147028,0,e);s->stage=3;}
 if(s->stage==3){if(!b->load_wait_001fe8a0)return fail(s,0x1fe8a0,0,GH_R100_INIT_UNSUPPORTED);e=b->load_wait_001fe8a0(b->context,0x3eb,1,&f);if(e)return fail(s,f?f:0x1fe8a0,0,e);s->stage=4;}
 while(s->stage<16){unsigned index=(s->stage-4)/3,phase=(s->stage-4)%3;
  if(phase==0){if(!b->create_object_0030fa98)return fail(s,0x30fa98,0,GH_R100_INIT_UNSUPPORTED);gh_r100_om_view object={0};e=b->create_object_0030fa98(b->context,0x3eb,0xffff,&object,&f);if(e)return fail(s,f?f:0x30fa98,0,e);if(!object.bytes){if(object.identity||object.size)return fail(s,0x30fa98,0,GH_R100_INIT_BOUNDS);s->stage+=3;continue;}if(!object.identity||object.size<0x601)return fail(s,0x30fa98,0,GH_R100_INIT_BOUNDS);s->pending_object=object;s->stage++;}
  else if(phase==1){if(!b->virtual_init44)return fail(s,init_call[index],1,GH_R100_INIT_UNSUPPORTED);e=b->virtual_init44(b->context,&s->pending_object,&f);if(e)return fail(s,f?f:init_call[index],!f,e);s->stage++;}
  else {if(!b->virtual_set_position74)return fail(s,position_call[index],1,GH_R100_INIT_UNSUPPORTED);float p[4];memcpy(p,position[index],sizeof(p));e=b->virtual_set_position74(b->context,&s->pending_object,p,&f);if(e)return fail(s,f?f:position_call[index],!f,e);s->pending_object.bytes[0x600]=(uint8_t)index;s->pending_object=(gh_r100_om_view){0};s->stage++;}
 }
 s->complete=1;s->frontier=0;s->frontier_relative=0;return GH_R100_INIT_OK;
}
