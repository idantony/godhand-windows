#include "game_effect_create.h"
#include "id_layout_rotation.h"
#include "game_effects_frame.h"
#include "game_actor_matrix.h"
#include "game_effect_node.h"
#include "game_actor_two_bone.h"
#include "game_esp14.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
typedef struct {const gh_title_player_view *actor,*selector;const gh_actor_model *model;uint32_t part_index,part_handle;
 const uint8_t *record;uint32_t serial;uint8_t associated,initialized,quiesced;int32_t first_esp_child;} emitter_metadata;
typedef struct {gh_effect_view record,texture,secondary_texture;const gh_title_player_view *actor,*selector;uint32_t serial,emitter_serial;unsigned emitter_slot;uint8_t associated,initialized,type,quiesced;int32_t parent_emitter,previous,next;uint32_t parent_serial;uint8_t target120_known_null,knockdown,player_attachment;gh_effect_retired_actor retired_actor;gh_room_pools *trail_pools;gh_room_arena_allocation trail[4];} esp_metadata;
struct gh_effect_native {gh_game_effects *owner;uint8_t *esp,*emitters;uint32_t serial;gh_room_pools *trail_pools;
 emitter_metadata slots[256];esp_metadata esps[1024];struct gh_effects_frame_globals *frame_globals;gh_game_scene_tables *frame_actors;};
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static void half(uint8_t*p,uint16_t v){memcpy(p,&v,2);}
static gh_effect_status fail(gh_effect_create_call*c,uint32_t pc,gh_effect_status e){c->frontier=pc;return e;}
static void vf0(uint8_t*p){memset(p,0,12);ww(p+12,0x3f800000);}
static void identity(uint8_t*p){memset(p,0,64);for(unsigned i=0;i<4;i++)ww(p+20*i,0x3f800000);}
/*300458 memset guarantees the cold parentless2B5C20 path. 2B6420's local
 * scale1/angle0/position0 compose identity before GetWorld* and detach;
 *2B6400 invalidates90 again. No live cNode hierarchy is generalized here. */
static void construct_emitter_00302790(uint8_t*p){
 ww(p+0xf0,0x44b310);for(unsigned i=0;i<4;i++)ww(p+0x20+4*i,0x3f800000);vf0(p+0x30);vf0(p+0x40);vf0(p+0xa0);identity(p+0x50);identity(p+0xb0);ww(p+0x90,0);
 ww(p+0xf0,0x44fd58);for(unsigned i=0;i<4;i++){ww(p+0x1a0+4*i,0x3f800000);ww(p+0x1d0+4*i,0x3f800000);}vf0(p+0x1b0);vf0(p+0x1c0);ww(p+0x1e0,0x3f800000);p[0x1ec]=p[0x1ef]=0xff;vf0(p+0x200);ww(p+0x22c,0x3f800000);p[0x190]=0;
}
gh_effect_status gh_effect_create_begin(gh_effect_create_call*c,const gh_effect_create_request*r){if(!c||!r||(c->active&&!c->complete))return GH_EFFECT_ARGUMENT;*c=(gh_effect_create_call){.identity=c,.request=*r,.active=1};return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_player_stun_part4_00306900(gh_effect_create_call *c,
 const gh_title_player_view *actor)
{
 if(!actor||!actor->bytes||actor->size<0x300||actor->original_vtable214!=0x41d220||
    (w(actor->bytes+0x2fc)>>16)!=0x100)return GH_EFFECT_ARGUMENT;
 const gh_effect_create_request request={.bank=0,.index=4,.actor=actor,.mode=4,.selector=-1};
 gh_effect_status status=gh_effect_create_begin(c,&request);
 if(!status)c->configuration_player_stun=1;
 return status;
}

int gh_effect_player_stun_row(gh_effect_view record)
{
 if(!record.bytes||record.size!=0x2f0)return 0;
 const uint8_t *row=record.bytes;
 return row[0xbe]==0xff&&!row[0x128]&&!row[0x136]&&
   ((row[0x133]==0&&w(row+4)==0x00e00000u)||(row[0x133]==9&&w(row+4)==0x00e01000u));
}

gh_effect_status gh_effect_create_enemy_death_part0_00306900(gh_effect_create_call *c,
 const gh_title_player_view *actor,int32_t mode)
{
 if(!actor||mode!=-1)return GH_EFFECT_ARGUMENT;
 const gh_effect_create_request request={.bank=0x58,.index=2,.actor=actor,.mode=mode,.selector=-1};
 gh_effect_status status=gh_effect_create_begin(c,&request);
 if(!status)c->configuration_death_part_zero=1;
 return status;
}

gh_effect_status gh_effect_create_enemy_knockdown_002a8608(gh_effect_create_call *c,
 const gh_title_player_view *actor,uint32_t index)
{
 if(!actor||!actor->bytes||actor->size<0x436||(index!=0x2f&&index!=0x30))return GH_EFFECT_ARGUMENT;
 if(actor->bytes[0x434]&2)return GH_EFFECT_UNSUPPORTED;
 const gh_effect_create_request request={.bank=0x58,.index=index,.actor=actor,.mode=0,.selector=-1};
 gh_effect_status status=gh_effect_create_begin(c,&request);
 if(!status)c->configuration_death_part_zero=2;
 return status;
}

gh_effect_status gh_effect_create_nested_begin_002f1d3c(gh_effect_create_call *c,
 const gh_effect_create_request *request,uint8_t group,uint32_t scale,uint32_t flags){
 if(!request||!group||!request->actor||request->mode!=-1||request->selector!=-1||
    scale!=0x3f800000u||(flags&8u))return GH_EFFECT_UNSUPPORTED;
 gh_effect_status status=gh_effect_create_begin(c,request);
 if(!status){
  c->configuration_nested_group=group;
  c->configuration_nested_scale=scale;
  c->configuration_nested_flags=flags;
 }
 return status;
}
gh_effect_status gh_effect_create_ambient_begin_00305e88(gh_effect_create_call*c,uint32_t index){if(!c)return GH_EFFECT_ARGUMENT;uint32_t bits=index+0x2200u;int32_t mode;memcpy(&mode,&bits,4);gh_effect_create_request request={.bank=3,.index=index,.actor=NULL,.mode=mode,.selector=-1};gh_effect_status e=gh_effect_create_begin(c,&request);if(!e)c->configuration_305e88=1;return e;}
gh_effect_status gh_effect_create_event_cut_begin_002fa158(gh_effect_create_call *call,uint32_t cut)
{
 if(cut>(uint32_t)INT32_MAX-0x2500u)return GH_EFFECT_BOUNDS;
 const gh_effect_create_request request={.bank=6,.index=cut+1,.mode=(int32_t)(cut+0x2500u),.selector=-1};
 gh_effect_status status=gh_effect_create_begin(call,&request);
 if(!status)call->configuration_2fa158=1;
 return status;
}

gh_effect_status gh_effect_create_event_sequence_00005288(gh_effect_create_call *call)
{
 const gh_effect_create_request request={.bank=1,.index=1,.mode=0,.selector=-1};
 gh_effect_status status=gh_effect_create_begin(call,&request);
 if(!status)call->configuration_05288=1;
 return status;
}

static unsigned r101_weapon_est_count(unsigned index)
{
 if(index==0x7a||index==0x7b||index==0x7f||index==0x80)return 2;
 if(index==0x7c||index==0x81)return 4;
 if(index==0x7d||index==0x82)return 5;
 return 0;
}

static int r101_weapon_est_row(unsigned index,const uint8_t *row,unsigned ordinal)
{
 if(ordinal>=r101_weapon_est_count(index)||row[0xbe]!=3||row[0x128]||row[0x136])
  return 0;
 if(index==0x7d||index==0x82){
  static const uint8_t type[]={9,0x12,0x18,0x1a,0x1a},texture[]={15,0,0,0,132};
  return row[0x133]==type[ordinal]&&row[0x12e]==texture[ordinal]&&
         w(row+4)==(ordinal<3?0x00800000u:0x00c00000u);
 }
 return w(row+4)==0x00800000u&&row[0x133]==((ordinal&1u)?0x12:9)&&
        row[0x12e]==((ordinal&1u)?0:15);
}

static int16_t weapon_swing_room(const gh_title_player_view *weapon,
 const gh_title_player_view *selector)
{
 if(!weapon||!selector||weapon==selector||!weapon->bytes||weapon->size<0x300||
    !selector->bytes||selector->size<=0x1650||(w(selector->bytes+0x2fc)>>16)!=0x202)
  return 0;
 const uint32_t kind=w(weapon->bytes+0x2fc)>>16,dependency=w(selector->bytes+0x564);
 const unsigned stamp=w(selector->bytes+0x640)&0xffffu,appearance=selector->bytes[0x1650];
 if(kind==0x361&&dependency==0x203&&stamp==13&&!appearance)return 0x101;
 if(kind==0x363&&dependency==0x200&&stamp==3&&appearance==1)return 0x102;
 return 0;
}

static unsigned weapon_est_count(int16_t room,unsigned index)
{
 return room?r101_weapon_est_count(index):0;
}

gh_effect_status gh_effect_create_r101_weapon_002a8608(gh_effect_create_call *c,
 const gh_title_player_view *weapon,const gh_title_player_view *selector,uint32_t index,uint32_t scale)
{
 if(!weapon_est_count(weapon_swing_room(weapon,selector),index)||
    scale!=w(selector->bytes+0x114))return GH_EFFECT_ARGUMENT;
 const gh_effect_create_request request={.bank=0x58,.index=index,.actor=weapon,.mode=0,.selector=-1};
 gh_effect_status status=gh_effect_create_begin(c,&request);
 if(!status){
  c->attachment_selector=selector;
  c->configuration_nested_scale=scale;
 }
 return status;
}

static gh_effect_status positioned_begin(gh_effect_create_call *call,uint32_t bank,uint32_t index,
 const uint32_t world_xyz[3],uint32_t yaw_bits)
{
 if(!call||!world_xyz)return GH_EFFECT_ARGUMENT;
 for(unsigned i=0;i<3;++i)
  if((world_xyz[i]&0x7f800000u)==0x7f800000u)return GH_EFFECT_UNSUPPORTED;
 if((yaw_bits&0x7f800000u)==0x7f800000u)return GH_EFFECT_UNSUPPORTED;

 uint32_t position[3];
 memcpy(position,world_xyz,sizeof(position));
 const gh_effect_create_request request={.bank=bank,.index=index,.mode=-1,.selector=-1};
 gh_effect_status status=gh_effect_create_begin(call,&request);
 if(status)return status;
 memcpy(call->configuration_positioned_position,position,sizeof(position));
 call->configuration_positioned_yaw=yaw_bits;
 call->configuration_positioned=1;
 return GH_EFFECT_OK;
}

gh_effect_status gh_effect_create_devil_projectile_impact_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0x58,0x41,world_xyz,0);
}

gh_effect_status gh_effect_create_om324_break_begin_001bfd28(gh_effect_create_call *call,
 const uint32_t world_xyz[3],uint32_t yaw_bits)
{
 return positioned_begin(call,0,0x1c,world_xyz,yaw_bits);
}

gh_effect_status gh_effect_create_enemy_impact_begin_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0x58,0x10,world_xyz,0);
}

gh_effect_status gh_effect_create_enemy_amplified_impact_begin_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0x58,0x12,world_xyz,0);
}

gh_effect_status gh_effect_create_enemy_counter_impact_begin_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0,0xc,world_xyz,0);
}
gh_effect_status gh_effect_create_om_sparkle_begin_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0,0x1b,world_xyz,0);
}


gh_effect_status gh_effect_create_om30a_break_begin_0018c580(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0,7,world_xyz,0);
}
gh_effect_status gh_effect_create_om305_break_begin_001c2ce0(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0,0xc9,world_xyz,0);
}

gh_effect_status gh_effect_create_player_impact_begin_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0,3,world_xyz,0);
}

static int em27_attack_actor(const gh_title_player_view *actor)
{
 return actor&&actor->bytes&&actor->size>0x1650&&
        actor->original_vtable214==0x4479c8&&(w(actor->bytes+0x2fc)>>16)==0x202&&
        w(actor->bytes+0x564)==0x227&&(w(actor->bytes+0x640)&0xffffu)==2&&
        !actor->bytes[0x1650];
}

/* R105 EM27 selects the original EM00-family SEQ 58:2C or 58:5A.
 * The episode's TXE exchange leaves the shared MOT/SEQ resource in EM02.DAT. */
static int em00_family_attack_actor(const gh_title_player_view *actor)
{
 return em27_attack_actor(actor)||
        (actor&&actor->bytes&&actor->size>=0x568&&actor->original_vtable214==0x4479c8&&
         (w(actor->bytes+0x2fc)>>16)==0x202&&w(actor->bytes+0x564)==0x200);
}

static int em0a_attack_actor(const gh_title_player_view *actor)
{
 return actor&&actor->bytes&&actor->size>0x1650&&actor->original_vtable214==0x4479c8&&
        (w(actor->bytes+0x2fc)>>16)==0x202&&w(actor->bytes+0x564)==0x20a&&
        !(w(actor->bytes+0x640)&0xffffu)&&actor->bytes[0x1650]==3;
}

static int stamp8_attack_actor(const gh_title_player_view *actor)
{
 return em00_family_attack_actor(actor)&&actor->size>0x1650&&
        w(actor->bytes+0x564)==0x200&&(w(actor->bytes+0x640)&0xffffu)==8&&
        actor->bytes[0x1650]==5;
}

static int stamp8_attack_effect_row(const uint8_t *row)
{
 const unsigned part=row[0xbe];
 if(row[0x128]||row[0x136]||w(row+4)!=0x00c00000u)return 0;
 if(row[0x133]==0x12)return (part==25||part==21)&&w(row)==0x1efe8010u;
 if(row[0x133]==0x20)return (part==24||part==20)&&w(row)==0x1e3e0010u;
 return row[0x133]==0x1a&&((part==24&&w(row)==0x1e3e8010u)||
                            (part==21&&w(row)==0x1efe8010u));
}

static int em0a_attack_effect_row(const uint8_t *row)
{
 return row[0xbe]==16&&!row[0x128]&&!row[0x136]&&w(row+4)==0x00c00000u&&
        ((row[0x133]==9&&w(row)==0x1e3e8010u)||(row[0x133]==0x12&&w(row)==0x1efe8010u));
}

static int devil_effect_actor(const gh_title_player_view *actor,const gh_title_player_view *selector);

static int devil_projectile_effect_actor(const gh_title_player_view *actor)
{
 return actor&&actor->bytes&&actor->size==0x630&&actor->original_vtable214==0x4290c8&&
    (w(actor->bytes+0x2fc)>>16)==0x3be&&w(actor->bytes+0x610)==3;
}

/* Original CORE58 emission groups for the four ranged EM20 sequences. */
static unsigned devil_ranged_effect_rows(uint32_t index)
{
 static const uint8_t counts[]={7,7,21,25};
 return index>=0x42&&index<=0x45?counts[index-0x42]:0;
}

static int em0f_route_actor(const gh_title_player_view *actor)
{
 return actor&&actor->bytes&&actor->size>0x1650&&actor->original_vtable214==0x4479c8&&
    (w(actor->bytes+0x2fc)>>16)==0x202&&w(actor->bytes+0x564)==0x20f&&
    (w(actor->bytes+0x640)&0xffffu)==0x12&&(actor->bytes[0x1650]==0||actor->bytes[0x1650]==4);
}

static int em0f_route_effect_row(const uint8_t *row)
{
 return w(row)==0x1efe8010u&&w(row+4)==0x00800000u&&!row[0xbe]&&
    !row[0x128]&&row[0x133]==0x1a&&!row[0x136]&&row[0x12e]==132&&
    row[0x28f]==1&&row[0x290]==2&&row[0x291]==1&&row[0x292]==1;
}

