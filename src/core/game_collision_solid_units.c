#include "game_collision_solid_units.h"
#include <string.h>
#include <stdlib.h>
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static void vf0(uint8_t*p){memset(p,0,12);ww(p+12,0x3f800000);}
static gh_solid_unit_status fail(uint32_t*f,uint32_t pc,gh_solid_unit_status e){if(f)*f=pc;return e;}
static int valid(const gh_game_collision_solids*m){return m&&m->unit_head==m->native_units;}
static gh_solid_unit_status find(gh_game_collision_solids*m,gh_title_player_view*object,gh_collision_solid_unit**out,uint32_t*f){
 *out=NULL;if(!valid(m))return fail(f,0x1346c8,GH_SOLID_UNIT_UNSUPPORTED);size_t count=0;for(gh_collision_solid_unit*u=m->native_units;u;u=u->next){if(++count>m->unit_count||u->manager!=m)return fail(f,0x1346c8,GH_SOLID_UNIT_UNSUPPORTED);if(object&&u->object->bytes==object->bytes){*out=u;return GH_SOLID_UNIT_OK;}}return GH_SOLID_UNIT_OK;
}
gh_solid_unit_status gh_collision_solid_create_unit_001346f8(gh_game_collision_solids*m,gh_heap*h,gh_title_player_view*object,uint32_t type,uint32_t radius,gh_collision_solid_unit**out,uint32_t*f){
 if(f)*f=0;if(!m||!out)return GH_SOLID_UNIT_ARGUMENT;*out=NULL;if(!object)return GH_SOLID_UNIT_OK;if(!h||!object->bytes||object->size<0xc0)return GH_SOLID_UNIT_ARGUMENT;
 gh_collision_solid_unit*u;gh_solid_unit_status e=find(m,object,&u,f);if(e)return e;if(u){*out=u;return GH_SOLID_UNIT_OK;}if(m->unit_count==UINT32_MAX)return fail(f,0x1346f8,GH_SOLID_UNIT_UNSUPPORTED);
 u=calloc(1,sizeof(*u));if(!u)return fail(f,0x30f550,GH_SOLID_UNIT_MEMORY);if(gh_heap_allocate(h,0x40,16,&u->allocation)){free(u);return fail(f,0x30f550,GH_SOLID_UNIT_MEMORY);}u->manager=m;u->object=object;u->heap=h;u->type=type;u->bound_radius_bits=radius;
 uint8_t*p=u->allocation.pointer;ww(p,0);ww(p+4,0);ww(p+8,type);ww(p+12,radius);ww(p+16,0);vf0(p+0x20);vf0(p+0x30);
 if(!m->native_units)m->native_units=u;else{gh_collision_solid_unit*tail=m->native_units;while(tail->next)tail=tail->next;tail->next=u;}m->unit_head=m->native_units;m->unit_count++;*out=u;return GH_SOLID_UNIT_OK;
}
gh_solid_unit_status gh_collision_solid_create_sphere_00134880(gh_game_collision_solids*m,gh_title_player_view*object,const uint8_t*matrix,const uint32_t*xyz,uint32_t radius,gh_collision_native_sphere**out,uint32_t*f){
 if(f)*f=0;if(!m||!out)return GH_SOLID_UNIT_ARGUMENT;*out=NULL;if(!object)return GH_SOLID_UNIT_OK;gh_collision_solid_unit*u;gh_solid_unit_status e=find(m,object,&u,f);if(e)return e;if(!u)return GH_SOLID_UNIT_OK;if(!matrix||!xyz)return GH_SOLID_UNIT_ARGUMENT;if(m->global_00460d18==UINT32_MAX)return fail(f,0x134520,GH_SOLID_UNIT_UNSUPPORTED);
 gh_collision_native_sphere*s=calloc(1,sizeof(*s));if(!s)return fail(f,0x30f550,GH_SOLID_UNIT_MEMORY);if(gh_heap_allocate(u->heap,0x80,16,&s->allocation)){free(s);return fail(f,0x30f550,GH_SOLID_UNIT_MEMORY);}s->matrix=matrix;s->radius_bits=radius;s->constructed=1;
 uint8_t*p=s->allocation.pointer;ww(p,0);ww(p+4,1);ww(p+8,1);vf0(p+0x10);vf0(p+0x20);ww(p+0x30,0x41d7e0);ww(p+0x40,0);vf0(p+0x50);vf0(p+0x60);for(unsigned k=0;k<3;k++)ww(p+0x50+k*4,xyz[k]);ww(p+0x70,radius);
 if(!u->spheres)u->spheres=s;else{gh_collision_native_sphere*tail=u->spheres;while(tail->next)tail=tail->next;tail->next=s;}m->global_00460d18++;*out=s;return GH_SOLID_UNIT_OK;
}
gh_solid_unit_status gh_collision_solid_release_unit_001347e8(gh_game_collision_solids*m,gh_title_player_view*object,uint32_t*f){
 if(f)*f=0;if(!m)return GH_SOLID_UNIT_ARGUMENT;if(!object)return GH_SOLID_UNIT_OK;gh_collision_solid_unit*u;gh_solid_unit_status e=find(m,object,&u,f);if(e)return e;if(!u)return GH_SOLID_UNIT_OK;
 while(u->spheres){gh_collision_native_sphere*s=u->spheres;if(!s->constructed||!m->global_00460d18)return fail(f,0x1344a0,GH_SOLID_UNIT_UNSUPPORTED);if(gh_heap_release(u->heap,s->allocation))return fail(f,0x1339c0,GH_SOLID_UNIT_BUSY);u->spheres=s->next;m->global_00460d18--;free(s);}
 if(gh_heap_release(u->heap,u->allocation))return fail(f,0x134458,GH_SOLID_UNIT_BUSY);gh_collision_solid_unit**link=&m->native_units;while(*link&&*link!=u)link=&(*link)->next;if(*link!=u)return fail(f,0x1347e8,GH_SOLID_UNIT_UNSUPPORTED);*link=u->next;m->unit_head=m->native_units;m->unit_count--;free(u);return GH_SOLID_UNIT_OK;
}
int gh_collision_solid_units_empty(const gh_game_collision_solids*m){return m&&!m->unit_head&&!m->native_units&&!m->unit_count&&!m->global_00460d18;}
gh_solid_unit_status gh_collision_solid_units_shutdown(gh_game_collision_solids*m,uint32_t*f){if(f)*f=0;if(!valid(m))return fail(f,0x1347e8,GH_SOLID_UNIT_UNSUPPORTED);while(m->native_units){gh_solid_unit_status e=gh_collision_solid_release_unit_001347e8(m,m->native_units->object,f);if(e)return e;}return GH_SOLID_UNIT_OK;}


