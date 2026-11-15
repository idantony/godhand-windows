/* Readable original waiting-state Move for verified R100 OM classes.
 * Patched SLUS_215.03; semantic host pointers replace raw644/634/414. */
#include "game_om_frame.h"
#include "game_actor_matrix.h"
#include "game_actor_two_bone.h"
#include "game_om041c.h"
#include "game_model_texture_scroll.h"
#include "game_actor_model_nodes.h"
#include "game_om303.h"
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
#include "game_player_targets.h"
#include "game_action_button_set.h"
#include "id_layout_rotation.h"
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
static int paper(const gh_om_object*o){return o&&o->runtime_created&&o->runtime_kind==0x376&&!o->source.bytes;}
static int weapon(const gh_om_object*o){return o&&(o->source.type==0x382||paper(o));}
static int fragments(const gh_om_object*o){return o&&(o->source.type==0x303||o->source.type==0x304||o->source.type==0x305||o->source.type==0x31a||o->source.type==0x320||o->source.type==0x339);}
static int simple(const gh_om_object*o){return o&&(o->source.type==0x30a||o->source.type==0x310||o->source.type==0x33f||o->source.type==0x309);}
static int plain(const gh_om_object*o){return o&&(o->source.type==0x30e||o->source.type==0x324);}
static int segments(const gh_om_object*o){return o&&(o->source.type==0x322||o->source.type==0x323||o->source.type==0x346);}
static int door(const gh_om_object*o){return o&&(o->source.type==0x300||o->source.type==0x354);}
static gh_om_init_state*common(const gh_om_frame_state*s){gh_om_object*o=s->object;if(!o)return NULL;if(tutorial(o))return o->initialization;if(weapon(o))return &((gh_om382_state*)o->initialization)->common;if(door(o)||o->source.type==0x355)return o->initialization;if(o->source.type==0x41c)return &((gh_om041c_state*)o->initialization)->common;if(o->source.type==0x35d)return &((gh_om35d_state*)o->initialization)->common;if(o->source.type==0x307)return &((gh_om307_state*)o->initialization)->common;if(o->source.type==0x310)return &((gh_om310_state*)o->initialization)->base.common;if(o->source.type==0x322)return &((gh_om322_state*)o->initialization)->common;if(o->source.type==0x323)return &((gh_om323_state*)o->initialization)->common;if(o->source.type==0x346)return &((gh_om346_state*)o->initialization)->common;return fragments(o)||simple(o)||plain(o)?&((gh_om303_state*)o->initialization)->common:NULL;}
static uint32_t state_entry(uint32_t kind,unsigned index){
 static const uint32_t entries[][5]={{0x307,0x1896e0,0x1897b8,0x189818,0x18a110},{0x30a,0x18c4a8,0x18c520,0x18c580,0x18c8a8},{0x310,0x18f5d0,0x18f618,0x18f858,0x18fbf0},{0x33f,0x19f4b0,0x19f528,0x19f588,0x19f8b0},{0x309,0x18b6a0,0x18b718,0x18b778,0x18baa0},{0x30e,0x1bfc88,0x1bfce0,0x1bfd28,0x1bff48},{0x324,0x1bfc88,0x1bfce0,0x1bfd28,0x1bff48},{0x322,0x1968a0,0x196978,0x1969d8,0x197188},{0x323,0x197ec8,0x197fa0,0x198000,0x1987b0},{0x346,0x1a1690,0x1a1768,0x1a17c8,0x1a20a8}};
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
 uint32_t expected=o->source.type==0x300?0x186928:o->source.type==0x355?0x1a85b8:o->source.type==0x354?0x1a82d8:o->source.type==0x303?0x187c18:o->source.type==0x304?0x187fe8:o->source.type==0x305?0x188c88:o->source.type==0x31a?0x1933f8:o->source.type==0x320?0x195e58:o->source.type==0x307?0x1895d8:o->source.type==0x30a?0x18c370:o->source.type==0x310?0x18f488:o->source.type==0x339?0x19e3d8:o->source.type==0x33f?0x19f378:o->source.type==0x309?0x18b568:o->source.type==0x30e?0x18ee38:o->source.type==0x324?0x198f48:o->source.type==0x322?0x1965f8:o->source.type==0x323?0x197c20:o->source.type==0x346?0x1a13e8:weapon(o)?0x1c9860:o->source.type==0x35d?0x1aedb0:o->source.type==0x41c?0x17ba28:tutorial(o)?0x1bf518:0;
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

static gh_object_step_status fragment305_tick(gh_om_frame_state *s,const gh_boot_work *work,uint32_t *f)
{
 gh_om303_state *fragment=(gh_om303_state*)s->object->initialization;
 gh_actor_model *model=&fragment->common.model;
 uint8_t *actor=s->object->view.bytes;
 const unsigned count=model->part_count<6?model->part_count:6;
 const float rate=mul(number(s->runtime.init->enemies+0x548),number((const uint8_t *)work+0x574));
 if(actor[0x60c]!=6||actor[0x2b4]!=model->part_count||fragment->segments!=actor+0x620||!isfinite(rate))
  return fail(s,0x292f50,f,GH_OBJSTEP_UNSUPPORTED);
 uint8_t records[6][0x60];
 float matrices[6][16];
 memset(matrices,0,sizeof(matrices));
 uint32_t frontier=0;
 for(unsigned i=0;i<count;i++){
  uint8_t *record=records[i];
  memcpy(record,fragment->segments+i*0x60,0x60);
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
 for(unsigned i=0;i<count;i++){
  uint8_t *live=fragment->segments+i*0x60;
  if(live[0x54]!=1)continue;
  memcpy(live,records[i],0x60);
  memcpy(model->parts[i].bytes+0x80,matrices[i],sizeof(matrices[i]));
 }
 return GH_OBJSTEP_OK;
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
 return fragment305_tick(s,work,f);
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

static gh_object_step_status body(gh_om_frame_state*s,const gh_boot_work*w,gh_static_transform_policy policy,uint32_t*f){
 uint8_t*p=s->object->view.bytes;gh_om_init_state*c=common(s);
 if(!s->phase){if(word((const uint8_t*)w+0x5d8)&0x20000000u){s->complete=1;return GH_OBJSTEP_OK;}put(p+0x5b4,0);s->phase=1;}
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
 if(s->phase==1&&s->object->source.type==0x355){
  const unsigned mode=p[0x2f4];
  /* 1A8680 and 1A8838 are empty original state methods. */
  if(mode!=0&&mode!=2)return fail(s,mode==1?0x1a8688:mode==3?0x1a8840:0x1a85b8,f,GH_OBJSTEP_UNSUPPORTED);
  s->phase=4;
 }
 if(s->phase==1&&s->object->source.type==0x35d){gh_object_step_status e=special35d(s,policy,f);if(e)return e;s->phase=4;}
 if(s->phase==1){if(fragments(s->object)){size_t timer_offset=s->object->source.type==0x304?0x602:0x610;unsigned timer=half(p+timer_offset);if(timer)sh(p+timer_offset,(uint16_t)(timer-1));}else if(door(s->object)&&!(word(p+0x5b0)&2)){gh_object_step_status e=gh_om_frame_contacts_001bd430(s,f);if(e)return e;}if(simple(s->object)&&(word(p+0x5b0)&4))return fail(s,0x1c7e30,f,GH_OBJSTEP_UNSUPPORTED);s->phase=2;}
 if(s->phase==2){
  if (p[0x2f4] == 1 && (fragments(s->object) || segments(s->object))) {
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
  } else if(s->object->source.type==0x305&&(p[0x2f4]==2||p[0x2f4]==3||s->special_phase==8)){
   gh_object_step_status status=(p[0x2f4]==2||s->special_phase==8)?fragment305_break(s,w,policy,f):remove_object(s,f);
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
  else if(simple(s->object)||plain(s->object)||segments(s->object)||s->object->source.type==0x307){if(p[0x2f4]){uint32_t pc=state_entry(s->object->source.type,p[0x2f4]);return fail(s,pc?pc:s->entry,f,GH_OBJSTEP_UNSUPPORTED);}}
  else{
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
  if (s->phase == 2) s->phase = 3;
 }
 if(s->phase==3){unsigned count=(s->object->source.type==0x307||s->object->source.type==0x346)?2:1;
  while(s->hit_index<count){unsigned i=s->hit_index;const gh_hitarea_object*hit=&c->hitarea;int present=c->hitarea_created;
   if(i){if(s->object->source.type==0x307){gh_om307_state*b=(gh_om307_state*)s->object->initialization;hit=&b->box_b2;present=b->b2_created;}else{gh_om346_state*b=(gh_om346_state*)s->object->initialization;hit=&b->box_b2;present=b->b2_created;}}
   if(present&&(hit->state!=c->bindings->init))return fail(s,0x1b7380,f,GH_OBJSTEP_ARGUMENT);
   const uint8_t*mode=present?hit->state->heat+0x10:NULL;int original_hit=0;uint32_t pc=0;gh_om_damage_status e=gh_om_check_damage_001b7380(&s->damage_calls[i],&s->object->view,present?hit:NULL,mode,policy,&original_hit,&pc);
   if(e)return fail(s,pc?pc:0x1b7380,f,e==GH_OM_DAMAGE_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
   if(original_hit)hit_transition(s,i,hit);s->hit_index++;
  }s->phase=4;
 }
 if(s->phase==4){if(segments(s->object)){unsigned count=p[0x2b4],limit=s->object->source.type==0x346?20:10;if(count!=c->model.part_count)return fail(s,0x14bfe0,f,GH_OBJSTEP_UNSUPPORTED);if(count>limit)count=limit;for(unsigned i=0;i<count;i++){size_t offset=0x610+(size_t)i*0x60;if(offset>s->object->view.size||0x55>s->object->view.size-offset)return fail(s,0x173900,f,GH_OBJSTEP_ARGUMENT);if(p[offset+0x54]==1)return fail(s,0x292f50,f,GH_OBJSTEP_UNSUPPORTED);}}
 if((fragments(s->object)||simple(s->object)||s->object->source.type==0x355||s->object->source.type==0x35d||s->object->source.type==0x41c||tutorial(s->object))&&!(s->object->source.type==0x310&&(word(p+0x5b0)&2))&&!s->opacity_complete){uint32_t pc=0;gh_camera_status e=gh_game_object_opacity_002a87e8(&s->object->view,NULL,w,s->environment_bound?&s->environment:NULL,policy==GH_STATIC_BINARY32_REFERENCE?GH_CAMERA_NATIVE_BINARY32_REFERENCE:GH_CAMERA_EXACT_UNAVAILABLE,&pc);if(e)return fail(s,pc?pc:0x2a87e8,f,e==GH_CAMERA_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);s->opacity_complete=1;}if((word(p+0x5b0)&1)&&!(word(p+0x5b8)&0x200)&&!(word((const uint8_t*)w+0x5e4)&0x1000000))put(p+0x5b8,word(p+0x5b8)&~0x40u);s->phase=5;}
 if(s->phase==5){if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x149220,f,GH_OBJSTEP_UNSUPPORTED);gh_object_step_status e=model_rate(s,weapon(s->object)?s->runtime_rate:1,f);if(e)return e;s->phase=6;}
 if(s->phase==6){const int weapon_model=weapon(s->object);
  if(weapon_model&&word(p+0x600))return fail(s,0x1cf3a8,f,GH_OBJSTEP_UNSUPPORTED);
  if(weapon_model&&c->model.external_owner){
   const uint8_t *parent;
   if(!paper(s->object)||p[0x2f4]||p[0x2f5]!=1||c->model.external_owner!=s->object||
      !s->environment_bound||!s->environment.current_player||
      gh_actor_model_external_parent_at(&c->model,0,&parent)||
      c->model.external_parent->root.bytes!=s->environment.current_player->bytes||
      word(p+0x604)!=c->model.external_parent_index)return fail(s,0x1cf3a8,f,GH_OBJSTEP_UNSUPPORTED);
   const gh_event_weapon_attachment attachment={&s->object->view,&c->model,
    s->environment.current_player,c->model.external_parent,(int32_t)word(p+0x604)};
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
  if (!plain(s->object)&&s->object->source.type!=0x41c&&!weapon(s->object)) return fail(s,0x1b79b0,f,GH_OBJSTEP_UNSUPPORTED);
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
   old->release_effect==b->release_effect&&old->fragment_atan_coeff==b->fragment_atan_coeff&&
   old->action_buttons==b->action_buttons?
   GH_OBJSTEP_OK:GH_OBJSTEP_ARGUMENT;
 }
 s->runtime=*b;s->runtime_bound=1;return GH_OBJSTEP_OK;
}