gh_effect_status gh_effect_create_actor_sequence_0014fb48(gh_effect_create_call *call,
 const gh_effect_create_request *request,uint32_t scale)
{
 if(!request||!request->actor||request->selector!=-1||!scale||
    (scale&0x80000000u)||(scale&0x7f800000u)==0x7f800000u)return GH_EFFECT_UNSUPPORTED;
 const gh_title_player_view *actor=request->actor;
 if(request->bank==0&&((request->index==0x3e&&request->mode==-1)||
                      (request->index==0x42&&!request->mode))){
  if(!actor->bytes||actor->size<0x300||actor->original_vtable214!=0x41d220||
     (w(actor->bytes+0x2fc)>>16)!=0x100)return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&(request->index==0x27||request->index==0x59)&&!request->mode){
  if(!actor->bytes||actor->size<0x644||actor->original_vtable214!=0x4479c8||
     (w(actor->bytes+0x2fc)>>16)!=0x202||w(actor->bytes+0x564)!=0x200||
     (w(actor->bytes+0x640)&0xffffu)!=20||actor->bytes[0x2f4]!=1||actor->bytes[0x2f5]!=0x1d)
   return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&request->index==0x2b&&!request->mode){
  if(!stamp8_attack_actor(actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x15)
   return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&request->index==0x1e&&!request->mode){
  if(!em0a_attack_actor(actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x15)
   return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&request->index==0x5d&&!request->mode){
  if(!em0f_route_actor(actor))return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&request->index==0x3f&&!request->mode){
  if(!devil_effect_actor(actor,actor))return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&devil_ranged_effect_rows(request->index)&&!request->mode){
  if(!devil_effect_actor(actor,actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x23||
     actor->bytes[0x2f7]!=2)return GH_EFFECT_UNSUPPORTED;
 }else{
  if(request->bank!=0x58||(request->index!=0x2d&&request->index!=0x2c&&request->index!=0x5a)||
     request->mode)return GH_EFFECT_UNSUPPORTED;
  if(request->index!=0x2d&&!em00_family_attack_actor(actor))return GH_EFFECT_UNSUPPORTED;
 }
 gh_effect_status status=gh_effect_create_begin(call,request);
 if(!status)call->configuration_nested_scale=scale;
 return status;
}

static gh_effect_status lookup_est(const gh_game_effects *owner,uint32_t bank,uint32_t index,
 gh_effect_view *result)
{
 *result=(gh_effect_view){0};
 const gh_effect_resource *resource=NULL;
 for(unsigned i=0;i<64;i++)if(owner->resources[i].kind==(int32_t)bank){resource=owner->resources+i;break;}
 if(!resource||!resource->tbl1.bytes||resource->tbl1.size<4)return GH_EFFECT_OK;
 gh_effect_view table=resource->tbl1;
 const uint32_t count=w(table.bytes);
 if(count>(table.size-4)/4)return GH_EFFECT_BOUNDS;
 if(index>count)return GH_EFFECT_OK;
 const uint32_t offset=w(table.bytes+(size_t)index*4);
 if(!offset)return GH_EFFECT_OK;
 if(offset>table.size||16>table.size-offset)return GH_EFFECT_BOUNDS;
 size_t end=table.size;
 for(unsigned i=1;i<=count;i++){
  const uint32_t next=w(table.bytes+(size_t)i*4);
  if(next>offset&&next<end)end=next;
 }
 *result=(gh_effect_view){table.bytes+offset,end-offset};
 return GH_EFFECT_OK;
}

gh_effect_status gh_effect_est_at_00306030(const gh_game_effects*owner,uint32_t bank,uint32_t index,
 gh_effect_view*out){
 if(!owner||!owner->ready||!out||!index||bank==3)return GH_EFFECT_ARGUMENT;
 return lookup_est(owner,bank,index,out);
}

gh_effect_status gh_effect_event_cut_present_002fa128(const gh_game_effects *owner,
 uint32_t cut,int *present)
{
 if(!owner||!owner->ready||!present||cut==UINT32_MAX)return GH_EFFECT_ARGUMENT;
 gh_effect_view entry;
 gh_effect_status status=lookup_est(owner,6,cut+1,&entry);
 if(!status)*present=entry.bytes!=NULL;
 return status;
}
/* Actual305FB0: locate group in SSD's first relative table, then choose the
 * same ordinal from its second table. Views stay within the owned SSD. */
static gh_effect_status ambient_est(gh_effect_create_call*c){
 gh_effect_view ssd=c->owner->ssd;if(!ssd.bytes){if(ssd.size)return fail(c,0x305fb0,GH_EFFECT_BOUNDS);c->original_return=0;c->complete=1;return GH_EFFECT_OK;}if(ssd.size<24)return fail(c,0x305fb0,GH_EFFECT_BOUNDS);
 uint64_t group=(uint64_t)w(ssd.bytes+16)+16,est=(uint64_t)w(ssd.bytes+20)+16;if(group>ssd.size||16>ssd.size-group||est>ssd.size||16>ssd.size-est)return fail(c,0x305fb0,GH_EFFECT_BOUNDS);uint32_t count=w(ssd.bytes+(size_t)group+4);if(count>(ssd.size-(size_t)group-16)/4||count>(ssd.size-(size_t)est-16)/4)return fail(c,0x305fb0,GH_EFFECT_BOUNDS);
 for(unsigned i=0;i<count;i++){uint64_t row=group+w(ssd.bytes+(size_t)group+16+(size_t)i*4)+16;if(row>ssd.size||2>ssd.size-row)return fail(c,0x305fb0,GH_EFFECT_BOUNDS);if((uint32_t)(ssd.bytes[row]|(unsigned)ssd.bytes[row+1]<<8)!=c->request.index)continue;uint64_t start=est+w(ssd.bytes+(size_t)est+16+(size_t)i*4)+16;if(start>ssd.size||16>ssd.size-start)return fail(c,0x305fb0,GH_EFFECT_BOUNDS);size_t end=ssd.size;for(unsigned j=0;j<count;j++){uint64_t next=est+w(ssd.bytes+(size_t)est+16+(size_t)j*4)+16;if(next>start&&next<end)end=(size_t)next;}if(w(ssd.bytes+(size_t)start)!=0x545345){c->original_return=0;c->complete=1;return GH_EFFECT_OK;}c->est=(gh_effect_view){ssd.bytes+(size_t)start,end-(size_t)start};c->count=w(c->est.bytes+4);if(c->count>(c->est.size-16)/0x2f0)return fail(c,0x3066d0,GH_EFFECT_BOUNDS);c->config_flags=0x40;c->stage=1;return GH_EFFECT_OK;
 }
 c->original_return=0;c->complete=1;return GH_EFFECT_OK;
}
static gh_effect_status storage(gh_effect_create_call*c,const gh_effect_create_bindings*b){
 const gh_effect_release_bindings*s=&b->storage;gh_game_effects*e=s->effects;if(!e||!e->ready||!s->esp||s->esp_bytes<0x100180||!s->emitters||s->emitter_bytes<0x26060)return GH_EFFECT_ARGUMENT;
 if(c->owner&&c->owner!=e)return GH_EFFECT_ARGUMENT;c->owner=e;
 if(e->native&&(e->native->owner!=e||e->native->esp!=s->esp||e->native->emitters!=s->emitters))return GH_EFFECT_ARGUMENT;
 return GH_EFFECT_OK;
}
/* Actual ten emitter rows of CORE58:59, from the long escape reaction clip. */
static int escape_effect_row_59(const uint8_t *row,unsigned i)
{
 static const uint8_t parts[]={2,2,0,0,2,0,0,0,0,0};
 static const uint8_t kind[]={7,7,0x1a,0x1a,7,0x14,0x14,0x14,0x1a,0};
 static const uint8_t item[]={0,0,0,7,0,4,5,5,132,0};
 static const uint32_t flags[]={0x00802000,0x00800000,0x00c00000,0x00c00000,
  0x00800000,0x00800000,0x00802000,0x00802000,0x00c00000,0x00c00000};
 return i<10&&!row[0x128]&&!row[0x136]&&row[0xbe]==parts[i]&&row[0x133]==kind[i]&&
    row[0x12e]==item[i]&&w(row+4)==flags[i];
}

static int player_dodge_effect_row(uint32_t index,const uint8_t *row)
{
 if(row[0x128]||row[0x136])return 0;
 if(index==0x15||index==0x16){
  const unsigned part=row[0xbe],child=row[0x133];
  const uint32_t flags=w(row+4);
  if(child==0x12)return part==2&&flags==0x00c00000u;
  if(child==0)return (part==2||part==(index==0x15?23u:19u))&&flags==0x00800000u;
  return child==0x1a&&flags==0x00c00000u&&
         (index==0x15?(part==0||part==21):(part==25||part==24));
 }
 if(index==0x17&&row[0xbe]==0&&row[0x133]==7)
  return w(row+4)==0x00800000u;
 return (index==0x17||index==0x18)&&row[0xbe]==21&&row[0x133]==0&&w(row+4)==0x00c00000u;
}
static int r101_door_break_actor(const gh_title_player_view *actor)
{
 return actor&&actor->bytes&&actor->size>=0x8f0&&
        actor->original_vtable214==0x424128u&&w(actor->bytes+0x214)==0x424128u&&
        (w(actor->bytes+0x2fc)>>16)==0x313;
}

int gh_effect_r101_door_break_part_is_bound(gh_effect_view row,
 const gh_title_player_view *actor,const gh_title_player_view *selector)
{
 if(!r101_door_break_actor(actor)||selector!=actor||!row.bytes||row.size!=0x2f0)return 0;
 const uint8_t *r=row.bytes;
 return w(r)==0x1efe8010u&&w(r+4)==0x00c00000u&&
        !r[0xbe]&&!r[0x128]&&!r[0x133]&&!r[0x136]&&(r[0x12e]==132||r[0x12e]==2);
}

static gh_effect_status select_est(gh_effect_create_call*c,const gh_effect_create_bindings*b){
 uint32_t flags=w((const uint8_t*)b->work+0x5d8);if((flags&0x10000000u)||(!c->request.index&&c->request.bank!=3)){c->original_return=1;c->complete=1;return GH_EFFECT_OK;}
 if(c->request.bank==3){if(!c->configuration_305e88||c->request.actor||c->request.selector!=-1)return fail(c,0x305fb0,GH_EFFECT_UNSUPPORTED);return ambient_est(c);}
 if(c->request.bank==0x58&&(c->request.index==0x2c||c->request.index==0x5a)&&
    c->configuration_nested_scale&&em27_attack_actor(c->request.actor)&&
    (b->work->room!=0x105||!b->model||b->model->work!=b->work||
     !b->model->parts_ready||b->model->root.bytes!=c->request.actor->bytes))
  return fail(c,0x303a30,GH_EFFECT_UNSUPPORTED);
 gh_effect_view entry;gh_effect_status status=lookup_est(c->owner,c->request.bank,c->request.index,&entry);
 if(status)return fail(c,0x306030,status);
 if(entry.size<16||w(entry.bytes)!=0x545345){c->original_return=0;c->complete=1;return GH_EFFECT_OK;}
 c->est=entry;c->count=w(c->est.bytes+4);if(c->count>(c->est.size-16)/0x2f0)return fail(c,0x3066d0,GH_EFFECT_BOUNDS);c->config_flags=(w((const uint8_t*)b->work+0x5e4)&0x40)?2:0;
 if(c->request.bank==0&&c->request.index==0x1a&&r101_door_break_actor(c->request.actor)){
  const gh_title_player_view *actor=c->request.actor;
  if(b->work->room!=0x101||!b->model||b->model->work!=b->work||
     !b->model->parts_ready||b->model->root.bytes!=actor->bytes||c->request.mode!=-1||
     c->request.selector!=-1||c->count!=2||actor->bytes[0x2f4]!=2||actor->bytes[0x2f5])
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<2;i++){
   const uint8_t *r=entry.bytes+16+i*0x2f0;
   if(!gh_effect_r101_door_break_part_is_bound((gh_effect_view){r,0x2f0},actor,actor)||
      r[0x12e]!=(i?2:132))return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
  c->config_flags=(w(actor->bytes+0x5b8)&0x20u)?2u:0;
 }
 if(c->request.bank==0&&c->request.index>=0x15&&c->request.index<=0x18){
  static const unsigned variants[]={3,2,0,1},counts[]={5,5,2,1};
  static const uint8_t parts[][5]={{2,2,23,0,21},{2,2,19,25,24},{0,21},{21}};
  static const uint8_t children[][5]={{0x12,0,0,0x1a,0x1a},{0x12,0,0,0x1a,0x1a},{7,0},{0}};
  const unsigned index=c->request.index-0x15;
  const gh_title_player_view *actor=c->request.actor;
  if(!actor||!actor->bytes||actor->size<0x300||actor->original_vtable214!=0x41d220||
     (w(actor->bytes+0x2fc)>>16)!=0x100||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=6||
     actor->bytes[0x2f6]||actor->bytes[0x2f7]!=variants[index]||
     c->request.mode!=-1||c->request.selector!=-1||c->count!=counts[index])
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;++i){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(!player_dodge_effect_row(c->request.index,row)||row[0xbe]!=parts[index][i]||row[0x133]!=children[index][i])
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->configuration_2fa158)c->config_flags|=0xe;
 if(c->configuration_05288)c->config_flags|=2;
 if(c->configuration_nested_group)c->config_flags|=c->configuration_nested_flags;
 if(c->request.bank==0x58&&(c->request.index==0x2c||c->request.index==0x5a)&&
    c->configuration_nested_scale){
  static const uint8_t parts[]={10,0,25,21};
  static const uint8_t children[]={0x12,9,0x1a,0x1a};
  static const uint32_t row_flags[]={0x00c00000,0x00e01000,0x00c00000,0x00c00000};
  if(c->count!=4)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<4;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(row[0xbe]!=parts[i]||row[0x133]!=children[i]||row[0x128]||row[0x136]||
      w(row+4)!=row_flags[i])return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->request.bank==0x58&&c->request.index==0x2b&&c->configuration_nested_scale){
  static const uint8_t parts[]={25,24,21,20,24,21};
  static const uint8_t children[]={0x12,0x20,0x12,0x20,0x1a,0x1a};
  if(c->count!=6||!b->work||b->work->room!=0x100)
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<6;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(!stamp8_attack_effect_row(row)||row[0xbe]!=parts[i]||row[0x133]!=children[i])
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->request.bank==0x58&&c->request.index==0x1e&&c->configuration_nested_scale){
  if(c->count!=2||!em0a_attack_effect_row(entry.bytes+16)||
     !em0a_attack_effect_row(entry.bytes+16+0x2f0)||entry.bytes[16+0x133]!=9||
     entry.bytes[16+0x2f0+0x133]!=0x12)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }
 if(c->request.bank==0x58&&c->request.index==0x5d&&c->configuration_nested_scale){
  if(c->count!=1||b->work->room!=0x102||!b->model||b->model->work!=b->work||
     !c->request.actor||b->model->root.bytes!=c->request.actor->bytes||
     !em0f_route_effect_row(entry.bytes+16))return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }
 if(c->request.bank==0x58&&c->request.index==0x59&&c->configuration_nested_scale){
  if(c->count!=10)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;i++)
   if(!escape_effect_row_59(entry.bytes+16+i*0x2f0,i))return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }
 if(c->request.bank==0x58&&c->request.index==0x27&&c->configuration_nested_scale){
  if(c->count!=4)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<4;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(row[0xbe]!=(i<2?2:0)||row[0x128]||row[0x133]!=(i<2?7:0x1a)||
      row[0x136]||w(row+4)!=(i<2?0x00800000u:0x00c00000u))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->request.bank==0x58&&c->request.index==0x3f&&c->configuration_nested_scale){
  static const uint8_t parts[]={0,0,0,10,10,10,10,8,16,16,16,16,14,4,4,26,26,26,26,26,0,26,26};
  if(c->count!=sizeof(parts))return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<sizeof(parts);i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   const int ground=i<3||i==20;
   const unsigned child=ground?7:(i==3||i==8)?0x1f:0;
   const uint32_t flags=ground?0x00802000u:
    (i<=12||i==14||i==19)?0x00c00000u:0x00800000u;
   if(row[0xbe]!=parts[i]||row[0x128]||row[0x133]!=child||row[0x136]||w(row+4)!=flags)
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->request.bank==0x58&&c->request.index==0x40&&devil_projectile_effect_actor(c->request.actor)){
  if(c->count!=4||c->request.mode||c->request.selector!=-1)
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(w(row+4)!=0x00800000u||row[0xbe]||row[0x128]||row[0x133]||row[0x136])
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 const unsigned devil_ranged=c->request.bank==0x58&&c->configuration_nested_scale?
    devil_ranged_effect_rows(c->request.index):0;
 if(devil_ranged){
  if(c->count!=devil_ranged)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   const unsigned part=row[0xbe],child=row[0x133];
   const uint32_t flags=w(row+4);
   if(row[0x128]||row[0x136]||
      (child==7?((part!=10&&part!=16)||flags!=0x00800000u):
       child==0?((part!=0&&part!=10&&part!=16)||flags!=0x00c00000u):
       child!=0x1a||part||flags!=0x00800000u))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->request.bank==0&&c->request.index==0x3e&&c->configuration_nested_scale){
  const uint8_t *row=entry.bytes+16;
  if(c->count!=1||row[0xbe]!=27||row[0x128]||row[0x133]||row[0x136]||
     w(row+4)!=0x00c00000u)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }
 if(c->request.bank==0&&c->request.index==0x42&&c->configuration_nested_scale){
  static const uint32_t flags[]={0x00802000,0x00800000,0x00c00000,0x00c00000,0x00c00000};
  if(c->count!=5)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(row[0xbe]!=(i<2?2:0)||row[0x128]||row[0x133]!=(i<2?7:0x1a)||
      row[0x136]||w(row+4)!=flags[i])return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->configuration_positioned&&c->request.bank==0x58&&c->request.index==0x41){
  if(c->count!=5)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(row[0xbe]!=0xff||row[0x128]||row[0x133]||row[0x136]||w(row+4)!=0x00800000u)
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->configuration_positioned&&c->request.bank==0x58&&c->request.index==0x12){
  static const uint8_t particles[]={7,0x18,0,0},textures[]={0,0,1,7};
  if(c->count!=sizeof(particles))return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<sizeof(particles);++i){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(row[0xbe]!=0xff||row[0x128]||row[0x133]!=particles[i]||row[0x136]||
      row[0x12e]!=textures[i]||w(row+4)!=0x00800000u)
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->configuration_positioned&&c->request.bank==0&&c->request.index==7){
  static const uint8_t textures[]={0,0,2,5};
  if(c->count!=sizeof(textures))return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(w(row+4)!=0xc00000u||row[0xbe]!=0xff||row[0x128]||row[0x133]||
      row[0x136]||row[0x12e]!=textures[i])
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->configuration_positioned&&c->request.bank==0&&c->request.index==0x1b){
  static const uint8_t textures[]={11,51,36};
  if(c->count!=sizeof(textures))return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<sizeof(textures);++i){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(w(row)!=0x1efe8012u||w(row+4)!=0x00800000u||row[0xbe]!=0xff||row[0x128]||
      row[0x133]||row[0x136]||row[0x12e]!=textures[i])
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->configuration_positioned&&c->request.bank==0&&c->request.index==0xc){
  static const uint8_t texture_items[]={11,57,0,76};
  if(c->count!=4)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<4;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(row[0xbe]!=0xff||row[0x128]||row[0x133]||row[0x136]||row[0x12e]!=texture_items[i]||
      w(row+4)!=(i==2?0x00a02000u:0x00802000u))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->configuration_player_stun){
  if(c->count!=2)return fail(c,0x306900,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<2;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(!gh_effect_player_stun_row((gh_effect_view){row,0x2f0})||row[0x133]!=(i?9:0))
    return fail(c,0x306900,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->attachment_selector){
  const int16_t room=weapon_swing_room(c->request.actor,c->attachment_selector);
  if(!room||b->work->room!=room||!b->model||b->model->work!=b->work||
     !b->model->parts_ready||b->model->root.bytes!=c->request.actor->bytes||
     c->count!=weapon_est_count(room,c->request.index))
   return fail(c,0x303a30,GH_EFFECT_UNSUPPORTED);
  if(room==0x102&&(!b->model->external_parent||
     b->model->external_parent->root.bytes!=c->attachment_selector->bytes))
   return fail(c,0x303a30,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(!r101_weapon_est_row(c->request.index,row,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
  c->config_flags=0;
 }
 if(c->configuration_death_part_zero==2)c->config_flags=0;
 if(c->configuration_death_part_zero==1&&
    (c->count!=1||entry.bytes[16+0xbe]||entry.bytes[16+0x128]||entry.bytes[16+0x136]))
  return fail(c,0x306900,GH_EFFECT_UNSUPPORTED);
 c->stage=1;return GH_EFFECT_OK;
}

static gh_effect_status resolve_event_actor(gh_effect_create_call *call,
 const gh_effect_create_bindings *bindings,const uint8_t *row)
{
 call->event_row_actor=NULL;
 call->event_row_model=NULL;
 const uint32_t id=w(row+0x2a0);
 const uint16_t kind=(uint16_t)(id>>16);
 if(!kind)return GH_EFFECT_OK;
 if(!bindings->event_actor)return fail(call,0x297c50,GH_EFFECT_UNSUPPORTED);
 uint32_t frontier=0;
 const uint8_t code=(uint8_t)id;
 gh_effect_status status=bindings->event_actor(bindings->event_actor_context,kind,code,
  &call->event_row_actor,&call->event_row_model,&frontier);
 if(!status&&!call->event_row_actor&&(kind==0x605||kind==0x3d6)){
  status=bindings->event_actor(bindings->event_actor_context,kind==0x605?0x600:0x668,code,
   &call->event_row_actor,&call->event_row_model,&frontier);
 }
 if(status)return fail(call,frontier?frontier:0x297c50,status);
 if((call->event_row_actor&&(!call->event_row_actor->bytes||call->event_row_actor->size<0x2b5))||
    (call->event_row_model&&(!call->event_row_actor||call->event_row_model->root.bytes!=call->event_row_actor->bytes)))
  return fail(call,0x297c50,GH_EFFECT_ARGUMENT);
 return GH_EFFECT_OK;
}
static int event_class2(const gh_effect_create_call *c,const gh_effect_create_bindings *b,
 const uint8_t *r)
{
 if(r[0x128]!=2||!c->configuration_2fa158||c->request.bank!=6||
    c->event_row_actor||c->event_row_model||w(r+4)!=0x00800000u||
    r[0xbe]!=0xff||r[0x133]||r[0x137]||r[0x27c]||w(r+0x2a0)||
    w(r+0x24c)||w(r+0x250)||w(r+0x254)||w(r+0x258))
  return 0;
 if(b->work->room==0x102)
  return c->request.index==11&&c->count==1&&w(r)==0x1e3e0010u&&
      w(r+0xbc)==0x00ff000cu&&w(r+0xc0)==0x00350034u;
 /* EV1A repeats the same parentless radial ESP00 across several cuts. */
 return b->work->room==0x103&&w(r)==0x1e300010u&&
     r[0xbc]>=9&&r[0xbc]<=16&&r[0xbd]==1&&w(r+0xc0)==0x00010000u;
}

static gh_effect_status allocate(gh_effect_create_call*c,const gh_effect_create_bindings*b,const uint8_t*r){
 const int r103_ambient_class2=r[0x128]==2&&b->work->room==0x103&&
   c->configuration_305e88&&!c->request.index&&
   (c->row==11||c->row==13||c->row==15)&&
   w(r)==0x1e300010u&&w(r+4)==0x00800000u&&r[0xbe]==0xff&&!r[0x133]&&
   w(r+0x24c)==0&&w(r+0x250)==0&&w(r+0x254)==0&&!r[0x27c];
 const int curved=r103_ambient_class2||event_class2(c,b,r);
 if(r[0x128]&&!curved)
  return fail(c,r[0x128]==1?0x304d70:r[0x128]==2?0x304db0:r[0x128]==3?0x304df0:0x304d48,GH_EFFECT_UNSUPPORTED);
 gh_game_effects*e=c->owner;if(!e->native){e->native=calloc(1,sizeof(*e->native));if(!e->native)return fail(c,0x300458,GH_EFFECT_UNSUPPORTED);e->native->owner=e;e->native->esp=b->storage.esp;e->native->emitters=b->storage.emitters;}
 if(e->native->serial==UINT32_MAX)return fail(c,0x300458,GH_EFFECT_UNSUPPORTED);
 uint8_t*base=b->storage.emitters;unsigned slot=0;for(;slot<256;slot++)if(!(w(base+0x26000+(slot>>5)*4)&(0x80000000u>>(slot&31))))break;
 if(slot==256){c->original_return=0;c->complete=1;return GH_EFFECT_OK;}if(e->native->slots[slot].associated&&e->native->slots[slot].first_esp_child!=-1)return fail(c,0x300458,GH_EFFECT_UNSUPPORTED);uint32_t mask=0x80000000u>>(slot&31),off=(slot>>5)*4;
 ww(base+0x26000+off,w(base+0x26000+off)|mask);ww(base+0x26020+off,w(base+0x26020+off)&~mask);ww(base+0x26040+off,w(base+0x26040+off)&~mask);
 uint8_t*p=base+(size_t)slot*0x260;memset(p,0,0x260);construct_emitter_00302790(p);
 if(curved)ww(p+0xf0,0x44fc98); /* 304DB0's only override. */
 c->slot=slot;c->serial=++e->native->serial;
 const gh_title_player_view *actor=c->configuration_2fa158?c->event_row_actor:c->request.actor;
 e->native->slots[slot]=(emitter_metadata){.actor=actor,.selector=c->attachment_selector?c->attachment_selector:(c->request.selector==-1?c->request.actor:NULL),.record=r,.serial=c->serial,.associated=1,.first_esp_child=-1};c->stage=2;return GH_EFFECT_OK;
}
static gh_effect_status initialize_prefix(gh_effect_create_call*c,const gh_effect_create_bindings*b,const uint8_t*r){
 emitter_metadata*m=&c->owner->native->slots[c->slot];uint8_t*p=b->storage.emitters+(size_t)c->slot*0x260;
 if(!m->associated||m->serial!=c->serial||m->record!=r)return GH_EFFECT_ARGUMENT;
 if(c->request.selector!=-1)return fail(c,0x303a30,GH_EFFECT_UNSUPPORTED);
 ww(p+0x110,(uint32_t)c->request.mode);half(p+0x18c,(uint16_t)c->request.bank);half(p+0x18e,(uint16_t)c->request.index);
 ww(p+0x118,c->config_flags&~8u);/*303EFC/303F00 copies distinct config+48 verbatim.*/ww(p+0x1e8,c->config_flags);if(c->configuration_nested_group)p[0x214]=c->configuration_nested_group;
 if(c->configuration_death_part_zero)p[0x1ec]=0;
 if(c->configuration_player_stun)p[0x1ec]=4;
 if(!c->configuration_nested_group&&c->configuration_nested_scale)
  ww(p+0x1e0,c->configuration_nested_scale);
 if(!r[0x28e]){if(!b->random)return fail(c,0x31cfe0,GH_EFFECT_UNSUPPORTED);if(c->random_identity&&c->random_identity!=b->random)return GH_EFFECT_ARGUMENT;c->random_identity=b->random;uint32_t seed;if(gh_game_mt_next_0030f348(b->random,&seed))return fail(c,0x31cfe0,GH_EFFECT_ARGUMENT);ww(p+0x220,seed);}else ww(p+0x220,r[0x28e]);c->stage=3;return GH_EFFECT_OK;
}
static gh_effect_status initialize_positioned_transform(gh_effect_create_call *call,
 const gh_effect_create_bindings *bindings,uint8_t *payload)
{
 if(!bindings->atan_coeff)return fail(call,0x147c88,GH_EFFECT_UNSUPPORTED);
 if(call->configuration_positioned_atan_coeff&&call->configuration_positioned_atan_coeff!=bindings->atan_coeff)
  return fail(call,0x147c88,GH_EFFECT_ARGUMENT);
 float config_euler[3]={0},euler[3],rotation[16],translation[16],local[16],world[16];
 memcpy(config_euler+1,&call->configuration_positioned_yaw,4);
 identity((uint8_t *)translation);
 memcpy(translation+12,call->configuration_positioned_position,12);
 identity((uint8_t *)world);

 uint32_t frontier=0;
 const gh_static_transform_policy policy=GH_STATIC_BINARY32_REFERENCE;
 /* 3040AC reads the original zero-rotation/zero-position cNode basis.
  * 3040EC forms config T*R, and 3040FC postmultiplies that basis. */
 if(gh_actor_matrix_rotation_00147728(config_euler,0,rotation,policy,&frontier)||
    gh_actor_matrix_product_00147548(translation,rotation,local,policy,&frontier)||
    gh_actor_matrix_product_00147548(world,local,world,policy,&frontier)||
    gh_actor_matrix_extract_euler_00147c88(world,bindings->atan_coeff,euler,policy,&frontier))
  return fail(call,frontier?frontier:0x147c88,GH_EFFECT_UNSUPPORTED);

 call->configuration_positioned_atan_coeff=bindings->atan_coeff;
 memcpy(payload+0x1b0,config_euler,12);
 memcpy(payload+0x1c0,call->configuration_positioned_position,12);
 memcpy(payload+0x30,euler,12);
 memcpy(payload+0x40,world+12,12);
 return GH_EFFECT_OK;
}

/* 303A30 composes the authored parentless cNode with the default config
 * translation and rotation, both identity for these class 2 rows. */
static gh_effect_status initialize_class2_node_00303a30(gh_effect_create_call *c,
 const gh_effect_create_bindings *b,uint8_t *p)
{
 if(!b->atan_coeff)return fail(c,0x147c88,GH_EFFECT_ARGUMENT);
 float local[16],angle[3],position[3],scale[3];
 uint32_t pc=0;
 memcpy(angle,p+0x30,12);
 memcpy(position,p+0x40,12);
 memcpy(scale,p+0x20,12);
 const gh_static_transform_policy policy=GH_STATIC_BINARY32_REFERENCE;
 if(gh_actor_matrix_compose_00147a48(position,angle,scale,0,local,policy,&pc)||
    gh_actor_matrix_extract_euler_00147c88(local,b->atan_coeff,angle,policy,&pc))
  return fail(c,pc?pc:0x147c88,GH_EFFECT_UNSUPPORTED);
 p[0x16]=0;
 ww(p+0x90,0);
 memcpy(p+0x30,angle,12);
 memcpy(p+0x40,local+12,12);
 return GH_EFFECT_OK;
}

static gh_effect_status initialize_record(gh_effect_create_call*c,const gh_effect_create_bindings*b,const uint8_t*r){
 emitter_metadata*m=&c->owner->native->slots[c->slot];uint8_t*p=b->storage.emitters+(size_t)c->slot*0x260;
 /* Exact observed zero-vector domain of30A370: VDIV1/0 saturates finiteQ;
  * VMUL.xyz zero * finiteQ produces zero and destW is explicitlyzero.
  * No VU status flags are consumed by303A30. General norm pending. */
 for(unsigned i=0;i<3;i++)if(w(r+0x230+4*i)!=0)return fail(c,0x30a370,GH_EFFECT_UNSUPPORTED);
 if((w(r+0x23c)&0x80000000u)||(w(r+0x23c)&0x7f800000u)==0x7f800000u)return fail(c,0x30aba0,GH_EFFECT_UNSUPPORTED);
 /* Default config+75 is uninitialized padding in original306528. Raw215
  * stays initialhostzero; typedview marks it UNKNOWN, never an originalvalue
  * claim. It is not read by remaining303A30/type0Init. */
 memset(p+0x11c,0,12);memcpy(p+0x174,r+0x240,4);uint32_t negative=w(r+0x244)^0x80000000u;ww(p+0x178,negative);half(p+0x18a,(uint16_t)(w(r+0x19c)<<1));
 memcpy(p+0x30,r+0x1a4,12);memcpy(p+0x128,r+0x1b4,12);memcpy(p+0x14c,r+0x1d4,12);memcpy(p+0x158,r+0x1e4,12);memcpy(p+0x40,r+0x204,12);memcpy(p+0x164,r+0x214,12);
 int part=c->configuration_player_stun?4:(int8_t)r[0xbe];if(part==-3){/* Original303A30 false -> original3005D8 marks pendingfree. */unsigned off=(c->slot>>5)*4;ww(b->storage.emitters+0x26020+off,w(b->storage.emitters+0x26020+off)|(0x80000000u>>(c->slot&31)));c->original_return=0;c->complete=1;return GH_EFFECT_OK;}
 if(part>=0){const gh_actor_model *model=c->configuration_2fa158?c->event_row_model:b->model;if(!model||!model->parts_ready||!m->actor||model->root.bytes!=m->actor->bytes)return fail(c,0x303a30,GH_EFFECT_UNSUPPORTED);const gh_actor_part*view;if(gh_actor_model_part_at(model,(unsigned)part,&view))return fail(c,0x303a30,GH_EFFECT_BOUNDS);m->model=model;m->part_index=(uint32_t)part;m->part_handle=view->handle;}
 else if(part==-2){if(!m->actor)return fail(c,0x303a30,GH_EFFECT_UNSUPPORTED);}
 else if(w(p+0xf0)==0x44fc98){
  gh_effect_status status=initialize_class2_node_00303a30(c,b,p);
  if(status)return status;
 }else{
  /* Actual NULL-config, parentless identity cNode branch. All21 rows except
   * this one bind realparts. 2B64C0/147A48 computes identity; default draw
   * rotation147728/translation1476D0 alsoidentity. 147548 staysidentity;
   *147C88 mode0 zero-angle earlyleaves return(+0,-0,+0),147C60 XYZzero.
   * General unattached transforms remain explicit, not a bindpose fallback. */
  for(unsigned k=0;k<3;k++)if(w(p+0x30+4*k)!=0||w(p+0x40+4*k)!=0)return fail(c,0x2b61b0,GH_EFFECT_UNSUPPORTED);
  identity(p+0x50);identity(p+0xb0);p[0x16]=0;ww(p+0x90,0);
  if(c->configuration_positioned){
   if(part!=-1)return fail(c,0x303a30,GH_EFFECT_UNSUPPORTED);
   gh_effect_status status=initialize_positioned_transform(c,b,p);
   if(status)return status;
  }else{
   ww(p+0x30,0);ww(p+0x34,0x80000000u);ww(p+0x38,0);memset(p+0x40,0,12);
  }
 }
 half(p+0x186,(uint16_t)((uint16_t)(r[0xc0]|(unsigned)r[0xc1]<<8)+p[0x1ed]));uint16_t duration=(uint16_t)(r[0xc2]|(unsigned)r[0xc3]<<8);if(duration&&duration<0x8000)duration=(uint16_t)(duration+p[0x1ed]);half(p+0x188,duration);
 if((w(r+4)&0x48000)&&m->actor){if(m->actor->size<0x2f6)return fail(c,0x303a30,GH_EFFECT_BOUNDS);p[0x226]=m->actor->bytes[0x2f4];p[0x227]=m->actor->bytes[0x2f5];}
 c->stage=4;return GH_EFFECT_OK;
}
gh_effect_status gh_game_effect_create_00306528(gh_effect_create_call*c,const gh_effect_create_bindings*b){
 if(!c||!b||!b->work||!c->active||c->identity!=c)return GH_EFFECT_ARGUMENT;c->frontier=0;if(c->complete)return GH_EFFECT_OK;
 if(c->request.actor&&(!c->request.actor->bytes||c->request.actor->size<0x2b5))return GH_EFFECT_ARGUMENT;gh_effect_status e=storage(c,b);if(e)return e;
 if(!c->stage){e=select_est(c,b);if(e||c->complete)return e;}
 while(c->row<c->count){const uint8_t*r=c->est.bytes+16+(size_t)c->row*0x2f0;if(c->stage==1){if(r[0x136]!=c->configuration_nested_group){c->row++;continue;}if(c->configuration_2fa158){e=resolve_event_actor(c,b,r);if(e)return e;}const gh_title_player_view *actor=c->configuration_2fa158?c->event_row_actor:c->request.actor;if(!actor&&(int8_t)r[0xbe]!=-1&&(int8_t)r[0xbe]!=-4){c->original_return=0;c->complete=1;return GH_EFFECT_OK;}e=allocate(c,b,r);if(e||c->complete)return e;}
  if(c->stage==2){e=initialize_prefix(c,b,r);if(e||c->complete)return e;}
  if(c->stage==3){e=initialize_record(c,b,r);if(e||c->complete)return e;}
  if(c->stage==4){
   uint8_t*p=b->storage.emitters+(size_t)c->slot*0x260;
   if(w(p+0xf0)==0x44fc98){
    /* Class 2 virtual initializer 304BB8, including the drag complement. */
    memcpy(p+0x240,r+0x248,12);
    ww(p+0x24c,0x3f800000u);
    p[0x254]=r[0x27c];
   }else memcpy(p+0x224,p+0x210,2); /* Class 0 virtual 303A20. */
   c->owner->native->slots[c->slot].initialized=1;
   c->row++;
   c->stage=1;
  }
 }
 c->complete=1;c->original_return=1;return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_actor_match(void*raw,int emitter,unsigned slot,const gh_title_player_view*a,int*match,uint32_t*f){
 gh_game_effects*e=raw;if(f)*f=0;if(!e||!match||!f||slot>=(emitter?256u:1024u))return GH_EFFECT_ARGUMENT;if(!emitter){gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status){*f=0x2feb80;return GH_EFFECT_UNSUPPORTED;}if(w(v.payload+0x108)){*f=0x2feb80;return GH_EFFECT_UNSUPPORTED;}*match=(!a&&!v.selector)||(a&&v.selector&&a->bytes==v.selector->bytes);return GH_EFFECT_OK;}if(!e->native){*f=0x3039e8;return GH_EFFECT_UNSUPPORTED;}if(e->native->owner!=e)return GH_EFFECT_ARGUMENT;emitter_metadata*m=&e->native->slots[slot];if(!m->associated||m->quiesced){*f=0x3039e8;return GH_EFFECT_UNSUPPORTED;}*match=(!a&&!m->actor)||(a&&m->actor&&a->bytes==m->actor->bytes);return GH_EFFECT_OK;
}
static gh_effect_status release_trail(esp_metadata *view)
{
 for(unsigned i=0;i<sizeof(view->trail)/sizeof(view->trail[0]);i++){
  if(!view->trail[i].pointer)continue;
  if(gh_room_arena_free_002a9680(view->trail_pools,view->trail[i]))return GH_EFFECT_ARGUMENT;
  view->trail[i]=(gh_room_arena_allocation){0};
 }
 view->trail_pools=NULL;
 return GH_EFFECT_OK;
}
void gh_effect_create_forget_on_owner_destroy(gh_game_effects*s){if(!s||!s->native)return;if(s->native->owner!=s)return;for(unsigned i=0;i<1024;i++)(void)release_trail(s->native->esps+i);free(s->native);s->native=NULL;}

int gh_effect_player_attachment_emitter_is_bound(const gh_effect_emitter_view *view)
{
 if(!view)return 0;
 const gh_title_player_view *actor=view->actor;
 const uint8_t *record=view->record.bytes;
 const uint8_t *payload=view->payload;
 /* 58:5D uses the same part-attached ESP1A path and original unlink order. */
 if(em0f_route_actor(actor)&&actor==view->selector&&view->model&&view->model->work&&
    view->model->work->room==0x102&&view->model->root.bytes==actor->bytes&&view->model->part_count&&
    view->part==view->model->parts&&view->record.size==0x2f0&&record&&payload&&view->payload_size==0x260&&
    w(payload+0x18c)==0x005d0058u&&!w(payload+0x110)&&em0f_route_effect_row(record))return 1;
 /* 58:27 uses the same attached type7/1A pose and lifetime as PL00 0:42. */
 if(actor&&actor==view->selector&&actor->bytes&&actor->size>=0x644&&
    actor->original_vtable214==0x4479c8&&(w(actor->bytes+0x2fc)>>16)==0x202&&
    w(actor->bytes+0x564)==0x200&&(w(actor->bytes+0x640)&0xffffu)==20&&
    view->model&&view->model->root.bytes==actor->bytes&&view->record.size==0x2f0&&
    record&&view->payload_size==0x260&&payload&&
    (w(payload+0x18c)==0x00270058u||w(payload+0x18c)==0x00590058u)&&
    !w(payload+0x110)&&!record[0x128]&&!record[0x136]){
  const unsigned part=record[0xbe];
  if(part>=view->model->part_count||view->part!=view->model->parts+part)return 0;
  if(w(payload+0x18c)==0x00590058u){
   for(unsigned i=0;i<10;i++)if(escape_effect_row_59(record,i))return 1;
   return 0;
  }
  return (part==2&&record[0x133]==7&&w(record+4)==0x00800000u)||
         (part==0&&record[0x133]==0x1a&&w(record+4)==0x00c00000u);
 }
 if(!actor||actor!=view->selector||!actor->bytes||actor->size<0x300||
    actor->original_vtable214!=0x41d220||(w(actor->bytes+0x2fc)>>16)!=0x100||
    !view->model||view->model->root.bytes!=actor->bytes||view->record.size!=0x2f0||
    !record||view->payload_size!=0x260||!payload)return 0;
 const uint32_t profile=w(payload+0x18c);
 if(!(profile&0xffffu)&&(profile>>16)>=0x15&&(profile>>16)<=0x18&&
    w(payload+0x110)==UINT32_MAX){
  const unsigned part=record[0xbe];
  return player_dodge_effect_row(profile>>16,record)&&part<view->model->part_count&&
     view->part==view->model->parts+part;
 }
 if(w(payload+0x18c)==0x00040000u&&w(payload+0x110)==4&&payload[0x1ec]==4)
  return view->model->part_count>4&&view->part==view->model->parts+4&&gh_effect_player_stun_row(view->record);
 if(w(payload+0x18c)==0x00420000u&&!w(payload+0x110)&&!record[0x128]&&!record[0x136]){
  const uint32_t flags=w(record+4);
  const unsigned part=record[0xbe];
  if(part>=view->model->part_count||view->part!=view->model->parts+part)return 0;
  return (part==2&&record[0x133]==7&&(flags==0x00802000u||flags==0x00800000u))||
      (part==0&&record[0x133]==0x1a&&flags==0x00c00000u);
 }
 return view->model->part_count>27&&view->part==view->model->parts+27&&
    w(record+4)==0x00c00000u&&record[0xbe]==27&&!record[0x128]&&!record[0x133]&&!record[0x136]&&
    w(payload+0x18c)==0x003e0000u&&w(payload+0x110)==UINT32_MAX;
}

int gh_effect_r101_weapon_emitter_is_bound(const gh_effect_emitter_view *v)
{
 if(!v)return 0;
 const int16_t room=weapon_swing_room(v->actor,v->selector);
 if(!room||!v->model||!v->model->work||v->model->work->room!=room||
    v->model->root.bytes!=v->actor->bytes||
    v->model->part_count<=3||v->part!=v->model->parts+3||
    !v->payload||v->payload_size!=0x260||
    !v->record.bytes||v->record.size!=0x2f0)return 0;
 if(room==0x102&&(!v->model->external_parent||
    v->model->external_parent->root.bytes!=v->selector->bytes))return 0;
 const uint32_t profile=w(v->payload+0x18c);
 const uint8_t *row=v->record.bytes;
 if((profile&0xffffu)!=0x58||w(v->payload+0x110))return 0;
 for(unsigned i=0;i<weapon_est_count(room,profile>>16);i++)
  if(r101_weapon_est_row(profile>>16,row,i))return 1;
 return 0;
}

static int em02_em03_attack_actor(const gh_title_player_view *actor)
{
 if(!actor||!actor->bytes||actor->size<0x644||(w(actor->bytes+0x2fc)>>16)!=0x202)return 0;
 const unsigned stamp=w(actor->bytes+0x640)&0xffffu;
 const uint32_t dependency=w(actor->bytes+0x564);
 const int appearance0=actor->size>0x1650&&actor->original_vtable214==0x4479c8&&!actor->bytes[0x1650];
 return (dependency==0x203&&(stamp==19||(stamp==1&&appearance0)))||
        (dependency==0x202&&appearance0&&
         (stamp==0x17||(stamp==0x19&&actor->size>=0x16c8&&w(actor->bytes+0x16c4)==2)));
}

int gh_effect_stamp19_attack_part_is_bound(const gh_effect_emitter_view *v)
{
 if(!v||!em02_em03_attack_actor(v->actor)||!v->model)return 0;
 /* R105 EM03 and R102 EM02 use the original 58:2D SEQ on part 15. */
 const unsigned stamp=w(v->actor->bytes+0x640)&0xffffu;
 if(stamp!=19&&(!v->model->work||v->model->work->room!=(stamp==1?0x105:0x102)))return 0;
 return v->selector==v->actor&&
        v->model->root.bytes==v->actor->bytes&&v->model->part_count>15&&
        v->part==v->model->parts+15&&v->record.bytes&&v->record.size==0x2f0&&
        v->payload&&v->payload_size==0x260&&w(v->payload+0x18c)==0x002d0058u&&
        v->record.bytes[0xbe]==15&&
        !v->record.bytes[0x128]&&!v->record.bytes[0x136]&&
        (v->record.bytes[0x133]==0x12||v->record.bytes[0x133]==9)&&
        !w(v->payload+0x110)&&v->payload[0x1ec]==0xff;
}

int gh_effect_em00_attack_emitter_is_bound(const gh_effect_emitter_view *v)
{
 if(!v||!v->payload||v->payload_size!=0x260||!v->initialized||
    !v->actor||v->actor!=v->selector||
    !v->model||v->model->root.bytes!=v->actor->bytes||
    !v->record.bytes||v->record.size!=0x2f0||v->record.bytes[0x128]||v->record.bytes[0x136]||
    w(v->payload+0x110)||v->payload[0x1ec]!=0xff)return 0;
 if(em27_attack_actor(v->actor)&&(!v->model->work||v->model->work->room!=0x105))return 0;
 const uint32_t profile=w(v->payload+0x18c);
 if(profile==0x002b0058u){
  const unsigned part=v->record.bytes[0xbe];
  return v->model->work&&v->model->work->room==0x100&&stamp8_attack_actor(v->actor)&&
         stamp8_attack_effect_row(v->record.bytes)&&part<v->model->part_count&&
         v->part==v->model->parts+part;
 }
 if(profile==0x001e0058u)
  return v->model->work&&v->model->work->room==0x100&&em0a_attack_actor(v->actor)&&
         em0a_attack_effect_row(v->record.bytes)&&v->model->part_count>16&&v->part==v->model->parts+16;
 if(!em00_family_attack_actor(v->actor)||(profile!=0x002c0058u&&profile!=0x005a0058u))return 0;
 const unsigned part=v->record.bytes[0xbe];
 const unsigned type=v->record.bytes[0x133];
 if(!((part==10&&type==0x12)||(part==0&&type==9)||((part==25||part==21)&&type==0x1a)))
    return 0;
 return part<v->model->part_count&&v->part==v->model->parts+part;
}

static int enemy_light_row(const gh_game_effects *owner,gh_effect_view record,
 const gh_title_player_view *actor,const gh_title_player_view *selector)
{
 if(!actor||actor!=selector||!actor->bytes||actor->size<0x568||
    actor->original_vtable214!=0x4479c8||!record.bytes||record.size!=0x2f0)
  return 0;
 const uint8_t *p=actor->bytes;
 if(p[0x2fe]!=2||p[0x2ff]!=2)return 0;
 unsigned index;
 switch(w(p+0x564)){
 case 0x200:case 0x202:case 0x203:index=0xb;break;
 case 0x20a:index=0xc;break;
 default:return 0;
 }
 const uint8_t *row=record.bytes;
 if(w(row)!=0x1e300010u||w(row+4)!=0x00800000u||
    row[0xbe]!=4||row[0x128]||row[0x133]||row[0x136])
  return 0;
 gh_effect_view est;
 return !gh_effect_est_at_00306030(owner,0x58,index,&est)&&est.bytes&&
    est.size>=16+2*0x2f0&&w(est.bytes+4)==2&&
    (row==est.bytes+16||row==est.bytes+16+0x2f0);
}

int gh_effect_enemy_light_emitter_is_bound(const gh_game_effects *owner,
 const gh_effect_emitter_view *v)
{
 return v&&v->initialized&&v->payload&&v->payload_size==0x260&&
     (w(v->payload+0x18c)==0x000b0058u||w(v->payload+0x18c)==0x000c0058u)&&
     w(v->payload+0x110)==4&&
     enemy_light_row(owner,v->record,v->actor,v->selector)&&
     v->model&&v->model->work&&v->model->work->room==0x100&&
     v->model->root.bytes==v->actor->bytes&&v->model->part_count>4&&v->part==v->model->parts+4;
}

int gh_effect_enemy_light_esp_is_bound(const gh_game_effects *owner,const gh_effect_esp_view *v)
{
 return v&&v->type==0&&v->payload&&v->payload_size==0x400&&
     enemy_light_row(owner,v->record,v->actor,v->selector);
}

int gh_effect_knockdown_emitter_is_bound(const gh_effect_emitter_view *v)
{
 if(!v||!v->payload||v->payload_size!=0x260)return 0;
 const uint32_t program=w(v->payload+0x18c);
 if((program!=0x002f0058u&&program!=0x00300058u)||!v->initialized||
    !v->actor||!v->actor->bytes||v->actor->size<0x644||v->actor!=v->selector||
    !v->model||v->model->root.bytes!=v->actor->bytes||
    !v->record.bytes||v->record.size!=0x2f0||v->record.bytes[0x128]||v->record.bytes[0x136]||
    w(v->payload+0x110)||v->payload[0x1ec])return 0;
 const unsigned stamp=w(v->actor->bytes+0x640)&0xffffu;
 const uint32_t dependency=w(v->actor->bytes+0x564);
 if((w(v->actor->bytes+0x2fc)>>16)!=0x202||
    !((stamp==19&&dependency==0x203)||(stamp==20&&dependency==0x200)||
      (program==0x002f0058u&&v->model->work&&v->model->work->room==0x100&&
       stamp8_attack_actor(v->actor))))return 0;
 const unsigned part=v->record.bytes[0xbe];
 return part<v->model->part_count&&v->part==v->model->parts+part;
}
/* The 240548 recovery SEQ shares bank 58/index2 with the part-0
 * emitter already used by the original knockdown/death paths. */
static int r102_landing_emitter(const gh_effect_emitter_view *v)
{
 const gh_title_player_view *actor=v->actor;
 if(!v->model->work||v->model->work->room!=0x102||actor->size<=0x1650||
    actor->original_vtable214!=0x4479c8)return 0;
 const unsigned stamp=w(actor->bytes+0x640)&0xffffu,appearance=actor->bytes[0x1650];
 const uint32_t kind=w(actor->bytes+0x564);
 if(!appearance)return stamp==7&&kind==0x200;
 static const uint16_t dependency[]={0x202,0x200,0x203};
 return appearance==6&&stamp>=0x20&&stamp<=0x22&&kind==dependency[stamp-0x20];
}

int gh_effect_stamp19_death_part_is_bound(const gh_effect_emitter_view *v)
{
 return v&&v->actor&&v->actor->bytes&&v->actor->size>=0x644&&v->selector==v->actor&&
        v->model&&v->model->root.bytes==v->actor->bytes&&v->part==v->model->parts&&v->part&&
        v->record.bytes&&v->record.size==0x2f0&&v->payload&&v->payload_size==0x260&&w(v->payload+0x18c)==0x00020058u&&
        !v->payload[0x1ec]&&(w(v->actor->bytes+0x2fc)>>16)==0x202&&
        ((w(v->actor->bytes+0x564)==0x203&&(w(v->actor->bytes+0x640)&0xffff)==19)||
         (w(v->actor->bytes+0x564)==0x200&&(w(v->actor->bytes+0x640)&0xffff)==20)||
         (v->model->work&&v->model->work->room==0x100&&stamp8_attack_actor(v->actor))||
         r102_landing_emitter(v)||
         (w(v->actor->bytes+0x564)==0x20a&&!(w(v->actor->bytes+0x640)&0xffff)&&
          v->actor->size>0x1650&&v->actor->bytes[0x1650]==3))&&
        !v->record.bytes[0xbe]&&!v->record.bytes[0x128]&&!v->record.bytes[0x136];
}
gh_effect_status gh_effect_create_emitter_at(const gh_game_effects*s,unsigned slot,gh_effect_emitter_view*out){
 if(!s||!out||slot>=256||!s->native||s->native->owner!=s)return GH_EFFECT_ARGUMENT;
 const struct gh_effect_native*n=s->native;const emitter_metadata*m=&n->slots[slot];uint32_t mask=0x80000000u>>(slot&31);if(!m->associated||m->quiesced||!(w(n->emitters+0x26000+(slot>>5)*4)&mask))return GH_EFFECT_UNSUPPORTED;
 const gh_actor_part*part=NULL;if(m->model){if(gh_actor_model_part_at(m->model,m->part_index,&part)||part->handle!=m->part_handle)return GH_EFFECT_UNSUPPORTED;}
 *out=(gh_effect_emitter_view){.record={m->record,0x2f0},.actor=m->actor,.selector=m->selector,.model=m->model,.part=part,.serial=m->serial,.initialized=m->initialized,.payload=n->emitters+(size_t)slot*0x260,.payload_size=0x260,.first_esp_child=m->first_esp_child};return GH_EFFECT_OK;
}

static uint32_t esp_vtable(unsigned type){
 switch(type){
 case 0: return 0x44fad0u;
 case 6: return 0x44e6d8u;
 case 7: return 0x44e770u;
 case 0x1f: return 0x44f448u;
 case 8: return 0x44e820u;
 case 9: return 0x44e8c8u;
 case 0x12: return 0x44eb70u;
 case 0x13: return 0x44ec68u;
 case 0x14: return GH_ESP14_VTABLE;
 case 0x18: return 0x44f078u;
 case 0x1a: return 0x44f270u;
 case 0x20: return 0x44f4f8u;
 default: return 0;
 }
}

static void construct_esp_base(uint8_t*p,unsigned type,uint32_t vtable){
 /* 2FFBD0 ->2E81D8 ->2FBCC8 ->2FBD98 ->2B5C20(NULL). */
 for(unsigned i=0;i<4;i++)ww(p+0x20+i*4,0x3f800000);vf0(p+0x30);ww(p+0x34,0x80000000u);vf0(p+0x40);vf0(p+0xa0);identity(p+0x50);identity(p+0xb0);
 const unsigned unit[]={0x188,0x18c,0x190,0x194,0x1f8,0x1fc,0x200,0x204,0x210,0x240};for(unsigned i=0;i<sizeof(unit)/sizeof(unit[0]);i++)ww(p+unit[i],0x3f800000);
 half(p+0x238,0xffff);p[0x246]=p[0x23e]=p[0x23f]=p[0x245]=1;vf0(p+0x280);vf0(p+0x290);ww(p+0x11c,0x06000000);ww(p+0xf0,vtable);p[0x102]=(uint8_t)type;
}
int gh_effect_event_part_is_bound(gh_effect_view row,const gh_title_player_view *actor,
 const gh_title_player_view *selector){
 if(!row.bytes||row.size!=0x2f0||!actor||selector||!actor->bytes||actor->size<0x300||
    actor->original_vtable214!=0x44b528u||row.bytes[0x136])return 0;
 unsigned kind=actor->bytes[0x2fe]|(unsigned)actor->bytes[0x2ff]<<8;
 if(w(row.bytes+0x2a0)!=(uint32_t)kind<<16)return 0;
 const uint32_t flags=w(row.bytes+4);
 const unsigned part=row.bytes[0xbe];
 /* EV02 cut 1 bank 6 entry 2 attaches to EL50 part zero (303A30).
  * Its ordinary attachment uses the selected part's entire basis (302BB4). */
 if(row.bytes[0x133]==0){
  /* EV1B bank 6 EST3/4/6/7/8/9/10 retain their original part basis. */
  if(kind==0x640||kind==0x2a2||kind==0x3bc){
   const uint32_t header=w(row.bytes),texture=w(row.bytes+0xbc),timing=w(row.bytes+0xc0);
   if(flags!=0x00800000u||w(row.bytes+0x130)!=1||w(row.bytes+0x134))return 0;
   if(kind==0x640){
    if(header==0x1e3e0010u)return texture==0xff040001u&&timing==0x00230022u;
    return header==0x1efe8010u&&(texture==0x00070401u||texture==0x000a0401u)&&
       (timing==0x002c0018u||timing==0x00140000u||!timing);
   }
   if(kind==0x2a2)return header==0x1e3e8010u&&texture==0x00030804u&&
       (timing==0x000b000au||timing==0x001b001au||timing==0x00060005u);
   return (header==0x1e3e0010u||header==0x1e3e8010u)&&texture==0xff000001u&&
       timing==0x00290028u;
  }
  if(kind==0x650)
   return flags==0x00800000u&&!part&&
      w(row.bytes)==0x1efe8010u&&w(row.bytes+0xbc)==0xff001203u&&
      w(row.bytes+0x130)==1&&w(row.bytes+0x134)==0;
  /* EV02 cut 6 bank 6 entry 7 uses EL0A root orientation and part 10
   * translation (302B58). The two orientations share the same ESP00. */
  if(kind==0x60a)
   return flags==0x00c00000u&&part==10&&
      w(row.bytes)==0x1e3e8010u&&w(row.bytes+0xbc)==0x000a0004u&&
      w(row.bytes+0x130)==1&&w(row.bytes+0x134)==0;
  /* EV04 bank6 uses EL10 part29 with 302BB4's full part basis or
   * 302B58's root basis/part position. EL11 uses its own part4. */
  if((kind==0x610&&part==29)||(kind==0x611&&part==4)){
   const uint32_t header=w(row.bytes),texture=w(row.bytes+0xbc),child=w(row.bytes+0x130);
   if(w(row.bytes+0x134))return 0;
   if(kind==0x611)return flags==0x00800000u&&(header==0x1eee0010u||header==0x1eee8010u)&&
      texture==0x00040001u&&child==2;
   if(flags==0x00c00000u)return header==0x1eee0010u&&child==2&&
      (texture==0x031d0201u||texture==0x031d0401u);
   if(flags!=0x00800000u)return 0;
   if(header==0x1e308010u)return child==1&&texture==0xff1d0001u;
   if(header==0x1e3e8010u)return child==1&&
      (texture==0xff1d0001u||texture==0x001d0003u||texture==0x001d0002u);
   return header==0x1eee0010u&&child==2&&
      (texture==0x001d0501u||texture==0x031d0201u||texture==0x021d0501u||texture==0x031d0301u);
  }
  return kind==0x600&&flags==0x00802000u&&part==9;
 }
 /* EV04 bank6 entry5 rows5/6 use EL10 part29 and the native
  * original ESP09 trail child. Resource identity is pinned at Init. */
 if(kind==0x610&&part==29&&row.bytes[0x133]==9){
  const uint32_t timing=w(row.bytes+0xc0);
  return flags==0x00800000u&&w(row.bytes)==0x1e3e8010u&&
     w(row.bytes+0xbc)==0xff1d0001u&&w(row.bytes+0x130)==0x09000001u&&
     !w(row.bytes+0x134)&&(timing==0x000a0009u||timing==0x00070006u);
 }
 /* EV02 cut12 EL59 trail, bank6 entry13 row1. */
 if(row.bytes[0x133]==0x20)
  return kind==0x659&&flags==0x00800000u&&!part&&
     w(row.bytes)==0x1e0e0010u&&w(row.bytes+0xbc)==0xff000001u&&
     w(row.bytes+0x130)==0x20000001u&&w(row.bytes+0x134)==0;
 /* EV02 cut18 binds EL05's part4 warp and part0 ground sprites. */
 if(kind==0x605&&flags==0x00800000u){
  if(row.bytes[0x133]==7)
   return part==4&&w(row.bytes)==0x1e3e0010u&&w(row.bytes+0xbc)==0x00040001u;
  if(row.bytes[0x133]==0x1a)
   return !part&&w(row.bytes)==0x1efe8010u&&row.bytes[0x28e]==2&&row.bytes[0x12e]==132;
 }
 if(row.bytes[0x133]!=0x1a)return 0;
 if(kind==0x600)return flags==0x00c00000u&&(part==0||part==3);
 return kind==0x60a&&flags==0x00800000u&&part==0;
}
/* The original stage bank supplies EM20's initializer, rise and ranged effects. */
static int devil_effect_row(const gh_game_effects *owner,gh_effect_view row,uint32_t tag)
{
 const uint32_t index=tag>>16;
 const unsigned expected=index==3?8:index==0x3f?23:index==0x40?4:devil_ranged_effect_rows(index);
 if((tag&0xffffu)!=0x58||!expected||!row.bytes||row.size!=0x2f0||
    row.bytes[0x128]||row.bytes[0x136])return 0;
 gh_effect_view est;
 if(gh_effect_est_at_00306030(owner,0x58,index,&est)||!est.bytes||est.size<16)return 0;
 const unsigned count=w(est.bytes+4);
 if(count!=expected||count>(est.size-16)/0x2f0)return 0;
 for(unsigned i=0;i<count;i++)if(row.bytes==est.bytes+16+i*0x2f0)return 1;
 return 0;
}

static int devil_effect_actor(const gh_title_player_view *actor,const gh_title_player_view *selector)
{
 return actor&&actor==selector&&actor->bytes&&actor->size>=0x568&&
    actor->original_vtable214==0x4479c8&&(w(actor->bytes+0x2fc)>>16)==0x220&&
    w(actor->bytes+0x564)==0x220;
}

static int devil_effect_binding(uint32_t tag,const gh_title_player_view *actor,
 const gh_title_player_view *selector)
{
 if(tag==0x00400058u)return actor==selector&&devil_projectile_effect_actor(actor);
 return devil_effect_actor(actor,selector);
}

int gh_effect_devil_emitter_is_bound(const gh_game_effects *owner,const gh_effect_emitter_view *v)
{
 if(!v||!v->payload||v->payload_size!=0x260||!v->initialized)return 0;
 const uint32_t tag=w(v->payload+0x18c);
 if(!devil_effect_binding(tag,v->actor,v->selector)||!v->model||v->model->root.bytes!=v->actor->bytes||
    !devil_effect_row(owner,v->record,tag))return 0;
 const unsigned part=v->record.bytes[0xbe];
 return part<v->model->part_count&&v->part==v->model->parts+part;
}

int gh_effect_devil_esp_is_bound(const gh_game_effects *owner,const gh_effect_esp_view *v)
{
 if(!v||!v->payload||v->payload_size!=0x400)return 0;
 const uint32_t tag=w(v->payload+0x23a);
 return devil_effect_binding(tag,v->actor,v->selector)&&
    devil_effect_row(owner,v->record,tag)&&v->record.bytes[0x133]==v->type;
}

static int emad_effect_binding(const gh_game_effects *owner,gh_effect_view row,
 const gh_title_player_view *actor,const gh_title_player_view *selector)
{
 if(!actor||actor!=selector||!actor->bytes||actor->size<0x568||
    actor->original_vtable214!=0x44a528||(w(actor->bytes+0x2fc)>>16)!=0x2ad||
    w(actor->bytes+0x564)!=0x2ad||!row.bytes||row.size!=0x2f0||
    w(row.bytes+4)!=0x00800000u||row.bytes[0xbe]||row.bytes[0x128]||
    row.bytes[0x133]||row.bytes[0x136])return 0;
 gh_effect_view est;
 if(gh_effect_est_at_00306030(owner,0xd7,1,&est)||!est.bytes||est.size<16+3*0x2f0||
    w(est.bytes+4)!=3)return 0;
 for(unsigned i=0;i<3;i++)
    if(row.bytes==est.bytes+16+i*0x2f0)return 1;
 return 0;
}

int gh_effect_emad_emitter_is_bound(const gh_game_effects *owner,const gh_effect_emitter_view *v)
{
 return v&&v->initialized&&v->payload&&v->payload_size==0x260&&
    w(v->payload+0x18c)==0x000100d7u&&w(v->payload+0x110)==UINT32_MAX&&
    emad_effect_binding(owner,v->record,v->actor,v->selector)&&
    v->model&&v->model->root.bytes==v->actor->bytes&&v->model->part_count&&
    v->part==v->model->parts;
}

int gh_effect_emad_esp_is_bound(const gh_game_effects *owner,const gh_effect_esp_view *v)
{
 return v&&v->type==0&&emad_effect_binding(owner,v->record,v->actor,v->selector);
}

static int sprite_binding(gh_effect_view row,const gh_title_player_view *actor,
 const gh_title_player_view *selector){
 return gh_esp14_nested_sprite_is_bound(row,actor,selector)||gh_effect_event_part_is_bound(row,actor,selector)||
        gh_effect_r101_door_break_part_is_bound(row,actor,selector);
}

static int authenticated_esp(const gh_game_effects *e,const gh_effect_esp_view *v,int player_attachment)
{
 if(!e||!e->native||e->native->owner!=e||!v||!v->payload||
    (v->type!=0&&v->type!=9&&v->type!=0x1a&&!(player_attachment&&(v->type==7||v->type==0x12||v->type==0x14))))return 0;
 /* At allocation the source emitter and immutable EST row were authenticated.
  * Retain this fact independently of emitter lifetime/slot reuse. */
 const uintptr_t base=(uintptr_t)e->native->esp, current=(uintptr_t)v->payload;
 if(current<base||current-base>=0x100000||(current-base)%0x400)return 0;
 const esp_metadata *m=e->native->esps+(current-base)/0x400;
 return m->associated&&(player_attachment?m->player_attachment:m->knockdown)&&m->serial==v->serial&&m->record.bytes==v->record.bytes&&
    m->actor==v->actor&&m->selector==v->selector&&m->type==v->type;
}

int gh_effect_knockdown_esp_is_bound(const gh_game_effects *e,const gh_effect_esp_view *v)
{
 return authenticated_esp(e,v,0);
}

int gh_effect_player_attachment_esp_is_bound(const gh_game_effects *e,const gh_effect_esp_view *v)
{
 return authenticated_esp(e,v,1);
}

int gh_effect_em00_attack_esp_is_bound(const gh_game_effects *owner,const gh_effect_esp_view *v)
{
 if(!v||!v->payload||v->payload_size!=0x400||(!em00_family_attack_actor(v->actor)&&!em0a_attack_actor(v->actor))||v->actor!=v->selector||
    !v->record.bytes||v->record.size!=0x2f0||(v->type!=9&&v->type!=0x12&&v->type!=0x1a&&v->type!=0x20))return 0;
 gh_effect_emitter_view emitter;
 if(!gh_effect_create_emitter_at(owner,v->emitter_slot,&emitter)&&emitter.serial==v->emitter_serial)
    return emitter.actor==v->actor&&emitter.selector==v->selector&&
       emitter.record.bytes==v->record.bytes&&gh_effect_em00_attack_emitter_is_bound(&emitter);
 const unsigned index=v->payload[0x23c]|((unsigned)v->payload[0x23d]<<8);
 if(index==0x2b&&stamp8_attack_actor(v->actor)){
  gh_effect_view est;
  if(gh_effect_est_at_00306030(owner,0x58,index,&est)||!est.bytes||
     est.size<16+6*0x2f0||w(est.bytes+4)!=6)return 0;
  for(unsigned i=0;i<6;i++)
   if(v->record.bytes==est.bytes+16+i*0x2f0)
    return v->record.bytes[0x133]==v->type&&stamp8_attack_effect_row(v->record.bytes);
  return 0;
 }
 if(index==0x1e&&em0a_attack_actor(v->actor)){
  gh_effect_view est;
  if(gh_effect_est_at_00306030(owner,0x58,index,&est)||!est.bytes||
     est.size<16+2*0x2f0||w(est.bytes+4)!=2)return 0;
  return em0a_attack_effect_row(v->record.bytes)&&v->record.bytes[0x133]==v->type&&
         (v->record.bytes==est.bytes+16||v->record.bytes==est.bytes+16+0x2f0);
 }
 if(!em00_family_attack_actor(v->actor)||(index!=0x2c&&index!=0x5a))return 0;
 gh_effect_view est;
 if(gh_effect_est_at_00306030(owner,0x58,index,&est)||!est.bytes||est.size<16+4*0x2f0)return 0;
 for(unsigned i=0;i<4;i++){
    const uint8_t *row=est.bytes+16+i*0x2f0;
    if(row==v->record.bytes)return row[0x133]==v->type;
 }
 return 0;
}

static int stamp19_attack_esp_bound(const gh_game_effects *owner,const gh_effect_esp_view *v)
{
 if(!v||!v->payload||v->payload_size!=0x400||!em02_em03_attack_actor(v->actor)||v->actor!=v->selector||
    v->actor->original_vtable214!=0x4479c8||
    !v->record.bytes||v->record.size!=0x2f0||(v->type!=9&&v->type!=0x12))
  return 0;
 gh_effect_emitter_view emitter;
 if(!gh_effect_create_emitter_at(owner,v->emitter_slot,&emitter)&&emitter.serial==v->emitter_serial)
  return emitter.actor==v->actor&&emitter.selector==v->selector&&
      emitter.record.bytes==v->record.bytes&&gh_effect_stamp19_attack_part_is_bound(&emitter);
 if(w(v->payload+0x23a)!=0x002d0058u)return 0;
 gh_effect_view est;
 if(gh_effect_est_at_00306030(owner,0x58,0x2d,&est)||!est.bytes||est.size<16+4*0x2f0||
    w(est.bytes+4)!=4)return 0;
 for(unsigned i=0;i<4;i++){
  const uint8_t *row=est.bytes+16+i*0x2f0;
  if(row==v->record.bytes)return row[0x133]==v->type&&row[0xbe]==15;
 }
 return 0;
}

static int r101_weapon_esp_bound(const gh_game_effects *owner,const gh_effect_esp_view *v)
{
 if(!v||!v->payload||v->payload_size!=0x400||!v->record.bytes||v->record.size!=0x2f0||
    (v->type!=9&&v->type!=0x12&&v->type!=0x18&&v->type!=0x1a)||
    v->type!=v->record.bytes[0x133]||!weapon_swing_room(v->actor,v->selector))
  return 0;
 gh_effect_emitter_view emitter;
 if(!gh_effect_create_emitter_at(owner,v->emitter_slot,&emitter)&&emitter.serial==v->emitter_serial)
  return emitter.actor==v->actor&&emitter.selector==v->selector&&
         emitter.record.bytes==v->record.bytes&&gh_effect_r101_weapon_emitter_is_bound(&emitter);
 const uint32_t profile=w(v->payload+0x23a);
 const unsigned count=weapon_est_count(weapon_swing_room(v->actor,v->selector),profile>>16);
 if((profile&0xffffu)!=0x58||!count)return 0;
 gh_effect_view est;
 if(gh_effect_est_at_00306030(owner,0x58,profile>>16,&est)||!est.bytes||
    est.size<16+count*0x2f0||w(est.bytes+4)!=count)return 0;
 for(unsigned i=0;i<count;i++){
  const uint8_t *row=est.bytes+16+i*0x2f0;
  if(row==v->record.bytes)return r101_weapon_est_row(profile>>16,row,i);
 }
 return 0;
}

int gh_effect_attack_esp_is_bound(const gh_game_effects *owner,const gh_effect_esp_view *v)
{
 return gh_effect_em00_attack_esp_is_bound(owner,v)||stamp19_attack_esp_bound(owner,v)||
        r101_weapon_esp_bound(owner,v);
}

int gh_effect_r003_esp08_row(gh_effect_view row)
{
 const uint8_t *r=row.bytes;
 return r&&row.size==0x2f0&&w(r)==0x1e3e8010u&&w(r+4)==0x00a00000u&&
    r[0xbe]==0xfcu&&r[0x133]==8&&r[0x18c]==0&&r[0x18f]==0;
}

int gh_effect_esp08_grid_row(gh_effect_view row)
{
 const uint8_t *r=row.bytes;
 return r&&row.size==0x2f0&&w(r)==0x1e3e8010u&&w(r+4)==0x00800000u&&
    r[0xbe]==0xfcu&&r[0x133]==8&&w(r+0x18c)==0x00640002u&&
    r[0x12e]==50&&!r[0x28c]&&!r[0x28d]&&r[0x290]==2;
}

int gh_effect_esp08_screen_row(gh_effect_view row)
{
 return gh_effect_r003_esp08_row(row)||gh_effect_esp08_grid_row(row);
}

static int ev1a_esp08_emitter_bound(const gh_game_effects *e,const gh_effect_emitter_view *source)
{
 const uint32_t tag=w(source->payload+0x18c);
 const unsigned index=tag>>16;
 /* EV1A uses one original parentless screen row across these eight cuts. */
 const unsigned cut_rows=(1u<<2)|(1u<<3)|(1u<<5)|(1u<<6)|(1u<<7)|(1u<<8)|(1u<<10)|(1u<<11);
 if((tag&0xffffu)!=6||index>11||!(cut_rows&(1u<<index))||
    w(source->payload+0x110)!=index+0x24ffu||!e->event_registered_41c||
    !gh_effect_r003_esp08_row(source->record)||source->record.bytes[0x12e]!=2||
    source->record.bytes[0xbf])return 0;
 gh_effect_view est;
 return !gh_effect_est_at_00306030(e,6,index,&est)&&est.bytes&&
    est.size>=16+2*0x2f0&&w(est.bytes+4)>=2&&source->record.bytes==est.bytes+16+0x2f0;
}

gh_effect_status gh_effect_create_esp_allocate_002faa88(const gh_effect_release_bindings*b,unsigned source_slot,uint32_t source_serial,unsigned*slot,uint32_t*serial,int*allocated,uint32_t*f){
 if(!b||!slot||!serial||!allocated||!f)return GH_EFFECT_ARGUMENT;*f=0;*allocated=0;*slot=0;*serial=0;
 gh_game_effects*e=b->effects;if(!e||!e->native||e->native->owner!=e||e->native->esp!=b->esp||e->native->emitters!=b->emitters||b->esp_bytes<0x100180||b->emitter_bytes<0x26060)return GH_EFFECT_ARGUMENT;
 gh_effect_emitter_view source;gh_effect_status status=gh_effect_create_emitter_at(e,source_slot,&source);if(status||source.serial!=source_serial||!source.initialized)return GH_EFFECT_ARGUMENT;
 const int attack=(source.record.bytes[0x133]==9||source.record.bytes[0x133]==0x12||source.record.bytes[0x133]==0x1a||source.record.bytes[0x133]==0x20)&&
    gh_effect_em00_attack_emitter_is_bound(&source);
 const uint8_t *r=source.record.bytes;
 const int knock=(r[0x133]==0||r[0x133]==9||
    (r[0x133]==0x1a&&(r[0xbe]==25||r[0xbe]==21)&&w(source.payload+0x18c)==0x002f0058u))&&
    gh_effect_knockdown_emitter_is_bound(&source);
 const int death_ground=r[0x133]==0x1a&&gh_effect_stamp19_death_part_is_bound(&source);
 const int player_attachment=gh_effect_player_attachment_emitter_is_bound(&source);
 const int stamp19=gh_effect_stamp19_attack_part_is_bound(&source);
 const int weapon=gh_effect_r101_weapon_emitter_is_bound(&source);
 const int emad=gh_effect_emad_emitter_is_bound(e,&source);
 const int enemy_light=gh_effect_enemy_light_emitter_is_bound(e,&source);
 const int event_part=enemy_light||emad||stamp19||weapon||player_attachment||attack||knock||death_ground||gh_effect_event_part_is_bound(source.record,source.actor,source.selector)||
    gh_effect_r101_door_break_part_is_bound(source.record,source.actor,source.selector);
 const int devil=gh_effect_devil_emitter_is_bound(e,&source);
 const int bound=devil||enemy_light||emad||stamp19||weapon||player_attachment||attack||knock||death_ground||sprite_binding(source.record,source.actor,source.selector);
 if((!bound&&(source.actor||source.selector))||((event_part||devil)?(!source.model||!source.part):(source.model||source.part))){*f=0x2faa88;return GH_EFFECT_UNSUPPORTED;}
 unsigned i;for(i=0;i<1024;i++)if(!(w(b->esp+0x100000+(i>>5)*4)&(UINT32_C(0x80000000)>>(i&31))))break;if(i==1024)return GH_EFFECT_OK;
 unsigned type=source.record.bytes[0x133];uint32_t vtable=esp_vtable(type);
 if(!vtable||(type==0x1f&&!devil)){*f=0x2faca8;return GH_EFFECT_UNSUPPORTED;}
 if(type==8){
  const uint32_t tag=w(source.payload+0x18c);
  const int grid=gh_effect_esp08_grid_row(source.record);
  const int screen_bound=grid ? tag==0x00090000u&&w(source.payload+0x110)==1 :
     tag==3||ev1a_esp08_emitter_bound(e,&source);
  if(!gh_effect_esp08_screen_row(source.record)||!screen_bound||source.actor||source.selector){
   *f=0x2faca8;
   return GH_EFFECT_UNSUPPORTED;
  }
 }
 if(e->native->serial==UINT32_MAX){*f=0x2faa88;return GH_EFFECT_UNSUPPORTED;}
 uint8_t*p=b->esp+(size_t)i*0x400;memset(p,0,0x400);construct_esp_base(p,type,vtable);if(type==9||type==0x20||type==0x1f)ww(p+0x2d4,0x3f800000);if(type==0x1f)ww(p+0x30c,0x3f800000);if(type==0x14)gh_esp14_construct_tail_002ffd88(p);uint32_t id=++e->native->serial;esp_metadata*m=&e->native->esps[i];*m=(esp_metadata){.record=source.record,.actor=source.actor,.selector=source.selector,.serial=id,.emitter_serial=source_serial,.emitter_slot=source_slot,.associated=1,.type=(uint8_t)type,.parent_emitter=-1,.previous=-1,.next=-1,.knockdown=(uint8_t)(knock||death_ground),.player_attachment=(uint8_t)player_attachment,.target120_known_null=(uint8_t)((bound||(!source.actor&&!source.selector))&&!w(source.payload+0x218))};unsigned off=(i>>5)*4;uint32_t mask=UINT32_C(0x80000000)>>(i&31);ww(b->esp+0x100000+off,w(b->esp+0x100000+off)|mask);ww(b->esp+0x100080+off,w(b->esp+0x100080+off)&~mask);ww(b->esp+0x100100+off,w(b->esp+0x100100+off)&~mask);*slot=i;*serial=id;*allocated=1;return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_at(const gh_game_effects*e,unsigned slot,gh_effect_esp_view*out){
 if(!e||!out||slot>=1024||!e->native||e->native->owner!=e)return GH_EFFECT_ARGUMENT;const esp_metadata*m=&e->native->esps[slot];if(!m->associated||m->quiesced||!(w(e->native->esp+0x100000+(slot>>5)*4)&(UINT32_C(0x80000000)>>(slot&31))))return GH_EFFECT_UNSUPPORTED;
 *out=(gh_effect_esp_view){.record=m->record,.texture=m->texture,.secondary_texture=m->secondary_texture,.actor=m->actor,.selector=m->selector,.payload=e->native->esp+(size_t)slot*0x400,.payload_size=0x400,.serial=m->serial,.emitter_serial=m->emitter_serial,.emitter_slot=m->emitter_slot,.initialized=m->initialized,.type=m->type,.globals=e->native->frame_globals,.scene_actors=e->native->frame_actors,.parent_emitter=m->parent_emitter,.previous=m->previous,.next=m->next,.parent_serial=m->parent_serial,.target120_known_null=m->target120_known_null,.retired_actor=m->retired_actor};return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_bind_textures(gh_game_effects*e,unsigned slot,uint32_t serial,gh_effect_view texture,gh_effect_view secondary){gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial)return GH_EFFECT_ARGUMENT;e->native->esps[slot].texture=texture;e->native->esps[slot].secondary_texture=secondary;return GH_EFFECT_OK;}

static gh_effect_status abort_esp_native(gh_game_effects*e,unsigned slot,uint32_t serial,int ready_allowed){gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||(!ready_allowed&&v.initialized))return GH_EFFECT_ARGUMENT;if((v.actor||v.selector)&&!sprite_binding(v.record,v.actor,v.selector)&&!gh_effect_attack_esp_is_bound(e,&v)&&!gh_effect_knockdown_esp_is_bound(e,&v)&&!gh_effect_player_attachment_esp_is_bound(e,&v)&&!gh_effect_devil_esp_is_bound(e,&v)&&!gh_effect_enemy_light_esp_is_bound(e,&v)&&!gh_effect_emad_esp_is_bound(e,&v))return GH_EFFECT_ARGUMENT;uint32_t vtable=esp_vtable(v.type);if(!vtable||w(v.payload+0xf0)!=vtable)return GH_EFFECT_UNSUPPORTED;if(v.parent_emitter>=0){status=gh_effect_create_esp_unlink_002b5ec0(e,slot,serial);if(status)return status;}if(release_trail(e->native->esps+slot))return GH_EFFECT_ARGUMENT;unsigned off=(slot>>5)*4;uint32_t keep=~(UINT32_C(0x80000000)>>(slot&31));for(unsigned bank=0;bank<3;bank++)ww(e->native->esp+0x100000+bank*0x80+off,w(e->native->esp+0x100000+bank*0x80+off)&keep);e->native->esps[slot]=(esp_metadata){0};return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_abort_partial(gh_game_effects*e,unsigned slot,uint32_t serial){return abort_esp_native(e,slot,serial,0);}

gh_effect_status gh_effect_create_bind_frame_globals(gh_game_effects*e,struct gh_effects_frame_globals*g){if(!e||!e->native||e->native->owner!=e||!g||!g->initialized)return GH_EFFECT_ARGUMENT;if(e->native->frame_globals&&e->native->frame_globals!=g)return GH_EFFECT_ARGUMENT;e->native->frame_globals=g;return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_commit_base(gh_game_effects*e,unsigned slot,uint32_t serial){
 gh_effect_esp_view view;
 gh_effect_status status=gh_effect_create_esp_at(e,slot,&view);
 if(status||view.serial!=serial||view.type||view.initialized||
    w(view.payload+0xf0)!=esp_vtable(0)||!view.globals||!view.globals->initialized||
    (view.parent_emitter>=0&&!(w(view.payload+0x11c)&0x01000000u)))return GH_EFFECT_ARGUMENT;
 if((view.actor||view.selector)&&!sprite_binding(view.record,view.actor,view.selector)&&!gh_effect_attack_esp_is_bound(e,&view)&&!gh_effect_knockdown_esp_is_bound(e,&view)&&!gh_effect_player_attachment_esp_is_bound(e,&view)&&!gh_effect_devil_esp_is_bound(e,&view)&&
    !gh_effect_enemy_light_esp_is_bound(e,&view)&&!gh_effect_emad_esp_is_bound(e,&view))
    return GH_EFFECT_UNSUPPORTED;
 e->native->esps[slot].initialized=1;
 return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_commit_type6(gh_game_effects*e,unsigned slot,uint32_t serial){gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||v.type!=6||v.initialized||w(v.payload+0xf0)!=0x44e6d8u||!v.globals||!v.globals->initialized||v.actor||v.selector)return GH_EFFECT_ARGUMENT;e->native->esps[slot].initialized=1;return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_commit_8(gh_game_effects *e,unsigned slot,uint32_t serial)
{
 gh_effect_esp_view view;
 gh_effect_status status=gh_effect_create_esp_at(e,slot,&view);
 if(status||view.serial!=serial||view.type!=8||view.initialized||
    w(view.payload+0xf0)!=esp_vtable(8)||!view.globals||!view.globals->initialized||
    view.actor||view.selector||view.parent_emitter!=-1||!gh_effect_esp08_screen_row(view.record))
  return GH_EFFECT_ARGUMENT;
 e->native->esps[slot].initialized=1;
 return GH_EFFECT_OK;
}
int gh_effect_esp_type7_record_bound(const gh_game_effects *e,const gh_effect_esp_view *v)
{
 if(!v||!v->payload||v->payload_size!=0x400||v->type!=7||
    !v->record.bytes||v->record.size!=0x2f0||v->record.bytes[0x133]!=7)
    return 0;
 const uint32_t tag=w(v->payload+0x23a);
 if(tag==0x00090000u){
  gh_effect_view est;
  return !v->actor&&!v->selector&&w(v->payload+0x10c)==1&&v->record.bytes[0xbe]==0xfcu&&
    !gh_effect_est_at_00306030(e,0,9,&est)&&est.bytes&&est.size>=16+4*0x2f0&&
    w(est.bytes+4)==4&&(v->record.bytes==est.bytes+16+0x2f0||v->record.bytes==est.bytes+16+2*0x2f0);
 }
 if(tag==0x00150006u){
  gh_effect_view est;
  const uint8_t *r=v->record.bytes;
  /* EV04 cut20's authored screen distortion is row3 of ten. */
  return e&&e->event_registered_41c&&!v->actor&&!v->selector&&
     w(r)==0x1e3e0010u&&w(r+4)==0x00802000u&&w(r+0xbc)==0xffff0001u&&
     w(r+0xc0)==0x00040003u&&w(r+0x130)==0x07000001u&&!w(r+0x134)&&
     !gh_effect_est_at_00306030(e,6,21,&est)&&est.size>=16+10*0x2f0&&
     w(est.bytes+4)==10&&r==est.bytes+16+3*0x2f0;
 }
 if(tag==0x00060006u||tag==0x00070006u||tag==0x00080006u){
  gh_effect_view est;
  const unsigned index=tag>>16;
  const unsigned row=index==6?0:index==7?2:1;
  const unsigned count=index==6?5:index==7?3:2;
  const uint8_t *r=v->record.bytes;
  /* EV1B distortion is parentless beside the attached ESP00 rows. */
  return e&&e->event_registered_41c&&!v->actor&&!v->selector&&
     w(r)==0x1e3e0010u&&w(r+4)==0x00800000u&&w(r+0xbc)==0x00ff0001u&&
     w(r+0xc0)==(index==6?0x002d002cu:0x00010000u)&&
     w(r+0x130)==0x07000001u&&!w(r+0x134)&&
     !gh_effect_est_at_00306030(e,6,index,&est)&&est.size>=16+count*0x2f0&&
     w(est.bytes+4)==count&&r==est.bytes+16+row*0x2f0;
 }
 if(tag==0x003f0058u||((tag&0xffffu)==0x58&&devil_ranged_effect_rows(tag>>16)))
  return gh_effect_devil_esp_is_bound(e,v);
 if(tag==0x00420000u||tag==0x00270058u||tag==0x00590058u||tag==0x005d0058u||tag==0x00170000u)
  return gh_effect_player_attachment_esp_is_bound(e,v);
 if(tag==0x00130006u){
  if(!gh_effect_event_part_is_bound(v->record,v->actor,v->selector))return 0;
 }else if(v->actor||v->selector)return 0;
 switch(tag){
 case 0x00030000u: /* Player impact. */
 case 0x00120058u: /* Positioned amplified enemy impact. */
 case 0x000d0006u: /* EV02 cuts 12, 13, 15, 16, 17 and 25. */
 case 0x000e0006u:
 case 0x00100006u:
 case 0x00110006u:
 case 0x00120006u:
 case 0x00130006u:
 case 0x001a0006u:
  break;
 default:
  return 0;
 }
 gh_effect_view est;
 return !gh_effect_est_at_00306030(e,tag&0xffffu,tag>>16,&est)&&
    est.bytes&&est.size>=0x300&&v->record.bytes==est.bytes+16;
}

gh_effect_status gh_effect_create_esp_commit_type7(gh_game_effects *e,unsigned slot,uint32_t serial)
{
 gh_effect_esp_view v;
 if(gh_effect_create_esp_at(e,slot,&v)||v.serial!=serial||v.initialized||
    w(v.payload+0xf0)!=esp_vtable(7)||!v.globals||!v.globals->initialized||
    !gh_effect_esp_type7_record_bound(e,&v))
    return GH_EFFECT_ARGUMENT;
 e->native->esps[slot].initialized=1;
 return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_commit_14(gh_game_effects *e,unsigned slot,uint32_t serial){
 gh_effect_esp_view view;
 gh_effect_status status=gh_effect_create_esp_at(e,slot,&view);
 if(status||view.serial!=serial||view.type!=0x14||view.initialized||
    w(view.payload+0xf0)!=GH_ESP14_VTABLE||!view.globals||!view.globals->initialized||
    ((view.actor||view.selector)&&!gh_effect_player_attachment_esp_is_bound(e,&view)))return GH_EFFECT_ARGUMENT;
 e->native->esps[slot].initialized=1;
 return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_commit_12(gh_game_effects*e,unsigned slot,uint32_t serial){
 gh_effect_esp_view view;
 gh_effect_status status=gh_effect_create_esp_at(e,slot,&view);
 if(status||view.serial!=serial||view.type!=0x12||view.initialized||
    w(view.payload+0xf0)!=esp_vtable(0x12)||!view.globals||!view.globals->initialized||
    ((view.actor||view.selector||view.parent_emitter!=-1)&&
     !gh_effect_attack_esp_is_bound(e,&view)&&!gh_effect_knockdown_esp_is_bound(e,&view)&&!gh_effect_player_attachment_esp_is_bound(e,&view)&&!gh_effect_devil_esp_is_bound(e,&view)))return GH_EFFECT_ARGUMENT;
 e->native->esps[slot].initialized=1;
 return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_abort_native(gh_game_effects*e,unsigned slot,uint32_t serial){return abort_esp_native(e,slot,serial,1);}

gh_effect_status gh_effect_create_bind_frame_actors(gh_game_effects*e,gh_game_scene_tables*actors){if(!e||!e->native||e->native->owner!=e||!actors)return GH_EFFECT_ARGUMENT;if(e->native->frame_actors&&e->native->frame_actors!=actors)return GH_EFFECT_ARGUMENT;e->native->frame_actors=actors;return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_commit_13(gh_game_effects*e,unsigned slot,uint32_t serial){
 gh_effect_esp_view view;
 gh_effect_status status=gh_effect_create_esp_at(e,slot,&view);
 if(status||view.serial!=serial||view.type!=0x13||view.initialized||
    w(view.payload+0xf0)!=esp_vtable(0x13)||!view.globals||!view.globals->initialized||
    view.actor||view.selector)return GH_EFFECT_ARGUMENT;
 e->native->esps[slot].initialized=1;
 return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_commit_18(gh_game_effects*e,unsigned slot,uint32_t serial){
 gh_effect_esp_view view;
 gh_effect_status status=gh_effect_create_esp_at(e,slot,&view);
 if(status||view.serial!=serial||view.type!=0x18||view.initialized||
    w(view.payload+0xf0)!=esp_vtable(0x18)||!view.globals||!view.globals->initialized||
    ((view.actor||view.selector)&&!r101_weapon_esp_bound(e,&view)))return GH_EFFECT_ARGUMENT;
 e->native->esps[slot].initialized=1;
 return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_commit_1a(gh_game_effects*e,unsigned slot,uint32_t serial){gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||v.type!=0x1a||v.initialized||w(v.payload+0xf0)!=0x44f270u||!v.globals||!v.globals->initialized||!v.scene_actors||((v.actor||v.selector)&&!gh_effect_event_part_is_bound(v.record,v.actor,v.selector)&&!gh_effect_attack_esp_is_bound(e,&v)&&!gh_effect_knockdown_esp_is_bound(e,&v)&&!gh_effect_player_attachment_esp_is_bound(e,&v)&&!gh_effect_devil_esp_is_bound(e,&v)))return GH_EFFECT_ARGUMENT;e->native->esps[slot].initialized=1;return GH_EFFECT_OK;}

gh_effect_status gh_effect_create_esp_attach_emitter_002b5d58(gh_game_effects*e,unsigned slot,uint32_t serial,unsigned parent,uint32_t parent_serial){
 gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial)return GH_EFFECT_ARGUMENT;esp_metadata*m=&e->native->esps[slot];
 if(m->parent_emitter>=0){gh_effect_emitter_view existing;if(gh_effect_create_emitter_at(e,(unsigned)m->parent_emitter,&existing)||existing.serial!=m->parent_serial)return GH_EFFECT_ARGUMENT;return GH_EFFECT_OK;}
 gh_effect_emitter_view p;if(gh_effect_create_emitter_at(e,parent,&p)||p.serial!=parent_serial)return GH_EFFECT_ARGUMENT;if(m->previous!=-1||m->next!=-1||w(v.payload+4)||w(v.payload+0xc)||w(v.payload+0x10)||w(p.payload+8))return GH_EFFECT_UNSUPPORTED;
 int32_t previous=-1,current=p.first_esp_child;unsigned walked=0;while(current!=-1){if(current<0||current>=1024||++walked>1024)return GH_EFFECT_ARGUMENT;gh_effect_esp_view child;if(gh_effect_create_esp_at(e,(unsigned)current,&child)||child.parent_emitter!=(int32_t)parent||child.parent_serial!=parent_serial||child.previous!=previous)return GH_EFFECT_ARGUMENT;previous=current;current=child.next;}
 m->parent_emitter=(int32_t)parent;m->parent_serial=parent_serial;m->previous=previous;m->next=-1;if(previous==-1)e->native->slots[parent].first_esp_child=(int32_t)slot;else e->native->esps[previous].next=(int32_t)slot;return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_unlink_002b5ec0(gh_game_effects*e,unsigned slot,uint32_t serial){
 gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial)return GH_EFFECT_ARGUMENT;if(v.parent_emitter<0)return GH_EFFECT_OK;gh_effect_emitter_view p;if(gh_effect_create_emitter_at(e,(unsigned)v.parent_emitter,&p)||p.serial!=v.parent_serial)return GH_EFFECT_ARGUMENT;
 if(v.previous< -1||v.next< -1||v.previous>=1024||v.next>=1024)return GH_EFFECT_ARGUMENT;if(v.previous==-1){if(p.first_esp_child!=(int32_t)slot)return GH_EFFECT_ARGUMENT;}else {gh_effect_esp_view previous;if(gh_effect_create_esp_at(e,(unsigned)v.previous,&previous)||previous.next!=(int32_t)slot||previous.parent_emitter!=v.parent_emitter||previous.parent_serial!=v.parent_serial)return GH_EFFECT_ARGUMENT;}
 if(v.next>=0){gh_effect_esp_view next;if(gh_effect_create_esp_at(e,(unsigned)v.next,&next)||next.previous!=(int32_t)slot||next.parent_emitter!=v.parent_emitter||next.parent_serial!=v.parent_serial)return GH_EFFECT_ARGUMENT;e->native->esps[v.next].previous=v.previous;}
 if(v.previous==-1)e->native->slots[v.parent_emitter].first_esp_child=v.next;else e->native->esps[v.previous].next=v.next;esp_metadata*m=&e->native->esps[slot];m->parent_emitter=m->previous=m->next=-1;m->parent_serial=0;return GH_EFFECT_OK;
}

int gh_effect_esp_trail_is_bound(const gh_game_effects *e,const gh_effect_esp_view *v)
{
 if(!v)return 0;
 if(v->type==0x1f)return gh_effect_devil_esp_is_bound(e,v);
 if(v->type==0x20&&gh_effect_em00_attack_esp_is_bound(e,v))return 1;
 gh_effect_view est;
 if(v->type==9){
  if(gh_effect_attack_esp_is_bound(e,v)||gh_effect_knockdown_esp_is_bound(e,v)||
     gh_effect_player_attachment_esp_is_bound(e,v))return 1;
  return v->payload&&v->payload_size==0x400&&w(v->payload+0x23a)==0x00050006u&&
     gh_effect_event_part_is_bound(v->record,v->actor,v->selector)&&
     !gh_effect_est_at_00306030(e,6,5,&est)&&est.bytes&&est.size>=16+7*0x2f0&&
     (v->record.bytes==est.bytes+16+5*0x2f0||v->record.bytes==est.bytes+16+6*0x2f0);
 }
 return v->type==0x20&&v->payload&&v->payload_size==0x400&&w(v->payload+0x23a)==0x000d0006u&&
    gh_effect_event_part_is_bound(v->record,v->actor,v->selector)&&
    !gh_effect_est_at_00306030(e,6,13,&est)&&est.bytes&&est.size>=16+2*0x2f0&&
    v->record.bytes==est.bytes+16+0x2f0;
}
gh_effect_status gh_effect_create_bind_trail_pools(gh_game_effects *e,gh_room_pools *pools)
{
 if(!e||!e->ready||!pools)return GH_EFFECT_ARGUMENT;
 /* Before the first live emitter there is no companion to bind. */
 if(!e->native)return GH_EFFECT_OK;
 if(e->native->owner!=e)return GH_EFFECT_ARGUMENT;
 if(e->native->trail_pools&&e->native->trail_pools!=pools)return GH_EFFECT_ARGUMENT;
 e->native->trail_pools=pools;
 return GH_EFFECT_OK;
}
static gh_effect_status initialize_trail(gh_game_effects *e,unsigned slot,
 const gh_effect_esp_view *view,uint32_t work594,const float position[4],
 const float *other_edge,unsigned count,uint32_t *frontier)
{
 *frontier=0x2ebd10;
 gh_room_pools *pools=e->native->trail_pools;
 if(!pools)return GH_EFFECT_UNSUPPORTED;
 esp_metadata *m=e->native->esps+slot;
 if(m->trail_pools)return GH_EFFECT_ARGUMENT;
 for(unsigned j=0;j<3;j++)
  if(!isfinite(position[j])||(other_edge&&!isfinite(other_edge[j])))return GH_EFFECT_ARGUMENT;
 m->trail_pools=pools;
 const unsigned allocations=other_edge?4:3;
 for(unsigned i=0;i<allocations;i++){
  const size_t bytes=i<2?count*0x40u+0xf0u:count*0x10u;
  if(gh_room_arena_allocate_002a9538(pools,0x754240,work594,(uint32_t)bytes,16,m->trail+i)||
     !m->trail[i].pointer){
   (void)release_trail(m);
   return GH_EFFECT_UNSUPPORTED;
  }
 }
 uint8_t *p=view->payload;
 const float u=(w(p+0x11c)&0x10000000u)?-1.0f:1.0f;
 const float uv_direction=(w(p+0x11c)&0x08000000u)?1.0f:-1.0f;
 const float direction=view->type==0x1f?-uv_direction:uv_direction;
 for(unsigned i=0;i<count;i++){
  uint8_t *history=m->trail[2].pointer;
  memcpy(history+16*i,position,12);
  ww(history+16*i+12,0x3f800000);
  if(other_edge){
   uint8_t *history2=m->trail[3].pointer;
   memcpy(history2+16*i,other_edge,12);
   ww(history2+16*i+12,0x3f800000);
  }
  const volatile float v=(direction*(float)i)/(float)(count-1);
  for(unsigned parity=0;parity<2;parity++){
   uint8_t *packet=m->trail[parity].pointer;
   ww(packet+0x60,0x80808080);ww(packet+0x64,0x3f800000);
   ww(packet+0x68,1);ww(packet+0x6c,0);
   uint8_t *entry=packet+0xa0+i*0x40;
   ww(entry,0);memcpy(entry+4,(const void *)&v,4);ww(entry+8,2);ww(entry+12,0);
   memcpy(entry+0x20,&u,4);memcpy(entry+0x24,(const void *)&v,4);
   ww(entry+0x28,2);ww(entry+0x2c,0);
  }
 }
 p[0x249]=(uint8_t)count;
 ww(p+0x2e4,(uint32_t)view->record.bytes[0x18c]+1);
 m->initialized=1;
 *frontier=0;
 return GH_EFFECT_OK;
}

static float devil_trail_add(float a,float b){volatile float v=a+b;return v;}
static float devil_trail_mul(float a,float b){volatile float v=a*b;return v;}

static float devil_trail_roll(uint32_t *seed)
{
 *seed=*seed*0x19660du+0x3c6ef35fu;
 uint32_t bits=(*seed&0x7fffffu)|0x3f800000u;
 float value;
 memcpy(&value,&bits,4);
 return devil_trail_add(devil_trail_add(devil_trail_add(value,value),-2),-1);
}

gh_effect_status gh_effect_create_esp_devil_trail_init_002f5788(gh_game_effects *e,
 unsigned slot,uint32_t serial,uint32_t work594,gh_static_transform_policy policy,uint32_t *frontier)
{
 if(!frontier)return GH_EFFECT_ARGUMENT;
 *frontier=0x2f5788;
 gh_effect_esp_view v;
 if(gh_effect_create_esp_at(e,slot,&v)||v.serial!=serial||v.initialized||v.type!=0x1f||
    !gh_effect_devil_esp_is_bound(e,&v)||v.parent_emitter<0)return GH_EFFECT_ARGUMENT;
 const uint8_t *r=v.record.bytes;
 uint8_t *p=v.payload;
 /* All five EM20 rows have no length clamp, lateral oscillation or
  * angular variation. Their constant Y acceleration is 0.01 per Move. */
 if(r[0x18d]!=20||r[0x18e]!=5||r[0x197]||
    w(r+0x13c)||w(r+0x140)!=0x3f800000||w(r+0x144)!=0x3e4ccccd||
    w(r+0x148)||w(r+0x14c)!=0x3c23d70a||w(r+0x150)||
    r[0x191]||r[0x192]||r[0x193]||r[0x194]||w(p+0x2e0)||w(p+0x2dc))
  return GH_EFFECT_UNSUPPORTED;
 float position[4],zero[4]={0,0,0,1};
 gh_effect_status status=gh_effect_node_world_position_native(e,0,slot,serial,position,policy,frontier);
 if(status)return status;
 status=initialize_trail(e,slot,&v,work594,position,zero,20,frontier);
 if(status)return status;
 uint32_t seed=w(p+0x220);
 const float phase=devil_trail_mul(devil_trail_roll(&seed),3.1415927f);
 (void)devil_trail_roll(&seed);
 (void)devil_trail_roll(&seed);
 memcpy(p+0x330,&phase,4);
 ww(p+0x220,seed);
 ww(p+0x2e0,p[0x245]==1&&p[0x246]==1?0:1);
 ww(p+0x2e4,1);
 memcpy(p+0x300,r+0x148,12);
 memcpy(p+0x310,r+0x13c,8);
 const float damping=devil_trail_add(1,-0.2f);
 memcpy(p+0x318,&damping,4);
 const float frequencies[]={devil_trail_mul((float)(int8_t)r[0x18f],0.03f),
     devil_trail_mul((float)(int8_t)r[0x190],0.01f)};
 memcpy(p+0x31c,frequencies,sizeof(frequencies));
 p[0x32c]=7;
 return GH_EFFECT_OK;
}

gh_effect_status gh_effect_create_esp_devil_trail_move_002f5bf8(gh_game_effects *e,
 unsigned slot,uint32_t serial,const gh_boot_work *work,gh_static_transform_policy policy,uint32_t *frontier)
{
 if(!frontier||!work)return GH_EFFECT_ARGUMENT;
 *frontier=0x2f5bf8;
 gh_effect_esp_view v;
 if(gh_effect_create_esp_at(e,slot,&v)||v.serial!=serial||!v.initialized||v.type!=0x1f||
    !gh_effect_devil_esp_is_bound(e,&v))return GH_EFFECT_ARGUMENT;
 esp_metadata *m=e->native->esps+slot;
 uint8_t *p=v.payload;
 if(!m->trail_pools||!m->trail[2].pointer||!m->trail[3].pointer||
    m->trail[2].bytes!=320||m->trail[3].bytes!=320||p[0x249]!=20||
    w(p+0x2dc)||w(p+0x310)||w(p+0x324)||w(p+0x328))
  return GH_EFFECT_UNSUPPORTED;
 float position[4],phase,increment;
 gh_effect_status status=gh_effect_node_world_position_native(e,0,slot,serial,position,policy,frontier);
 if(status)return status;
 memcpy(&phase,p+0x330,4);memcpy(&increment,p+0x320,4);
 if(gh_id_angle_reference_001f7d48(devil_trail_add(phase,increment),&phase)){
  *frontier=0x1f7d48;
  return GH_EFFECT_UNSUPPORTED;
 }
 float point[20][4],velocity[20][4];
 memcpy(point,m->trail[2].pointer,sizeof(point));
 memcpy(velocity,m->trail[3].pointer,sizeof(velocity));
 for(unsigned i=19;i>0;i--)for(unsigned axis=0;axis<3;axis++)
  velocity[i][axis]=devil_trail_add(velocity[i][axis],
      devil_trail_add(point[i-1][axis],-point[i][axis]));
 for(unsigned i=0;i<19;i++)for(unsigned axis=0;axis<3;axis++)
  velocity[i][axis]=devil_trail_add(velocity[i][axis],
      devil_trail_add(point[i+1][axis],-point[i][axis]));
 float rate=1;
 if((w(p+0x11c)&0x100u)||(w((const uint8_t *)work+0x5e4)&0x20000u)){
  const gh_effects_frame_globals *globals=v.globals;
  if(!globals||!globals->initialized)return GH_EFFECT_ARGUMENT;
  memcpy(&rate,&globals->factor_741dc0,4);
  rate=devil_trail_mul(rate,work->canonical_rate574);
 }
 const float gravity=devil_trail_mul(0.01f,rate);
 for(unsigned i=1;i<20;i++)for(unsigned axis=0;axis<3;axis++){
  if(axis==1)velocity[i][axis]=devil_trail_add(velocity[i][axis],gravity);
  velocity[i][axis]=devil_trail_mul(velocity[i][axis],0.8f);
  point[i][axis]=devil_trail_add(point[i][axis],velocity[i][axis]);
  if(!isfinite(point[i][axis]))return GH_EFFECT_UNSUPPORTED;
 }
 memcpy(point[0],position,12);
 memcpy(m->trail[2].pointer,point,sizeof(point));
 memcpy(m->trail[3].pointer,velocity,sizeof(velocity));
 memcpy(p+0x330,&phase,4);
 *frontier=0;
 return GH_EFFECT_OK;
}

gh_effect_status gh_effect_create_esp_trail_init_002ebf98(gh_game_effects *e,unsigned slot,
 uint32_t serial,uint32_t work594,const float position[4],uint32_t *frontier)
{
 if(!frontier)return GH_EFFECT_ARGUMENT;
 *frontier=0x2ebf98;
 gh_effect_esp_view view;
 if(gh_effect_create_esp_at(e,slot,&view)||view.serial!=serial||view.initialized||
    view.type!=9||!gh_effect_esp_trail_is_bound(e,&view)||!position)return GH_EFFECT_ARGUMENT;
 const uint8_t *r=view.record.bytes,*p=view.payload;
 const unsigned count=!r[0x18d]&&gh_effect_player_stun_row(view.record)?16u:r[0x18d];
 if(count<2||count>120||r[0x18e]||p[0x245]!=1||p[0x246]!=1||
    w(p+0x2e0)||w(p+0x2dc)||w(p+0x2d4)!=0x3f800000u||view.parent_emitter<0)
  return GH_EFFECT_UNSUPPORTED;
 return initialize_trail(e,slot,&view,work594,position,NULL,count,frontier);
}

/* 2F6258 evaluates the same node after translating it in parent space,
 * then restores the original local position and invalidates both caches. */
static gh_effect_status trail_other_edge(gh_game_effects *e,unsigned slot,
 const gh_effect_esp_view *view,gh_static_transform_policy policy,const float offset[3],
 float position[4],uint32_t *frontier)
{
 float saved[3],translated[3];
 memcpy(saved,view->payload+0x40,12);
 for(unsigned j=0;j<3;j++){
  const volatile float value=saved[j]+offset[j];
  translated[j]=value;
 }
 gh_effect_status status=gh_effect_node_set_position_native(e,0,slot,view->serial,translated,frontier);
 if(status)return status;
 status=gh_effect_node_world_position_native(e,0,slot,view->serial,position,policy,frontier);
 uint32_t restore_frontier=0;
 gh_effect_status restored=gh_effect_node_set_position_native(e,0,slot,view->serial,saved,&restore_frontier);
 if(restored){*frontier=restore_frontier;return restored;}
 return status;
}

gh_effect_status gh_effect_create_esp_trail_edges_002f62f8(gh_game_effects *e,unsigned slot,
 uint32_t serial,uint32_t work594,gh_static_transform_policy policy,uint32_t *frontier)
{
 if(!frontier)return GH_EFFECT_ARGUMENT;
 *frontier=0x2f62f8;
 gh_effect_esp_view view;
 if(gh_effect_create_esp_at(e,slot,&view)||view.serial!=serial||view.initialized||
    view.type!=0x20||!gh_effect_esp_trail_is_bound(e,&view))return GH_EFFECT_ARGUMENT;
 const uint8_t *r=view.record.bytes;
 uint8_t *p=view.payload;
 const int stamp8=stamp8_attack_actor(view.actor)&&
    gh_effect_em00_attack_esp_is_bound(e,&view);
 const unsigned count=stamp8?r[0x18d]:16;
 const uint32_t trail_fields=w(r+0x18c);
 if(w(r+0x16c)||w(r+0x170)||w(r+0x148)||p[0x245]!=1||
    (stamp8?(p[0x246]!=1||(trail_fields!=0x701&&trail_fields!=0x601)):
             (p[0x246]!=2||trail_fields!=3))||
    w(p+0x2e0)||w(p+0x2dc)||w(p+0x2d4)!=0x3f800000u||view.parent_emitter<0)
  return GH_EFFECT_UNSUPPORTED;
 float first[4],second[4],offset[3];
 for(unsigned j=0;j<3;j++){
  float authored;
  memcpy(&authored,r+0x13c+j*4,4);
  const volatile float value=authored*0.01f;
  if(!isfinite(value))return GH_EFFECT_UNSUPPORTED;
  offset[j]=value;
 }
 gh_effect_status status=gh_effect_node_world_position_native(e,0,slot,serial,first,policy,frontier);
 if(status)return status;
 status=trail_other_edge(e,slot,&view,policy,offset,second,frontier);
 if(status)return status;
 status=initialize_trail(e,slot,&view,work594,first,second,count,frontier);
 if(status)return status;
 /* 2EBD10 sets mask 1 for an atlas, 2F62F8 sets mask 4 for two edges. */
 ww(p+0x2e0,stamp8?4:5);
 memcpy(p+0x2f0,offset,12);
 ww(p+0x300,0x3f800000u);
 return GH_EFFECT_OK;
}

gh_effect_status gh_effect_create_esp_trail_view_edges(const gh_game_effects *e,unsigned slot,
 uint32_t serial,const float **positions,const float **other_edge,unsigned *count)
{
 if(!positions||!other_edge||!count)return GH_EFFECT_ARGUMENT;
 gh_effect_esp_view view;
 if(gh_effect_create_esp_at(e,slot,&view)||view.serial!=serial||!view.initialized||
    !gh_effect_esp_trail_is_bound(e,&view))return GH_EFFECT_ARGUMENT;
 const esp_metadata *m=e->native->esps+slot;
 const size_t bytes=16u*view.payload[0x249];
 if(!m->trail_pools||!m->trail[2].pointer||view.payload[0x249]<2||m->trail[2].bytes!=bytes||
    (view.type==0x20&&(!m->trail[3].pointer||m->trail[3].bytes!=bytes)))return GH_EFFECT_ARGUMENT;
 *positions=m->trail[2].pointer;
 *other_edge=view.type==0x1f?NULL:m->trail[3].pointer;
 *count=view.payload[0x249];
 return GH_EFFECT_OK;
}

gh_effect_status gh_effect_create_esp_trail_view(const gh_game_effects *e,unsigned slot,
 uint32_t serial,const float **positions,unsigned *count)
{
 const float *other_edge;
 return gh_effect_create_esp_trail_view_edges(e,slot,serial,positions,&other_edge,count);
}

static void sample_trail(float *history,unsigned count,const float position[4])
{
 for(unsigned i=count-1;i>0;i--)memcpy(history+4*i,history+4*(i-1),12);
 memcpy(history,position,12);
}

gh_effect_status gh_effect_create_esp_trail_sample_002ec218(gh_game_effects *e,unsigned slot,
 uint32_t serial,const float position[4],uint32_t *frontier)
{
 if(!frontier)return GH_EFFECT_ARGUMENT;
 *frontier=0x2ec218;
 const float *history,*other_edge;
 unsigned count;
 if(!position||gh_effect_create_esp_trail_view_edges(e,slot,serial,&history,&other_edge,&count)||other_edge)
  return GH_EFFECT_ARGUMENT;
 for(unsigned j=0;j<3;j++)if(!isfinite(position[j]))return GH_EFFECT_ARGUMENT;
 sample_trail(e->native->esps[slot].trail[2].pointer,count,position);
 *frontier=0;
 return GH_EFFECT_OK;
}

gh_effect_status gh_effect_create_esp_trail_edges_002f6740(gh_game_effects *e,unsigned slot,
 uint32_t serial,gh_static_transform_policy policy,uint32_t *frontier)
{
 if(!frontier)return GH_EFFECT_ARGUMENT;
 *frontier=0x2f6740;
 gh_effect_esp_view view;
 const float *history,*other_edge;
 unsigned count;
 if(gh_effect_create_esp_at(e,slot,&view)||view.serial!=serial||view.type!=0x20||
    gh_effect_create_esp_trail_view_edges(e,slot,serial,&history,&other_edge,&count))
  return GH_EFFECT_ARGUMENT;
 const uint8_t *p=view.payload;
 if(w(p+0x2dc)||w(p+0x304)||w(p+0x300)!=0x3f800000u||!(w(p+0x11c)&0x01000000u))
  return GH_EFFECT_UNSUPPORTED;
 float first[4],second[4],offset[3];
 memcpy(offset,p+0x2f0,12);
 gh_effect_status status=gh_effect_node_world_position_native(e,0,slot,serial,first,policy,frontier);
 if(status)return status;
 status=trail_other_edge(e,slot,&view,policy,offset,second,frontier);
 if(status)return status;
 esp_metadata *m=e->native->esps+slot;
 sample_trail(m->trail[2].pointer,count,first);
 sample_trail(m->trail[3].pointer,count,second);
 return GH_EFFECT_OK;
}

gh_effect_status gh_effect_create_esp_pause_002fbe50(gh_game_effects*e,unsigned slot,uint32_t serial,uint32_t*f){if(!f)return GH_EFFECT_ARGUMENT;*f=0;gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||!v.initialized)return GH_EFFECT_ARGUMENT;if(!esp_vtable(v.type)||((v.actor||v.selector)&&!sprite_binding(v.record,v.actor,v.selector)&&!gh_effect_attack_esp_is_bound(e,&v)&&!gh_effect_knockdown_esp_is_bound(e,&v)&&!gh_effect_player_attachment_esp_is_bound(e,&v)&&!gh_effect_devil_esp_is_bound(e,&v)&&!gh_effect_enemy_light_esp_is_bound(e,&v)&&!gh_effect_emad_esp_is_bound(e,&v))){*f=0x2fbe50;return GH_EFFECT_UNSUPPORTED;}
 /* Supported constructors create no ESPchildren and the only supportedattach
  * API accepts emitterparents. Unknown rawchild/parent tokens remainstrict. */
 if(w(v.payload+4)||w(v.payload+8)||w(v.payload+0xc)||w(v.payload+0x10)){*f=0x2b5cf0;return GH_EFFECT_UNSUPPORTED;}status=gh_effect_create_esp_unlink_002b5ec0(e,slot,serial);if(status){*f=0x2b5ec0;return status;}unsigned off=(slot>>5)*4;uint32_t bit=UINT32_C(0x80000000)>>(slot&31);ww(e->native->esp+0x100080+off,w(e->native->esp+0x100080+off)|bit);return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_destroy_hidden_native(gh_game_effects*e,unsigned slot,uint32_t serial,uint32_t*f){if(!f)return GH_EFFECT_ARGUMENT;*f=0;gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||!v.initialized)return GH_EFFECT_ARGUMENT;unsigned off=(slot>>5)*4;uint32_t bit=UINT32_C(0x80000000)>>(slot&31);if(!(w(e->native->esp+0x100100+off)&bit))return GH_EFFECT_ARGUMENT;if(v.type==0x14){*f=0x2f27e8;return GH_EFFECT_UNSUPPORTED;}if(!esp_vtable(v.type)||((v.actor||v.selector)&&!sprite_binding(v.record,v.actor,v.selector)&&!gh_effect_attack_esp_is_bound(e,&v)&&!gh_effect_knockdown_esp_is_bound(e,&v)&&!gh_effect_player_attachment_esp_is_bound(e,&v)&&!gh_effect_devil_esp_is_bound(e,&v)&&!gh_effect_enemy_light_esp_is_bound(e,&v)&&!gh_effect_emad_esp_is_bound(e,&v))||v.parent_emitter!=-1||v.previous!=-1||v.next!=-1||w(v.payload+4)||w(v.payload+8)||w(v.payload+0xc)||w(v.payload+0x10)){*f=0x2fbe28;return GH_EFFECT_UNSUPPORTED;}if(w(v.payload+0xf0)!=esp_vtable(v.type)){*f=0x2fbe28;return GH_EFFECT_UNSUPPORTED;}
 if((v.type==9||v.type==0x1f||v.type==0x20)&&release_trail(e->native->esps+slot)){*f=0x2ebc50;return GH_EFFECT_ARGUMENT;}
 /* Actualvirtual dtor(flags2): concreteVT ->44FAD0 ->44B310; bit1clear
  * means original30F5A0 is not called. Caller clearsactive onlyafterreturn. */
 ww(v.payload+0xf0,0x44fad0u);ww(v.payload+0xf0,0x44b310u);e->native->esps[slot].associated=0;return GH_EFFECT_OK;}

gh_effect_status gh_effect_create_esp_check_hidden_14(gh_game_effects *e,unsigned slot,uint32_t serial){
 gh_effect_esp_view v;
 if(gh_effect_create_esp_at(e,slot,&v)||v.serial!=serial||!v.initialized||v.type!=0x14)
  return GH_EFFECT_ARGUMENT;
 unsigned off=(slot>>5)*4;
 uint32_t mask=UINT32_C(0x80000000)>>(slot&31);
 if(!(w(e->native->esp+0x100100+off)&mask))return GH_EFFECT_ARGUMENT;
 if(((v.actor||v.selector)&&!gh_effect_player_attachment_esp_is_bound(e,&v))||
    v.parent_emitter!=-1||v.previous!=-1||v.next!=-1||
    w(v.payload+4)||w(v.payload+8)||w(v.payload+0xc)||w(v.payload+0x10)||
    w(v.payload+0xf0)!=GH_ESP14_VTABLE)return GH_EFFECT_UNSUPPORTED;
 return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_finish_hidden_14(gh_esp14_instance *owner,uint32_t *frontier){
 if(!frontier)return GH_EFFECT_ARGUMENT;
 *frontier=0x2fbe28;
 if(!owner||!owner->child_delete_requested||!owner->child.object.ready||
    (w(owner->child.object.view.bytes+0x250)&0x80000002u)!=0x80000002u)return GH_EFFECT_ARGUMENT;
 gh_effect_status status=gh_effect_create_esp_check_hidden_14(owner->effects,owner->slot,owner->serial);
 if(status)return status;
 struct gh_effect_native *n=owner->effects->native;
 uint8_t *payload=n->esp+(size_t)owner->slot*0x400;
 /* 2FBE28 ->2FBDD8(flags2), neither calls operator delete. */
 ww(payload+0xf0,0x44fad0u);
 ww(payload+0xf0,0x44b310u);
 n->esps[owner->slot].associated=0;
 *frontier=0;
 return GH_EFFECT_OK;
}

static int attaches_actor(const gh_title_player_view *actor,const gh_title_player_view *selector,
 const gh_title_player_view *owner)
{
 return (actor&&actor->bytes==owner->bytes)||(selector&&selector->bytes==owner->bytes);
}

gh_effect_status gh_effect_create_quiesce_actor_room_exit(gh_game_effects *effects,
 const gh_title_player_view *actor)
{
 if(!effects||!effects->native||effects->native->owner!=effects||!actor||!actor->bytes||
    actor->size<0x1560||(w(actor->bytes+0x250)&0xe0000000u)!=0xe0000000u)
  return GH_EFFECT_ARGUMENT;
 struct gh_effect_native *native=effects->native;
 unsigned emitters[256],particles[1024],emitter_count=0,particle_count=0;
 for(unsigned i=0;i<256;i++){
  const unsigned off=(i>>5)*4;
  const uint32_t mask=UINT32_C(0x80000000)>>(i&31);
  if(!(w(native->emitters+0x26000+off)&mask))continue;
  const emitter_metadata *m=native->slots+i;
  if(!attaches_actor(m->actor,m->selector,actor))continue;
  gh_effect_emitter_view view;
  if(gh_effect_create_emitter_at(effects,i,&view)||view.first_esp_child!=-1||
     !(w(view.payload+0x114)&0x40000000u)||w(view.payload+4)||w(view.payload+8))
   return GH_EFFECT_UNSUPPORTED;
  const int devil=w(view.payload+0x18c)==0x00030058u&&
    gh_effect_devil_emitter_is_bound(effects,&view);
  if(!devil&&!gh_effect_emad_emitter_is_bound(effects,&view)&&
     !gh_effect_enemy_light_emitter_is_bound(effects,&view))
   return GH_EFFECT_UNSUPPORTED;
  emitters[emitter_count++]=i;
 }
 for(unsigned i=0;i<1024;i++){
  const unsigned off=(i>>5)*4;
  const uint32_t mask=UINT32_C(0x80000000)>>(i&31);
  if(!(w(native->esp+0x100000+off)&mask))continue;
  const esp_metadata *m=native->esps+i;
  if(!attaches_actor(m->actor,m->selector,actor))continue;
  gh_effect_esp_view view;
  if(gh_effect_create_esp_at(effects,i,&view)||!view.initialized||
     !((w(native->esp+0x100080+off)|w(native->esp+0x100100+off))&mask)||
     view.parent_emitter!=-1||view.previous!=-1||view.next!=-1||
     w(view.payload+4)||w(view.payload+8)||w(view.payload+0xc)||w(view.payload+0x10))
   return GH_EFFECT_UNSUPPORTED;
  const int devil=w(view.payload+0x23a)==0x00030058u&&
    (view.type==0||view.type==0x1f)&&gh_effect_devil_esp_is_bound(effects,&view);
  if(!devil&&!gh_effect_emad_esp_is_bound(effects,&view)&&
     !gh_effect_enemy_light_esp_is_bound(effects,&view))return GH_EFFECT_UNSUPPORTED;
  if(m->trail_pools&&(!devil||view.type!=0x1f||m->trail_pools!=native->trail_pools))
   return GH_EFFECT_UNSUPPORTED;
  particles[particle_count++]=i;
 }
 /* 305B90 has already stopped the emitter and unlinked its particles.
  * State5 clears original arena 754240 wholesale. Release the paused
  * trails' native allocations before that bounded arena is destroyed. */
 for(unsigned j=0;j<particle_count;j++){
  esp_metadata *m=native->esps+particles[j];
  if(m->trail_pools&&release_trail(m))return GH_EFFECT_ARGUMENT;
 }
 /* State5 destroys actors and their data before State3 resets these masks.
  * These companions may no longer resolve their former source or actor. */
 for(unsigned j=0;j<emitter_count;j++){
  emitter_metadata *m=native->slots+emitters[j];
  *m=(emitter_metadata){.serial=m->serial,.associated=1,.quiesced=1,.first_esp_child=-1};
 }
 for(unsigned j=0;j<particle_count;j++){
  esp_metadata *m=native->esps+particles[j];
  *m=(esp_metadata){.serial=m->serial,.associated=1,.quiesced=1,
      .parent_emitter=-1,.previous=-1,.next=-1};
 }
 return GH_EFFECT_OK;
}


/* After the final destruction bit, 300628 calls 3028E8 directly.
 * Its stopped type-0 emitter still needs its own payload and EST row,
 * but it does not read the attached actor or model again. */
static void detach_destroying_emitters(struct gh_effect_native *n,
 const gh_title_player_view *actor)
{
 for(unsigned i=0;i<256;i++){
  const unsigned off=(i>>5)*4;
  const uint32_t bit=UINT32_C(0x80000000)>>(i&31);
  if(!(w(n->emitters+0x26000+off)&w(n->emitters+0x26040+off)&bit))continue;
  emitter_metadata *m=n->slots+i;
  if(!m->associated||!m->initialized||m->quiesced||!m->record||
     m->first_esp_child!=-1||!attaches_actor(m->actor,m->selector,actor))continue;
  const uint8_t *p=n->emitters+(size_t)i*0x260;
  if(w(p+0xf0)!=0x44fd58||!(w(p+0x114)&0x40000000u)||
     w(p+4)||w(p+8)||w(p+0x10))continue;
  m->actor=NULL;
  m->selector=NULL;
  m->model=NULL;
  m->part_index=0;
  m->part_handle=0;
 }
}

gh_effect_status gh_effect_create_prepare_actor_retirement(gh_game_effects *e,
 const gh_title_player_view *actor)
{
 if(!e||!actor||!actor->bytes)return GH_EFFECT_ARGUMENT;
 if(!e->native||actor->size<0x254||
    (w(actor->bytes+0x250)&0xa0000000u)!=0xa0000000u)return GH_EFFECT_OK;
 struct gh_effect_native *n=e->native;
 if(n->owner!=e)return GH_EFFECT_ARGUMENT;
 detach_destroying_emitters(n,actor);
 if(actor->size<0x644||actor->original_vtable214!=0x4479c8||
    (w(actor->bytes+0x2fc)>>16)!=0x202||w(actor->bytes+0x564)!=0x203||
    (w(actor->bytes+0x640)&0xffffu)!=19||
    (w(actor->bytes+0x250)&0xa0008000u)!=0xa0000000u)return GH_EFFECT_OK;
 gh_effect_view est={0};
 unsigned rows=0;
 for(unsigned i=0;i<1024;i++){
  esp_metadata *m=n->esps+i;
  const uint32_t bit=UINT32_C(0x80000000)>>(i&31);
  if(!(w(n->esp+0x100000+(i>>5)*4)&bit)||!m->associated||!m->initialized||
     m->actor!=actor||m->selector!=actor||m->type!=0x1a||!m->knockdown||
     m->parent_emitter!=-1||m->previous!=-1||m->next!=-1)continue;
  const uint8_t *payload=n->esp+(size_t)i*0x400;
  if(w(payload+0x23a)!=0x00020058u||w(payload+0x10c)!=UINT32_MAX||
     (w(payload+0x11c)&0x80u)||w(payload+0x120))continue;
  if(!est.bytes){
   if(gh_effect_est_at_00306030(e,0x58,2,&est)||!est.bytes||est.size<16||w(est.bytes)!=0x545345u)
    return GH_EFFECT_UNSUPPORTED;
   rows=w(est.bytes+4);
   if(rows>(est.size-16)/0x2f0)return GH_EFFECT_BOUNDS;
  }
  for(unsigned row=0;row<rows;row++){
   if(m->record.bytes!=est.bytes+16+(size_t)row*0x2f0)continue;
   /* The unlinked texture particle continues its authored timer.
    * Copy the last 2FB0E8/2FEC38 actor inputs before 30F5A0 frees them. */
   const uint8_t *p=actor->bytes;
   float tint;
   memcpy(&tint,p+0x24c,4);
   if(!isfinite(tint))return GH_EFFECT_UNSUPPORTED;
   m->retired_actor=(gh_effect_retired_actor){
    .flags=w(p+0x250),.tint_alpha=tint,
    .opacity={p[0x2b6],p[0x2b7],p[0x2bc]},.valid=1};

   m->actor=m->selector=NULL;
   break;
  }
 }
 return GH_EFFECT_OK;
}

gh_effect_status gh_effect_create_actor_has_live_refs(const gh_game_effects *e,
 const gh_title_player_view *actor,int *borrowed){
 if(!e||!actor||!actor->bytes||!borrowed)return GH_EFFECT_ARGUMENT;
 *borrowed=0;
 if(!e->native)return GH_EFFECT_OK;
 const struct gh_effect_native *n=e->native;
 if(n->owner!=e)return GH_EFFECT_ARGUMENT;
 for(unsigned i=0;i<256;i++){
  const emitter_metadata *m=&n->slots[i];
  uint32_t bit=UINT32_C(0x80000000)>>(i&31);
  if(!(w(n->emitters+0x26000+(i>>5)*4)&bit))continue;
  if(!m->associated)return GH_EFFECT_UNSUPPORTED;
  if((m->actor&&m->actor->bytes==actor->bytes)||(m->selector&&m->selector->bytes==actor->bytes)){
   *borrowed=1;return GH_EFFECT_OK;
  }
 }
 for(unsigned i=0;i<1024;i++){
  const esp_metadata *m=&n->esps[i];
  uint32_t bit=UINT32_C(0x80000000)>>(i&31);
  if(!(w(n->esp+0x100000+(i>>5)*4)&bit))continue;
  if(!m->associated)return GH_EFFECT_UNSUPPORTED;
  if((m->actor&&m->actor->bytes==actor->bytes)||(m->selector&&m->selector->bytes==actor->bytes)){
   *borrowed=1;return GH_EFFECT_OK;
  }
 }
 return GH_EFFECT_OK;
}


static int contains_source(uintptr_t base,size_t bytes,const void *source){
 uintptr_t at=(uintptr_t)source;
 return source&&at>=base&&at-base<bytes;
}
gh_effect_status gh_effect_create_source_has_live_refs(const gh_game_effects*e,
 const void*source,size_t bytes,int*borrowed){
 if(!e||!source||!bytes||!borrowed||(uintptr_t)source>UINTPTR_MAX-bytes)return GH_EFFECT_ARGUMENT;
 const uintptr_t base=(uintptr_t)source;
 *borrowed=contains_source(base,bytes,e->event_data.bytes);
 if(*borrowed||!e->native)return GH_EFFECT_OK;
 const struct gh_effect_native*n=e->native;
 if(n->owner!=e)return GH_EFFECT_ARGUMENT;
 for(unsigned i=0;i<256;i++){
  uint32_t bit=UINT32_C(0x80000000)>>(i&31);
  if(!(w(n->emitters+0x26000+(i>>5)*4)&bit))continue;
  const emitter_metadata*m=n->slots+i;
  if(!m->associated)return GH_EFFECT_UNSUPPORTED;
  if(contains_source(base,bytes,m->record)){*borrowed=1;return GH_EFFECT_OK;}
 }
 for(unsigned i=0;i<1024;i++){
  uint32_t bit=UINT32_C(0x80000000)>>(i&31);
  if(!(w(n->esp+0x100000+(i>>5)*4)&bit))continue;
  const esp_metadata*m=n->esps+i;
  if(!m->associated)return GH_EFFECT_UNSUPPORTED;
  if(contains_source(base,bytes,m->record.bytes)||contains_source(base,bytes,m->texture.bytes)||
     contains_source(base,bytes,m->secondary_texture.bytes)){*borrowed=1;return GH_EFFECT_OK;}
 }
 return GH_EFFECT_OK;
}
