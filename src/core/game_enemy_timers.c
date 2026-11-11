#include "game_enemy_timers.h"
#include <string.h>
#include <stdlib.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
/* Positive normalbinary32 operations inthisboundedreference domain. Exact
 * uint64 significands replace hostFEnv/implicit conversions. */
static uint32_t pack(uint64_t n,int power){if(!n)return 0;unsigned high=0;for(uint64_t k=n;k>>=1;)high++;int exponent=(int)high+power;uint64_t mantissa=high>23?n>>(high-23):n<<(23-high);return (uint32_t)(exponent+127)<<23|((uint32_t)mantissa&0x7fffff);}
static uint32_t multiply(uint32_t a,uint32_t b){if(!a||!b)return 0;uint64_t n=(uint64_t)((a&0x7fffff)|0x800000)*((b&0x7fffff)|0x800000);return pack(n,(int)(a>>23)+(int)(b>>23)-300);}
/* Inputs are bounded positive normals from the enemy timer expressions.
 * Their significand exponent difference is at most 32 bits. */
static uint32_t add_timer_base(uint32_t value,uint32_t base)
{
 if(!value)return base;
 int power=(int)(value>>23)-150,base_power=(int)(base>>23)-150;
 uint64_t a=(value&0x7fffff)|0x800000,b=(base&0x7fffff)|0x800000;
 if(power<base_power)return pack(a+(b<<(unsigned)(base_power-power)),power);
 return pack((a<<(unsigned)(power-base_power))+b,base_power);
}
static uint32_t mt_unit_bits(uint32_t raw)
{
 uint32_t half=raw>>1;
 return half?pack(half,0)-(31u<<23):0;
}
gh_actor_motion_status gh_enemy_mt_timer_reference(uint32_t raw,gh_em_numeric_policy policy,uint32_t*unit,uint32_t*timer){
 if(!unit||!timer)return GH_ACTOR_MOTION_ARGUMENT;if(policy!=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE)return GH_ACTOR_MOTION_UNSUPPORTED;
 uint32_t u=mt_unit_bits(raw);
 uint32_t value=multiply(u,0x40a00000);value=multiply(value,0x41f00000);value=add_timer_base(value,0x43160000);*unit=u;*timer=value;return GH_ACTOR_MOTION_OK;
}
gh_actor_motion_status gh_enemy2a2_mt_timer_reference(uint32_t raw,gh_em_numeric_policy policy,
 uint32_t *timer_bits)
{
 if(!timer_bits)return GH_ACTOR_MOTION_ARGUMENT;
 if(policy!=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE)return GH_ACTOR_MOTION_UNSUPPORTED;
 *timer_bits=add_timer_base(multiply(mt_unit_bits(raw),0x42b40000),0x42b40000);
 return GH_ACTOR_MOTION_OK;
}
struct gh_enemy_held_weapon {
 gh_enemy_weapon factory;
 gh_enemy_weapon_init init;
 gh_game_scene_tables *actors;
 gh_collision_scroll_mutation *scroll;
 uint8_t finished,present;
};

static gh_actor_motion_status weapon_error(gh_enemy_timers *s,uint32_t pc,gh_actor_model_status status)
{
 s->frontier=pc;
 switch(status){
 case GH_ACTOR_MODEL_ARGUMENT: return GH_ACTOR_MOTION_ARGUMENT;
 case GH_ACTOR_MODEL_BOUNDS: return GH_ACTOR_MOTION_BOUNDS;
 case GH_ACTOR_MODEL_BUSY: return GH_ACTOR_MOTION_BUSY;
 case GH_ACTOR_MODEL_UNSUPPORTED: return GH_ACTOR_MOTION_UNSUPPORTED;
 default: return GH_ACTOR_MOTION_CALLEE;
 }
}

static gh_actor_motion_status equip_363(gh_enemy_timers *s,gh_enemy_entry *e,
 gh_game_scene_tables *actors,gh_collision_scroll_mutation *scroll,const gh_hitarea_shape_bindings *shapes)
{
 if(!actors||!scroll){s->frontier=0x26aa30;return GH_ACTOR_MOTION_UNSUPPORTED;}
 if(!s->held){
  s->held=calloc(1,sizeof(*s->held));
  if(!s->held){s->frontier=0x26aa30;return GH_ACTOR_MOTION_CALLEE;}
  s->held->actors=actors;
  s->held->scroll=scroll;
 }
 gh_enemy_held_weapon *held=s->held;
 if(held->actors!=actors||held->scroll!=scroll)return GH_ACTOR_MOTION_BUSY;
 if(held->finished)return GH_ACTOR_MOTION_OK;

 int present;
 gh_actor_model_status status=gh_enemy_weapon_create_0030fa98(&held->factory,&e->b,&present);
 if(status)return weapon_error(s,held->factory.frontier?held->factory.frontier:0x30fa98,status);
 if(present){
  /* Same collision-pair and math providers used by the original scene. */
  int initialized;
  status=gh_enemy_weapon_initialize_001c9268(&held->init,&held->factory,shapes,&initialized);
  if(status)return weapon_error(s,held->init.frontier?held->init.frontier:0x1c9268,status);
  if(!initialized){s->frontier=0x1c9268;return GH_ACTOR_MOTION_UNSUPPORTED;}
  status=gh_enemy_weapon_attach_0026ab20(&held->init,&e->object.view,&e->initialize.model.model,
      actors,scroll);
  if(status)return weapon_error(s,held->init.frontier?held->init.frontier:0x1cf160,status);
 }
 held->present=(uint8_t)present;
 held->finished=1;
 return GH_ACTOR_MOTION_OK;
}

