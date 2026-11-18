/* Readable original waiting-state Move for verified R100 OM classes.
 * Patched SLUS_215.03; semantic host pointers replace raw644/634/414. */
#include "game_om_frame.h"
#include "game_scene_at_geometry.h"
#include "game_om_fragments.h"
#include "game_actor_matrix.h"
#include "game_actor_axis_rotation.h"
#include "game_actor_two_bone.h"
#include "game_om041c.h"
#include "game_model_texture_scroll.h"
#include "game_actor_model_nodes.h"
#include "game_om303.h"
#include "game_om311.h"
#include "game_om307.h"
#include "game_om310.h"
#include "game_om322.h"
#include "game_om323.h"
#include "game_om346.h"
#include "game_om35d.h"
#include "game_om382.h"
#include "game_event_player_reset.h"
#include "game_actor_external_parent.h"
#include "frame_state.h"
#include "game_om_reaction.h"
#include "game_ray_query.h"
#include "game_player_targets.h"
#include "game_action_button_set.h"
#include "id_layout_rotation.h"
#include "sdk_rotation_trig.h"
#include "game_hitarea_frame.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static float number(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static void sh(uint8_t*p,uint16_t n){memcpy(p,&n,2);}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static gh_object_step_status fail(gh_om_frame_state*s,uint32_t pc,uint32_t*f,gh_object_step_status e){s->frontier=pc;if(f)*f=pc;return e;}
uint32_t gh_om_half_bits_00156488(uint16_t n){uint32_t fraction=n&0x3ff,exponent=(n>>10)&31;if(!exponent){exponent=1;if(!fraction)exponent=UINT32_C(0xffffff90);else{do{fraction<<=1;exponent--;}while(!(fraction&0x400));fraction&=0x3ff;}}return ((uint32_t)n&0x8000)<<16|((exponent+0x70)<<23)|(fraction<<13);}
static int tutorial(const gh_om_object*o){return o&&o->runtime_created&&o->runtime_kind==0x3eb&&!o->source.bytes;}
static int catapult(const gh_om_object *o)
{
 return o&&o->runtime_created&&o->runtime_kind==0x3bd&&!o->source.bytes&&
     o->view.size==0x640&&o->view.original_vtable214==0x428c20;
}
static int mount(const gh_om_object *o){return o&&o->runtime_created&&o->runtime_kind==0x353&&!o->source.bytes;}
static int paper(const gh_om_object*o){return o&&o->runtime_created&&o->runtime_kind==0x376&&!o->source.bytes;}
static int devil_projectile(const gh_om_object *o)
{
 return o&&o->runtime_created&&o->runtime_kind==0x3be&&!o->source.bytes&&o->view.original_vtable214==0x4290c8;
}
static int em0f_throw(const gh_om_object *o)
{
 return o&&o->runtime_created&&o->runtime_kind==0x364&&!o->source.bytes&&o->view.original_vtable214==0x4282f0;
}
static int weapon(const gh_om_object*o){return o&&(o->source.type==0x382||paper(o)||(o->runtime_created&&o->runtime_kind==0x363&&!o->source.bytes));}
static int r101_item(const gh_om_object *o)
{
 if(!o)return 0;
 if(o->runtime_created)return o->runtime_kind==0x3d6||o->runtime_kind==0x3c3||o->runtime_kind==0x3c4||o->runtime_kind==0x3c8||o->runtime_kind==0x3d0||o->runtime_kind==0x3d1||o->runtime_kind==0x3d7||o->runtime_kind==0x3df||o->runtime_kind==0x3e0||o->runtime_kind==0x3e1||o->runtime_kind==0x3e3||
     (o->runtime_kind==0x3da&&o->view.bytes&&o->view.size>=0x690&&(word(o->view.bytes+0x560)==0x905||word(o->view.bytes+0x560)==0xa5d));
 return o->source.type>=0x3d3&&o->source.type<=0x3d5;
}
static int fragment_cylinders(const gh_om_object *o)
{
 return o&&(o->source.type==0x308||o->source.type==0x30b||o->source.type==0x30c||o->source.type==0x331||o->source.type==0x332);
}
static int fragments(const gh_om_object*o){return o&&(gh_om_fragment_profile_for(o->source.type)||o->source.type==0x303||o->source.type==0x304||o->source.type==0x305||o->source.type==0x317||o->source.type==0x318||o->source.type==0x31a||o->source.type==0x320||o->source.type==0x338||o->source.type==0x339);}
static int simple(const gh_om_object*o){return o&&(o->source.type==0x30a||o->source.type==0x310||o->source.type==0x33f||o->source.type==0x309||o->source.type==0x325);}
static int plain(const gh_om_object*o){return o&&(o->source.type==0x30e||o->source.type==0x324||o->source.type==0x326);}
static int segments(const gh_om_object*o){return o&&(o->source.type==0x322||o->source.type==0x323||o->source.type==0x346);}
static int door(const gh_om_object*o){return o&&(o->source.type==0x300||o->source.type==0x313||o->source.type==0x330||o->source.type==0x333||o->source.type==0x354);}
static gh_om_init_state*common(const gh_om_frame_state*s){gh_om_object*o=s->object;if(!o)return NULL;if(tutorial(o)||mount(o)||catapult(o))return o->initialization;if(devil_projectile(o))return &((gh_om3be_state *)o->initialization)->common;if(em0f_throw(o))return &((gh_om364_state *)o->initialization)->common;if(weapon(o))return &((gh_om382_state*)o->initialization)->common;if(door(o)||r101_item(o)||o->source.type==0x351||o->source.type==0x355||o->source.type==0x356)return o->initialization;if(o->source.type==0x41c||o->source.type==0x357)return &((gh_om041c_state*)o->initialization)->common;if(o->source.type==0x35d)return &((gh_om35d_state*)o->initialization)->common;if(o->source.type==0x307)return &((gh_om307_state*)o->initialization)->common;if(o->source.type==0x311)return &((gh_om311_state*)o->initialization)->common;if(o->source.type==0x310)return &((gh_om310_state*)o->initialization)->base.common;if(o->source.type==0x322)return &((gh_om322_state*)o->initialization)->common;if(o->source.type==0x323)return &((gh_om323_state*)o->initialization)->common;if(o->source.type==0x346)return &((gh_om346_state*)o->initialization)->common;return fragments(o)||simple(o)||plain(o)||fragment_cylinders(o)?&((gh_om303_state*)o->initialization)->common:NULL;}
static uint32_t state_entry(uint32_t kind,unsigned index){
 static const uint32_t entries[][5]={{0x307,0x1896e0,0x1897b8,0x189818,0x18a110},{0x30a,0x18c4a8,0x18c520,0x18c580,0x18c8a8},{0x310,0x18f5d0,0x18f618,0x18f858,0x18fbf0},{0x33f,0x19f4b0,0x19f528,0x19f588,0x19f8b0},{0x309,0x18b6a0,0x18b718,0x18b778,0x18baa0},{0x325,0x1993c8,0x199440,0x1994a0,0x1997c8},{0x30e,0x1bfc88,0x1bfce0,0x1bfd28,0x1bff48},{0x324,0x1bfc88,0x1bfce0,0x1bfd28,0x1bff48},{0x326,0x1bfc88,0x1bfce0,0x1bfd28,0x1bff48},{0x322,0x1968a0,0x196978,0x1969d8,0x197188},{0x323,0x197ec8,0x197fa0,0x198000,0x1987b0},{0x346,0x1a1690,0x1a1768,0x1a17c8,0x1a20a8}};
 for(unsigned i=0;i<sizeof(entries)/sizeof(entries[0]);i++)if(entries[i][0]==kind)return index<4?entries[i][index+1]:0;return 0;
}
gh_object_step_status gh_om_frame_contacts_001bd430(gh_om_frame_state*s,uint32_t*f){
 if(f)*f=0;if(!s||!s->bound)return GH_OBJSTEP_ARGUMENT;gh_om_init_state*c=common(s);gh_om_object*o=s->object;
 if(!door(o)||!c||!c->geometry_state.complete||c->geometry_state.object!=o||!o->view.bytes||o->view.size<0x648)return fail(s,0x1bd430,f,GH_OBJSTEP_UNSUPPORTED);
 uint8_t*p=o->view.bytes;struct gh_scene_dynamic_box*box=c->geometry_state.box;
 if(!box){sh(p+0x630,0);put(p+0x634,0);s->contact634=NULL;return GH_OBJSTEP_OK;}
 gh_scene_dynamic_box_view view;if(gh_scene_dynamic_box_view_at(c->geometry_state.owner,box,&view))return fail(s,0x1bd43c,f,GH_OBJSTEP_CALLEE);
 uint32_t count=word(view.bytes+0x54);if(count>8)return fail(s,0x1bd454,f,GH_OBJSTEP_UNSUPPORTED);
 const gh_title_player_view*selected=NULL;
 if(!count){sh(p+0x630,0);put(p+0x634,0);s->contact634=NULL;}
 else for(unsigned i=0;i<count;i++){
  const gh_title_player_view*a=NULL;if(gh_scene_dynamic_sphere_contact_at(c->geometry_state.owner,box,i,&a)||!a||!a->bytes||a->size<0x300)return fail(s,0x1bd468,f,GH_OBJSTEP_UNSUPPORTED);
  unsigned kind=half(a->bytes+0x2fe);if((kind>=0x100&&kind<0x1ff)||(kind>=0x200&&kind<0x300)){selected=a;break;}
 }
 if(selected){sh(p+0x630,(uint16_t)(half(p+0x630)+1));put(p+0x634,0);s->contact634=selected;}
 if(gh_scene_dynamic_contacts_clear_001bd430(c->geometry_state.owner,box))return fail(s,0x1bd4e8,f,GH_OBJSTEP_CALLEE);return GH_OBJSTEP_OK;
}
static gh_object_step_status bind(gh_om_frame_state*s,const gh_om_factory*f,const gh_object_move_request*q,const gh_boot_work*w,uint32_t*pc){
 if(!s||!f||!f->bound||!q||!w||f->bindings.work!=w)return GH_OBJSTEP_ARGUMENT;
 gh_om_object*o=NULL;for(size_t i=0;i<f->count;i++)if(f->objects[i].handle==q->handle){o=f->objects+i;break;}
 void*registered=NULL;if(!o||!o->registered||!o->initialization||!o->view.bytes||o->view.size<0x600||q->actor.bytes!=o->view.bytes||q->actor.size!=o->view.size||q->this_delta||gh_room_object_lookup(f->bindings.pools,q->handle,GH_ROOM_OBJECT_OM,f,&registered)||registered!=o)return fail(s,0x30f15c,pc,GH_OBJSTEP_ARGUMENT);
 if(s->bound)return s->factory==f&&s->object==o&&s->handle==q->handle&&s->entry==q->original_entry?GH_OBJSTEP_OK:fail(s,0x30f15c,pc,GH_OBJSTEP_ARGUMENT);
 const gh_om_fragment_profile *fragment=gh_om_fragment_profile_for(o->source.type);
 uint32_t expected=fragment?fragment->move:o->source.type==0x300?0x186928:o->source.type==0x313?0x191f30:o->source.type==0x330?0x19c518:(o->source.type==0x331||o->source.type==0x332)?0x19c848:o->source.type==0x333?0x19d538:o->source.type==0x355?0x1a85b8:o->source.type==0x356?0x1a8778:o->source.type==0x351?0x1a6188:o->source.type==0x354?0x1a82d8:o->source.type==0x303?0x187c18:o->source.type==0x304?0x187fe8:o->source.type==0x305?0x188c88:o->source.type==0x317?0x192ef8:o->source.type==0x318?0x193180:o->source.type==0x31a?0x1933f8:o->source.type==0x320?0x195e58:o->source.type==0x307?0x1895d8:o->source.type==0x30a?0x18c370:o->source.type==0x310?0x18f488:o->source.type==0x311?0x190948:o->source.type==0x338?0x19e150:o->source.type==0x339?0x19e3d8:o->source.type==0x33f?0x19f378:o->source.type==0x309?0x18b568:o->source.type==0x308?0x18a870:o->source.type==0x30b?0x18d160:o->source.type==0x30c?0x18df18:o->source.type==0x30e?0x18ee38:o->source.type==0x324?0x198f48:o->source.type==0x325?0x199290:o->source.type==0x326?0x199f60:o->source.type==0x322?0x1965f8:o->source.type==0x323?0x197c20:o->source.type==0x346?0x1a13e8:r101_item(o)?0x1c0378:weapon(o)?0x1c9860:o->source.type==0x35d?0x1aedb0:o->source.type==0x41c?0x17ba28:o->source.type==0x357?0x1a8a70:tutorial(o)?0x1bf518:mount(o)?0x1a7880:catapult(o)?0x1b8990:devil_projectile(o)?0x1b92f0:em0f_throw(o)?0x1b0f70:0;
 if(!expected||q->original_entry!=expected)return fail(s,q->original_entry,pc,GH_OBJSTEP_UNSUPPORTED);
 /* Bind before first original Move; constructor1BBEB8 established these.
  * A nonzero guest pointer can never be silently adopted as host634. */
 if(door(o)&&(half(o->view.bytes+0x630)||word(o->view.bytes+0x634)))return fail(s,0x1bd430,pc,GH_OBJSTEP_UNSUPPORTED);
 s->factory=f;s->object=o;s->handle=q->handle;s->entry=q->original_entry;s->bound=1;return GH_OBJSTEP_OK;
}
static gh_object_step_status model_rate(gh_om_frame_state*s,float rate,uint32_t*f){
 gh_om_init_state*c=common(s);if(!c||!c->model.ready||c->model.root.bytes!=s->object->view.bytes)return fail(s,0x14e4d0,f,GH_OBJSTEP_UNSUPPORTED);
 uint32_t pc;
 gh_actor_model_status status=gh_game_model_texture_scroll_0014e4d0(&c->model,rate,&s->node_index,&pc);
 return !status?GH_OBJSTEP_OK:fail(s,pc,f,status==GH_ACTOR_MODEL_BOUNDS?GH_OBJSTEP_CALLEE:GH_OBJSTEP_UNSUPPORTED);
}
/* 1BF5E0 submits the tutorial action to the same priority list as other prompts. */
static gh_object_step_status tutorial_prompt(gh_om_frame_state *s, uint32_t *frontier)
{
 const gh_title_player_view *player=s->environment.current_player;
 const gh_title_player_view *tutorial_view=&s->object->view;
 if(!s->environment_bound||!s->environment.player_binding_known||!player||
    !player->bytes||player->size<0x108||!player->alias_f0||!tutorial_view->alias_f0)
  return fail(s,0x2db6b0,frontier,GH_OBJSTEP_UNSUPPORTED);
 float from[3],to[3];
 memcpy(from,player->alias_f0,sizeof(from));
 memcpy(to,tutorial_view->alias_f0,sizeof(to));
 const float x=add(from[0],-to[0]),y=add(from[1],-to[1]),z=add(from[2],-to[2]);
 const float distance=add(add(mul(x,x),mul(y,y)),mul(z,z));
 if(!isfinite(distance))return fail(s,0x1f7e28,frontier,GH_OBJSTEP_UNSUPPORTED);
 if(distance>=6.25f)return GH_OBJSTEP_OK;

 const gh_om_init_state *init=common(s);
 if(!init||!init->bindings)return fail(s,0x2da2d0,frontier,GH_OBJSTEP_UNSUPPORTED);
 const float arc=0x1.657184p+0f; /* ELF 3FB2B8C2 */
 int inside;
 uint32_t pc=0;
 if(gh_player_target_inside_arc_002da2d0(init->bindings->elf,init->bindings->elf_bytes,
    from,to,number(player->bytes+0x104),arc,&inside,&pc))
  return fail(s,pc?pc:0x2da2d0,frontier,GH_OBJSTEP_UNSUPPORTED);
 if(!inside)return GH_OBJSTEP_OK;
 if(!s->runtime_bound||!s->runtime.action_buttons)
  return fail(s,0x1f7858,frontier,GH_OBJSTEP_UNSUPPORTED);

 const gh_action_set_request request={.priority=4,.life=0x28,.callback_id=0x1bf7d8,.argument=s->object};
 int emitted;
 unsigned index;
 if(gh_action_button_set_001f7858(s->runtime.action_buttons,&request,&emitted,&index,&pc))
  return fail(s,pc?pc:0x1f7858,frontier,GH_OBJSTEP_CALLEE);
 return GH_OBJSTEP_OK;
}

/* Actual1BF518/1BF5E0 tutorial rotation and gated player proximity. */
static gh_object_step_status special3eb(gh_om_frame_state*s,gh_static_transform_policy policy,uint32_t*f){
 uint8_t*p=s->object->view.bytes;if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x1bf5e0,f,GH_OBJSTEP_UNSUPPORTED);
 if(p[0x2f4])return fail(s,0x1bf518,f,GH_OBJSTEP_UNSUPPORTED);
 if(!s->special_phase){uint32_t bits=0x3dd67750;float increment;memcpy(&increment,&bits,4);float angle=add(number(p+0x104),increment),wrapped;if(gh_id_angle_reference_001f7d48(angle,&wrapped))return fail(s,0x1f7d48,f,GH_OBJSTEP_UNSUPPORTED);memcpy(p+0x104,&wrapped,4);put(p+0x5b8,word(p+0x5b8)|0x10u);s->special_phase=1;}
 if(s->special_phase==1){
  if(!p[0x2f5]){
   gh_object_step_status status=tutorial_prompt(s,f);
   if(status)return status;
  }
  else if(p[0x2f5]==2){uint16_t timer=(uint16_t)half(p+0x602);if(!timer||timer>=0x8000)p[0x2f5]=0;sh(p+0x602,(uint16_t)(timer-1u));}
  s->special_phase=2;
 }return GH_OBJSTEP_OK;
}
/* Actual35D state0 body, preserving ordered prefixes on retry. */
static gh_object_step_status special35d(gh_om_frame_state*s,gh_static_transform_policy policy,uint32_t*f){
 gh_om_init_state*c=common(s);uint8_t*p=s->object->view.bytes;
 if(!s->runtime_bound||s->runtime.init!=c->bindings->init||s->runtime.solids!=c->bindings->solids)return fail(s,0x292f50,f,GH_OBJSTEP_ARGUMENT);
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x292f50,f,GH_OBJSTEP_UNSUPPORTED);
 if(!s->special_phase){
  if(p[0x2f4])return fail(s,p[0x2f4]==1?0x1af870:p[0x2f4]==2?0x1afbb0:0x1af230,f,GH_OBJSTEP_UNSUPPORTED);
  s->runtime_rate=mul(number(s->runtime.init->enemies+0x548),*s->runtime.canonical_rate);if(!isfinite(s->runtime_rate))return fail(s,0x292f50,f,GH_OBJSTEP_UNSUPPORTED);
  memcpy(p+0x5a8,&s->runtime_rate,4);float rate=s->runtime_rate;if(half(p+0x434)&4)rate=-rate;memcpy(p+0x444,&rate,4);s->special_phase=1;
 }
 if(s->special_phase==1){
  if(p[0x2f5])return fail(s,p[0x2f5]==1?0x1af630:p[0x2f5]==2?0x1af680:0x1b0838,f,GH_OBJSTEP_UNSUPPORTED);
  if(!s->environment_bound||!s->environment.player_binding_known||!s->environment.current_player||!s->environment.current_player->alias_f0||!s->object->view.alias_f0)return fail(s,0x2db6b0,f,GH_OBJSTEP_UNSUPPORTED);
  const uint8_t*a=s->object->view.alias_f0,*b=s->environment.current_player->alias_f0;float d[3];for(unsigned k=0;k<3;k++){d[k]=add(number(a+4*k),-number(b+4*k));if(!isfinite(d[k]))return fail(s,0x1af230,f,GH_OBJSTEP_UNSUPPORTED);}
  if(p[0x2f6]==0){p[0x2f7]=0;p[0x2f6]=1;}
  if(p[0x2f6]!=1)return fail(s,0x1af230,f,GH_OBJSTEP_UNSUPPORTED);
  if(fabsf(d[1])<1.5f&&add(mul(d[0],d[0]),mul(d[2],d[2]))<25){
   if(!word(p+0x980)){uint32_t raw;if(gh_game_mt_next_0030f348(&s->runtime.init->random,&raw))return fail(s,0x31d000,f,GH_OBJSTEP_UNSUPPORTED);uint32_t bits=gh_frame_unit_bits_0030f518_word(raw);float u;memcpy(&u,&bits,4);uint32_t selection=u<=.33f?1:u<=.66f?2:3;put(p+0x980,selection);sh(p+0x984,selection==1?0:selection==2?150:300);}
   p[0x2f7]=0;p[0x2f6]=2;
  }s->special_phase=2;
 }
 if(s->special_phase==2){
  const gh_hitarea_object*h=c->hitarea_created?&c->hitarea:NULL;if(h&&h->state!=s->runtime.init)return fail(s,0x1b7380,f,GH_OBJSTEP_ARGUMENT);
  int hit=0;uint32_t pc=0;gh_om_damage_status e=gh_om_check_damage_001b7380(&s->damage_calls[0],&s->object->view,h,s->runtime.init->heat+0x10,policy,&hit,&pc);
  if(e)return fail(s,pc?pc:0x1b7380,f,GH_OBJSTEP_CALLEE);
  if(hit){unsigned hp=half(p+0x54a);if(!hp||hp>=0x8000){p[0x2f4]=1;p[0x2f5]=p[0x2f6]=p[0x2f7]=0;put(p+0x5b0,word(p+0x5b0)|1u);}else return fail(s,0x1aeea8,f,GH_OBJSTEP_UNSUPPORTED);}
  s->special_phase=3;
 }
 if(s->special_phase==3){
  int current=(int16_t)half(p+0x54a),maximum=(int16_t)half(p+0x548);if(!maximum)return fail(s,0x1b00b8,f,GH_OBJSTEP_UNSUPPORTED);volatile float ratio=(float)current/(float)maximum;
  if(c->model.part_count>1){const gh_actor_part*part;if(gh_actor_model_part_at(&c->model,1,&part)||!part->alias_d0)return fail(s,0x1b0110,f,GH_OBJSTEP_CALLEE);float y=add(number(p+0x60c),-mul(ratio,.4f));memcpy(part->alias_d0+4,&y,4);}
  float scale=add(mul(ratio,.5f),.5f);for(unsigned i=0;i<c->model.script.mesh_count;i++){const gh_actor_node_packets*packet;if(gh_actor_model_packet_at(&c->model,i,&packet))return fail(s,0x1b71a0,f,GH_OBJSTEP_CALLEE);if(!packet->node[0x40c]){put(packet->node+0x3a0,0x3f800000);memcpy(packet->node+0x3a4,&scale,4);memcpy(packet->node+0x3a8,&scale,4);}}
  s->special_phase=4;
 }
 if(s->special_phase==4){if(p[0xaf0]==1)return fail(s,0x1b7ee0,f,GH_OBJSTEP_UNSUPPORTED);if((int16_t)half(p+0x986)>0)return fail(s,0x1aef40,f,GH_OBJSTEP_UNSUPPORTED);s->special_phase=5;}
 if(s->special_phase==5){memcpy(p+0x5a8,&s->runtime_rate,4);gh_object_step_status e=model_rate(s,s->runtime_rate,f);if(e)return e;s->node_index=0;s->special_phase=6;}
 if(s->special_phase==6){int hit=0;uint32_t pc=0;gh_solid_unit_status e=gh_collision_solid_step_resume_001348e8(s->runtime.solids,&s->object->view,policy,&s->solid_frame,&hit,&pc);if(e)return fail(s,pc?pc:0x1348e8,f,GH_OBJSTEP_CALLEE);s->special_phase=7;}
 return GH_OBJSTEP_OK;
}
static void hit_transition(gh_om_frame_state*s,unsigned index,const gh_hitarea_object*h){
 uint8_t*p=s->object->view.bytes;uint32_t kind=s->object->source.type;int dead=0;unsigned state=1;
 if(kind==0x307||kind==0x322||kind==0x323||kind==0x346){size_t hp=kind==0x307?0xcd2:kind==0x346?0xd98:0x9d8,selector=kind==0x307?0xcd6:kind==0x346?0xd9c:0x9da;uint32_t next=half(p+hp+index*2)-word(h->bytes+0x4c);sh(p+hp+index*2,(uint16_t)next);dead=(next&0x8000)!=0;if(dead)p[selector]=(uint8_t)index;}
 else if(plain(s->object))dead=1;
 else if(kind!=0x310){unsigned hp=half(p+0x54a);dead=hp>=0x8000||hp==0;}
 if(dead){state=2;put(p+0x5b0,word(p+0x5b0)|1u);}p[0x2f4]=(uint8_t)state;p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
}

