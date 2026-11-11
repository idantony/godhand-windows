#include "game_enemy_hitareas.h"
#include "game_enemy_row2.h"
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
gh_actor_motion_status gh_enemy_hitareas_initialize_00207018(gh_enemy_hitareas*s,gh_enemy_behavior*behavior,const gh_hitarea_shape_bindings*b){
 if(!s||!behavior||behavior->stage!=2||!behavior->timers||!b||!b->pairs||!b->math)return GH_ACTOR_MOTION_ARGUMENT;
 if(s->stage&&(s->behavior!=behavior||s->b.pairs!=b->pairs||s->b.math!=b->math||s->b.policy!=b->policy))return GH_ACTOR_MOTION_BUSY;
 gh_enemy_entry*e=behavior->timers->nodes->solids->shadow->pose->motion->hp->tail->entry;uint8_t*p=e->object.view.bytes;gh_actor_model*m=&e->initialize.model.model;
 if(!e->b.init->hitarea_cold_constructed){s->frontier=0x1fde00;return GH_ACTOR_MOTION_UNSUPPORTED;}
 uint32_t scale=word(p,0x118);
 if(scale!=0x3f800000&&!gh_enemy202_is_r100_supported_profile(&e->request)){
  s->frontier=0x2070cc;return GH_ACTOR_MOTION_UNSUPPORTED;
 }
 if(!s->stage){s->behavior=behavior;s->b=*b;for(unsigned i=0;i<9;i++)s->indices[i]=-1;s->stage=1;}
 if(s->stage==1){int original=0;gh_hitarea_status status=gh_hitarea_object_create_001fe218(e->b.init,&e->object.view,1,&s->units[0],&original,&s->frontier);if(status)return GH_ACTOR_MOTION_UNSUPPORTED;s->created[0]=1;put(p,0x698,0);if(original)put(p,0x6ac,UINT32_MAX);else s->body_cursor=4;s->stage=2;}
 if(s->stage==2){static const unsigned parts[]={1,3,19,23};static const uint32_t radii[]={0x3ecccccd,0x3e99999a,0x3e99999a,0x3e99999a};const uint32_t zero[]={0,0,0};
  for(;s->body_cursor<4;s->body_cursor++){unsigned i=s->body_cursor;const gh_actor_part*part;if(gh_actor_model_part_at(m,parts[i],&part)){s->frontier=0x1fdaf0;return GH_ACTOR_MOTION_UNSUPPORTED;}
   uint32_t radius;
   if(!gh_enemy202_body_radius_002070cc(scale,radii[i],&radius)){
    s->frontier=0x2070cc;return GH_ACTOR_MOTION_UNSUPPORTED;
   }
   if(gh_hitarea_add_sphere_001fdaf0(&s->units[0],b,part->bytes+0x80,zero,radius,&s->indices[i],&s->frontier))return GH_ACTOR_MOTION_UNSUPPORTED;put(p,0x69c+i*4,(uint32_t)s->indices[i]);
  }s->stage=3;
 }
 if(s->stage==3){static const unsigned pointer_fields[]={0x670,0x678,0x680,0x688,0x690},index_fields[]={0x674,0x67c,0x684,0x68c,0x694};
  for(;s->aux_cursor<5;s->aux_cursor++){unsigned i=s->aux_cursor,u=i+1;
   if(!s->created[u]){int original=0;if(gh_hitarea_object_create_001fe0e8(e->b.init,&e->object.view,i==4?0:1,&s->units[u],&original,&s->frontier))return GH_ACTOR_MOTION_UNSUPPORTED;s->created[u]=1;put(p,pointer_fields[i],0);}
   if(!s->aux_stage){uint32_t xyz[]={0,0,0},rotation[]={0,0,0};gh_hitarea_status status;
    if(i<2){xyz[1]=0x3fa66666;xyz[2]=0x3f400000;}else if(i==3)xyz[1]=0x3fa66666;
    if(i==0)status=gh_hitarea_add_sphere_001fdaf0(&s->units[u],b,p+0x80,xyz,0x3f000000,&s->indices[4+i],&s->frontier);
    else if(i==1){rotation[0]=0x3fc90fdb;status=gh_hitarea_add_cylinder_001fdc00(&s->units[u],b,p+0x80,xyz,rotation,0x40000000,0x3f000000,&s->indices[4+i],&s->frontier);}
    else if(i==2)status=gh_hitarea_add_sphere_001fdaf0(&s->units[u],b,p+0x80,xyz,0x3f99999a,&s->indices[4+i],&s->frontier);
    else{const uint32_t size[]={0x3f800000,0x3f000000,0x3f800000};status=gh_hitarea_add_box_001fd9d8(&s->units[u],b,p+0x80,xyz,rotation,size,&s->indices[4+i],&s->frontier);}
    if(status)return GH_ACTOR_MOTION_UNSUPPORTED;put(p,index_fields[i],(uint32_t)s->indices[4+i]);s->aux_stage=1;
   }
   if(gh_hitarea_object_set_active_001fd668(&s->units[u],0,&s->frontier))return GH_ACTOR_MOTION_UNSUPPORTED;s->aux_stage=0;
  }s->stage=4;
 }
 put(p,0x1710,0);put(p,0x170c,0x3ca3d70a);put(p,0x758,0);put(p,0x754,0);s->frontier=0x26b9e8;e->frontier=s->frontier;return GH_ACTOR_MOTION_UNSUPPORTED;
}
gh_actor_motion_status gh_enemy_hitareas_abort_prefix(gh_enemy_hitareas*s){
 if(!s)return GH_ACTOR_MOTION_ARGUMENT;for(unsigned i=6;i>0;i--){unsigned u=i-1;if(!s->created[u])continue;if(!s->units[u].fallback){if(gh_hitarea_shapes_abort_object(&s->units[u],&s->frontier)||gh_hitarea_object_release_empty_001fe278(&s->units[u],&s->frontier))return GH_ACTOR_MOTION_UNSUPPORTED;}s->created[u]=0;}
 *s=(gh_enemy_hitareas){0};return GH_ACTOR_MOTION_OK;
}