static gh_enemy_weapon_init *registered_weapon(const gh_enemy_timers *s)
{
 if(!s||!s->held||!s->held->finished||!s->held->present)return NULL;
 gh_enemy_held_weapon *held=s->held;
 void *object=NULL;
 if(gh_room_object_lookup(held->factory.bindings.pools,held->factory.handle,
      GH_ROOM_OBJECT_ENEMY_WEAPON,&held->factory,&object)||object!=&held->factory.object)
  return NULL;
 return &held->init;
}

const gh_enemy_weapon_init *gh_enemy_timers_held_weapon(const gh_enemy_timers *s)
{
 return registered_weapon(s);
}

gh_enemy_weapon_init *gh_enemy_timers_held_weapon_runtime(gh_enemy_timers *s)
{
 return registered_weapon(s);
}


gh_actor_motion_status gh_enemy_timers_release_native(gh_enemy_timers *s)
{
 if(!s)return GH_ACTOR_MOTION_ARGUMENT;
 if(!s->held)return GH_ACTOR_MOTION_OK;
 s->held->present=0;
 gh_actor_model_status status=gh_enemy_weapon_release_init(&s->held->init);
 if(status)return weapon_error(s,s->held->init.frontier,status);
 status=gh_enemy_weapon_release_factory(&s->held->factory);
 if(status)return weapon_error(s,s->held->factory.frontier,status);
 free(s->held);
 s->held=NULL;
 return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_timers_with_weapon_00205ae4(gh_enemy_timers*s,
 gh_enemy_node_cache*nodes,gh_em_numeric_policy policy,gh_game_scene_tables *actors,
 gh_collision_scroll_mutation *scroll,const gh_hitarea_shape_bindings *shapes){
 if(!s||!nodes||nodes->stage!=2||!nodes->solids)return GH_ACTOR_MOTION_ARGUMENT;if(s->nodes&&s->nodes!=nodes)return GH_ACTOR_MOTION_BUSY;
 if(policy!=GH_EM_NUMERIC_EXACT&&policy!=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE)return GH_ACTOR_MOTION_ARGUMENT;
 gh_enemy_entry*e=nodes->solids->shadow->pose->motion->hp->tail->entry;uint8_t*p=e->object.view.bytes;
 if(!s->stage){
  if(word(p,0x6ec)||word(p,0x6f0))return GH_ACTOR_MOTION_BUSY;
  unsigned items=word(p,0x1644)&255u;
  if(items){
   if(items!=4||e->request.object_kind!=0x202||e->request.dependency_kind!=0x202||
       e->request.entry_number!=18||e->request.appearance!=9||!shapes||!actors||!scroll){
    s->frontier=0x26aa30;return GH_ACTOR_MOTION_UNSUPPORTED;
   }
   s->nodes=nodes;
   gh_actor_motion_status status=equip_363(s,e,actors,scroll,shapes);
   if(status)return status;
  }
  static const uint16_t clear[]={0x6f8,0x6fc,0x700,0x708,0x704,0x738,0x734,0x6f4,0x70c,0x1964,0x1968,0x196c,0x740,0x744,0x748,0x74c,0x750,0x1970,0x1974};
  for(unsigned i=0;i<sizeof(clear)/sizeof(clear[0]);i++)put(p,clear[i],0);put(p,0x17d4,0x40800000);s->nodes=nodes;s->stage=1;
 }
 if(s->stage==1){static const unsigned fields[]={0x16e4,0x1720,0x1724};for(;s->cursor<3;s->cursor++){
   if(!s->pending){if(gh_game_mt_next_0030f348(&e->b.init->random,&s->pending_word)){s->frontier=0x31d000;return GH_ACTOR_MOTION_ARGUMENT;}s->pending=1;}
   uint32_t unit,timer;gh_actor_motion_status status=gh_enemy_mt_timer_reference(s->pending_word,policy,&unit,&timer);if(status){s->frontier=0x30f530;return status;}
   s->words[s->cursor]=s->pending_word;s->timer_bits[s->cursor]=timer;put(p,fields[s->cursor],timer);s->pending=0;
  }p[0x1869]=0;s->stage=2;
 }
 s->frontier=0x2064fc;e->frontier=s->frontier;return GH_ACTOR_MOTION_UNSUPPORTED;
}

gh_actor_motion_status gh_enemy_timers_initialize_00205ae4(gh_enemy_timers *s,
 gh_enemy_node_cache *nodes,gh_em_numeric_policy policy)
{
 return gh_enemy_timers_with_weapon_00205ae4(s,nodes,policy,NULL,NULL,NULL);
}