/* Both 30E and 324 use the same original state table at 004297B8.
 * Separate resumable steps retain the order of the effect, sound and cleanup. */
static gh_object_step_status plain_break(gh_om_frame_state *s, const gh_boot_work *w,
 gh_static_transform_policy policy, uint32_t *f) {
 gh_om_object *o = s->object;
 gh_om_init_state *c = common(s);
 uint8_t *p = o->view.bytes;
 if (o->view.size < 0x606 || !c) return fail(s, 0x1bfd28, f, GH_OBJSTEP_ARGUMENT);
 if (policy != GH_STATIC_BINARY32_REFERENCE) return fail(s, 0x1bfd28, f, GH_OBJSTEP_UNSUPPORTED);
 uint32_t pc = 0;
 if (p[0x2f5] == 0) {
  if (!s->special_phase) {
   if (!s->runtime_bound || !s->runtime.break_effect || !o->view.alias_f0)
    return fail(s, 0x306528, f, GH_OBJSTEP_UNSUPPORTED);
   float yaw;
   if (gh_game_angle_001f8180(number(p + 0x5c0), number(p + 0x5c8), &yaw,
       GH_CAMERA_NATIVE_BINARY32_REFERENCE, &pc))
    return fail(s, pc ? pc : 0x1f8180, f, GH_OBJSTEP_UNSUPPORTED);
   uint32_t position[3], yaw_bits;
   memcpy(position, o->view.alias_f0, sizeof(position));
   memcpy(&yaw_bits, &yaw, sizeof(yaw_bits));
   gh_object_step_status status = s->runtime.break_effect(s->runtime.context,
       &c->model, position, yaw_bits, &pc);
   if (status) return fail(s, pc ? pc : 0x306528, f, status);
   s->special_phase = 1;
  }
  if (s->special_phase == 1) {
   if (!s->runtime.break_sound) return fail(s, 0x2cba48, f, GH_OBJSTEP_UNSUPPORTED);
   s->break_sound = (gh_motion_sound_request){.model=&c->model, .actor=&o->view,
     .bank=2, .program=10};
   uint32_t handle = 0;
   gh_actor_motion_status status = s->runtime.break_sound(s->runtime.context,
       &s->break_sound, &handle, &pc);
   if (status) return fail(s, pc ? pc : 0x2cba48, f,
       status == GH_ACTOR_MOTION_UNSUPPORTED ? GH_OBJSTEP_UNSUPPORTED : GH_OBJSTEP_CALLEE);
   s->special_phase = 2;
  }
  if (s->special_phase == 2) {
   if (word(p + 0x600)) return fail(s, 0x1fd668, f, GH_OBJSTEP_UNSUPPORTED);
   if (c->hitarea_created && gh_hitarea_object_set_active_001fd668(&c->hitarea, 0, &pc))
    return fail(s, pc ? pc : 0x1fd668, f, GH_OBJSTEP_CALLEE);
   s->special_phase = 3;
  }
  if (s->special_phase == 3) {
   if (gh_actor_model_node_mask_flags(&c->model, 0, 0x380, UINT32_MAX, 0x2001u))
    return fail(s, 0x14b6f8, f, GH_OBJSTEP_CALLEE);
   put(p + 0x250, word(p + 0x250) | 0x10000u);
   if (!(word(p + 0x5b8) & 0x200u) && !(word((const uint8_t *)w + 0x5e4) & 0x1000000u))
    put(p + 0x5b8, word(p + 0x5b8) & ~0x40u);
   s->special_phase = 4;
  }
  if (s->special_phase == 4) {
   uint64_t key = (uint64_t)word(p + 0x53c) << 32 | word(p + 0x538);
   if (gh_room_state_mark_object_broken_002becb0(s->factory->bindings.room_state, key))
    return fail(s, 0x2becb0, f, GH_OBJSTEP_CALLEE);
   s->special_phase = 5;
  }
  if (s->special_phase == 5) {
   uint64_t key = (uint64_t)word(p + 0x544) << 32 | word(p + 0x540);
   if (!s->runtime.break_collision) return fail(s, 0x1331b8, f, GH_OBJSTEP_UNSUPPORTED);
   gh_object_step_status status = s->runtime.break_collision(s->runtime.context, key, &pc);
   if (status) return fail(s, pc ? pc : 0x1331b8, f, status);
   s->special_phase = 6;
  }
  if (s->special_phase == 6) {
   if (gh_collision_solid_release_unit_001347e8(s->runtime.solids, &o->view, &pc))
    return fail(s, pc ? pc : 0x1347e8, f, GH_OBJSTEP_CALLEE);
   const uint32_t flags = word(p + 0x5b0), item = word(p + 0x560);
   s->special_phase = (flags & 0x12u) || item == 0xffffu || item == 0x9c3u ? 8 : 7;
   put(p + 0x5b0, flags | 0x10u);
  }
  if (s->special_phase == 7) return fail(s, 0x2923e8, f, GH_OBJSTEP_UNSUPPORTED);
  put(p + 0x5b0, (word(p + 0x5b0) & ~8u) | 2u);
  p[0x2f5] = 1;
  p[0x2f6] = p[0x2f7] = 0;
  sh(p + 0x604, 30);
 }

 switch (p[0x2f5]) {
  case 1:
  case 2: {
   uint16_t next = (uint16_t)(half(p + 0x604) - 1u);
   int remaining = next < 0x8000u ? (int)next : (int)next - 0x10000;
   sh(p + 0x604, next);
   if (p[0x2f5] == 2) {
    volatile float opacity = (float)remaining / 15.0f;
    float value = opacity;
    memcpy(p + 0x24c, &value, sizeof(value));
   }
   if (remaining > 0) return GH_OBJSTEP_OK;
   if (p[0x2f5] == 1) sh(p + 0x604, 15);
   p[0x2f5]++;
   p[0x2f6] = p[0x2f7] = 0;
   break;
  }
  case 3:
   p[0x2f4] = 3;
   p[0x2f5] = p[0x2f6] = p[0x2f7] = 0;
   break;
 }
 return GH_OBJSTEP_OK;
}

static gh_object_step_status remove_object(gh_om_frame_state *s, uint32_t *f) {
 uint8_t *flags = s->object->view.bytes + 0x250;
 if (word(flags) & 0x80000000u) return GH_OBJSTEP_OK;
 if (!s->runtime_bound || !s->runtime.remove_sound)
  return fail(s, 0x2cbe18, f, GH_OBJSTEP_UNSUPPORTED);
 uint32_t pc = 0;
 gh_object_step_status status = s->runtime.remove_sound(s->runtime.context, &common(s)->model, &pc);
 if (status) return fail(s, pc ? pc : 0x2cbe18, f, status);
 /* The supported vtable +4C targets the empty base 0014E928. */
 put(flags, word(flags) | 0x80000002u);
 return GH_OBJSTEP_OK;
}

static float fragment_random(gh_game_mt *random,int unit)
{
 uint32_t word=0;
 gh_game_mt_next_0030f348(random,&word);
 uint32_t bits=unit?gh_frame_unit_bits_0030f518_word(word):
  gh_frame_jitter_bits_0031d020_word(word);
 float value;
 memcpy(&value,&bits,4);
 return value;
}

static gh_object_step_status fragment305_spawn_001c3540(gh_om_frame_state *state,
 uint32_t *frontier)
{
 gh_om_init_state *owner=common(state);
 gh_om303_state *fragments=(gh_om303_state*)state->object->initialization;
 gh_actor_model *model=&owner->model;
 uint8_t *actor=state->object->view.bytes;
 if(!state->runtime.fragment_atan_coeff||!state->runtime.init||word(actor+0x600)||
    model->part_count!=actor[0x2b4]||actor[0x60c]!=6||fragments->segments!=actor+0x620)
  return fail(state,0x1c3540,frontier,GH_OBJSTEP_UNSUPPORTED);
 const unsigned count=model->part_count<6?model->part_count:6;
 uint32_t entry=0;
 if(owner->hitarea_created&&gh_hitarea_object_set_active_001fd668(&owner->hitarea,0,&entry))
  return fail(state,entry?entry:0x1fd668,frontier,GH_OBJSTEP_CALLEE);
 for(unsigned i=0;i<model->script.mesh_count;i++){
  const gh_actor_node_packets *packet;
  if(gh_actor_model_packet_at(model,i,&packet))return fail(state,0x14b6f8,frontier,GH_OBJSTEP_CALLEE);
  uint8_t *node=packet->node;
  uint32_t flags=word(node+0x380);
  if(node[0x40c]==0)put(node+0x380,flags|0x2001u);
  else if(node[0x40c]==0x40)put(node+0x380,(flags&~1u)|0x2000u);
 }
 put(actor+0x250,word(actor+0x250)|0x10000u);
 for(unsigned i=1;i<count;i++){
  uint8_t *part=model->parts[i].bytes;
  put(part+0x154,word(part+0x154)&~0x18u);
 }
 if(gh_actor_model_update_hierarchy_00149eb8(model,GH_STATIC_BINARY32_REFERENCE))
  return fail(state,model->frontier?model->frontier:0x149eb8,frontier,GH_OBJSTEP_CALLEE);
 float position[6][3],euler[6][3];
 for(unsigned i=0;i<count;i++){
  const uint8_t *part=model->parts[i].bytes;
  float matrix[16];
  memcpy(matrix,part+0x80,sizeof(matrix));
  memcpy(position[i],matrix+12,sizeof(position[i]));
  if(gh_actor_matrix_extract_euler_00147c88(matrix,state->runtime.fragment_atan_coeff,
     euler[i],GH_STATIC_BINARY32_REFERENCE,&entry))
   return fail(state,entry?entry:0x147c88,frontier,GH_OBJSTEP_UNSUPPORTED);
 }
 gh_game_mt *random=&state->runtime.init->random;
 if(random->index>624)return fail(state,0x31cfe0,frontier,GH_OBJSTEP_ARGUMENT);
 float kick[3]={mul(number(actor+0x5c0),.01f),
  add(mul(number(actor+0x5c4),.01f),.099999994f),mul(number(actor+0x5c8),.01f)};
 for(unsigned i=0;i<count;i++){
  uint8_t *part=model->parts[i].bytes,*record=fragments->segments+i*0x60;
  const float velocity_x=fragment_random(random,0);
  const float velocity_y=fragment_random(random,1);
  const float velocity_z=fragment_random(random,0);
  const float y=add(add(position[i][1],-number(state->object->view.alias_f0+4)),-2.0f);
  const float attenuation=y<0.0f?1.0f:y<2.0f?mul(add(2.0f,-y),.5f):0.0f;
  const float velocity[3]={mul(add(kick[0],mul(velocity_x,.2f)),attenuation),
   mul(add(kick[1],mul(velocity_y,.4f)),attenuation),
   mul(add(kick[2],mul(velocity_z,.2f)),attenuation)};
  float spin[3];
  for(unsigned axis=0;axis<3;axis++)spin[axis]=mul(fragment_random(random,0),.1f);
  memcpy(record,position[i],12);
  memcpy(record+0x10,euler[i],12);
  memcpy(record+0x20,velocity,12);
  memcpy(record+0x30,spin,12);
  const uint32_t scale[3]={0x3f800000,0x3f800000,0x3f800000};
  memcpy(record+0x40,scale,sizeof(scale));
  put(record+0x50,0); /* Attached part matrix uses the same typed index. */
  record[0x54]=1;
  put(record+0x58,0xbca3d70a);
  put(part+0x154,word(part+0x154)|0x18u);
 }
 return GH_OBJSTEP_OK;
}

static gh_object_step_status fragment_tick_records(gh_om_frame_state *s,const gh_boot_work *work,
 gh_actor_model *model,uint8_t *segments,unsigned first,unsigned maximum,uint32_t *f)
{
 enum { MAX_FRAGMENTS = 20 };
 if(maximum>MAX_FRAGMENTS)return fail(s,0x173900,f,GH_OBJSTEP_ARGUMENT);
 uint8_t *actor=s->object->view.bytes;
 const unsigned count=model->part_count<maximum?model->part_count:maximum;
 const float rate=mul(number(s->runtime.init->enemies+0x548),number((const uint8_t *)work+0x574));
 if(actor[0x2b4]!=model->part_count||!isfinite(rate)||count>MAX_FRAGMENTS)
  return fail(s,0x292f50,f,GH_OBJSTEP_UNSUPPORTED);
 uint8_t records[MAX_FRAGMENTS][0x60];
 float matrices[MAX_FRAGMENTS][16];
 memset(matrices,0,sizeof(matrices));
 uint32_t frontier=0;
 for(unsigned i=first;i<count;i++){
  uint8_t *record=records[i];
  memcpy(record,segments+i*0x60,0x60);
  if(record[0x54]!=1)continue;
  for(unsigned j=0;j<16;j+=4){
   const float position=add(number(record+j),mul(number(record+0x20+j),rate));
   const float euler=add(number(record+0x10+j),mul(number(record+0x30+j),rate));
   memcpy(record+j,&position,4);
   memcpy(record+0x10+j,&euler,4);
  }
  float angles[4],position[3],scale[3];
  memcpy(angles,record+0x10,16);memcpy(position,record,12);memcpy(scale,record+0x40,12);
  if(gh_actor_euler_adjust_001f7da0(angles,angles,GH_STATIC_BINARY32_REFERENCE,&frontier)||
     gh_actor_matrix_compose_00147a48(position,angles,scale,0,matrices[i],GH_STATIC_BINARY32_REFERENCE,&frontier))
   return fail(s,frontier?frontier:0x147bf8,f,GH_OBJSTEP_CALLEE);
  memcpy(record+0x10,angles,16);
  const float y=add(number(record+0x24),mul(number(record+0x58),rate));
  memcpy(record+0x24,&y,4);
  if(y< -1000.0f)record[0x54]=0;
 }
 for(unsigned i=first;i<count;i++){
  uint8_t *live=segments+i*0x60;
  if(live[0x54]!=1)continue;
  memcpy(live,records[i],0x60);
  memcpy(model->parts[i].bytes+0x80,matrices[i],sizeof(matrices[i]));
 }
 return GH_OBJSTEP_OK;
}

static gh_object_step_status fragment_tick(gh_om_frame_state *s,const gh_boot_work *work,
 unsigned first,unsigned maximum,uint32_t *f)
{
 gh_om303_state *fragment=(gh_om303_state *)s->object->initialization;
 return fragment_tick_records(s,work,&fragment->common.model,fragment->segments,first,maximum,f);
}

