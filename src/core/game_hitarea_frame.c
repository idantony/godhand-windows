#include "game_hitarea_frame.h"
#include "game_hitarea_internal.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static float fl(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static gh_hitarea_status fail(uint32_t*f,uint32_t pc,gh_hitarea_status e){if(f)*f=pc;return e;}
static uint8_t*raw(gh_game_init_state*s,unsigned i){return i<160?s->object_pool+(size_t)i*0x60:s->object_pool+0x3c00+(size_t)(i-160)*0x70;}
/* Equality lookup precedes every dereference of a possibly foreign link. */
static gh_hitarea_status find(gh_game_init_state*s,unsigned slot,gh_game_damage_link*link,unsigned*out,uint32_t*f){
 *out=0;if(!link)return GH_HITAREA_OK;if(!s->hitarea_native||s->hitarea_native->owner!=s)return fail(f,0x1fd770,GH_HITAREA_ARGUMENT);
 for(unsigned i=0;i<672;i++){gh_native_hitarea_node*n=&s->hitarea_native->nodes[i];if(&n->link!=link)continue;uint8_t*r=i<160?s->records_a[i]:s->records_b[i-160];
  if(!n->associated||n->record!=r||!r[0x2e]||n->object.state!=s||n->object.index!=slot||n->object.bytes!=raw(s,slot)||n->object.generation!=s->hitarea_generation||n->object.serial!=s->hitarea_native->serials[slot])return fail(f,0x1fd770,GH_HITAREA_ARGUMENT);
  const uint8_t*p;if(gh_collision_pair_shape_position(&n->shape,&p))return fail(f,0x1fc810,GH_HITAREA_ARGUMENT);*out=i+1;return GH_HITAREA_OK;}
 return fail(f,0x1fd770,GH_HITAREA_ARGUMENT);
}
static gh_hitarea_status prepare(gh_game_init_state*s,gh_hitarea_frame*c,uint32_t*f){
 gh_game_damage_link*l=s->object_links[c->slot];if(w(raw(s,c->slot)+0x3c)&&!l)return fail(f,0x1fceb0,GH_HITAREA_UNSUPPORTED);
 unsigned count=0,i;for(;l;){gh_hitarea_status e=find(s,c->slot,l,&i,f);if(e)return e;if(++count>672)return fail(f,0x1fd770,GH_HITAREA_BOUNDS);l=s->hitarea_native->nodes[i-1].link.next;}
 gh_hitarea_status e=find(s,c->slot,s->object_links[c->slot],&i,f);if(e)return e;c->node=i;c->visited=0;c->slot_started=1;c->query_done=0;c->target=0;c->slot_serial=s->hitarea_native?s->hitarea_native->serials[c->slot]:0;return GH_HITAREA_OK;
}
static gh_hitarea_status reset_b(gh_game_init_state*s,gh_hitarea_frame*c,const gh_boot_work*work,float rate,uint32_t*f){
 uint8_t*b=raw(s,c->slot);gh_title_player_view*a=s->hitarea_native?s->hitarea_native->actors[c->slot]:NULL;uint32_t flags=w((const uint8_t*)work+0x5d8);float next=0;int update=0;
 if(!(flags&0x80000000u)&&!a&&w(b+0x30))return fail(f,0x1fd448,GH_HITAREA_UNSUPPORTED);
 if(!(flags&0x80000000u)&&a){if(!a->bytes||a->size<0x5ac)return fail(f,0x1fd448,GH_HITAREA_BOUNDS);float old=fl(a->bytes+0x54c),speed=fl(a->bytes+0x5a8);if(!isfinite(old)||!isfinite(speed))return fail(f,0x1fd448,GH_HITAREA_UNSUPPORTED);if(old!=0){volatile float product=speed*rate;volatile float sub=old-product;next=sub;if(!isfinite(next))return fail(f,0x1fd448,GH_HITAREA_UNSUPPORTED);if(next<0)next=0;update=1;}}
 if(s->hitarea_native)s->hitarea_native->last_hit_actors[c->slot]=NULL;ww(b+0x34,0);ww(b+0x40,UINT32_MAX);b[0x44]=b[0x45]=0;ww(b+0x60,w(b+0x60)&0xfffffc40u);memset(b+0x10,0,12);if(update)memcpy(a->bytes+0x54c,&next,4);return GH_HITAREA_OK;
}
static int elf_read(const gh_hitarea_frame_bindings*b,uint32_t va,size_t n,uint8_t*out){
 if(!b||!b->boot_elf||b->boot_elf_size<52)return 0;const uint8_t*p=b->boot_elf;size_t z=b->boot_elf_size;if(memcmp(p,"\177ELF\1\1",6))return 0;uint32_t off=w(p+28);unsigned count=p[44]|(unsigned)p[45]<<8,stride=p[42]|(unsigned)p[43]<<8;if(stride<32||off>z||count>(z-off)/stride)return 0;int found=0;
 for(unsigned i=0;i<count;i++){const uint8_t*h=p+off+(size_t)i*stride;if(w(h)!=1)continue;uint32_t base=w(h+8),len=w(h+16),file=w(h+4);if(va<base||(uint64_t)va+n>(uint64_t)base+len)continue;uint64_t at=(uint64_t)file+va-base;if(at>z||n>z-at||found++)return 0;memcpy(out,p+(size_t)at,n);}return found==1;
}
static gh_hitarea_status actor_position(const gh_title_player_view*a,const gh_hitarea_frame_bindings*b,float*out,uint32_t*f){
 if(!a||!a->bytes||a->size<0x218)return fail(f,0x2a9468,GH_HITAREA_BOUNDS);uint32_t vt=w(a->bytes+0x214);if(vt!=a->original_vtable214)return fail(f,0x2a9468,GH_HITAREA_UNSUPPORTED);uint8_t d[8];
 if(!elf_read(b,vt+0x90,8,d))return fail(f,0x2a9468,GH_HITAREA_UNSUPPORTED);
 if(d[0]||d[1]||w(d+4)!=0x2a9468)return fail(f,w(d+4)?w(d+4):0x2a9468,GH_HITAREA_UNSUPPORTED);
 if(!elf_read(b,vt+0x80,8,d))return fail(f,0x2a9458,GH_HITAREA_UNSUPPORTED);
 if(d[0]||d[1]||w(d+4)!=0x2a9458)return fail(f,w(d+4)?w(d+4):0x2a9458,GH_HITAREA_UNSUPPORTED);
 uintptr_t start=(uintptr_t)a->bytes,alias=(uintptr_t)a->alias_f0;if(alias<start||alias-start>a->size||16>a->size-(alias-start))return fail(f,0x2a9458,GH_HITAREA_BOUNDS);
 memcpy(out,a->alias_f0,16);for(unsigned k=0;k<4;k++)if(!isfinite(out[k]))return fail(f,0x131e10,GH_HITAREA_UNSUPPORTED);return GH_HITAREA_OK;
}
static int same_actor(const gh_title_player_view*a,const gh_title_player_view*b){return a&&b&&a->bytes==b->bytes;}
static gh_hitarea_status publish(gh_game_init_state*s,gh_hitarea_frame*c,gh_native_hitarea_node*n,const gh_hitarea_object*owner,gh_title_player_view*a,gh_title_player_view*t,const gh_hitarea_frame_bindings*b,uint32_t*f){
 if(owner->state!=s||owner->fallback||owner->index>=416)return fail(f,0x2bb138,GH_HITAREA_UNSUPPORTED);
 if(!b||!b->current_player)return fail(f,0x2db6b0,GH_HITAREA_UNSUPPORTED);
 int source_player=same_actor(a,b->current_player),target_player=same_actor(t,b->current_player);
 if((source_player||target_player)&&!b->player_pause_words)return fail(f,0x1fe348,GH_HITAREA_UNSUPPORTED);
 uint8_t*source=raw(s,c->slot),*dest=owner->bytes;float direction[3];memcpy(direction,source+0x10,12);
 for(unsigned k=0;k<3;k++)if(!isfinite(direction[k]))return fail(f,0x1fd100,GH_HITAREA_UNSUPPORTED);
 if(direction[0]==0&&direction[1]==0&&direction[2]==0&&t){uintptr_t begin=(uintptr_t)t->bytes,pos=(uintptr_t)t->alias_f0;if(pos<begin||pos-begin>t->size||16>t->size-(pos-begin))return fail(f,0x1fd100,GH_HITAREA_BOUNDS);
  for(unsigned k=0;k<3;k++){float target=fl(t->alias_f0+4*k),origin=fl(n->record+4*k);if(!isfinite(target)||!isfinite(origin))return fail(f,0x1fd100,GH_HITAREA_UNSUPPORTED);volatile float d=target-origin;direction[k]=d;}direction[1]=0;
 }
 uint8_t*geom=n->shape.record+0xe0;uint32_t vt=w(geom+0x100);uint8_t descriptor[8];if(!elf_read(b,vt+0x40,8,descriptor))return fail(f,0x1fd100,GH_HITAREA_UNSUPPORTED);if(descriptor[0]||descriptor[1])return fail(f,w(descriptor+4),GH_HITAREA_UNSUPPORTED);
 const uint8_t*vector=NULL;uint32_t method=w(descriptor+4);
 if(method==0x2ba4b0){if(gh_collision_pair_shape_position(&n->shape,&vector))return fail(f,0x2bb010,GH_HITAREA_ARGUMENT);}
 else if(method==0x2babc8&&w(geom)==4)vector=geom+0x120;
 else return fail(f,method?method:0x1fd100,GH_HITAREA_UNSUPPORTED);
 ww(dest+0x60,w(dest+0x60)|3u);memcpy(dest+0x46,source+0x46,2);memcpy(dest+0x40,source+0x40,4);memcpy(dest+0x44,source+0x44,2);memcpy(dest+0x10,direction,12);memmove(dest+0x20,vector,12);memcpy(dest+0x4c,source+0x4c,4);
 s->hitarea_native->last_hit_actors[owner->index]=a;s->hitarea_native->last_hit_actors[c->slot]=t;
 if(source_player)ww(s->object_pool+0xacd0,b->player_pause_words[0]);if(target_player)ww(s->object_pool+0xacd0,b->player_pause_words[1]);return GH_HITAREA_OK;
}
static gh_hitarea_status query(gh_game_init_state*s,gh_hitarea_frame*c,gh_native_hitarea_node*n,const gh_hitarea_frame_bindings*b,uint32_t*f){
 gh_title_player_view*a=s->hitarea_native->actors[c->slot];uint8_t*source=raw(s,c->slot);
 if(!a&&w(source+0x30))return fail(f,0x1fcf20,GH_HITAREA_UNSUPPORTED);
 if(a&&(!a->bytes||a->size<0x254))return fail(f,0x1fcf20,GH_HITAREA_BOUNDS);
 if(a&&(w(a->bytes+0x250)&0x8000))return GH_HITAREA_OK;
 if((w(n->shape.record)&0x203)!=3||(int32_t)w(n->shape.record+0xd8)<=0)return GH_HITAREA_OK;
 uint32_t pc=0;gh_pair_shape_status e=gh_collision_pair_shape_sort_hits_001fc810(&n->shape,&pc);if(e)return fail(f,pc?pc:0x1fc810,e==GH_PAIR_SHAPE_ARGUMENT?GH_HITAREA_ARGUMENT:GH_HITAREA_UNSUPPORTED);
 unsigned count=w(n->shape.record+0xd8);
 for(unsigned i=c->target;i<count;i++,c->target=i){gh_collision_pair_shape target;uint32_t distance;if(gh_collision_pair_shape_hit_at(&n->shape,i,&target,&distance))return fail(f,0x1fc810,GH_HITAREA_ARGUMENT);(void)distance;
  if((w(target.record)&0x203)!=3)continue;gh_hitarea_object owner;if(gh_collision_pair_shape_hitarea_owner(&target,&owner))return fail(f,0x2bb138,GH_HITAREA_UNSUPPORTED);
  gh_title_player_view*t=NULL;if(gh_hitarea_object_actor(&owner,&t))return fail(f,0x2bb138,GH_HITAREA_ARGUMENT);
  if(!t&&w(owner.bytes+0x30))return fail(f,0x1fcf20,GH_HITAREA_UNSUPPORTED);
  if(t){if(!t->bytes||t->size<0x550)return fail(f,0x1fcf20,GH_HITAREA_BOUNDS);float cooldown=fl(t->bytes+0x54c);if(!isfinite(cooldown))return fail(f,0x1fcf20,GH_HITAREA_UNSUPPORTED);
   if(!(cooldown<=0)||same_actor(t,a))continue;
   if(w(source+0x38))return fail(f,0x1fcf20,GH_HITAREA_UNSUPPORTED);
   if(w(t->bytes+0x250)&0x8000)continue;
   if(!(w(owner.bytes)&1)&&!(w(source)&1)&&a){
    if(!b||!b->tables)return fail(f,0x131e10,GH_HITAREA_UNSUPPORTED);
    gh_scene_ray_request ray={.mask=7,.flags=0x34,.segmented=0};gh_hitarea_status he=actor_position(a,b,ray.from,f);if(he)return he;he=actor_position(t,b,ray.to,f);if(he)return he;
    volatile float from_y=ray.from[1]+1.0f,to_y=ray.to[1]+1.0f;ray.from[1]=from_y;ray.to[1]=to_y;
    int hit=0;uint32_t pc=0;gh_scene_ray_status re=gh_scene_ray_query_00131e10(b->tables,&ray,&hit,&pc);if(re)return fail(f,pc?pc:0x131e10,re==GH_RAY_ARGUMENT||re==GH_RAY_BOUNDS?GH_HITAREA_ARGUMENT:GH_HITAREA_UNSUPPORTED);if(hit)continue;
   }

  }
  gh_hitarea_status pe=publish(s,c,n,&owner,a,t,b,f);if(pe)return pe;
 }
 return GH_HITAREA_OK;
}
static gh_hitarea_status visit(gh_game_init_state*s,gh_hitarea_frame*c,const gh_hitarea_frame_bindings*b,uint32_t*f){
 while(c->node){if(c->node>672||c->visited>=672)return fail(f,0x1fd770,GH_HITAREA_BOUNDS);gh_native_hitarea_node*n=&s->hitarea_native->nodes[c->node-1];unsigned i;gh_hitarea_status e=find(s,c->slot,&n->link,&i,f);if(e)return e;
  if(c->phase==0){const uint8_t*p;if(gh_collision_pair_shape_position(&n->shape,&p))return fail(f,0x1fceb0,GH_HITAREA_ARGUMENT);memmove(n->record,p,12);memcpy(n->record+0x10,n->record,12);}
  else if(c->phase==2&&!c->query_done){e=query(s,c,n,b,f);if(e)return e;}
  else {uint16_t old;memcpy(&old,n->record+0x30,2);if(old){uint16_t value=(uint16_t)(old-1);memcpy(n->record+0x30,&value,2);if(old==1)ww(n->shape.record,w(n->shape.record)&~2u);}}
  e=find(s,c->slot,n->link.next,&i,f);if(e)return e;c->node=i;c->visited++;c->target=0;
 }return GH_HITAREA_OK;
}
gh_hitarea_status gh_hitarea_frame_resume_bound_001fdfd8(gh_game_init_state*s,const gh_boot_work*work,float rate,gh_static_transform_policy policy,gh_hitarea_frame*c,const gh_hitarea_frame_bindings*b,uint32_t*f){
 if(f)*f=0;if(!s||!work||!c||!isfinite(rate)||policy!=GH_STATIC_BINARY32_REFERENCE)return GH_HITAREA_ARGUMENT;if(s->hitarea_native&&s->hitarea_native->owner!=s)return GH_HITAREA_ARGUMENT;
 if(c->active&&(c->cursor_identity!=c||c->identity!=s||c->generation!=s->hitarea_generation||c->phase>3||c->slot>=416))return GH_HITAREA_ARGUMENT;
 if(!c->active){uint32_t pause=w(s->object_pool+0xacd0);if(pause){ww(s->object_pool+0xacd0,pause-1);return GH_HITAREA_OK;}*c=(gh_hitarea_frame){.cursor_identity=c,.identity=s,.generation=s->hitarea_generation,.active=1};}
 fenv_t env;if(feholdexcept(&env))return fail(f,0x1fd448,GH_HITAREA_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x1fd448,GH_HITAREA_UNSUPPORTED);}gh_hitarea_status e=GH_HITAREA_OK;
 while(c->phase<4){unsigned end=(c->phase==0||c->phase==2)?160:416;while(c->slot<end){uint8_t*record=raw(s,c->slot);if(!record[0x4a]){if(c->slot_started){e=GH_HITAREA_ARGUMENT;goto done;}c->slot++;continue;}
   if(!c->slot_started){e=prepare(s,c,f);if(e)goto done;if(c->phase==0){ww(record+0x34,0);if(s->hitarea_native)s->hitarea_native->last_hit_actors[c->slot]=NULL;}else if(c->phase==1){e=reset_b(s,c,work,rate,f);if(e){c->slot_started=0;goto done;}}}
   else if((s->hitarea_native?s->hitarea_native->serials[c->slot]:0)!=c->slot_serial){e=GH_HITAREA_ARGUMENT;goto done;}
   if(c->phase!=1){e=visit(s,c,b,f);if(e)goto done;if(c->phase==2&&!c->query_done){unsigned i;e=find(s,c->slot,s->object_links[c->slot],&i,f);if(e)goto done;c->node=i;c->visited=0;c->query_done=1;e=visit(s,c,b,f);if(e)goto done;}}
   c->slot++;c->slot_started=0;
  }c->phase++;c->slot=(c->phase==1||c->phase==3)?160:0;
 }*c=(gh_hitarea_frame){0};
 done:if(fesetenv(&env))return fail(f,0x1fd448,GH_HITAREA_UNSUPPORTED);return e;
}

gh_hitarea_status gh_hitarea_frame_resume_001fdfd8(gh_game_init_state*s,const gh_boot_work*w,float rate,gh_static_transform_policy p,gh_hitarea_frame*c,uint32_t*f){return gh_hitarea_frame_resume_bound_001fdfd8(s,w,rate,p,c,NULL,f);}

gh_hitarea_status gh_hitarea_object_last_hit_actor(const gh_hitarea_object*o,const gh_title_player_view**out){gh_title_player_view*a;if(!o||!out||gh_hitarea_object_actor(o,&a))return GH_HITAREA_ARGUMENT;if(o->fallback||!o->state->hitarea_native)return GH_HITAREA_ARGUMENT;*out=o->state->hitarea_native->last_hit_actors[o->index];return GH_HITAREA_OK;}