static gh_object_step_status fragment305_break(gh_om_frame_state *s,const gh_boot_work *work,
 gh_static_transform_policy policy,uint32_t *f)
{
 gh_om_object *object=s->object;
 gh_om_init_state *owner=common(s);
 uint8_t *actor=object->view.bytes;
 uint32_t entry=0;
 if(policy!=GH_STATIC_BINARY32_REFERENCE||!s->runtime_bound||!owner||!object->view.alias_f0)
  return fail(s,0x1c2da0,f,GH_OBJSTEP_UNSUPPORTED);
 if(!actor[0x2f5]&&s->special_phase!=8){
  if(!s->special_phase){
   if(word(actor+0x5ec)!=0xc9||!s->runtime.break_effect)
    return fail(s,0x306838,f,GH_OBJSTEP_UNSUPPORTED);
   uint32_t position[3];
   memcpy(position,object->view.alias_f0,sizeof(position));
   gh_object_step_status status=s->runtime.break_effect(s->runtime.context,&owner->model,position,0,&entry);
   if(status)return fail(s,entry?entry:0x306838,f,status);
   s->special_phase=1;
  }
  if(s->special_phase==1){
   const int16_t program=(int16_t)half(actor+0x5f6);
   if(program>=0){
    if(!s->runtime.break_sound)return fail(s,0x2cba48,f,GH_OBJSTEP_UNSUPPORTED);
    s->break_sound=(gh_motion_sound_request){.model=&owner->model,.actor=&object->view,.bank=2,.program=program};
    uint32_t handle=0;
    gh_actor_motion_status status=s->runtime.break_sound(s->runtime.context,&s->break_sound,&handle,&entry);
    if(status)return fail(s,entry?entry:0x2cba48,f,
      status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
   }
   s->special_phase=2;
  }
  if(s->special_phase==2){
   gh_object_step_status status=fragment305_spawn_001c3540(s,f);
   if(status)return status;
   s->special_phase=3;
  }
  if(s->special_phase==3){
   if(!(word(actor+0x5b8)&0x200u)&&!(word((const uint8_t *)work+0x5e4)&0x1000000u))
    put(actor+0x5b8,word(actor+0x5b8)&~0x40u);
   const uint64_t key=(uint64_t)word(actor+0x53c)<<32|word(actor+0x538);
   if(gh_room_state_mark_object_broken_002becb0(s->factory->bindings.room_state,key))
    return fail(s,0x2becb0,f,GH_OBJSTEP_CALLEE);
   s->special_phase=4;
  }
  if(s->special_phase==4){
   if(!s->runtime.break_collision)return fail(s,0x1331b8,f,GH_OBJSTEP_UNSUPPORTED);
   const uint64_t key=(uint64_t)word(actor+0x544)<<32|word(actor+0x540);
   gh_object_step_status status=s->runtime.break_collision(s->runtime.context,key,&entry);
   if(status)return fail(s,entry?entry:0x1331b8,f,status);
   s->special_phase=5;
  }
  if(s->special_phase==5){
   if(gh_collision_solid_release_unit_001347e8(s->runtime.solids,&object->view,&entry))
    return fail(s,entry?entry:0x1347e8,f,GH_OBJSTEP_CALLEE);
   s->special_phase=6;
  }
  if(s->special_phase==6){
   const uint32_t flags=word(actor+0x5b0),item=word(actor+0x560);
   if(!(flags&0x12u)&&item!=0xffffu&&item!=0x9c3u)
    return fail(s,0x2923e8,f,GH_OBJSTEP_UNSUPPORTED);
   put(actor+0x5b0,(flags&~8u)|0x12u);
   s->special_phase=7;
  }
  if(word(actor+0x604))return fail(s,0x1fd718,f,GH_OBJSTEP_UNSUPPORTED);
  actor[0x2f5]=1;
  actor[0x2f6]=actor[0x2f7]=0;
  sh(actor+0x60e,30);
 }
 if(s->special_phase!=8){
  switch(actor[0x2f5]){
  case 1:case 2:{
   const uint16_t bits=(uint16_t)(half(actor+0x60e)-1u);
   const int remaining=(int16_t)bits;
   sh(actor+0x60e,bits);
   if(actor[0x2f5]==2){
    const volatile float opacity=(float)remaining/15.0f;
    const float value=opacity;
    memcpy(actor+0x24c,&value,4);
   }
   if(remaining<=0){
    if(actor[0x2f5]==1)sh(actor+0x60e,15);
    actor[0x2f5]++;
    actor[0x2f6]=actor[0x2f7]=0;
   }
   break;
  }
  case 3:actor[0x2f4]=3;actor[0x2f5]=actor[0x2f6]=actor[0x2f7]=0;break;
  }
  s->special_phase=8;
 }
 gh_om303_state *fragment=(gh_om303_state*)s->object->initialization;
 if(actor[0x60c]!=6||fragment->segments!=actor+0x620)
  return fail(s,0x292f50,f,GH_OBJSTEP_UNSUPPORTED);
 return fragment_tick(s,work,0,6,f);
}

static gh_object_step_status box_fragments_spawn(gh_om_frame_state *s,uint32_t *f)
{
 gh_om303_state *fragment=(gh_om303_state*)s->object->initialization;
 gh_actor_model *model=&fragment->common.model;
 uint8_t *actor=s->object->view.bytes;
 uint32_t pc=0;
 const int kind304=s->object->source.type==0x304;
 const int kind33f=s->object->source.type==0x33f;
 const int kind331=s->object->source.type==0x331||s->object->source.type==0x332;
 const unsigned first=kind304?0:1, limit=(kind304||kind331)?6:kind33f?13:9;
 const uint32_t entry=kind304?0x188798:kind331?0x19cfe0:kind33f?0x19fb20:
  s->object->source.type==0x309?0x18bd10:0x18cb18;
 const unsigned count=model->part_count<limit?model->part_count:limit;
 /* 188798, 18CB18 and 19FB20 share the mesh/part changes but have different
  * starting indices and record storage. */
 if(!model->part_count||model->part_count!=actor[0x2b4]||
    model->script.mesh_count<2||!s->runtime.fragment_atan_coeff||
    fragment->segments!=actor+(kind304?0x610:0x660)||
    word(actor+(kind304?0x604:0x650)))
  return fail(s,entry,f,GH_OBJSTEP_UNSUPPORTED);
 if(fragment->common.hitarea_created&&
    gh_hitarea_object_set_active_001fd668(&fragment->common.hitarea,0,&pc))
  return fail(s,pc?pc:0x1fd668,f,GH_OBJSTEP_CALLEE);
 if(kind331){
  /* 1B7280/1B7300 select the authored layer across the mesh list. */
  for(unsigned i=0;i<model->script.mesh_count;i++){
   const gh_actor_node_packets *packet;
   if(gh_actor_model_packet_at(model,i,&packet))return fail(s,0x14b6f8,f,GH_OBJSTEP_CALLEE);
   uint8_t *node=packet->node;
   const uint32_t flags=word(node+0x380);
   if(!node[0x40c])put(node+0x380,flags|0x2001u);
   else if(node[0x40c]==0x40)put(node+0x380,(flags&~1u)|0x2000u);
  }
 }else if(gh_actor_model_node_mask_flags(model,1,0x380,UINT32_MAX,0x2001u)||
          gh_actor_model_node_mask_flags(model,0,0x380,~1u,0x2000u))
  return fail(s,0x14b6f8,f,GH_OBJSTEP_CALLEE);
 put(actor+0x250,word(actor+0x250)|0x10000u);
 for(unsigned i=1;i<count;i++){
  uint8_t *part=model->parts[i].bytes;
  put(part+0x154,word(part+0x154)&~0x18u);
 }
 if(gh_actor_model_update_hierarchy_00149eb8(model,GH_STATIC_BINARY32_REFERENCE))
  return fail(s,model->frontier?model->frontier:0x149eb8,f,GH_OBJSTEP_CALLEE);
 float position[13][3],angles[13][3];
 for(unsigned i=first;i<count;i++){
  float matrix[16];
  memcpy(matrix,model->parts[i].bytes+0x80,sizeof(matrix));
  memcpy(position[i],matrix+12,sizeof(position[i]));
  if(gh_actor_matrix_extract_euler_00147c88(matrix,s->runtime.fragment_atan_coeff,
      angles[i],GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x147c88,f,GH_OBJSTEP_UNSUPPORTED);
 }
 gh_game_mt *random=&s->runtime.init->random;
 if(random->index>624)return fail(s,0x31cfe0,f,GH_OBJSTEP_ARGUMENT);
 const float kick[3]={mul(number(actor+0x5c0),.01f),
     add(mul(number(actor+0x5c4),.01f),.099999994f),mul(number(actor+0x5c8),.01f)};
 for(unsigned i=first;i<count;i++){
  uint8_t *part=model->parts[i].bytes,*record=fragment->segments+(size_t)i*0x60;
  float velocity[3],spin[3];
  velocity[0]=add(kick[0],mul(fragment_random(random,0),.2f));
  velocity[1]=add(kick[1],mul(fragment_random(random,1),kind331?.2f:.4f));
  velocity[2]=add(kick[2],mul(fragment_random(random,0),.2f));
  for(unsigned j=0;j<3;j++)spin[j]=mul(fragment_random(random,0),.1f);
  memcpy(record,position[i],12);
  memcpy(record+0x10,angles[i],12);
  memcpy(record+0x20,velocity,12);
  memcpy(record+0x30,spin,12);
  for(unsigned j=0;j<4;j++)put(record+0x40+j*4,0x3f800000u);
  put(record+0x50,0); /* Original part destination retains its typed identity. */
  record[0x54]=1;
  put(record+0x58,0xbca3d70au);
  put(part+0x154,word(part+0x154)|0x18u);
 }
 return GH_OBJSTEP_OK;
}

/* 292650 continues the parameter-zero lottery after the pity gate. */
static gh_object_step_status gong_drop_choice_00292650(
    gh_om_frame_state *s,unsigned rank,unsigned difficulty,uint32_t *frontier)
{
 gh_game_init_state *init=s->runtime.init;
 gh_game_mt random=init->random;
 const float primary=fragment_random(&random,1);
 uint16_t kind;
 if(primary>=.9f){
  /* 2927DC can use a live nearby enemy before final selection. */
  return fail(s,0x291010,frontier,GH_OBJSTEP_UNSUPPORTED);
 }else if(primary>=.85f){
  kind=0x3e2;
 }else if(primary>=.5f){
  const float secondary=fragment_random(&random,1);
  kind=secondary>=.95f?0x3de:secondary>=.75f?0x3e1:0x3e0;
 }else{
  const float secondary=fragment_random(&random,1);
  kind=secondary>=.95f?0x3d0:secondary>=.85f?0x3d1:secondary>=.55f?0x3c8:0x3c1;
  if(kind==0x3d0&&rank>=3){
   uint32_t draw;
   if(gh_game_mt_next_0030f348(&random,&draw))
    return fail(s,0x31cfe0,frontier,GH_OBJSTEP_CALLEE);
   if(draw&1u)kind=0x3d1;
  }
 }
 const float final=fragment_random(&random,1);
 if(init->continue_count>=4&&difficulty!=2){
  if(final>=.8f){
   const float recovery=fragment_random(&random,1);
   kind=recovery>=.95f?0x3d0:recovery>=.85f?0x3d1:recovery>=.55f?0x3c8:0x3c1;
  }
 }else if(final>=.8f){
  kind=final>=.95f?0x3d8:0x3d7;
 }
 if(word(init->enemies+0x5a8)==kind){
  uint32_t draw;
  if(gh_game_mt_next_0030f348(&random,&draw))
   return fail(s,0x31cfe0,frontier,GH_OBJSTEP_CALLEE);
  kind=(draw&1u)?0x3c1:0x3c8;
 }
 init->random=random;
 s->box_item_drop.selected_kind=kind;
 s->special_phase=7;
 return GH_OBJSTEP_OK;
}

static gh_object_step_status fragment30a_drop_002923e8(gh_om_frame_state *s,uint32_t *f)
{
 const gh_om_initializer_bindings *initializer=s->factory->bindings.initializer;
 gh_game_init_state *init=s->runtime.init;
 if(!initializer||!initializer->save.bytes||initializer->save.size<GH_CORE_SAVE_BYTES)
  return fail(s,0x1fa010,f,GH_OBJSTEP_UNSUPPORTED);
 /* 3BF160 is the immutable 1FA010 rank table in the reference ELF.
  * Current rank is derived from the same active save record every call. */
 const int16_t score=(int16_t)half(initializer->save.bytes+0x1c);
 const unsigned rank=score<1000?1:score<3000?2:score<4000?3:5;
 const unsigned difficulty=initializer->save.bytes[0x1f];
 const uint32_t modulus=difficulty!=2&&(!difficulty||init->continue_count>=4)?40u:50u;
 const uint32_t pity=word(init->enemies+0x534);
 if(pity>INT32_MAX/10u)return fail(s,0x292498,f,GH_OBJSTEP_UNSUPPORTED);
 static const float factor[]={0,1,.9f,.8f,.8f,.65f};
 const float threshold=mul((float)(pity*10u),factor[rank]);
 if(!isfinite(threshold)||threshold>=2147483648.0f||init->random.index>624)
  return fail(s,0x292498,f,GH_OBJSTEP_UNSUPPORTED);
 uint32_t draw;
 if(gh_game_mt_next_0030f348(&init->random,&draw))
  return fail(s,0x31cfe0,f,GH_OBJSTEP_CALLEE);
 if((uint32_t)threshold<draw%modulus){
  put(init->enemies+0x534,pity+1);
  if(gh_game_mt_next_0030f348(&init->random,&draw))
   return fail(s,0x31cfe0,f,GH_OBJSTEP_CALLEE);
  if(draw&3u)return GH_OBJSTEP_OK;
  /* Original param4=0 selects runtime item 3D6 on this branch. */
  s->special_phase=7;
  return GH_OBJSTEP_OK;
 }
 put(init->enemies+0x534,0);
 if(initializer->work->room==0x104&&s->object->source.type==0x332)
  return gong_drop_choice_00292650(s,rank,difficulty,f);
 s->special_phase=9;
 return fail(s,0x31d000,f,GH_OBJSTEP_UNSUPPORTED);
}

static gh_object_step_status box_break(gh_om_frame_state *s,
 const gh_boot_work *work,gh_static_transform_policy policy,uint32_t *f)
{
 gh_om_object *object=s->object;
 gh_om303_state *fragment=(gh_om303_state*)object->initialization;
 gh_om_init_state *owner=&fragment->common;
 uint8_t *actor=object->view.bytes;
 uint32_t pc=0;
 const int kind304=object->source.type==0x304;
 const int kind33f=object->source.type==0x33f;
 const int r104_om332=work->room==0x104&&object->source.type==0x332&&
     object->view.original_vtable214==0x425c78u;
 const int kind331=object->source.type==0x331||r104_om332;
 const int kind309=object->source.type==0x309;
 const uint32_t entry=kind304?0x1881f0:kind331?0x19ca58:kind309?0x18b778:kind33f?0x19f588:0x18c580;
 uint8_t *timer=actor+(kind304?0x600:0x654);
 const int arena_box=work->room==0x103&&
    ((object->source.type==0x30a&&object->view.original_vtable214==0x4238d8)||
     (kind33f&&object->view.original_vtable214==0x4267f8));
 const int west_box=work->room==0x100&&object->source.type==0x30a&&
    object->view.original_vtable214==0x4238d8;
 if((work->room!=((kind331||kind309)?0x102:0x101)&&!arena_box&&!west_box&&!r104_om332)||policy!=GH_STATIC_BINARY32_REFERENCE||!s->runtime_bound||
    !s->runtime.init||fragment->segments!=actor+(kind304?0x610:0x660)||!object->view.alias_f0)
  return fail(s,entry,f,GH_OBJSTEP_UNSUPPORTED);
 if(!actor[0x2f5]&&s->special_phase!=8){
  if(!s->special_phase){
   if(!s->runtime.break_effect)return fail(s,0x306838,f,GH_OBJSTEP_UNSUPPORTED);
   uint32_t position[3];
   memcpy(position,object->view.alias_f0,sizeof(position));
   gh_object_step_status status=s->runtime.break_effect(s->runtime.context,&owner->model,position,0,&pc);
   if(status)return fail(s,pc?pc:0x306838,f,status);
   s->special_phase=1;
  }
  if(s->special_phase==1){
   if(!s->runtime.break_sound)return fail(s,0x2cba48,f,GH_OBJSTEP_UNSUPPORTED);
   s->break_sound=(gh_motion_sound_request){.model=&owner->model,.actor=&object->view,
    .bank=2,.program=kind331?0x10:0};
   uint32_t handle=0;
   gh_actor_motion_status status=s->runtime.break_sound(s->runtime.context,&s->break_sound,&handle,&pc);
   if(status)return fail(s,pc?pc:0x2cba48,f,
       status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
   s->special_phase=2;
  }
  if(s->special_phase==2){
   gh_object_step_status status=box_fragments_spawn(s,f);
   if(status)return status;
   s->special_phase=3;
  }
  if(s->special_phase==3){
   if(!(word(actor+0x5b8)&0x200u)&&!(word((const uint8_t *)work+0x5e4)&0x1000000u))
    put(actor+0x5b8,word(actor+0x5b8)&~0x40u);
   const uint64_t key=(uint64_t)word(actor+0x53c)<<32|word(actor+0x538);
   if(gh_room_state_mark_object_broken_002becb0(s->factory->bindings.room_state,key))
    return fail(s,0x2becb0,f,GH_OBJSTEP_CALLEE);
   s->special_phase=4;
  }
  if(s->special_phase==4){
   if(!s->runtime.break_collision)return fail(s,0x1331b8,f,GH_OBJSTEP_UNSUPPORTED);
   const uint64_t key=(uint64_t)word(actor+0x544)<<32|word(actor+0x540);
   gh_object_step_status status=s->runtime.break_collision(s->runtime.context,key,&pc);
   if(status)return fail(s,pc?pc:0x1331b8,f,status);
   s->special_phase=5;
  }
  if(s->special_phase==5){
   if(gh_collision_solid_release_unit_001347e8(s->runtime.solids,&object->view,&pc))
    return fail(s,pc?pc:0x1347e8,f,GH_OBJSTEP_CALLEE);
   s->special_phase=6;
  }
  if(s->special_phase==6){
   const uint32_t flags=word(actor+0x5b0);
   const uint32_t item=word(actor+0x560);
   if(!(flags&0x12u)&&item!=0x9c3u){
    put(actor+0x5b0,flags|0x10u);
    if(item!=0xffffu){s->special_phase=10;return fail(s,0x292048,f,GH_OBJSTEP_UNSUPPORTED);}
    /* 1B7A78 leaves an unconfigured 304 without random loot. */
    if(!kind304){
     gh_object_step_status status=fragment30a_drop_002923e8(s,f);
     if(status)return status;
    }
   }
   if(s->special_phase==6)s->special_phase=11;
  }
  if(s->special_phase==7){
   if(!s->runtime.drop_item)return fail(s,0x30fa98,f,GH_OBJSTEP_UNSUPPORTED);
   gh_object_step_status status=s->runtime.drop_item(s->runtime.context,&s->box_item_drop,&object->view,&pc);
   if(status)return fail(s,pc?pc:0x30fa98,f,status);
   s->special_phase=11;
  }
  if(s->special_phase==11){
   put(actor+0x5b0,(word(actor+0x5b0)&~8u)|(kind33f?2u:0x12u));
   if(kind304){
    if(word(actor+0x608))return fail(s,0x1fd718,f,GH_OBJSTEP_UNSUPPORTED);
    if(fragment->sphere_created&&
       gh_hitarea_object_set_active_timed_001fd718(&fragment->sphere_unit,1,1,&pc))
     return fail(s,pc?pc:0x1fd718,f,GH_OBJSTEP_CALLEE);
   }
   actor[0x2f5]=1;
   actor[0x2f6]=actor[0x2f7]=0;
   sh(timer,30);
  }
  if(s->special_phase==9)return fail(s,0x31d000,f,GH_OBJSTEP_UNSUPPORTED);
  if(s->special_phase==10)return fail(s,0x292048,f,GH_OBJSTEP_UNSUPPORTED);
 }
 if(s->special_phase!=8){
  switch(actor[0x2f5]){
  case 1:case 2:{
   const uint16_t bits=(uint16_t)(half(timer)-1u);
   const int remaining=(int16_t)bits;
   sh(timer,bits);
   if(actor[0x2f5]==2){
    const volatile float opacity=(float)remaining/15.0f;
    const float value=opacity;
    memcpy(actor+0x24c,&value,4);
   }
   if(remaining<=0){
    if(actor[0x2f5]==1)sh(timer,15);
    actor[0x2f5]++;
    actor[0x2f6]=actor[0x2f7]=0;
   }
   break;
  }
  case 3:actor[0x2f4]=3;actor[0x2f5]=actor[0x2f6]=actor[0x2f7]=0;break;
  default:return fail(s,entry,f,GH_OBJSTEP_UNSUPPORTED);
  }
  s->special_phase=8;
 }
 return fragment_tick(s,work,kind304?0:1,(kind304||kind331)?6:kind33f?13:9,f);
}

/* Original grouped fragments use the ranges loaded at Init. */
static gh_object_step_status om_break_seed_part(gh_om_frame_state *s,gh_actor_model *model,
 unsigned part,const float velocity[3],const float spin[3],int update_world,uint32_t *f)
{
 uint8_t *actor=s->object->view.bytes;
 const int om346=s->object->source.type==0x346;
 if(part>=model->part_count||part>=(om346?20u:18u)||!s->runtime.fragment_atan_coeff)
  return fail(s,om346?0x1a20c8:0x18a130,f,GH_OBJSTEP_UNSUPPORTED);
 gh_actor_part *node=&model->parts[part];
 uint32_t pc=0;
 put(node->bytes+0x154,word(node->bytes+0x154)|0x18u);
 if(update_world){
  const uint8_t *parent=actor;
  if(!node->parent_is_model){
   if(node->parent<0||(unsigned)node->parent>=model->part_count)
    return fail(s,0x1501d0,f,GH_OBJSTEP_UNSUPPORTED);
   parent=model->parts[node->parent].bytes;
  }
  if(gh_actor_part_world_001501d0(node,parent,word(actor+0x250),
      GH_STATIC_BINARY32_REFERENCE,&pc))return fail(s,pc?pc:0x1501d0,f,GH_OBJSTEP_CALLEE);
 }
 float matrix[16],angles[3];
 memcpy(matrix,node->bytes+0x80,sizeof(matrix));
 if(gh_actor_matrix_extract_euler_00147c88(matrix,s->runtime.fragment_atan_coeff,
     angles,GH_STATIC_BINARY32_REFERENCE,&pc))
  return fail(s,pc?pc:0x147c88,f,GH_OBJSTEP_CALLEE);
 uint8_t *record=actor+0x610+part*0x60;
 memcpy(record,matrix+12,12);
 memcpy(record+0x10,angles,12);
 memcpy(record+0x20,velocity,12);
 memcpy(record+0x30,spin,12);
 static const float scale[]={1,1,1};
 memcpy(record+0x40,scale,12);
 put(record+0x50,0);
 record[0x54]=1;
 put(record+0x58,0xbca3d70a);
 return GH_OBJSTEP_OK;
}

static gh_object_step_status om_break_segment_group(gh_om_frame_state *s,gh_actor_model *model,
 const uint8_t *ranges,unsigned group,uint32_t *f)
{
 uint8_t *actor=s->object->view.bytes;
 const unsigned first=ranges[(group+3)*2],count=ranges[(group+3)*2+1];
 const int om346=s->object->source.type==0x346;
 if(first+count>(om346?20u:18u)||first+count>model->part_count)
  return fail(s,om346?0x1a23c0:0x18a3e8,f,GH_OBJSTEP_UNSUPPORTED);
 for(unsigned i=0;i<model->script.mesh_count;++i){
  const gh_actor_node_packets *packet;
  if(gh_actor_model_packet_at(model,i,&packet))return fail(s,0x14b6f8,f,GH_OBJSTEP_CALLEE);
  uint8_t *node=packet->node;
  uint32_t flags=word(node+0x380);
  if(node[0x40c]==group)put(node+0x380,flags|0x2001u);
  if(node[0x40c]==(group|0x40u))put(node+0x380,(flags&~1u)|0x2000u);
 }
 put(actor+0x250,word(actor+0x250)|0x10000u);
 gh_game_mt random=s->runtime.init->random;
 for(unsigned part=first;part<first+count;++part){
  float velocity[3],spin[3];
  for(unsigned axis=0;axis<3;++axis){
   const float baseline=mul(number(actor+0x5c0+axis*4),0.02f);
   velocity[axis]=add(baseline,mul(fragment_random(&random,axis==1),0.05f));
  }
  for(unsigned axis=0;axis<3;++axis)spin[axis]=mul(fragment_random(&random,0),0.08f);
  const gh_object_step_status status=om_break_seed_part(s,model,part,velocity,spin,1,f);
  if(status)return status;
 }
 s->runtime.init->random=random;
 return GH_OBJSTEP_OK;
}

static gh_object_step_status om307_sound(gh_om_frame_state *s,gh_actor_model *model,uint32_t *f)
{
 if(!s->runtime.break_sound)return fail(s,0x2cba48,f,GH_OBJSTEP_UNSUPPORTED);
 const gh_motion_sound_request sound={.model=model,.actor=&s->object->view,.bank=2,.program=4};
 uint32_t pc=0,handle;
 const gh_actor_motion_status status=s->runtime.break_sound(s->runtime.context,&sound,&handle,&pc);
 return status?fail(s,pc?pc:0x2cba48,f,status==GH_ACTOR_MOTION_UNSUPPORTED?
                     GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE):GH_OBJSTEP_OK;
}

static gh_object_step_status om307_break_00189818(gh_om_frame_state *s,
 const gh_boot_work *work,gh_static_transform_policy policy,uint32_t *f)
{
 gh_om_object *object=s->object;
 gh_om307_state *owner=(gh_om307_state *)object->initialization;
 gh_actor_model *model=&owner->common.model;
 uint8_t *p=object->view.bytes;
 const gh_om_initializer_bindings *bindings=owner->common.bindings;
 if(work->room!=0x100||object->view.original_vtable214!=0x423608||object->view.size<0xce0||
    !s->runtime_bound||!s->runtime.init||policy!=GH_STATIC_BINARY32_REFERENCE||!bindings->door_shared||
    !bindings->door_shared->ranges307_initialized||p[0xcd6]>1)
  return fail(s,0x189818,f,GH_OBJSTEP_UNSUPPORTED);
 const uint8_t *ranges=bindings->door_shared->ranges307;
 const unsigned struck=p[0xcd6];
 const unsigned phase=p[0x2f5];
 uint32_t pc=0;
 if(phase==0){
  if(!s->special_phase){
   const gh_object_step_status status=om307_sound(s,model,f);
   if(status)return status;
   s->special_phase=1;
  }
  if(s->special_phase==1){
   if(!(word(p+0x5b8)&0x200u)&&!(word((const uint8_t *)work+0x5e4)&0x1000000u))
    put(p+0x5b8,word(p+0x5b8)&~0x40u);
   const uint64_t key=(uint64_t)word(p+0x53c)<<32|word(p+0x538);
   if(gh_room_state_mark_object_broken_002becb0(s->factory->bindings.room_state,key))
    return fail(s,0x2becb0,f,GH_OBJSTEP_CALLEE);
   s->special_phase=2;
  }
  if(s->special_phase==2){
   if(!s->runtime.break_collision)return fail(s,0x1331b8,f,GH_OBJSTEP_UNSUPPORTED);
   const uint64_t key=(uint64_t)word(p+0x544)<<32|word(p+0x540);
   const gh_object_step_status status=s->runtime.break_collision(s->runtime.context,key,&pc);
   if(status)return fail(s,pc?pc:0x1331b8,f,status);
   s->special_phase=3;
  }
  if(s->special_phase==3){
   if(gh_collision_solid_release_unit_001347e8(s->runtime.solids,&object->view,&pc))
    return fail(s,pc?pc:0x1347e8,f,GH_OBJSTEP_CALLEE);
   const uint32_t flags=word(p+0x5b0),item=word(p+0x560);
   if(!(flags&0x12u)&&item!=0xffffu&&item!=0x9c3u)
    return fail(s,0x2923e8,f,GH_OBJSTEP_UNSUPPORTED);
   put(p+0x5b0,(flags&~8u)|0x12u);
   s->special_phase=4;
  }
  if(s->special_phase==4){
   /* Preserve the three draws before the group's per-part draws. */
   gh_game_mt random=s->runtime.init->random;
   float spin[3];
   for(unsigned i=0;i<3;++i)spin[i]=mul(fragment_random(&random,0),0.01f);
   s->runtime.init->random=random;
   const gh_object_step_status status=om_break_segment_group(s,model,ranges,struck,f);
   if(status)return status;
   static const float zero[3];
   if(om_break_seed_part(s,model,ranges[(1u-struck)*2],zero,spin,0,f)||
      om_break_seed_part(s,model,ranges[4],zero,spin,0,f))return GH_OBJSTEP_CALLEE;
   s->special_phase=5;
  }
  if(s->special_phase==5){
   gh_hitarea_object *broken=struck?&owner->box_b2:&owner->common.hitarea;
   const int present=struck?owner->b2_created:owner->common.hitarea_created;
   if(present&&gh_hitarea_object_set_active_001fd668(broken,0,&pc))
    return fail(s,pc?pc:0x1fd668,f,GH_OBJSTEP_CALLEE);
   s->special_phase=6;
  }
  if(s->special_phase==6){
   if(word(p+0x5ec)){
    if(word(p+0x5f0)!=1||!s->runtime.break_effect)return fail(s,0x306838,f,GH_OBJSTEP_UNSUPPORTED);
    static const uint32_t origin[3];
    const gh_object_step_status status=s->runtime.break_effect(s->runtime.context,model,origin,0,&pc);
    if(status)return fail(s,pc?pc:0x306838,f,status);
   }
   s->special_phase=10;
  }
  if(s->special_phase==10){
   const gh_object_step_status status=om307_sound(s,model,f);
   if(status)return status;
   if(owner->a_created&&gh_hitarea_object_set_active_001fd668(&owner->box_a,1,&pc))
    return fail(s,pc?pc:0x1fd668,f,GH_OBJSTEP_CALLEE);
   put(p+0x5c0,0);put(p+0x5c4,0x41400000);put(p+0x5c8,0);
   sh(p+0xcd0,18);
   p[0x2f5]=1;p[0x2f6]=p[0x2f7]=0;
   s->special_phase=7;
  }
 }
 if(s->special_phase!=12){
  if(p[0x2f5]==2){
   if(!s->special_phase){
    gh_hitarea_object *other=struck?&owner->common.hitarea:&owner->box_b2;
    const int present=struck?owner->common.hitarea_created:owner->b2_created;
    if(present&&gh_hitarea_object_set_active_001fd668(other,0,&pc))
     return fail(s,pc?pc:0x1fd668,f,GH_OBJSTEP_CALLEE);
    s->special_phase=8;
   }
   if(s->special_phase==8){
    const gh_object_step_status status=om_break_segment_group(s,model,ranges,1u-struck,f);
    if(status)return status;
    s->special_phase=9;
   }
   const gh_object_step_status status=om_break_segment_group(s,model,ranges,2,f);
   if(status)return status;
   sh(p+0xcd0,30);p[0x2f5]=3;p[0x2f6]=p[0x2f7]=0;
  }else if(p[0x2f5]>=1&&p[0x2f5]<=4){
   const unsigned step=p[0x2f5];
   const int remaining=(int16_t)(half(p+0xcd0)-1u);
   sh(p+0xcd0,(uint16_t)remaining);
   if(step==4){
    volatile float opacity=(float)remaining/15.0f;
    float value=opacity;
    memcpy(p+0x24c,&value,4);
   }
   if(remaining<=0){
    p[0x2f5]=(uint8_t)(step+1);p[0x2f6]=p[0x2f7]=0;
    if(step==3){
     sh(p+0xcd0,15);
     if(owner->a_created){
      if(gh_hitarea_shapes_abort_object(&owner->box_a,&pc)||
         gh_hitarea_object_release_empty_001fe278(&owner->box_a,&pc))
       return fail(s,pc?pc:0x1fe148,f,GH_OBJSTEP_CALLEE);
      owner->a_created=0;
     }
    }
   }
   if(step==4&&remaining<=0){s->special_phase=12;return GH_OBJSTEP_OK;}
  }else if(p[0x2f5]==5){
   p[0x2f4]=3;p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
   s->special_phase=12;
   return GH_OBJSTEP_OK;
  }
  s->special_phase=12;
 }
 return fragment_tick_records(s,work,model,p+0x610,0,18,f);
}

/* Original 1A17C8, first strike and 1E10 countdown. */
static gh_object_step_status om346_break_001a17c8(gh_om_frame_state *s,
 const gh_boot_work *work,gh_static_transform_policy policy,uint32_t *f)
{
 gh_om_object *object=s->object;
 gh_om346_state *owner=(gh_om346_state *)object->initialization;
 gh_actor_model *model=&owner->common.model;
 uint8_t *p=object->view.bytes;
 const gh_om_initializer_bindings *bindings=owner->common.bindings;
 if(work->room!=0x100||object->view.original_vtable214!=0x426e78||object->view.size<0xda0||
    !s->runtime_bound||!s->runtime.init||!bindings||!bindings->door_shared||
    !bindings->door_shared->ranges346_initialized||p[0xd9c]>1||
    policy!=GH_STATIC_BINARY32_REFERENCE)
  return fail(s,0x1a17c8,f,GH_OBJSTEP_UNSUPPORTED);
 const unsigned struck=p[0xd9c],phase=p[0x2f5];
 const uint8_t *ranges=bindings->door_shared->ranges346;
 uint32_t pc=0;
 if(phase==2){
  /* 1A1E34 releases whichever plank has not already split. */
  for(unsigned plank=0;plank<2;++plank){
   const uint32_t mask=0x80000000u>>plank;
   if(word(p+0xd90)&mask)continue;
   const gh_object_step_status status=om_break_segment_group(s,model,ranges,plank,f);
   if(status)return status;
   put(p+0xd90,word(p+0xd90)|mask);
  }
  const gh_actor_part *anchor;
  if(gh_actor_model_part_at(model,ranges[4],&anchor))
   return fail(s,0x14bfe0,f,GH_OBJSTEP_UNSUPPORTED);
  float world[16];
  memcpy(world,anchor->bytes+0x80,sizeof(world));
  for(unsigned i=ranges[10];i<(unsigned)ranges[10]+ranges[11];++i){
   const gh_actor_part *part;
   if(gh_actor_model_part_at(model,i,&part))
    return fail(s,0x14bfe0,f,GH_OBJSTEP_UNSUPPORTED);
   float local[16],result[16];
   memcpy(local,part->bytes,sizeof(local));
   if(gh_actor_matrix_product_00147548(world,local,result,policy,&pc))
    return fail(s,pc?pc:0x147548,f,GH_OBJSTEP_CALLEE);
   memcpy(part->bytes+0x80,result,sizeof(result));
  }
  const gh_object_step_status status=om_break_segment_group(s,model,ranges,2,f);
  if(status)return status;
  put(p+0xd90,word(p+0xd90)|0x20000000u);
  sh(p+0xd94,25);
  p[0x2f5]=3;p[0x2f6]=p[0x2f7]=0;
  return GH_OBJSTEP_OK;
 }
 if(phase==3){
  const int remaining=(int16_t)(half(p+0xd94)-1u);
  if(remaining<=0){
   if(owner->a_created&&gh_hitarea_object_set_active_001fd668(&owner->box_a,0,&pc))
    return fail(s,pc?pc:0x1fd668,f,GH_OBJSTEP_CALLEE);
   sh(p+0xd94,15);
   p[0x2f5]=4;p[0x2f6]=p[0x2f7]=0;
  }else sh(p+0xd94,(uint16_t)remaining);
  return GH_OBJSTEP_OK;
 }
 if(phase==4){
  const int remaining=(int16_t)(half(p+0xd94)-1u);
  sh(p+0xd94,(uint16_t)remaining);
  volatile float ratio=(float)remaining/15.0f;
  const float opacity=ratio;
  memcpy(p+0x24c,&opacity,4);
  if(remaining<=0){p[0x2f5]=5;p[0x2f6]=p[0x2f7]=0;}
  return GH_OBJSTEP_OK;
 }
 if(phase==5){
  p[0x2f4]=3;p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
  return GH_OBJSTEP_OK;
 }
 if(phase>5)return fail(s,0x1a17c8,f,GH_OBJSTEP_UNSUPPORTED);
 if(!phase){
  if(!s->special_phase){
   const gh_object_step_status status=om_break_segment_group(s,model,ranges,struck,f);
   if(status)return status;
   gh_hitarea_object *hit=struck?&owner->box_b2:&owner->common.hitarea;
   const int present=struck?owner->b2_created:owner->common.hitarea_created;
   if(present&&gh_hitarea_object_set_active_001fd668(hit,0,&pc))
    return fail(s,pc?pc:0x1fd668,f,GH_OBJSTEP_CALLEE);
   put(p+0xd90,word(p+0xd90)|(0x80000000u>>struck));
   s->special_phase=1;
  }
  if(s->special_phase==1){
   const gh_actor_part *anchor;
   if(gh_actor_model_part_at(model,ranges[struck*2],&anchor))
    return fail(s,0x14bfe0,f,GH_OBJSTEP_UNSUPPORTED);
   uint32_t position[3];
   memcpy(position,anchor->bytes+0xb0,sizeof(position));
   if(!s->runtime.break_effect)return fail(s,0x306838,f,GH_OBJSTEP_UNSUPPORTED);
   const gh_object_step_status status=s->runtime.break_effect(s->runtime.context,model,position,0,&pc);
   if(status)return fail(s,pc?pc:0x306838,f,status);
   s->special_phase=2;
  }
  if(s->special_phase==2){
   const gh_object_step_status status=om307_sound(s,model,f);
   if(status)return status;
   s->special_phase=3;
  }
  if(s->special_phase==3){
   if(!(word(p+0x5b8)&0x200u)&&!(word((const uint8_t *)work+0x5e4)&0x1000000u))
    put(p+0x5b8,word(p+0x5b8)&~0x40u);
   const uint64_t key=(uint64_t)word(p+0x53c)<<32|word(p+0x538);
   if(gh_room_state_mark_object_broken_002becb0(s->factory->bindings.room_state,key))
    return fail(s,0x2becb0,f,GH_OBJSTEP_CALLEE);
   s->special_phase=4;
  }
  if(s->special_phase==4){
   if(!s->runtime.break_collision)return fail(s,0x1331b8,f,GH_OBJSTEP_UNSUPPORTED);
   const uint64_t key=(uint64_t)word(p+0x544)<<32|word(p+0x540);
   const gh_object_step_status status=s->runtime.break_collision(s->runtime.context,key,&pc);
   if(status)return fail(s,pc?pc:0x1331b8,f,status);
   s->special_phase=5;
  }
  if(s->special_phase==5){
   if(gh_collision_solid_release_unit_001347e8(s->runtime.solids,&object->view,&pc))
    return fail(s,pc?pc:0x1347e8,f,GH_OBJSTEP_CALLEE);
   const uint32_t flags=word(p+0x5b0),item=word(p+0x560);
   if(!(flags&0x10u)&&item!=0xffffu&&item!=0x9c3u)
    return fail(s,0x2923e8,f,GH_OBJSTEP_UNSUPPORTED);
   put(p+0x5b0,(flags&~8u)|0x12u);
   s->special_phase=6;
  }
  if(s->special_phase==6){
   gh_game_mt random=s->runtime.init->random;
   float spin[3];
   for(unsigned axis=0;axis<3;++axis)spin[axis]=mul(fragment_random(&random,0),0.01f);
   static const float velocity[3];
   const gh_object_step_status other=om_break_seed_part(s,model,ranges[10]+1u-struck,velocity,spin,0,f);
   if(other)return other;
   const gh_object_step_status anchor=om_break_seed_part(s,model,ranges[4],velocity,spin,0,f);
   if(anchor)return anchor;
   s->runtime.init->random=random;
   s->special_phase=7;
  }
  if(s->special_phase==7){
   if(word(p+0x5ec)){
    if(word(p+0x5f0)!=1||word(p+0x5ec)!=5||!s->runtime.break_effect)
     return fail(s,0x306838,f,GH_OBJSTEP_UNSUPPORTED);
    static const uint32_t origin[3];
    const gh_object_step_status status=s->runtime.break_effect(s->runtime.context,model,origin,0,&pc);
    if(status)return fail(s,pc?pc:0x306838,f,status);
   }
   s->special_phase=8;
  }
  if(s->special_phase==8){
   const gh_object_step_status status=om307_sound(s,model,f);
   if(status)return status;
   s->special_phase=9;
  }
  if(s->special_phase==9){
   if(owner->a_created&&gh_hitarea_object_set_active_001fd668(&owner->box_a,1,&pc))
    return fail(s,pc?pc:0x1fd668,f,GH_OBJSTEP_CALLEE);
   put(p+0x5c0,0);put(p+0x5c4,0x41400000);put(p+0x5c8,0);
   sh(p+0xd94,19);
   p[0x2f5]=1;p[0x2f6]=p[0x2f7]=0;
  }
 }
 const int remaining=(int16_t)(half(p+0xd94)-1u);
 sh(p+0xd94,(uint16_t)remaining);
 if(remaining<=0){p[0x2f5]=2;p[0x2f6]=p[0x2f7]=0;}
 return GH_OBJSTEP_OK;
}

static void om307_hit_reaction_001897b8(uint8_t *actor)
{
 if(!actor[0x2f5]){
  sh(actor+0xcd0,15);actor[0x2f5]=1;actor[0x2f6]=actor[0x2f7]=0;
 }
 if(actor[0x2f5]!=1)return;
 const uint16_t timer=(uint16_t)(half(actor+0xcd0)-1u);
 sh(actor+0xcd0,timer);
 if((int16_t)timer<=0)actor[0x2f4]=actor[0x2f5]=actor[0x2f6]=actor[0x2f7]=0;
}

static gh_object_step_status flag_move(gh_om_frame_state *s,gh_static_transform_policy policy,uint32_t *f)
{
    gh_om041c_state *flag=(gh_om041c_state *)s->object->initialization;
    unsigned mode=s->object->view.bytes[0x2f4];
    if(mode==1)return remove_object(s,f); /* Original 17C008 tail-calls 1B79B0. */
    if(mode||policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x17ba28,f,GH_OBJSTEP_UNSUPPORTED);
    gh_scene_placement_status status=gh_om041c_move_0017baf0(flag,s->runtime.ik_globals,&s->special_phase);
    return status?fail(s,flag->frontier?flag->frontier:0x17baf0,f,GH_OBJSTEP_UNSUPPORTED):GH_OBJSTEP_OK;
}

static gh_object_step_status door_wait_contact(gh_om_frame_state *s,gh_om_init_state *c,
    gh_static_transform_policy policy,uint32_t *frontier)
{
    if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x1f7d48,frontier,GH_OBJSTEP_UNSUPPORTED);
    const gh_title_player_view *door_view=&s->object->view;
    const gh_title_player_view *contact=s->contact634;
    if(!door_view->alias_f0||!contact->alias_f0||!c->bindings)
        return fail(s,0x2da038,frontier,GH_OBJSTEP_ARGUMENT);
    float from[3],to[3],facing,turn;
    memcpy(from,door_view->alias_f0,sizeof(from));
    memcpy(to,contact->alias_f0,sizeof(to));
    if(gh_id_angle_reference_001f7d48(add(number(door_view->bytes+0x104),-1.5707964f),&facing))
        return fail(s,0x1f7d48,frontier,GH_OBJSTEP_UNSUPPORTED);
    uint32_t pc=0;
    if(gh_player_target_angle_002da038(c->bindings->elf,c->bindings->elf_bytes,
        from,to,facing,3.1415927f,&turn,&pc))
        return fail(s,pc?pc:0x2da038,frontier,GH_OBJSTEP_UNSUPPORTED);
    if(gh_om_door_set_open_001bcfb0(&c->geometry_state,turn<0.0f,0))
        return fail(s,0x1bcfb0,frontier,GH_OBJSTEP_UNSUPPORTED);
    return GH_OBJSTEP_OK;
}

static gh_object_step_status door_break_spawn_001bcd70(gh_om_frame_state *s,
 gh_om_init_state *c,uint32_t *frontier)
{
 uint8_t *p=s->object->view.bytes;
 gh_actor_model *model=&c->model;
 uint32_t pc=0;
 const unsigned count=model->part_count<p[0x624]?model->part_count:p[0x624];
 if(!s->runtime.init||!s->runtime.fragment_atan_coeff||c->door_segments!=p+0x650||
    p[0x624]>7||!count||model->part_count!=p[0x2b4]||!model->script.mesh_count||
    word(p+0x600)||word(p+0x628))
  return fail(s,0x1bcd70,frontier,GH_OBJSTEP_UNSUPPORTED);
 if(c->hitarea_created&&gh_hitarea_object_set_active_001fd668(&c->hitarea,0,&pc))
  return fail(s,pc?pc:0x1fd668,frontier,GH_OBJSTEP_CALLEE);
 /* 14B6F8 returns the first node when the requested link is absent. */
 const unsigned fragment_mesh=model->script.mesh_count>1?1:0;
 if(gh_actor_model_node_mask_flags(model,fragment_mesh,0x380,UINT32_MAX,0x2001u)||
    gh_actor_model_node_mask_flags(model,0,0x380,~1u,0x2000u))
  return fail(s,0x14b6f8,frontier,GH_OBJSTEP_CALLEE);
 put(p+0x250,word(p+0x250)|0x10000u);
 for(unsigned i=1;i<count;i++){
  uint8_t *part=model->parts[i].bytes;
  put(part+0x154,word(part+0x154)&~0x18u);
 }
 if(gh_actor_model_update_hierarchy_00149eb8(model,GH_STATIC_BINARY32_REFERENCE))
  return fail(s,model->frontier?model->frontier:0x149eb8,frontier,GH_OBJSTEP_CALLEE);
 float position[7][3],angles[7][3];
 for(unsigned i=1;i<count;i++){
  float world[16];
  memcpy(world,model->parts[i].bytes+0x80,sizeof(world));
  memcpy(position[i],world+12,sizeof(position[i]));
  if(gh_actor_matrix_extract_euler_00147c88(world,s->runtime.fragment_atan_coeff,
      angles[i],GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x147c88,frontier,GH_OBJSTEP_UNSUPPORTED);
 }
 gh_game_mt *random=&s->runtime.init->random;
 if(random->index>624)return fail(s,0x31cfe0,frontier,GH_OBJSTEP_ARGUMENT);
 const float kick[3]={mul(number(p+0x5c0),.01f),add(mul(number(p+0x5c4),.01f),.02f),
     mul(number(p+0x5c8),.01f)};
 for(unsigned i=1;i<count;i++){
  uint8_t *part=model->parts[i].bytes,*record=c->door_segments+(size_t)i*0x60;
  float velocity[3],spin[3];
  velocity[0]=add(kick[0],mul(fragment_random(random,0),.2f));
  velocity[1]=add(kick[1],mul(fragment_random(random,1),.3f));
  velocity[2]=add(kick[2],mul(fragment_random(random,0),.2f));
  for(unsigned j=0;j<3;j++)spin[j]=mul(fragment_random(random,0),.2f);
  memcpy(record,position[i],12);memcpy(record+0x10,angles[i],12);
  memcpy(record+0x20,velocity,12);memcpy(record+0x30,spin,12);
  for(unsigned j=0;j<4;j++)put(record+0x40+j*4,0x3f800000u);
  put(record+0x50,0);
  record[0x54]=1;
  put(record+0x58,0xbca3d70au);
  put(part+0x154,word(part+0x154)|0x18u);
 }
 return GH_OBJSTEP_OK;
}

static gh_object_step_status door_break_001bc748(gh_om_frame_state *s,gh_om_init_state *c,
 const gh_boot_work *work,gh_static_transform_policy policy,uint32_t *frontier)
{
 uint8_t *p=s->object->view.bytes;
 if(!s->runtime_bound||!s->runtime.init||policy!=GH_STATIC_BINARY32_REFERENCE||
    (work->room!=0x100&&work->room!=0x101)||p[0x624]>7||
    c->door_segments!=p+0x650||p[0x2f5]>3)
  return fail(s,0x1bc748,frontier,GH_OBJSTEP_UNSUPPORTED);
 uint32_t pc=0;
 if(!p[0x2f5]){
  if(!s->special_phase){
   if(!s->runtime.break_effect||!s->object->view.alias_f0)
    return fail(s,0x306528,frontier,GH_OBJSTEP_UNSUPPORTED);
   uint32_t position[3];memcpy(position,s->object->view.alias_f0,sizeof(position));
   const gh_object_step_status status=s->runtime.break_effect(
       s->runtime.context,&c->model,position,0,&pc);
   if(status)return fail(s,pc?pc:0x306528,frontier,status);
   s->special_phase=1;
  }
  if(s->special_phase==1){
   if(!s->runtime.break_sound)return fail(s,0x2cba48,frontier,GH_OBJSTEP_UNSUPPORTED);
   s->break_sound=(gh_motion_sound_request){.model=&c->model,.actor=&s->object->view,.bank=2,.program=0};
   uint32_t handle=0;
   const gh_actor_motion_status status=s->runtime.break_sound(
       s->runtime.context,&s->break_sound,&handle,&pc);
   if(status)return fail(s,pc?pc:0x2cba48,frontier,
       status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
   s->special_phase=2;
  }
  if(s->special_phase==2){
   const gh_object_step_status status=door_break_spawn_001bcd70(s,c,frontier);
   if(status)return status;
   s->special_phase=3;
  }
  if(s->special_phase==3){
   if(!(word(p+0x5b8)&0x200u)&&!(word((const uint8_t *)work+0x5e4)&0x1000000u))
    put(p+0x5b8,word(p+0x5b8)&~0x40u);
   const uint64_t key=(uint64_t)word(p+0x53c)<<32|word(p+0x538);
   if(gh_room_state_mark_object_broken_002becb0(s->factory->bindings.room_state,key))
    return fail(s,0x2becb0,frontier,GH_OBJSTEP_CALLEE);
   s->special_phase=4;
  }
  if(s->special_phase==4){
   if(!s->runtime.break_collision)return fail(s,0x1331b8,frontier,GH_OBJSTEP_UNSUPPORTED);
   const uint64_t key=(uint64_t)word(p+0x544)<<32|word(p+0x540);
   const gh_object_step_status status=s->runtime.break_collision(s->runtime.context,key,&pc);
   if(status)return fail(s,pc?pc:0x1331b8,frontier,status);
   s->special_phase=5;
  }
  if(s->special_phase==5){
   if(gh_collision_solid_release_unit_001347e8(s->runtime.solids,&s->object->view,&pc))
    return fail(s,pc?pc:0x1347e8,frontier,GH_OBJSTEP_CALLEE);
   s->special_phase=6;
  }
  if(s->special_phase==6){
   const uint32_t flags=word(p+0x5b0);
   const uint32_t item=word(p+0x560);
   if(!(flags&0x10u)&&item!=0xffffu&&item!=0x9c3u)
    return fail(s,0x2923e8,frontier,GH_OBJSTEP_UNSUPPORTED);
   /* 1B7A78 does not select a default drop for the 0313 class. */
   put(p+0x5b0,(flags&~8u)|0x12u);
   s->special_phase=7;
  }
  if(gh_om_door_release_collision_001bd3f0(&c->geometry_state))
   return fail(s,0x12ec58,frontier,GH_OBJSTEP_UNSUPPORTED);
  p[0x2f5]=1;p[0x2f6]=p[0x2f7]=0;sh(p+0x62c,30);
 }
 if(s->special_phase!=8){
  if(p[0x2f5]==3){
   p[0x2f4]=3;p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
  }else{
   const int16_t remaining=(int16_t)(uint16_t)(half(p+0x62c)-1u);
   sh(p+0x62c,(uint16_t)remaining);
   if(p[0x2f5]==2){
    const volatile float opacity=(float)remaining/15.0f;
    const float value=opacity;memcpy(p+0x24c,&value,4);
   }
   if(remaining<=0){
    if(p[0x2f5]==1)sh(p+0x62c,15);
    ++p[0x2f5];p[0x2f6]=p[0x2f7]=0;
   }
  }
  s->special_phase=8;
 }
 return fragment_tick_records(s,work,&c->model,c->door_segments,1,p[0x624],frontier);
}

static gh_object_step_status door_turn_step(gh_om_frame_state *s,gh_om_init_state *c,
    int closing,gh_static_transform_policy policy,uint32_t *frontier)
{
    uint8_t *p=s->object->view.bytes;
    if(policy!=GH_STATIC_BINARY32_REFERENCE)
        return fail(s,closing?0x1bc610:0x1bc538,frontier,GH_OBJSTEP_UNSUPPORTED);
    if(!p[0x2f6]){
        const unsigned kind=half(p+0x2fe);
        if(kind!=0x314&&kind!=0x354){
            if(!s->runtime_bound||!s->runtime.break_sound)
                return fail(s,0x2cba48,frontier,GH_OBJSTEP_UNSUPPORTED);
            s->break_sound=(gh_motion_sound_request){.model=&c->model,.actor=&s->object->view,
                .bank=2,.program=3};
            uint32_t handle=0,pc=0;
            gh_actor_motion_status status=s->runtime.break_sound(s->runtime.context,&s->break_sound,&handle,&pc);
            if(status)return fail(s,pc?pc:0x2cba48,frontier,
                status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
        }
        p[0x2f7]=0;
        p[0x2f6]=1;
    }
    if(p[0x2f6]!=1)return GH_OBJSTEP_OK;
    const float yaw=number(p+0x104),target=number(p+(closing?0x638:0x640));
    const uint32_t step_bits=0x3e0efa35u;
    const float cap=number((const uint8_t *)&step_bits);
    if(!isfinite(yaw)||!isfinite(target))
        return fail(s,0x2da1f8,frontier,GH_OBJSTEP_UNSUPPORTED);
    float delta=add(target,-yaw);
    if(delta>3.1415927f)delta=add(delta,-6.2831855f);
    if(delta<-3.1415927f)delta=add(delta,6.2831855f);
    if(delta>cap)delta=cap;
    if(delta<-cap)delta=-cap;
    float wrapped;
    if(gh_id_angle_reference_001f7d48(add(yaw,delta),&wrapped))
        return fail(s,0x1f7d48,frontier,GH_OBJSTEP_UNSUPPORTED);
    memcpy(p+0x104,&wrapped,4);
    if(wrapped==target)memset(p+0x2f4,0,4);
    return GH_OBJSTEP_OK;
}

/* 1BC300 tests the active player, then EnemyM+500 in native link order. */
static gh_object_step_status door_blocks_close(gh_om_frame_state *s,const gh_om_init_state *c,
    const float from[3],const gh_title_player_view *actor,float facing,float limit_squared,
    int *blocks,uint32_t *frontier)
{
    if(!actor||!actor->alias_f0)return fail(s,0x2db6b0,frontier,GH_OBJSTEP_ARGUMENT);
    const float arc=1.7453293f; /* ELF 3FDF66F3 */
    float to[3];
    memcpy(to,actor->alias_f0,sizeof(to));
    const float x=add(from[0],-to[0]),y=add(from[1],-to[1]),z=add(from[2],-to[2]);
    const float distance=add(add(mul(x,x),mul(y,y)),mul(z,z));
    if(!isfinite(distance))return fail(s,0x1f7e28,frontier,GH_OBJSTEP_UNSUPPORTED);
    *blocks=0;
    if(distance<limit_squared){
        uint32_t pc=0;
        if(gh_player_target_inside_arc_002da2d0(c->bindings->elf,c->bindings->elf_bytes,
            from,to,facing,arc,blocks,&pc))
            return fail(s,pc?pc:0x2da2d0,frontier,GH_OBJSTEP_UNSUPPORTED);
    }
    return GH_OBJSTEP_OK;
}

static gh_object_step_status door_wait_open(gh_om_frame_state *s,const gh_om_init_state *c,
    gh_static_transform_policy policy,uint32_t *frontier)
{
    if(policy!=GH_STATIC_BINARY32_REFERENCE)
        return fail(s,0x1bc300,frontier,GH_OBJSTEP_UNSUPPORTED);
    if(!s->environment_bound||!s->environment.player_binding_known||
       !s->environment.current_player||!s->environment.init||!c->bindings||
       c->bindings->init!=s->environment.init||!s->object->view.alias_f0)
        return fail(s,0x2db6b0,frontier,GH_OBJSTEP_UNSUPPORTED);
    uint8_t *p=s->object->view.bytes;
    float from[3],facing;
    memcpy(from,s->object->view.alias_f0,sizeof(from));
    const float range=add(number(p+0x610),0.7f);
    const float limit_squared=mul(range,range);
    if(!isfinite(limit_squared)||gh_id_angle_reference_001f7d48(add(number(p+0x638),-1.5707964f),&facing))
        return fail(s,0x1f7d48,frontier,GH_OBJSTEP_UNSUPPORTED);
    int blocks;
    gh_object_step_status status=door_blocks_close(s,c,from,s->environment.current_player,facing,
        limit_squared,&blocks,frontier);
    if(status||blocks)return status;
    const gh_game_init_state *init=s->environment.init;
    if(init->enemy_top){
        unsigned index=64;
        for(unsigned i=0;i<64;i++)if(init->enemy_top==init->enemies+i*20){index=i;break;}
        uint64_t visited=0;
        while(index!=UINT8_MAX){
            if(index>=64||(visited&(UINT64_C(1)<<index))||init->enemies[index*20+16]!=1||
               !init->enemy_actors[index])
                return fail(s,0x1bc414,frontier,GH_OBJSTEP_ARGUMENT);
            visited|=UINT64_C(1)<<index;
            const gh_game_title_player *enemy=init->enemy_actors[index];
            status=door_blocks_close(s,c,from,&enemy->view,facing,limit_squared,&blocks,frontier);
            if(status||blocks)return status;
            index=init->enemy_next[index];
        }
    }
    const uint32_t flags=word(p+0x620);
    if((flags&3u)==1&&p[0x2f4]<2){
        put(p+0x620,flags&~1u);
        p[0x2f4]=p[0x2f6]=p[0x2f7]=0;
        p[0x2f5]=2;
    }
    return GH_OBJSTEP_OK;
}

/* Original 2C51E8 selects a named object from canonical 754C58 and
 * returns its room retirement marker or object inactive bit. */
static gh_object_step_status r101_item_wait_gate(gh_om_frame_state *s,
 uint64_t key,int *can_settle,uint32_t *frontier)
{
 *can_settle=1;
 if(!key)return GH_OBJSTEP_OK;
 const gh_room_vector *actors;
 if(!s->runtime.resolve_actor||
    gh_room_registry_view(s->factory->bindings.pools,0x754c58,&actors)||
    (!actors->allocation.pointer&&actors->count))
  return fail(s,0x14fd98,frontier,GH_OBJSTEP_UNSUPPORTED);
 for(uint32_t i=0;i<actors->count;i++){
  const uint32_t handle=word((const uint8_t *)actors->allocation.pointer+4u*i);
  gh_title_player_view actor;
  uint32_t pc=0;
  if(s->runtime.resolve_actor(s->runtime.context,handle,&actor,&pc)||!actor.bytes||actor.size<0x300)
   return fail(s,0x14fd98,frontier,GH_OBJSTEP_UNSUPPORTED);
  const unsigned kind=half(actor.bytes+0x2fe);
  if(kind<0x300||kind>=0x500)continue;
  if(actor.size<0x5b4)return fail(s,0x14fd98,frontier,GH_OBJSTEP_UNSUPPORTED);
  uint64_t name;
  memcpy(&name,actor.bytes+0x538,sizeof(name));
  if(name!=key)continue;
  int retired;
  if(gh_om_room_slot_used_002bed08(s->factory->bindings.room_state,key,&retired))
   return fail(s,0x2bed08,frontier,GH_OBJSTEP_UNSUPPORTED);
  *can_settle=retired||((word(actor.bytes+0x5b0)&2u)!=0);
  break;
 }
 return GH_OBJSTEP_OK;
}
/* 2DB8B8 offers the original placed-item interaction at the two priorities. */
static gh_object_step_status r101_placed_item_offer(gh_om_frame_state *s,
 const gh_title_player_view *player,const float position[4],uint32_t *frontier)
{
 if(player->size<0x15f8)return fail(s,0x10b698,frontier,GH_OBJSTEP_ARGUMENT);
 if((word(player->bytes+0x15f4)&0x400000u)||(int32_t)word(s->runtime.init->enemies+0x514)>0)
  return GH_OBJSTEP_OK;
 const gh_om_init_state *init=s->object->initialization;
 if(!init||!init->bindings||!s->runtime.action_buttons)
  return fail(s,0x1f7858,frontier,GH_OBJSTEP_ARGUMENT);
 float from[3];
 memcpy(from,player->alias_f0,sizeof(from));
 const float x=add(position[0],-from[0]),z=add(position[2],-from[2]);
 const float distance=add(mul(x,x),mul(z,z));
 int direct=distance<=2.0f;
 uint32_t pc=0;
 if(direct&&distance>=0.5f){
  float turn;
  if(gh_player_target_angle_002da038(init->bindings->elf,init->bindings->elf_bytes,
      from,position,number(player->bytes+0x104),3.1415927f,&turn,&pc))
   return fail(s,pc?pc:0x2da038,frontier,GH_OBJSTEP_CALLEE);
  direct=fabsf(turn)<0.2617994f;
 }
 const gh_action_set_request request={.priority=direct?8:7,.life=6,
     .callback_id=0x2db978,.argument=s->object};
 int emitted;
 unsigned index;
 if(gh_action_button_set_001f7858(s->runtime.action_buttons,&request,&emitted,&index,&pc))
  return fail(s,pc?pc:0x1f7858,frontier,GH_OBJSTEP_CALLEE);
 return GH_OBJSTEP_OK;
}

/* 1C0678's ordinary unattached idle/fall states. Unknown pickup
 * dispatch and active effect requests retain their original boundaries. */
static gh_object_step_status r101_item_move(gh_om_frame_state *s,const gh_boot_work *work,uint32_t *f)
{
 gh_title_player_view *actor=&s->object->view;
 uint8_t *p=actor->bytes;
 if(!actor->alias_f0||actor->size<0x680||!s->runtime.scene_tables||!s->runtime.init)
  return fail(s,0x1c0678,f,GH_OBJSTEP_ARGUMENT);
 const unsigned kind=s->object->runtime_kind;
 const int16_t room=work->room;
 const int placed_item=!s->object->runtime_created&&room==0x101&&
     (s->object->source.type==0x3d3||s->object->source.type==0x3d4)&&
     half(p+0x2fe)==s->object->source.type&&actor->original_vtable214==0x429b88;
 const int dropped_item=s->object->runtime_created&&gh_om_runtime_pickup_supported(room,kind);
 if(p[0x2f4]){
  if(p[0x2f4]==1&&(dropped_item||placed_item))
   return remove_object(s,f); /* Original PMF[1] 1C2260 -> 1B79B0. */
  return fail(s,s->entry,f,GH_OBJSTEP_UNSUPPORTED);
 }
 if(s->special_phase)return GH_OBJSTEP_OK;
 if((dropped_item||placed_item)&&(p[0x2f6]==4||p[0x2f6]==5)){
  if(p[0x2f6]==4){
   if(!s->runtime.award_item)return fail(s,0x1c2408,f,GH_OBJSTEP_UNSUPPORTED);
   if(!s->pickup.stage){
    sh(p+0x568,30);
    p[0x660]=p[0x675]=1;
   }
   uint32_t pc=0;
   gh_object_step_status status=s->runtime.award_item(s->runtime.context,&s->pickup,actor,&pc);
   if(status)return fail(s,pc?pc:0x1c2408,f,status);
   sh(p+0x568,5);
   p[0x2f6]=5;
  }
  float yaw=add(number(p+0x104),0.5235988f);
  if(gh_id_angle_reference_001f7d48(yaw,&yaw))return fail(s,0x1f7d48,f,GH_OBJSTEP_UNSUPPORTED);
  memcpy(p+0x104,&yaw,4);
  const uint16_t remaining=half(p+0x568);
  if(remaining){
   sh(p+0x568,remaining-1);
   const float height=add(number(actor->alias_f0+4),0.2f);
   memcpy(actor->alias_f0+4,&height,4);
  }
  float alpha=number(p+0x24c);
  if(alpha>1.0f)alpha=1.0f;
  alpha=add(alpha,-0.02f);
  const uint32_t flags=word(p+0x250);
  if(alpha<=0.15f){
   p[0x2f4]=1;p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
   put(p+0x250,(flags|0x12u)&~0x10u);
   put(p+0x24c,0);
  }else{
   put(p+0x250,flags|0x10u);
   memcpy(p+0x24c,&alpha,4);
  }
  s->special_phase=1;
  return GH_OBJSTEP_OK;
 }
 const uint32_t attached=word(p+0x630);
 if(attached){
  const gh_om_init_state *init=s->object->initialization;
  gh_title_player_view owner={0};
  uint32_t pc=0;
  if(room!=0x100||kind!=0x3c8||!init->item_attachment_owner||!s->runtime.resolve_actor||
     s->runtime.resolve_actor(s->runtime.context,attached,&owner,&pc)||
     owner.bytes!=init->item_attachment_owner->root.bytes)
   return fail(s,pc?pc:0x1c2378,f,GH_OBJSTEP_UNSUPPORTED);
 }
 if(p[0x2f6]==0){
  if(p[0x676]&2u){
   p[0x2f6]=2;
   s->special_phase=1;
   return GH_OBJSTEP_OK;
  }
  uint32_t draw;
  if(gh_game_mt_next_0030f348(&s->runtime.init->random,&draw))
   return fail(s,0x31cfe0,f,GH_OBJSTEP_UNSUPPORTED);
  sh(p+0x56a,(uint16_t)((int32_t)draw%30+30));
  put(p+0x620,word(actor->alias_f0+4));
  p[0x2f6]=1;
 }
 if(p[0x2f6]==2){
  uint64_t parent_key;
  memcpy(&parent_key,p+0x668,sizeof(parent_key));
  int can_settle;
  gh_object_step_status status=r101_item_wait_gate(s,parent_key,&can_settle,f);
  if(status)return status;
  put(p+0x610,0);put(p+0x618,0);
  put(p+0x614,word(p+0x600)^0x80000000u);
  if(can_settle)p[0x2f6]=3;
 }else if(p[0x2f6]==1||p[0x2f6]==3){
  const int settled=p[0x2f6]==3;
  const gh_title_player_view *player=s->environment.current_player;
  if(settled&&(!s->environment_bound||!player||!player->bytes||player->size<0x54c))
   return fail(s,0x2db6b0,f,GH_OBJSTEP_UNSUPPORTED);
  float position[4],next[4],hit[4];
  memcpy(position,actor->alias_f0,sizeof(position));
  memcpy(next,position,sizeof(next));
  float speed=number(p+0x614);
  unsigned next_phase=p[0x2f6];
  if(!attached&&!(settled&&(p[0x676]&2u))){
   for(unsigned i=0;i<3;i++)next[i]=add(position[i],number(p+0x610+4u*i));
   speed=add(speed,-number(p+0x600));
   const int up=!settled&&(speed>=0||number(p+0x620)<next[1]);
   if(!settled||speed<=0){
    gh_scene_ray_request q={.mask=up?7u:2u,.flags=0x400,.hit_position=hit,.segmented=1};
    memcpy(q.from,position,sizeof(position));memcpy(q.to,next,sizeof(next));
    q.from[1]=add(q.from[1],up?-0.5f:0.5f);
    q.to[1]=add(q.to[1],up?1.0f:-1.0f);
    uint32_t pc=0;
    int did_hit;
    if(gh_scene_ray_query_00131e10(s->runtime.scene_tables,&q,&did_hit,&pc))
     return fail(s,pc?pc:0x131e10,f,GH_OBJSTEP_UNSUPPORTED);
    if(did_hit){
     const float boundary=add(hit[1],up?-0.5f:0.5f);
     if((up&&boundary<next[1])||(!up&&next[1]<boundary)){
      next[1]=boundary;
      if(settled)speed=-number(p+0x600);
      else if(!up)next_phase=2;
     }
    }
   }
  }
  if(!settled&&next[1]<-900.0f){
   p[0x2f4]=1;p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
  }else{
   if(settled){
    const int alive=(int16_t)half(player->bytes+0x54a)>0;
    if(alive&&half((const uint8_t *)work+0x610)==0xffff){
     if(!player->alias_f0)return fail(s,0x1c20d8,f,GH_OBJSTEP_UNSUPPORTED);
     float origin[4],distance=0;
     memcpy(origin,player->alias_f0,sizeof(origin));
     origin[1]=add(origin[1],0.5f);
     for(unsigned i=0;i<3;i++){
      const float delta=add(next[i],-origin[i]);
      distance=add(distance,mul(delta,delta));
     }
     if(distance<((p[0x676]&4u)?16.0f:4.0f)){
      if((!dropped_item&&!placed_item)||word(p+0x670)!=0x31)
       return fail(s,0x1c20d8,f,GH_OBJSTEP_UNSUPPORTED);
      int blocked=0;
      if(p[0x676]&1u){
       gh_scene_ray_request ray={.mask=7,.flags=0x204,.segmented=1};
       memcpy(ray.from,next,sizeof(ray.from));
       memcpy(ray.to,origin,sizeof(ray.to));
       uint32_t pc=0;
       if(gh_scene_ray_query_00131e10(s->runtime.scene_tables,&ray,&blocked,&pc))
        return fail(s,pc?pc:0x131e10,f,GH_OBJSTEP_UNSUPPORTED);
      }
      if(!blocked&&placed_item){
       const gh_object_step_status status=r101_placed_item_offer(s,player,next,f);
       if(status)return status;
      }
      if(!blocked&&dropped_item){
       gh_om_init_state *init=s->object->initialization;
       if(gh_om_r101_item_collect_001c24e8(init))
        return fail(s,init->frontier?init->frontier:0x1c24e8,f,GH_OBJSTEP_UNSUPPORTED);
       next_phase=4;
      }
     }
    }
    if(alive){
     const uint16_t remaining=(uint16_t)(half(p+0x56a)-1u);
     if((int16_t)remaining<1){
      if(!s->runtime.break_effect)return fail(s,0x306838,f,GH_OBJSTEP_UNSUPPORTED);
      uint32_t position_bits[3],pc=0;
      memcpy(position_bits,next,sizeof(position_bits));
      gh_object_step_status status=s->runtime.break_effect(s->runtime.context,
          &s->object->initialization->model,position_bits,0,&pc);
      if(status)return fail(s,pc?pc:0x306838,f,status);
      sh(p+0x56a,60);
     }else sh(p+0x56a,remaining);
    }
    if(!(p[0x676]&2u)){
     float yaw=add(number(p+0x104),0.08726646f);
     if(gh_id_angle_reference_001f7d48(yaw,&yaw))return fail(s,0x1f7d48,f,GH_OBJSTEP_UNSUPPORTED);
     memcpy(p+0x104,&yaw,4);
    }
   }
   p[0x2f6]=(uint8_t)next_phase;
  }
  memcpy(actor->alias_f0,next,sizeof(next));
  memcpy(p+0x614,&speed,4);
 }else return fail(s,0x1c0678,f,GH_OBJSTEP_UNSUPPORTED);
 /* 1C24E8 may detach the item during this frame's proximity step. */
 if(word(p+0x630)){
  gh_om_init_state *init=s->object->initialization;
  if(gh_om_r101_item_follow_001c2378(init))
   return fail(s,init->frontier?init->frontier:0x1c2378,f,GH_OBJSTEP_UNSUPPORTED);
 }
 s->special_phase=1;
 return GH_OBJSTEP_OK;
}
static gh_object_step_status r101_item_save_position(gh_om_frame_state *s,uint32_t *f)
{
 gh_title_player_view *actor=&s->object->view;
 uint8_t *p=actor->bytes;
 if(word(p+0x5b0)&0x20u)return GH_OBJSTEP_OK;
 if(s->factory->bindings.work->room==0x104&&s->object->runtime_kind==0x3da&&
    word(p+0x560)==0xa5d){
  /* 1C0450 also synchronizes the authored ID, flags and fixed rotation. */
  if(word(p+0x560)!=word(p+0x67c)||p[0x676]!=p[0x677]||
     memcmp(p+0x490,actor->alias_f0,16)||((p[0x676]&2u)&&memcmp(p+0x5d0,p+0x100,16))){
   gh_om_init_state *init=common(s);
   if(gh_om_r101_item_save_001fb1c0(init,actor->alias_f0))
    return fail(s,init->frontier?init->frontier:0x1fb1c0,f,GH_OBJSTEP_UNSUPPORTED);
  }
  put(p+0x67c,word(p+0x560));
  p[0x677]=p[0x676];
  return GH_OBJSTEP_OK;
 }
 if(word(p+0x560)!=word(p+0x67c)||p[0x676]!=p[0x677]||(p[0x676]&2u))
  return fail(s,0x1c0450,f,GH_OBJSTEP_UNSUPPORTED);
 if(!memcmp(p+0x490,actor->alias_f0,16))return GH_OBJSTEP_OK;
 uint8_t previous[12];
 memcpy(previous,p+0x490,12);
 const uint32_t flags=word(p+0x5b8);
 gh_om_init_state *init=common(s);
 if(gh_om_r101_item_position_001c2630(init,actor->alias_f0))
  return fail(s,init->frontier?init->frontier:0x1fb1c0,f,GH_OBJSTEP_UNSUPPORTED);
 memcpy(p+0x490,previous,12);
 put(p+0x5b8,flags);
 return GH_OBJSTEP_OK;
}
/* 190948 uses its own four base hit checks before the shared 1B76D8 tail. */
static gh_object_step_status om311_move(gh_om_frame_state *s,gh_static_transform_policy policy,uint32_t *f)
{
 gh_om311_state *state=(gh_om311_state*)s->object->initialization;
 uint8_t *p=s->object->view.bytes;
 if(!s->special_phase){
  const unsigned mode=p[0x2f4];
  if(mode==0){
   for(;s->hit_index<4;++s->hit_index){
    const unsigned index=s->hit_index;
    const gh_hitarea_object *box=state->created[index]?&state->boxes[index]:NULL;
    if(box&&box->state!=state->common.bindings->init)return fail(s,0x1b7380,f,GH_OBJSTEP_ARGUMENT);
    int hit=0;
    uint32_t frontier=0;
    const gh_om_damage_status damage=gh_om_check_damage_001b7380(&s->damage_calls[index],
     &s->object->view,box,box?box->state->heat+0x10:NULL,policy,&hit,&frontier);
    if(damage)return fail(s,frontier?frontier:0x1b7380,f,
     damage==GH_OM_DAMAGE_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
    if(hit){
     uint8_t *health=p+0xf88+2*index;
     const uint16_t updated=(uint16_t)(half(health)-word(box->bytes+0x4c));
     sh(health,updated);
     p[0x2f4]=updated&0x8000u?2:1;
     p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
     if(p[0x2f4]==2){
      p[0xf90]=(uint8_t)index;
      put(p+0x5b0,word(p+0x5b0)|1u);
     }
    }
   }
  }else if(mode==1){
   if(!p[0x2f5]){
    sh(p+0xf84,15);
    p[0x2f5]=1;
    p[0x2f6]=p[0x2f7]=0;
   }
   if(p[0x2f5]==1){
    const uint16_t timer=(uint16_t)(half(p+0xf84)-1u);
    sh(p+0xf84,timer);
    if(timer==0||(timer&0x8000u))memset(p+0x2f4,0,4);
   }
  }else{
   const uint32_t methods[]={0x190bf0,0x190cc8,0x190d28,0x1915b8};
   return fail(s,mode<4?methods[mode]:s->entry,f,GH_OBJSTEP_UNSUPPORTED);
  }
  s->special_phase=1;
 }
 if(p[0x2b4]!=GH_OM311_FRAGMENTS)return fail(s,0x14bfe0,f,GH_OBJSTEP_UNSUPPORTED);
 for(unsigned i=0;i<GH_OM311_FRAGMENTS;++i)
  if(p[0x620+(size_t)i*0x60+0x54]==1)return fail(s,0x292f50,f,GH_OBJSTEP_UNSUPPORTED);
 return GH_OBJSTEP_OK;
}

/* 1B93E8 integrates stored velocity and checks the swept world segment
 * before publishing the subtype's attack and contact shapes. */
static gh_object_step_status projectile_move_001b93e8(gh_om_frame_state *frame,
 const gh_boot_work *work,gh_static_transform_policy policy,uint32_t *frontier)
{
 gh_om3be_state *state=(gh_om3be_state *)frame->object->initialization;
 uint8_t *p=frame->object->view.bytes,*position=frame->object->view.alias_f0;
 const uint32_t kind=word(p+0x610);
 const int roulette=kind==0x2c&&(work->room==0x103||work->room==0x104||work->room==0x200)&&state->shooter;
 const int devil=kind==3&&work->room==0x100;
 const int em03=kind==0&&work->room==0x104&&state->shooter;
 const int uses_contact=devil||em03;
 if(!frame->runtime_bound||(!roulette&&!uses_contact)||!position||state->stage!=9||
    policy!=GH_STATIC_BINARY32_REFERENCE)
  return fail(frame,0x1b92f0,frontier,GH_OBJSTEP_UNSUPPORTED);
 if(p[0x2f4]>1)return fail(frame,0x1b92f0,frontier,GH_OBJSTEP_UNSUPPORTED);
 uint32_t pc=0;
 if(frame->special_phase==0){
  const gh_hitarea_object *const areas[]={&state->attack,&state->contact};
  const uint8_t present[]={state->attack_created,state->contact_created};
  for(unsigned i=0;i<2;i++){
   if(!present[i])continue;
   if(gh_hitarea_object_set_active_001fd668(areas[i],0,&pc))
    return fail(frame,pc?pc:0x1fd668,frontier,GH_OBJSTEP_UNSUPPORTED);
  }
  const float rate=mul(number(frame->runtime.init->enemies+0x548),*frame->runtime.canonical_rate);
  if(!isfinite(rate)||rate<0)return fail(frame,0x292f50,frontier,GH_OBJSTEP_UNSUPPORTED);
  frame->runtime_rate=rate;
  memcpy(p+0x5a8,&rate,4);
  p[0x531]=2;
  frame->special_phase=1;
 }
 if(frame->special_phase==1&&p[0x2f4]==1){
  p[0x531]=0xff;
  if(!frame->runtime.release_effect)return fail(frame,0x306200,frontier,GH_OBJSTEP_UNSUPPORTED);
  if(frame->runtime.release_effect(frame->runtime.context,&frame->object->view,0,2,&pc))
   return fail(frame,pc?pc:0x306200,frontier,GH_OBJSTEP_UNSUPPORTED);
  frame->special_phase=6;
 }
 if(frame->special_phase==6){
  gh_object_step_status status=remove_object(frame,frontier);
  if(status)return status;
  frame->special_phase=7;
 }
 if(frame->special_phase==1&&uses_contact&&state->attack_created){
  const gh_title_player_view *hit_actor=NULL;
  if(!state->attack.fallback&&gh_hitarea_object_last_hit_actor(&state->attack,&hit_actor))
   return fail(frame,0x1fcf20,frontier,GH_OBJSTEP_UNSUPPORTED);
  if(!hit_actor&&word(state->attack.bytes+0x34))
   return fail(frame,0x1fcf20,frontier,GH_OBJSTEP_UNSUPPORTED);
  if(hit_actor){
   memcpy(frame->projectile_impact,position,12);
   frame->special_phase=5;
  }
 }
 if(frame->special_phase==1){
  float next[3];
  for(unsigned i=0;i<3;i++){
   const float velocity=number(p+0x580+4*i);
   next[i]=add(number(position+4*i),roulette?velocity:mul(velocity,frame->runtime_rate));
   if(!isfinite(next[i]))return fail(frame,0x1ba018,frontier,GH_OBJSTEP_UNSUPPORTED);
  }
  memcpy(position,next,12);
  frame->special_phase=roulette?8:2;
 }
 if(frame->special_phase==8){
  const float countdown=add(number(p+0x618),-1.0f);
  if(countdown<=0){
   const uint32_t count=word(p+0x61c);
   if(count){
    if(!frame->runtime.scene_tables)return fail(frame,0x2ddd18,frontier,GH_OBJSTEP_UNSUPPORTED);
    float ground[4];
    gh_scene_ray_request floor={.mask=2,.hit_position=ground};
    memcpy(floor.from,position,12);
    memcpy(floor.to,position,12);
    floor.from[1]=add(floor.from[1],.5f);
    floor.to[1]=add(floor.to[1],-20.0f);
    floor.from[3]=floor.to[3]=1;
    int hit=0;
    if(gh_scene_ray_query_00131e10(frame->runtime.scene_tables,&floor,&hit,&pc))
     return fail(frame,pc?pc:0x131e10,frontier,GH_OBJSTEP_UNSUPPORTED);
    put(p+0x61c,count-1);
    put(p+0x618,0x40400000);
    frame->special_phase=2;
   }else p[0x2f4]=1;
  }
  if(frame->special_phase==8)memcpy(p+0x618,&countdown,4);
  frame->special_phase=2;
 }
 if(frame->special_phase==2){
  if(!frame->runtime.scene_tables)return fail(frame,0x131e10,frontier,GH_OBJSTEP_UNSUPPORTED);
  float impact[4];
  gh_scene_ray_request ray={.mask=1,.flags=0x13c,.hit_position=impact};
  memcpy(ray.from,p+0x490,12);
  memcpy(ray.to,position,12);
  ray.from[3]=ray.to[3]=1;
  int hit=0;
  if(gh_scene_ray_query_00131e10(frame->runtime.scene_tables,&ray,&hit,&pc))
   return fail(frame,pc?pc:0x131e10,frontier,GH_OBJSTEP_UNSUPPORTED);
  if(hit&&roulette){
   p[0x2f4]=1;
   frame->special_phase=4;
  }else if(hit){
   memcpy(frame->projectile_impact,impact,12);
   frame->special_phase=5;
  }else frame->special_phase=3;
 }
 if(frame->special_phase==5){
  if(!frame->runtime.break_effect)return fail(frame,0x306838,frontier,GH_OBJSTEP_UNSUPPORTED);
  gh_object_step_status status=frame->runtime.break_effect(frame->runtime.context,&state->common.model,
       frame->projectile_impact,0,&pc);
  if(status)return fail(frame,pc?pc:0x306838,frontier,status);
  p[0x2f4]=1;
  frame->special_phase=4;
 }
 if(frame->special_phase==3){
  if(state->attack_created){
   if(gh_hitarea_object_set_active_001fd668(&state->attack,1,&pc))
    return fail(frame,pc?pc:0x1fd668,frontier,GH_OBJSTEP_UNSUPPORTED);
   if(gh_hitarea_object_set_mode_001fd988(&state->attack,uses_contact,&pc))
    return fail(frame,pc?pc:0x1fd988,frontier,GH_OBJSTEP_UNSUPPORTED);
   if(number(p+0x100)||number(p+0x108))return fail(frame,0x1fd338,frontier,GH_OBJSTEP_UNSUPPORTED);
   put(state->attack.bytes+0x4c,roulette?80:em03?60:30);
   sh(state->attack.bytes+0x46,roulette?0x27:0x41);
   if(uses_contact&&state->contact_created&&gh_hitarea_object_set_active_001fd668(&state->contact,1,&pc))
    return fail(frame,pc?pc:0x1fd668,frontier,GH_OBJSTEP_UNSUPPORTED);
   if(!state->attack.fallback&&state->attack_index>=0){
    const uint8_t *node;
    gh_collision_pair_shape shape;
    if(gh_hitarea_node_at(&state->attack,0,&node,&shape))
     return fail(frame,0x1fd888,frontier,GH_OBJSTEP_UNSUPPORTED);
    const float offset[3]={0,roulette?-1.5f:em03?-1.0f:-.3f,0};
    const float size[3]={roulette?.7f:em03?1.0f:.3f,roulette?3.0f:em03?2.0f:.6f,
        roulette?2.0f:em03?1.0f:.3f};
    if(gh_collision_pair_shape_offset_002bb258(&shape,offset,&pc))
     return fail(frame,pc?pc:0x1fd888,frontier,GH_OBJSTEP_UNSUPPORTED);
    if(gh_collision_pair_box_size_002baaa8(&shape,size,&pc))
     return fail(frame,pc?pc:0x1fd820,frontier,GH_OBJSTEP_UNSUPPORTED);
   }
   float yaw=number(p+0x104),sine,cosine;
   if(roulette){
    uint32_t draw;
    if(gh_game_mt_next_0030f348(&frame->runtime.init->random,&draw))
     return fail(frame,0x31cfe0,frontier,GH_OBJSTEP_UNSUPPORTED);
    const float spread=(draw&1u)?0.17453292f:-0.17453292f;
    if(gh_id_angle_reference_001f7d48(add(yaw,spread),&yaw))
     return fail(frame,0x1f7d48,frontier,GH_OBJSTEP_UNSUPPORTED);
   }
   if(gh_sdk_rotation_sincos_reference_0030a5d0(yaw,&sine,&cosine,policy,&pc))
    return fail(frame,pc?pc:0x1fd338,frontier,GH_OBJSTEP_UNSUPPORTED);
   const float direction[3]={sine,0,cosine};
   memcpy(state->attack.bytes+0x10,direction,12);
  }
  const float life=add(number(p+0x624),-frame->runtime_rate);
  if(!isfinite(life))return fail(frame,0x1bb4c8,frontier,GH_OBJSTEP_UNSUPPORTED);
  memcpy(p+0x624,&life,4);
  if(life<=0)p[0x2f4]=1;
  frame->special_phase=4;
 }
 return GH_OBJSTEP_OK;
}

static gh_object_step_status mount_descend_001a7a28(gh_om_frame_state *s,uint32_t *frontier)
{
 gh_title_player_view *actor=&s->object->view;
 uint8_t *p=actor->bytes;
 if(!actor->alias_f0)return fail(s,0x1a7a28,frontier,GH_OBJSTEP_ARGUMENT);
 if(p[0x2f6]>1)return fail(s,0x1a7a28,frontier,GH_OBJSTEP_UNSUPPORTED);
 float position[4];
 memcpy(position,actor->alias_f0,sizeof(position));
 if(p[0x2f6]==0){
  float floor=add(position[1],-50.0f);
  if(number(p+0x620)==0.0f){
   if(!s->runtime.scene_tables)return fail(s,0x131e10,frontier,GH_OBJSTEP_UNSUPPORTED);
   float surface[4]={0,0,0,1};
   gh_scene_ray_request ray={.mask=7,.hit_position=surface,.segmented=1};
   memcpy(ray.from,position,sizeof(position));
   memcpy(ray.to,position,sizeof(position));
   ray.to[1]=floor;
   int hit=0;
   uint32_t pc=0;
   if(gh_scene_ray_query_00131e10(s->runtime.scene_tables,&ray,&hit,&pc))
    return fail(s,pc?pc:0x131e10,frontier,GH_OBJSTEP_UNSUPPORTED);
   if(hit)floor=surface[1];
   const float destination=add(floor,2.0f);
   memcpy(p+0x61c,&floor,4);
   memcpy(p+0x620,&destination,4);
  }
  memcpy(p+0x624,position+1,4);
  put(p+0x5a0,word(p+0x5a0)&~1u);
  p[0x2f6]=1;
  p[0x2f7]=0;
 }
 position[1]=add(position[1],-0.5f);
 memcpy(actor->alias_f0+4,position+1,4);
 if(position[1]<=number(p+0x620)){
  p[0x2f6]=2;
  p[0x2f7]=0;
 }
 return GH_OBJSTEP_OK;
}

static gh_object_step_status mount_turn_lids(gh_om_frame_state *s,uint32_t *frontier)
{
 uint8_t *actor=s->object->view.bytes;
 const uint8_t phase=actor[0x613];
 if(!phase||phase>3||actor[0x2b4]<5)return GH_OBJSTEP_OK;
 const gh_om_init_state *init=common(s);
 const gh_actor_part *parts[4];
 static const size_t axis[]={0x100,0x108,0x100,0x108};
 for(size_t i=0;i<4;++i){
  if(gh_actor_model_part_at(&init->model,(unsigned)i+1u,parts+i)||!parts[i]->bytes)
   return fail(s,0x14bfe0,frontier,GH_OBJSTEP_UNSUPPORTED);
 }
 if(phase==2){
  const uint16_t remaining=(uint16_t)(half(actor+0x618)-1u);
  sh(actor+0x618,remaining);
  if((int16_t)remaining<=0)++actor[0x613];
  return GH_OBJSTEP_OK;
 }
 if(!s->runtime.init||!s->runtime.canonical_rate)return fail(s,0x292f50,frontier,GH_OBJSTEP_UNSUPPORTED);
 const float rate=mul(number(s->runtime.init->enemies+0x548),*s->runtime.canonical_rate);
 if(!isfinite(rate))return fail(s,0x292f50,frontier,GH_OBJSTEP_UNSUPPORTED);
 const float amount=mul(rate,phase==1?0.17453292f:-0.08726646f);
 float angles[4];
 for(size_t i=0;i<4;++i){
  angles[i]=add(number(parts[i]->bytes+axis[i]),i<2?amount:-amount);
  if(!isfinite(angles[i]))return fail(s,0x1a7cb0,frontier,GH_OBJSTEP_UNSUPPORTED);
 }
 if(phase==1&&angles[0]>=2.9670596f){
  for(size_t i=0;i<4;++i)angles[i]=i<2?2.9670596f:-2.9670596f;
  sh(actor+0x618,60);
  ++actor[0x613];
 }else if(phase==3&&angles[0]<=0.0f){
  memset(angles,0,sizeof(angles));
  actor[0x613]=0;
 }
 for(size_t i=0;i<4;++i)memcpy(parts[i]->bytes+axis[i],angles+i,sizeof(angles[i]));
 return GH_OBJSTEP_OK;
}

static gh_object_step_status mount_visit_001a7a28(gh_om_frame_state *s,uint32_t *frontier)
{
 uint8_t *actor=s->object->view.bytes;
 switch(actor[0x2f6]){
 case 0:case 1:{
  const gh_object_step_status status=mount_descend_001a7a28(s,frontier);
  if(status)return status;
  break;
 }
 case 2:{
  if(!s->runtime.mount_release)return fail(s,0x1a7f58,frontier,GH_OBJSTEP_UNSUPPORTED);
  uint32_t pc=0;
  const gh_object_step_status status=s->runtime.mount_release(s->runtime.context,s->object,&s->special_phase,&pc);
  if(status)return fail(s,pc?pc:0x1a7f58,frontier,status);
  actor[0x2f6]=3;
  actor[0x2f7]=0;
  sh(actor+0x616,(uint16_t)half(actor+0x628));
  ++actor[0x613];
  break;
 }
 case 3:{
  const uint16_t time=(uint16_t)half(actor+0x616);
  if((int16_t)time<1||actor[0x614]==1){
   actor[0x614]=0;
   actor[0x2f6]=4;
   actor[0x2f7]=0;
  }
  sh(actor+0x616,(uint16_t)(time-1u));
  break;
 }
 case 4:{
  uint8_t *position=s->object->view.alias_f0;
  if(!position)return fail(s,0x1a7a28,frontier,GH_OBJSTEP_ARGUMENT);
  const float height=add(number(position+4),0.1f);
  memcpy(position+4,&height,sizeof(height));
  if(height>=number(actor+0x624)){
   actor[0x2f5]=0;
   actor[0x2f6]=0;
   actor[0x2f7]=0;
   put(actor+0x5a0,word(actor+0x5a0)|1u);
  }
  break;
 }
 default:return fail(s,0x1a7a28,frontier,GH_OBJSTEP_UNSUPPORTED);
 }
 return mount_turn_lids(s,frontier);
}

static gh_object_step_status catapult_launch_001b8b68(gh_om_frame_state *s,
 gh_static_transform_policy policy,uint32_t *frontier)
{
 const gh_title_player_view *player=s->environment.current_player;
 gh_om_init_state *init=common(s);
 uint8_t *actor=s->object->view.bytes;
 if(policy!=GH_STATIC_BINARY32_REFERENCE||!s->runtime.init||!s->runtime.break_sound||
    !s->environment_bound||!player||!player->alias_f0||!player->bytes||player->size<0x108||
    !s->object->view.alias_f0||s->runtime.init->random.index>624)
  return fail(s,0x1b8b68,frontier,GH_OBJSTEP_ARGUMENT);
 uint32_t pc=0;
 if(!s->special_phase){
  s->break_sound=(gh_motion_sound_request){.model=&init->model,.actor=&s->object->view,.bank=2,.program=0x1a};
  uint32_t handle;
  const gh_actor_motion_status e=s->runtime.break_sound(s->runtime.context,&s->break_sound,&handle,&pc);
  if(e)return fail(s,pc?pc:0x2cba48,frontier,
      e==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
  put(actor+0x62c,handle);
  s->special_phase=1;
 }
 if(s->special_phase==1){
  float from[3],target[3],sine,cosine;
  memcpy(from,s->object->view.alias_f0,sizeof(from));
  memcpy(target,player->alias_f0,sizeof(target));
  if(gh_sdk_rotation_sincos_reference_0030a5d0(number(player->bytes+0x104),
      &sine,&cosine,policy,&pc))return fail(s,pc?pc:0x150888,frontier,GH_OBJSTEP_CALLEE);
  gh_game_mt random=s->runtime.init->random;
  target[0]=add(add(mul(sine,4.0f),target[0]),mul(fragment_random(&random,0),8.0f));
  target[2]=add(add(mul(cosine,4.0f),target[2]),mul(fragment_random(&random,0),8.0f));
  const float yaw=number(actor+0x104);
  float delta;
  if(gh_player_target_angle_002da038(init->bindings->elf,init->bindings->elf_bytes,
     from,target,yaw,3.14159274f,&delta,&pc))
   return fail(s,pc?pc:0x2da7a8,frontier,GH_OBJSTEP_CALLEE);
  /* 1B7C48 adds a one-unit apex above the higher endpoint. */
  const float apex=add(1.0f,from[1]>target[1]?from[1]:target[1]);
  const float up=add(apex,-from[1]),down=add(apex,-target[1]);
  const float gravity=-0.01f;
  const float time=add(sqrtf(mul(up,-2.0f)/gravity),sqrtf(mul(down,-2.0f)/gravity));
  const float inverse=1.0f/time;
  const float velocity[3]={mul(add(target[0],-from[0]),inverse),sqrtf(mul(mul(gravity,-2.0f),up)),
      mul(add(target[2],-from[2]),inverse)};
  if(!isfinite(time)||time<=0||!isfinite(velocity[0])||!isfinite(velocity[1])||!isfinite(velocity[2]))
   return fail(s,0x1b7c48,frontier,GH_OBJSTEP_UNSUPPORTED);
  const float next_yaw=add(yaw,delta);
  memcpy(actor+0x104,&next_yaw,sizeof(next_yaw));
  memcpy(actor+0x610,velocity,sizeof(velocity));
  s->runtime.init->random=random;
  s->special_phase=2;
 }
 if(init->hitarea_created&&gh_hitarea_object_set_active_001fd668(&init->hitarea,1,&pc))
  return fail(s,pc?pc:0x1fd668,frontier,GH_OBJSTEP_CALLEE);
 sh(actor+0x628,15);
 actor[0x2f5]=2;
 actor[0x2f6]=actor[0x2f7]=0;
 return GH_OBJSTEP_OK;
}

static gh_object_step_status catapult_impact_001b8e08(gh_om_frame_state *s,uint32_t *frontier)
{
 gh_om_init_state *init=common(s);
 uint32_t pc=0;
 if(s->special_phase==4){
  if(!s->runtime.stop_sound)return fail(s,0x2cbba0,frontier,GH_OBJSTEP_UNSUPPORTED);
  const gh_object_step_status e=s->runtime.stop_sound(s->runtime.context,
     word(s->object->view.bytes+0x62c),&pc);
  if(e)return fail(s,pc?pc:0x2cbba0,frontier,e);
  s->special_phase=5;
 }
 if(s->special_phase==5){
  if(!s->runtime.release_effect)return fail(s,0x306200,frontier,GH_OBJSTEP_UNSUPPORTED);
  const gh_actor_motion_status e=s->runtime.release_effect(s->runtime.context,&s->object->view,0,2,&pc);
  if(e)return fail(s,pc?pc:0x306200,frontier,GH_OBJSTEP_CALLEE);
  s->special_phase=6;
 }
 if(s->special_phase==6&&init->catapult_attack_created){
  gh_hitarea_object *attack=&init->catapult_attack;
  if(!attack->fallback){
   const uint8_t *node;
   gh_collision_pair_shape shape;
   float point[3];
   memcpy(point,s->projectile_impact,sizeof(point));
   if(gh_hitarea_node_at(attack,0,&node,&shape)||
      gh_collision_pair_shape_offset_002bb258(&shape,point,&pc))
    return fail(s,pc?pc:0x1fd888,frontier,GH_OBJSTEP_CALLEE);
  }
  s->special_phase=7;
 }
 if(s->special_phase<8){
  if(init->catapult_attack_created&&gh_hitarea_object_set_active_001fd668(&init->catapult_attack,1,&pc))
   return fail(s,pc?pc:0x1fd668,frontier,GH_OBJSTEP_CALLEE);
  s->special_phase=8;
 }
 while(s->special_phase<10){
  const int core=s->special_phase==8;
  const uint32_t call=core?0x1b8e74:0x1b8ea4;
  if(!s->runtime.catapult_impact_sound)return fail(s,call,frontier,GH_OBJSTEP_UNSUPPORTED);
  const gh_object_step_status e=s->runtime.catapult_impact_sound(
      s->runtime.context,&s->object->view,core?0:2,core?0x5b:0x17,&pc);
  if(e)return fail(s,pc?pc:call,frontier,e);
  ++s->special_phase;
 }
 if(s->special_phase==10){
  if(!s->runtime.break_effect)return fail(s,0x1b8ec4,frontier,GH_OBJSTEP_UNSUPPORTED);
  const gh_object_step_status e=s->runtime.break_effect(s->runtime.context,&init->model,
      s->projectile_impact,0,&pc);
  if(e)return fail(s,pc?pc:0x1b8ec4,frontier,e);
  s->special_phase=11;
 }
 if(init->hitarea_created&&gh_hitarea_object_set_active_001fd668(&init->hitarea,0,&pc))
  return fail(s,pc?pc:0x1fd668,frontier,GH_OBJSTEP_CALLEE);
 uint8_t *actor=s->object->view.bytes;
 memcpy(s->object->view.alias_f0,actor+0x600,12);
 sh(actor+0x628,2);
 actor[0x2f5]=3;
 actor[0x2f6]=actor[0x2f7]=0;
 return GH_OBJSTEP_OK;
}

static gh_object_step_status catapult_flight_001b8c6c(gh_om_frame_state *s,
 gh_static_transform_policy policy,uint32_t *frontier)
{
 gh_om_init_state *init=common(s);
 uint8_t *actor=s->object->view.bytes;
 uint8_t *position=s->object->view.alias_f0;
 const gh_title_player_view *player=s->environment.current_player;
 if(!s->runtime_bound||!s->runtime.init||!s->runtime.canonical_rate||
    !s->runtime.scene_tables||!position||!player||!player->alias_f0||
    policy!=GH_STATIC_BINARY32_REFERENCE)
  return fail(s,0x1b8c6c,frontier,GH_OBJSTEP_ARGUMENT);
 if(s->special_phase>=4)return catapult_impact_001b8e08(s,frontier);
 uint32_t pc=0;
 if(!s->special_phase){
  int hit;
  const gh_hitarea_object *area=init->hitarea_created?&init->hitarea:NULL;
  if(gh_om_check_damage_001b7380(&s->damage_calls[0],&s->object->view,area,
      s->runtime.init->heat+0x10,policy,&hit,&pc))
   return fail(s,pc?pc:0x1b7380,frontier,GH_OBJSTEP_CALLEE);
  if(hit){
   float velocity[3];
   for(unsigned axis=0;axis<3;++axis)
    velocity[axis]=mul(number(actor+0x5c0+axis*4),0.08f);
   velocity[1]=add(velocity[1],0.1f);
   memcpy(actor+0x610,velocity,sizeof(velocity));
  }
  s->special_phase=hit?1:2;
 }
 if(s->special_phase==1){
  if(gh_hitarea_object_set_active_001fd668(&init->hitarea,0,&pc))
   return fail(s,pc?pc:0x1fd668,frontier,GH_OBJSTEP_CALLEE);
  s->special_phase=2;
 }
 if(s->special_phase==2){
  const float rate=mul(number(s->runtime.init->enemies+0x548),*s->runtime.canonical_rate);
  float next[3];
  for(unsigned axis=0;axis<3;++axis)
   next[axis]=add(number(position+axis*4),mul(number(actor+0x610+axis*4),rate));
  const float vertical=add(number(actor+0x614),mul(rate,-0.01f));
  if(!isfinite(vertical)||!isfinite(next[0])||!isfinite(next[1])||!isfinite(next[2]))
   return fail(s,0x292f50,frontier,GH_OBJSTEP_UNSUPPORTED);
  memcpy(position,next,sizeof(next));
  memcpy(actor+0x614,&vertical,sizeof(vertical));
  s->special_phase=3;
 }
 const uint16_t timer=(uint16_t)half(actor+0x628);
 if((int16_t)timer>0){
  sh(actor+0x628,(uint16_t)(timer-1));
  return GH_OBJSTEP_OK;
 }
 float impact[4];
 gh_scene_ray_request ray={.mask=7,.hit_position=impact,.segmented=1};
 memcpy(ray.from,actor+0x490,12);
 memcpy(ray.to,position,12);
 ray.from[3]=ray.to[3]=1.0f;
 int hit;
 if(gh_scene_ray_query_00131e10(s->runtime.scene_tables,&ray,&hit,&pc))
  return fail(s,pc?pc:0x131e10,frontier,GH_OBJSTEP_UNSUPPORTED);
 if(number(position+4)<add(number(player->alias_f0+4),-10.0f)){
  memcpy(impact,position,12);
  hit=1;
 }
 if(!hit)return GH_OBJSTEP_OK;
 memcpy(s->projectile_impact,impact,sizeof(s->projectile_impact));
 s->special_phase=4;
 return catapult_impact_001b8e08(s,frontier);
}

static gh_object_step_status catapult_windup_001b8a50(gh_om_frame_state *s,
 gh_static_transform_policy policy,uint32_t *frontier)
{
 uint8_t *actor=s->object->view.bytes;
 /* 3BD Move table state 3 -> 1BB508 when the room retires the catapult. */
 if(actor[0x2f4]==3){
  if(!s->special_phase){
   actor[0x531]=0xff;
   if(!s->runtime.release_effect)return fail(s,0x306200,frontier,GH_OBJSTEP_UNSUPPORTED);
   uint32_t pc=0;
   if(s->runtime.release_effect(s->runtime.context,&s->object->view,0,2,&pc))
    return fail(s,pc?pc:0x306200,frontier,GH_OBJSTEP_CALLEE);
   s->special_phase=1;
  }
  return remove_object(s,frontier);
 }
 if(actor[0x2f4])return fail(s,0x1b8990,frontier,GH_OBJSTEP_UNSUPPORTED);
 if(!actor[0x2f5]){
  if(!s->runtime_bound||!s->runtime.break_effect||!s->object->view.alias_f0)
   return fail(s,0x306528,frontier,GH_OBJSTEP_UNSUPPORTED);
  const uint32_t position[3]={word(s->object->view.alias_f0),
   word(s->object->view.alias_f0+4),word(s->object->view.alias_f0+8)};
  uint32_t pc=0;
  gh_object_step_status status=s->runtime.break_effect(s->runtime.context,
      &common(s)->model,position,0,&pc);
  if(status)return fail(s,pc?pc:0x306528,frontier,status);
  memcpy(actor+0x600,position,sizeof(position));
  sh(actor+0x628,actor[0x630]?180:90);
  actor[0x630]=1;
  actor[0x2f5]=1;
  actor[0x2f6]=actor[0x2f7]=0;
 }
 if(actor[0x2f5]==2)return catapult_flight_001b8c6c(s,policy,frontier);
 if(actor[0x2f5]==3){
  const uint16_t timer=(uint16_t)half(actor+0x628);
  if((int16_t)timer<=0){
   gh_om_init_state *init=common(s);
   uint32_t pc=0;
   if(init->catapult_attack_created&&gh_hitarea_object_set_active_001fd668(&init->catapult_attack,0,&pc))
    return fail(s,pc?pc:0x1fd668,frontier,GH_OBJSTEP_CALLEE);
   actor[0x2f5]=actor[0x2f6]=actor[0x2f7]=0;
  }
  sh(actor+0x628,(uint16_t)(timer-1));
  return GH_OBJSTEP_OK;
 }
 if(actor[0x2f5]!=1)return fail(s,0x1b8a50,frontier,GH_OBJSTEP_UNSUPPORTED);
 const uint16_t counter=(uint16_t)half(actor+0x628);
 if((int16_t)counter>0){
  sh(actor+0x628,(uint16_t)(counter-1u));
  return GH_OBJSTEP_OK;
 }
 if(word(actor+0x5b8)&1u)return catapult_launch_001b8b68(s,policy,frontier);
 return GH_OBJSTEP_OK;
}

/* 292F70 records the same positional warning as the original grenade. */
static gh_object_step_status om364_warn_00292f70(gh_om_frame_state *frame,
 const gh_om364_state *state,const float position[3],uint32_t *frontier)
{
 gh_game_init_state *init=frame->runtime.init;
 const gh_title_player_view *player=frame->environment.current_player;
 if(!init||!player||!player->bytes||player->size<0x108||!player->alias_f0||
    !state->common.bindings)return fail(frame,0x292f70,frontier,GH_OBJSTEP_ARGUMENT);
 float from[3];
 memcpy(from,player->alias_f0,12);
 const float dx=add(position[0],-from[0]),dz=add(position[2],-from[2]);
 const float distance=add(mul(dx,dx),mul(dz,dz));
 uint32_t pc=0;
 if(distance>=3.2399998f&&distance<=25.0f){
  float angle;
  if(gh_player_target_angle_002da038(state->common.bindings->elf,state->common.bindings->elf_bytes,
      from,position,number(player->bytes+0x104),3.1415927f,&angle,&pc))
   return fail(frame,pc?pc:0x2da038,frontier,GH_OBJSTEP_UNSUPPORTED);
  const float abs_angle=fabsf(angle);
  if(abs_angle>=0.7853982f)
   put(init->enemies+(abs_angle>2.3561945f?0x550:angle>0?0x554:0x558),2);
 }
 memcpy(init->enemies+0x520,position,12);
 put(init->enemies+0x530,2);
 return GH_OBJSTEP_OK;
}

/* 1B1070 expires the fuse before publishing the original two attack groups. */
static gh_object_step_status om364_blast_001b1070(gh_om_frame_state *frame,
 gh_om364_state *state,uint32_t *frontier)
{
 uint8_t *actor=frame->object->view.bytes;
 uint32_t pc=0;
 if(!frame->runtime.stop_throw_voice||!frame->runtime.break_sound||
    !frame->runtime.throw_blast_effect||!frame->runtime.release_effect)
  return fail(frame,0x1b1070,frontier,GH_OBJSTEP_UNSUPPORTED);
 if(frame->special_phase<3){
  const gh_object_step_status status=frame->runtime.stop_throw_voice(
      frame->runtime.context,word(actor+0x668),&pc);
  if(status)return fail(frame,pc?pc:0x2cbba0,frontier,status);
  put(actor+0x668,0);
  frame->special_phase=3;
 }
 if(frame->special_phase==3){
  const gh_motion_sound_request request={.model=&state->common.model,.actor=&frame->object->view,
      .bank=0,.program=2};
  uint32_t handle=0;
  const gh_actor_motion_status status=frame->runtime.break_sound(
      frame->runtime.context,&request,&handle,&pc);
  if(status)return fail(frame,pc?pc:0x2cba48,frontier,
      status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
  frame->special_phase=4;
 }
 if(frame->special_phase==4){
  const gh_object_step_status status=frame->runtime.throw_blast_effect(frame->runtime.context,state,&pc);
  if(status)return fail(frame,pc?pc:0x306838,frontier,status);
  frame->special_phase=5;
 }
 if(frame->special_phase==5){
  for(unsigned i=0;i<2;++i){
   sh(state->areas[i].bytes+0x46,i?0x48:0x30);
   put(state->areas[i].bytes+0x4c,50);
   if(gh_hitarea_object_set_active_001fd668(&state->areas[i],1,&pc))
    return fail(frame,pc?pc:0x1fd668,frontier,GH_OBJSTEP_CALLEE);
  }
  frame->special_phase=6;
 }
 if(frame->special_phase==6){
  const gh_actor_motion_status status=frame->runtime.release_effect(
      frame->runtime.context,&frame->object->view,7,2,&pc);
  if(status)return fail(frame,pc?pc:0x306200,frontier,
      status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
  actor[0x2f4]=3;
  frame->special_phase=7;
 }
 return GH_OBJSTEP_OK;
}

static gh_object_step_status om364_flight_001b1070(gh_om_frame_state *frame,
 gh_om364_state *state,uint32_t *frontier)
{
 uint8_t *p=frame->object->view.bytes;
 uint8_t *world=frame->object->view.alias_f0;
 const float rate=number(p+0x5a8);
 float time=number(p+0x630);
 uint32_t pc=0;
 if(!world||!frame->runtime.init||!frame->runtime.scene_tables||!isfinite(time))
  return fail(frame,0x1b1070,frontier,GH_OBJSTEP_ARGUMENT);
 if(time<=0&&frame->special_phase!=1)return om364_blast_001b1070(frame,state,frontier);
 if(!frame->special_phase){
  float previous[3],position[3],velocity[3],rotation[4],spin[3],wrapped[4];
  memcpy(previous,world,12);
  memcpy(velocity,p+0x640,12);
  memcpy(rotation,p+0x100,16);
  memcpy(spin,p+0x650,12);
  if(!frame->runtime.init->enemies[0x53c]&&
     (int32_t)word(frame->runtime.init->enemies+0x514)<1)time=add(time,-rate);
  for(unsigned i=0;i<3;++i)position[i]=add(previous[i],mul(velocity[i],rate));
  velocity[1]=add(velocity[1],-mul(rate,0.026f));
  const float horizontal=add(mul(velocity[0],velocity[0]),mul(velocity[2],velocity[2]));
  for(unsigned i=0;i<3;++i)
   rotation[i]=horizontal>0.000025f?add(rotation[i],mul(spin[i],rate)):
      mul(rotation[i],add(1,-mul(rate,0.1f)));
  if(gh_actor_euler_adjust_001f7da0(rotation,wrapped,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(frame,pc?pc:0x1f7da0,frontier,GH_OBJSTEP_UNSUPPORTED);
  float hitpos[4],normal[4];
  gh_scene_ray_request ray={
    .from={position[0],previous[1],position[2],1},
    .to={position[0],add(position[1],-10.5f),position[2],1},
    .mask=2,.hit_position=hitpos,.hit_normal=normal};
  int hit;
  if(gh_scene_ray_query_00131e10(frame->runtime.scene_tables,&ray,&hit,&pc))
   return fail(frame,pc?pc:0x131e10,frontier,GH_OBJSTEP_UNSUPPORTED);
  int bounce=0;
  if(hit&&position[1]<add(hitpos[1],0.2f)){
   position[1]=add(hitpos[1],0.2f);
   if(fabsf(velocity[1])<0.104f)memset(velocity,0,12);
   else{
    velocity[0]=mul(velocity[0],0.4f);
    velocity[1]=mul(velocity[1],-0.5f);
    velocity[2]=mul(velocity[2],0.4f);
    for(unsigned i=0;i<3;++i)spin[i]=mul(spin[i],0.4f);
    bounce=!p[0x666];
   }
  }
  ray=(gh_scene_ray_request){.mask=1,.flags=4,.hit_position=hitpos,.hit_normal=normal};
  memcpy(ray.from,previous,12);memcpy(ray.to,position,12);
  ray.from[3]=ray.to[3]=1;
  if(gh_scene_ray_query_00131e10(frame->runtime.scene_tables,&ray,&hit,&pc))
   return fail(frame,pc?pc:0x131e10,frontier,GH_OBJSTEP_UNSUPPORTED);
  if(hit){
   float reflected[3];
   for(unsigned i=0;i<3;++i){
    float row[3];
    for(unsigned j=0;j<3;++j)
     row[j]=add(i==j?1.0f:0.0f,mul(mul(-2.0f,normal[i]),normal[j]));
    reflected[i]=add(add(mul(row[0],velocity[0]),mul(row[1],velocity[1])),mul(row[2],velocity[2]));
    spin[i]=mul(spin[i],0.4f);
   }
   memcpy(velocity,reflected,12);
  }
  gh_object_step_status status=om364_warn_00292f70(frame,state,position,frontier);
  if(status)return status;
  memcpy(world,position,12);
  memcpy(p+0x630,&time,4);
  memcpy(p+0x640,velocity,12);
  memcpy(p+0x650,spin,12);
  memcpy(p+0x100,wrapped,12);
  frame->special_phase=bounce?1:2;
 }
 if(frame->special_phase==1){
  if(!frame->runtime.break_sound)return fail(frame,0x2cba48,frontier,GH_OBJSTEP_UNSUPPORTED);
  const gh_motion_sound_request request={.model=&state->common.model,.actor=&frame->object->view,
      .bank=0,.program=0x118};
  uint32_t handle=0;
  const gh_actor_motion_status status=frame->runtime.break_sound(frame->runtime.context,&request,&handle,&pc);
  if(status)return fail(frame,pc?pc:0x2cba48,frontier,
      status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
  p[0x666]=1;
  frame->special_phase=2;
 }
 if(number(p+0x630)<=0)return om364_blast_001b1070(frame,state,frontier);
 return GH_OBJSTEP_OK;
}


/* 1B0F70 keeps both collision groups inactive before the original
 * per-state move, then 1B1630 updates the held object's parent transform. */
static gh_object_step_status om364_move_001b0f70(gh_om_frame_state *frame,uint32_t *frontier)
{
 gh_om364_state *state=(gh_om364_state *)frame->object->initialization;
 uint8_t *p=frame->object->view.bytes;
 if(!state||state->setup_stage!=8||!frame->runtime_bound||!frame->runtime.canonical_rate)
  return fail(frame,0x1b0f70,frontier,GH_OBJSTEP_UNSUPPORTED);
 if(p[0x2f4]&&p[0x2f4]!=3)return fail(frame,0x1b0f70,frontier,GH_OBJSTEP_UNSUPPORTED);
 uint32_t pc=0;
 for(unsigned i=0;i<2;++i){
  if(!state->areas_created[i]||gh_hitarea_object_set_active_001fd668(&state->areas[i],0,&pc))
   return fail(frame,pc?pc:0x1fd668,frontier,GH_OBJSTEP_UNSUPPORTED);
  sh(state->areas[i].bytes+0x46,0);
  put(state->areas[i].bytes+0x4c,0);
 }
 if(p[0x2f4]==3){
  p[0x531]=0xff;
  return remove_object(frame,frontier);
 }
 const float rate=*frame->runtime.canonical_rate;
 if(!isfinite(rate)||rate<0)return fail(frame,0x292f50,frontier,GH_OBJSTEP_UNSUPPORTED);
 memcpy(p+0x5a8,&rate,4);
 p[0x531]=2;
 if(!word(p+0x600)){
  const gh_object_step_status status=om364_flight_001b1070(frame,state,frontier);
  if(status)return status;
 }
 const gh_actor_part *part;
 if(gh_actor_model_part_at(&state->common.model,1,&part))
  return fail(frame,0x1b1938,frontier,GH_OBJSTEP_ARGUMENT);
 float factor=mul(number(p+0x630),0.006666667f);
 if(factor>1)factor=1;
 if(factor<0)factor=0;
 const float fold=mul(add(1,-factor),1.5707964f);
 memcpy(part->bytes+0x108,&fold,4);
 if(!word(p+0x600))return GH_OBJSTEP_OK;
 if(!state->parent||!frame->runtime.resolve_actor||word(p+0x600)!=state->parent_handle)
  return fail(frame,0x1b1630,frontier,GH_OBJSTEP_ARGUMENT);
 gh_title_player_view parent;
 if(frame->runtime.resolve_actor(frame->runtime.context,state->parent_handle,&parent,&pc)||
    parent.bytes!=state->parent->root.bytes||parent.size!=state->parent->root.size)
  return fail(frame,pc?pc:0x1b1630,frontier,GH_OBJSTEP_ARGUMENT);

 if(gh_actor_model_part_at(state->parent,word(p+0x604),&part))
  return fail(frame,0x1b1630,frontier,GH_OBJSTEP_ARGUMENT);
 const float offset[3]={number(p+0x610),number(p+0x614),number(p+0x618)};
 float matrix[16],position[4];
 memcpy(matrix,part->bytes+0x80,sizeof(matrix));
 if(gh_scene_at_apply_world_001508d8(matrix,offset,position))
  return fail(frame,0x30a2b0,frontier,GH_OBJSTEP_UNSUPPORTED);
 memcpy(frame->object->view.alias_f0,position,12);
 put(p+0x24c,word(parent.bytes+0x24c));
 put(p+0x250,(word(p+0x250)&~0x10u)|(word(parent.bytes+0x250)&0x10u));
 if(!p[0x667]){
  if(!frame->runtime.throw_fuse_effect)return fail(frame,0x306528,frontier,GH_OBJSTEP_UNSUPPORTED);
  const gh_object_step_status status=frame->runtime.throw_fuse_effect(frame->runtime.context,state,&pc);
  if(status)return fail(frame,pc?pc:0x306528,frontier,status);
  p[0x667]=1;
 }
 if(!state->fuse_sound_submitted){
  if(!frame->runtime.break_sound)return fail(frame,0x2cba48,frontier,GH_OBJSTEP_UNSUPPORTED);
  const gh_motion_sound_request request={.model=&state->common.model,
      .actor=&frame->object->view,.bank=0,.program=0x117};
  uint32_t handle=0;
  const gh_actor_motion_status status=frame->runtime.break_sound(frame->runtime.context,&request,&handle,&pc);
  if(status)return fail(frame,pc?pc:0x2cba48,frontier,
      status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
  put(p+0x668,handle);
  state->fuse_sound_submitted=1;
 }
 return GH_OBJSTEP_OK;
}


static gh_object_step_status body(gh_om_frame_state*s,const gh_boot_work*w,gh_static_transform_policy policy,uint32_t*f){
 uint8_t*p=s->object->view.bytes;gh_om_init_state*c=common(s);
 if(!s->phase){if(word((const uint8_t*)w+0x5d8)&0x20000000u){s->complete=1;return GH_OBJSTEP_OK;}put(p+0x5b4,0);s->phase=1;}
 if(s->phase==1&&s->object->source.type==0x311){
  gh_object_step_status status=om311_move(s,policy,f);
  if(status)return status;
  s->phase=4;
 }
 if(s->phase==1&&catapult(s->object)){
  gh_object_step_status status=catapult_windup_001b8a50(s,policy,f);
  if(status)return status;
  s->phase=4;
 }
 if(s->phase==1&&em0f_throw(s->object)){
  gh_object_step_status status=om364_move_001b0f70(s,f);
  if(status)return status;
  s->phase=4;
 }
 if(s->phase==1&&devil_projectile(s->object)){
  gh_object_step_status status=projectile_move_001b93e8(s,w,policy,f);
  if(status)return status;
  s->phase=4;
 }
 if(s->phase==1&&r101_item(s->object)){
  gh_object_step_status status=r101_item_move(s,w,f);
  if(status)return status;
  s->phase=4;
 }
 if(s->phase==1&&s->object->source.type==0x357){
  if(p[0x2f4])return fail(s,s->entry,f,GH_OBJSTEP_UNSUPPORTED);
  gh_om041c_state *canopy=(gh_om041c_state *)s->object->initialization;
  gh_scene_placement_status status=gh_om357_move_001a8b38(canopy,s->runtime.ik_globals,
   s->factory->bindings.pools,s->runtime.resolve_actor,s->runtime.context,&s->special_phase);
  if(status)return fail(s,canopy->frontier?canopy->frontier:0x1a8b38,f,GH_OBJSTEP_UNSUPPORTED);
  s->phase=4;
 }
 if(s->phase==1&&s->object->source.type==0x41c){gh_object_step_status e=flag_move(s,policy,f);if(e)return e;s->phase=4;}
 if(s->phase==1&&weapon(s->object)){
  uint32_t pc=0;
  gh_object_step_status e=gh_om382_move_prefix_001c9860((gh_om382_state*)s->object->initialization,
   &s->runtime,s->environment_bound?&s->environment:NULL,&s->special_phase,&s->runtime_rate,policy,&pc);
  if(e)return fail(s,pc?pc:0x1c9860,f,e);
  if(s->special_phase==7){e=remove_object(s,f);if(e)return e;}
  s->phase=5;
 }
 if(s->phase==1&&tutorial(s->object)){gh_object_step_status e=special3eb(s,policy,f);if(e)return e;s->phase=4;}
 if(s->phase==1&&mount(s->object)){
  /* 1A7880 dispatches the primary state, then 1A7940 the secondary state. */
  static const uint32_t primary[]={0x1a7940,0x1a7ea0,0x1a8680,0x1a8688};
  const unsigned mode=p[0x2f4];
  if(mode)return fail(s,mode<4?primary[mode]:s->entry,f,GH_OBJSTEP_UNSUPPORTED);
  const unsigned step=p[0x2f5];
  if(step==1){
   gh_object_step_status status=mount_visit_001a7a28(s,f);
   if(status)return status;
  }else if(step)return fail(s,0x1a7940,f,GH_OBJSTEP_UNSUPPORTED);
  else if(p[0x615]){ /* Original 1A7A08 does not visit 1A7A28 until next Move. */
   p[0x2f7]=0;
   p[0x2f5]=1;
   p[0x2f6]=0;
  }
  s->phase=4;
 }
 if(s->phase==1&&s->object->source.type==0x356){
  /* 3BDBE0 dispatches empty 1A8838 on the original intact state. */
  const unsigned mode=p[0x2f4];
  static const uint32_t entries[]={0x1a8838,0x1a8840,0x1a8b38,0x1a91f0};
  if(mode)return fail(s,mode<4?entries[mode]:s->entry,f,GH_OBJSTEP_UNSUPPORTED);
  s->phase=4;
 }
 if(s->phase==1&&s->object->source.type==0x355){
  const unsigned mode=p[0x2f4];
  /* 1A8680 and 1A8838 are empty original state methods. */
  if(mode!=0&&mode!=2)return fail(s,mode==1?0x1a8688:mode==3?0x1a8840:0x1a85b8,f,GH_OBJSTEP_UNSUPPORTED);
  s->phase=4;
 }
 if(s->phase==1&&s->object->source.type==0x35d){gh_object_step_status e=special35d(s,policy,f);if(e)return e;s->phase=4;}
 if(s->phase==1){if(fragments(s->object)){size_t timer_offset=s->object->source.type==0x304?0x602:0x610;unsigned timer=half(p+timer_offset);if(timer)sh(p+timer_offset,(uint16_t)(timer-1));}else if(door(s->object)&&!(word(p+0x5b0)&2)){gh_object_step_status e=gh_om_frame_contacts_001bd430(s,f);if(e)return e;}if(simple(s->object)&&(word(p+0x5b0)&4))return fail(s,0x1c7e30,f,GH_OBJSTEP_UNSUPPORTED);s->phase=2;}
 if(s->phase==2){
  if(s->object->source.type==0x351){
   const unsigned state=p[0x2f4];
   static const uint32_t methods[]={0x1a62b8,0x1a6310,0x1a6550,0x1a6cd8};
   if(state)return fail(s,state<4?methods[state]:s->entry,f,GH_OBJSTEP_UNSUPPORTED);
   s->phase=3;
  }else if(fragment_cylinders(s->object)){
   const unsigned kind=s->object->source.type;
   const unsigned phase=p[0x2f4];
   static const uint32_t methods[][4]={
    {0x18a988,0x18aa00,0x18aa60,0x18ad88},
    {0x18d298,0x18d310,0x18d370,0x18d698},
    {0x18e050,0x18e0c8,0x18e128,0x18e450},
    {0x19c980,0x19c9f8,0x19ca58,0x19cd80},
   };
   const unsigned index=kind==0x308?0:kind==0x30b?1:kind==0x30c?2:3;
   if(kind!=0x308&&(word(p+0x5b0)&4u))
    return fail(s,0x1c7e30,f,GH_OBJSTEP_UNSUPPORTED);
   const int layered=kind==0x331||(kind==0x332&&w->room==0x104&&
       s->object->view.original_vtable214==0x425c78u);
   if(layered&&phase==1){
    /* 19C9F8 starts the fifteen-tick wait and decrements immediately. */
    if(!p[0x2f5]){
     sh(p+0x654,15);
     p[0x2f5]=1;
     p[0x2f6]=p[0x2f7]=0;
    }
    if(p[0x2f5]==1){
     const uint16_t left=(uint16_t)(half(p+0x654)-1u);
     sh(p+0x654,left);
     if(!left||(left&0x8000u))p[0x2f4]=p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
    }
    s->phase=4;
   }else if(layered&&(phase==2||phase==3||s->special_phase==8)){
    gh_object_step_status status=(phase==2||s->special_phase==8)?box_break(s,w,policy,f):remove_object(s,f);
    if(status)return status;
    s->phase=4;
   }else{
    if(phase)return fail(s,phase<4?methods[index][phase]:s->entry,f,GH_OBJSTEP_UNSUPPORTED);
    s->phase=3;
   }
  }else if (p[0x2f4] == 1 && (fragments(s->object) || segments(s->object))) {
   uint32_t entry = state_entry(s->object->source.type, 1);
   uint32_t pc = 0;
   gh_object_step_status status;
   if (segments(s->object)) {
    status = gh_om_segment_reaction(&s->object->view, entry, &pc);
   } else if (s->object->source.type == 0x304) {
    entry = 0x188190;
    status = gh_om_reaction_00188190(&s->object->view, &pc);
   } else {
    entry = 0x1c2c40;
    status = gh_om_reaction_001c2c40(&s->object->view, &pc);
   }
   if (status) return fail(s, pc ? pc : entry, f, status);
   /* Damage belongs to the state-0 handler. Even a state-1 countdown
    * that clears 2F4 does not dispatch another state in this call. */
   s->phase = 4;
  } else if (plain(s->object) && p[0x2f4] && p[0x2f4] <= 3) {
   gh_object_step_status status = GH_OBJSTEP_OK;
   if (p[0x2f4] == 2) status = plain_break(s, w, policy, f);
   else if (p[0x2f4] == 3) status = remove_object(s, f);
   if (status) return status;
   s->phase = 4;
  } else if((s->object->source.type==0x30a||s->object->source.type==0x304||
       (w->room==0x102&&s->object->source.type==0x309)||
       (w->room==0x103&&s->object->source.type==0x33f))&&
       (p[0x2f4]==2||p[0x2f4]==3||s->special_phase==8)){
   /* 18C8A8, 188528 and 19F8B0 share 1B79B0 -> 312708, VT+4C empty. */
   gh_object_step_status status=(p[0x2f4]==2||s->special_phase==8)?
       box_break(s,w,policy,f):remove_object(s,f);
   if(status)return status;
   s->phase=4;
  } else if(s->object->source.type==0x305&&(p[0x2f4]==2||p[0x2f4]==3||s->special_phase==8)){
   gh_object_step_status status=(p[0x2f4]==2||s->special_phase==8)?fragment305_break(s,w,policy,f):remove_object(s,f);
   if(status)return status;
   s->phase=4;
  } else if(s->object->source.type==0x346&&w->room==0x100&&p[0x2f4]==3){
   gh_object_step_status status=remove_object(s,f);
   if(status)return status;
   s->phase=4;
  } else if(s->object->source.type==0x346&&w->room==0x100&&p[0x2f4]==2){
   gh_object_step_status status=om346_break_001a17c8(s,w,policy,f);
   if(status)return status;
   s->phase=4;
  } else if(s->object->source.type==0x307&&w->room==0x100&&p[0x2f4]&&p[0x2f4]<=3){
   gh_object_step_status status=GH_OBJSTEP_OK;
   if(p[0x2f4]==1)om307_hit_reaction_001897b8(p);
   else if(p[0x2f4]==2)status=om307_break_00189818(s,w,policy,f);
   else status=remove_object(s,f);
   if(status)return status;
   s->phase=4;
  } else if (fragments(s->object) && p[0x2f4]) {
   unsigned index = p[0x2f4];
   uint32_t pc = s->object->source.type == 0x304
                 ? (index == 2 ? 0x1881f0 : index == 3 ? 0x188528 : 0x187fe8)
                 : (index == 2 ? 0x1c2da0 : index == 3 ? 0x1c3070 : 0x1c29d0);
   return fail(s, pc, f, GH_OBJSTEP_UNSUPPORTED);
  } else if (fragments(s->object)) {
   s->phase = 3;
  }
  else if(s->object->source.type==0x325&&p[0x2f4]==1){
   if(!p[0x2f5]){
    sh(p+0x654,15);
    p[0x2f5]=1;
    p[0x2f6]=p[0x2f7]=0;
   }
   if(p[0x2f5]==1){
    const uint16_t timer=(uint16_t)(half(p+0x654)-1);
    sh(p+0x654,timer);
    if((int16_t)timer<=0)memset(p+0x2f4,0,4);
   }
   s->phase=4;
  }
  else if(simple(s->object)||plain(s->object)||segments(s->object)||s->object->source.type==0x307){if(p[0x2f4]){uint32_t pc=state_entry(s->object->source.type,p[0x2f4]);return fail(s,pc?pc:s->entry,f,GH_OBJSTEP_UNSUPPORTED);}}
  else{
   if(((s->object->source.type==0x300&&w->room==0x100)||
       ((s->object->source.type==0x313||s->object->source.type==0x330)&&w->room==0x101))&&
      (p[0x2f4]==2||p[0x2f4]==3||s->special_phase==8)){
    gh_object_step_status status=(p[0x2f4]==2||s->special_phase==8)?
      door_break_001bc748(s,c,w,policy,f):remove_object(s,f);
    if(status)return status;
    s->phase=4;
   }else{
   if(p[0x2f4])return fail(s,p[0x2f4]==1?0x1bc6e8:p[0x2f4]==2?0x1bc748:p[0x2f4]==3?0x1bcb18:0x1bc0a0,f,GH_OBJSTEP_UNSUPPORTED);
   if(p[0x2f5]==1||p[0x2f5]==2){
    gh_object_step_status status=door_turn_step(s,c,p[0x2f5]==2,policy,f);
    if(status)return status;
   }else{
    if(p[0x2f5])return fail(s,p[0x2f5]==2?0x1bc610:0x1bc1c8,f,GH_OBJSTEP_UNSUPPORTED);
    if(word(p+0x620)&1){
     gh_object_step_status status=door_wait_open(s,c,policy,f);
     if(status)return status;
    }else{
    unsigned rawcounter=half(p+0x630);int counter=rawcounter<32768?(int)rawcounter:rawcounter-65536;
    if(s->contact634&&counter>4){
     gh_object_step_status status=door_wait_contact(s,c,policy,f);
     if(status)return status;
    }
    }
   }
  }
  }
  if (s->phase == 2) s->phase = 3;
 }
 if(s->phase==3){unsigned count=(s->object->source.type==0x307||s->object->source.type==0x346)?2:1;
  while(s->hit_index<count){unsigned i=s->hit_index;const gh_hitarea_object*hit=&c->hitarea;int present=c->hitarea_created;
   if(i){if(s->object->source.type==0x307){gh_om307_state*b=(gh_om307_state*)s->object->initialization;hit=&b->box_b2;present=b->b2_created;}else{gh_om346_state*b=(gh_om346_state*)s->object->initialization;hit=&b->box_b2;present=b->b2_created;}}
   if(present&&(hit->state!=c->bindings->init))return fail(s,0x1b7380,f,GH_OBJSTEP_ARGUMENT);
   const uint8_t*mode=present?hit->state->heat+0x10:NULL;int original_hit=0;uint32_t pc=0;gh_om_damage_status e=gh_om_check_damage_001b7380(&s->damage_calls[i],&s->object->view,present?hit:NULL,mode,policy,&original_hit,&pc);
   if(e)return fail(s,pc?pc:0x1b7380,f,e==GH_OM_DAMAGE_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
   if(original_hit){
    if(s->object->source.type==0x351){
     if((int16_t)half(p+0x54a)<1)p[0x605]=1;
     p[0x2f4]=1;
     p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
    }else hit_transition(s,i,hit);
   }
   s->hit_index++;
  }s->phase=4;
 }
 if(s->phase==4&&s->object->source.type==0x351&&!s->special_phase){
  switch(p[0x604]){
  case 0:p[0x605]=0;p[0x604]=1;p[0x606]=0;break;
  case 1:if(p[0x605]==1)p[0x604]=3;break;
  case 2:case 3:case 4:return fail(s,0x1a6558,f,GH_OBJSTEP_UNSUPPORTED);
  }
  s->special_phase=1;
 }
 if(s->phase==4){if(s->object->source.type==0x346&&w->room==0x100&&word(p+0xd90)){
  const gh_object_step_status result=fragment_tick_records(s,w,&c->model,p+0x610,0,20,f);
  if(result)return result;
 }else if(segments(s->object)){unsigned count=p[0x2b4],limit=s->object->source.type==0x346?20:10;if(count!=c->model.part_count)return fail(s,0x14bfe0,f,GH_OBJSTEP_UNSUPPORTED);if(count>limit)count=limit;for(unsigned i=0;i<count;i++){size_t offset=0x610+(size_t)i*0x60;if(offset>s->object->view.size||0x55>s->object->view.size-offset)return fail(s,0x173900,f,GH_OBJSTEP_ARGUMENT);if(p[offset+0x54]==1)return fail(s,0x292f50,f,GH_OBJSTEP_UNSUPPORTED);}}
 if((fragments(s->object)||fragment_cylinders(s->object)||(s->object->source.type==0x351&&!(word(p+0x5b0)&2u))||simple(s->object)||s->object->source.type==0x355||s->object->source.type==0x35d||s->object->source.type==0x41c||s->object->source.type==0x357||(r101_item(s->object)&&!p[0x675])||tutorial(s->object))&&!(s->object->source.type==0x310&&(word(p+0x5b0)&2))&&!s->opacity_complete){uint32_t pc=0;gh_camera_status e=gh_game_object_opacity_002a87e8(&s->object->view,NULL,w,s->environment_bound?&s->environment:NULL,policy==GH_STATIC_BINARY32_REFERENCE?GH_CAMERA_NATIVE_BINARY32_REFERENCE:GH_CAMERA_EXACT_UNAVAILABLE,&pc);if(e)return fail(s,pc?pc:0x2a87e8,f,e==GH_CAMERA_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);s->opacity_complete=1;}
 if(r101_item(s->object)){
  gh_object_step_status status=r101_item_save_position(s,f);
  if(status)return status;
 }
 if((word(p+0x5b0)&1)&&!(word(p+0x5b8)&0x200)&&!(word((const uint8_t*)w+0x5e4)&0x1000000))put(p+0x5b8,word(p+0x5b8)&~0x40u);s->phase=5;}
 if(s->phase==5){if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x149220,f,GH_OBJSTEP_UNSUPPORTED);gh_object_step_status e=model_rate(s,weapon(s->object)?s->runtime_rate:1,f);if(e)return e;s->phase=6;}
 if(s->phase==6){const int weapon_model=weapon(s->object);
  if(weapon_model&&word(p+0x600))return fail(s,0x1cf3a8,f,GH_OBJSTEP_UNSUPPORTED);
  if(weapon_model&&c->model.external_owner){
   const uint8_t *parent;
   if(p[0x2f4]||p[0x2f5]!=1||c->model.external_owner!=s->object||
      gh_actor_model_external_parent_at(&c->model,0,&parent)||
      word(p+0x604)!=c->model.external_parent_index)return fail(s,0x1cf3a8,f,GH_OBJSTEP_UNSUPPORTED);
   const gh_title_player_view *parent_actor=&c->model.external_parent->root;
   if(paper(s->object)){
    if(!s->environment_bound||!s->environment.current_player||
       parent_actor->bytes!=s->environment.current_player->bytes)
     return fail(s,0x1cf3a8,f,GH_OBJSTEP_UNSUPPORTED);
   }else if(!s->object->runtime_created||s->object->runtime_kind!=0x363||w->room!=0x102||
      parent_actor->size<0x568||word(parent_actor->bytes+0x564)!=0x200)
    return fail(s,0x1cf3a8,f,GH_OBJSTEP_UNSUPPORTED);
   const gh_event_weapon_attachment attachment={&s->object->view,&c->model,
    parent_actor,c->model.external_parent,(int32_t)word(p+0x604)};
   uint32_t pc=0;
   if(gh_event_weapon_follow_001cf3a8(&attachment,policy,&pc))
    return fail(s,pc?pc:0x1cf3a8,f,GH_OBJSTEP_UNSUPPORTED);
  }
  int changed=weapon_model||(word(p+0x5b8)&0x18)!=0;
  if(!changed){if(!s->object->view.alias_f0)return fail(s,0x1b7770,f,GH_OBJSTEP_ARGUMENT);for(unsigned k=0;k<4;k++)if(number(p+0x490+k*4)!=number(s->object->view.alias_f0+k*4)||number(p+0x5d0+k*4)!=number(p+0x100+k*4))changed=1;}
  if(changed){if(gh_actor_model_update_hierarchy_00149eb8(&c->model,policy))return fail(s,c->model.frontier?c->model.frontier:0x149eb8,f,GH_OBJSTEP_CALLEE);if(!weapon_model)put(p+0x5b8,word(p+0x5b8)&~0x10u);}s->phase=7;
 }
 if(s->phase==7){if(!s->object->view.alias_f0)return fail(s,0x1b7880,f,GH_OBJSTEP_ARGUMENT);memmove(p+0x490,s->object->view.alias_f0,12);memmove(p+0x5d0,p+0x100,12);s->phase=8;}
 if (!c->model.external_owner&&number(s->object->view.alias_f0+4)<-1000) {
  if (!plain(s->object)&&s->object->source.type!=0x41c&&!weapon(s->object)&&!mount(s->object)) return fail(s,0x1b79b0,f,GH_OBJSTEP_UNSUPPORTED);
  gh_object_step_status status = remove_object(s, f);
  if (status) return status;
 }
 s->complete=1;return GH_OBJSTEP_OK;
}
gh_object_step_status gh_om_frame_move(gh_om_frame_state*s,const gh_om_factory*f,const gh_object_move_request*q,const gh_boot_work*w,gh_static_transform_policy policy,uint32_t*pc){
 if(pc)*pc=0;gh_object_step_status e=bind(s,f,q,w,pc);if(e)return e;s->frontier=0;if(s->complete)return GH_OBJSTEP_OK;fenv_t env;if(feholdexcept(&env))return fail(s,s->entry,pc,GH_OBJSTEP_CALLEE);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(s,s->entry,pc,GH_OBJSTEP_CALLEE);}e=body(s,w,policy,pc);if(fesetenv(&env))return fail(s,s->entry,pc,GH_OBJSTEP_CALLEE);return e;
}
gh_object_step_status gh_om_frame_next_call(gh_om_frame_state*s){if(!s||!s->bound)return GH_OBJSTEP_ARGUMENT;if(!s->complete)return GH_OBJSTEP_CALLEE;s->phase=0;s->complete=0;s->node_index=0;s->opacity_complete=0;s->hit_index=0;s->special_phase=0;memset(s->damage_calls,0,sizeof(s->damage_calls));s->frontier=0;return GH_OBJSTEP_OK;}

gh_object_step_status gh_om_frame_bind_environment(gh_om_frame_state*s,const gh_object_opacity_environment*b){
 if(!s||!b)return GH_OBJSTEP_ARGUMENT;
 if(s->environment_bound){
  gh_object_opacity_environment *a=&s->environment;
  if(a->init!=b->init||a->current_player!=b->current_player||
     a->player_binding_known!=b->player_binding_known||
     (!s->complete&&a->camera!=b->camera))return GH_OBJSTEP_ARGUMENT;
  /* A completed Move may borrow the camera selected earlier in the next
   * scene pass. Keep a suspended Move tied to its original borrow. */
  a->camera=b->camera;
  return GH_OBJSTEP_OK;
 }
 s->environment=*b;s->environment_bound=1;return GH_OBJSTEP_OK;
}

gh_object_step_status gh_om_frame_model_at(const gh_om_frame_state*s,gh_actor_model**out){
 if(!s||!out||!s->bound||!s->factory||!s->factory->bound||!s->factory->objects||!s->object)return GH_OBJSTEP_ARGUMENT;
 const gh_om_factory*f=s->factory;gh_om_object*o=s->object;int found=0;for(size_t i=0;i<f->count;i++)if(f->objects+i==o&&f->objects[i].handle==s->handle){found=1;break;}if(!found||!o->registered)return GH_OBJSTEP_ARGUMENT;
 void*registered=NULL;if(gh_room_object_lookup(f->bindings.pools,s->handle,GH_ROOM_OBJECT_OM,f,&registered)||registered!=o)return GH_OBJSTEP_ARGUMENT;
 gh_om_init_state*c=common(s);if(!c||!c->model.bound||!c->model.ready||c->model.native_identity!=&c->model||c->model.pools!=f->bindings.pools||c->model.root.bytes!=o->view.bytes||c->model.root.size!=o->view.size)return GH_OBJSTEP_UNSUPPORTED;
 *out=&c->model;return GH_OBJSTEP_OK;
}

/* Read-only registered model borrow; no original Move or virtual call. */
gh_object_step_status gh_om_factory_model_at(const gh_om_factory*f,uint32_t handle,gh_actor_model**out){
 if(!f||!out||!f->bound||!f->objects||!f->bindings.pools)return GH_OBJSTEP_ARGUMENT;
 gh_om_object*o=NULL;for(size_t i=0;i<f->count;i++)if(f->objects[i].handle==handle){o=f->objects+i;break;}
 if(!o||!o->registered||!o->initialization)return GH_OBJSTEP_ARGUMENT;
 void*registered=NULL;if(gh_room_object_lookup(f->bindings.pools,handle,GH_ROOM_OBJECT_OM,f,&registered)||registered!=o)return GH_OBJSTEP_ARGUMENT;
 gh_om_frame_state borrow={0};borrow.object=o;gh_om_init_state*c=common(&borrow);
 if(o->source.type==0x35d)c=&((gh_om35d_state*)o->initialization)->common;
 if(o->runtime_created&&o->runtime_kind==0x3eb&&!o->source.bytes)c=o->initialization;
 if(!c||!c->model.bound||!c->model.ready||c->model.native_identity!=&c->model||c->model.pools!=f->bindings.pools||c->model.root.bytes!=o->view.bytes||c->model.root.size!=o->view.size)return GH_OBJSTEP_UNSUPPORTED;
 *out=&c->model;return GH_OBJSTEP_OK;
}

gh_object_step_status gh_om_frame_bind_runtime(gh_om_frame_state*s,const gh_om_frame_runtime*b){
 if(!s||!b||!b->init||!b->canonical_rate||!b->solids)return GH_OBJSTEP_ARGUMENT;
 if(s->runtime_bound){
  const gh_om_frame_runtime *old=&s->runtime;
  return old->init==b->init&&old->canonical_rate==b->canonical_rate&&old->solids==b->solids&&
   old->context==b->context&&old->break_effect==b->break_effect&&old->break_sound==b->break_sound&&
   old->break_collision==b->break_collision&&old->remove_sound==b->remove_sound&&old->ik_globals==b->ik_globals&&
   old->throw_fuse_effect==b->throw_fuse_effect&&old->release_effect==b->release_effect&&old->fragment_atan_coeff==b->fragment_atan_coeff&&
   old->action_buttons==b->action_buttons&&old->resolve_actor==b->resolve_actor&&
   old->scene_tables==b->scene_tables&&old->drop_item==b->drop_item&&old->award_item==b->award_item&&
   old->mount_release==b->mount_release?
   GH_OBJSTEP_OK:GH_OBJSTEP_ARGUMENT;
 }
 s->runtime=*b;s->runtime_bound=1;return GH_OBJSTEP_OK;
}
