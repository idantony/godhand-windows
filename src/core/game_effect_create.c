#include "game_effect_create.h"
#include "id_layout_rotation.h"
#include "game_effects_frame.h"
#include "game_actor_matrix.h"
#include "game_effect_node.h"
#include "game_actor_two_bone.h"
#include "game_esp14.h"
#include "game_esp03.h"
#include "game_scene_at_geometry.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
typedef struct {const gh_title_player_view *actor,*selector;const gh_actor_model *model,*heat_model;uint32_t part_index,part_handle;
 const uint8_t *record;uint32_t serial;uint8_t associated,initialized,quiesced;int32_t first_esp_child;} emitter_metadata;
typedef struct {gh_effect_view record,texture,secondary_texture;const gh_title_player_view *actor,*selector;const gh_actor_part *light_part;uint32_t serial,emitter_serial;unsigned emitter_slot;uint8_t associated,initialized,type,quiesced;int32_t parent_emitter,previous,next;uint32_t parent_serial;uint8_t target120_known_null,knockdown,player_attachment;gh_effect_retired_actor retired_actor;gh_room_pools *trail_pools;gh_room_arena_allocation trail[4];gh_esp03_active_state heat;const gh_actor_model *heat_model;} esp_metadata;
struct gh_effect_native {gh_game_effects *owner;uint8_t *esp,*emitters;uint32_t serial;gh_room_pools *trail_pools;
 emitter_metadata slots[256];esp_metadata esps[1024];struct gh_effects_frame_globals *frame_globals;gh_game_scene_tables *frame_actors;};
static int r103_em0a_effect_actor(const gh_title_player_view *actor);
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
static gh_effect_status create_stun_part4(gh_effect_create_call *c,const gh_title_player_view *actor,int32_t mode)
{
 const gh_effect_create_request request={.bank=0,.index=4,.actor=actor,.mode=mode,.selector=-1};
 gh_effect_status status=gh_effect_create_begin(c,&request);
 if(!status)c->configuration_part4_stun=1;
 return status;
}

gh_effect_status gh_effect_create_player_stun_part4_00306900(gh_effect_create_call *c,
 const gh_title_player_view *actor)
{
 if(!actor||!actor->bytes||actor->size<0x300||actor->original_vtable214!=0x41d220||
    (w(actor->bytes+0x2fc)>>16)!=0x100)return GH_EFFECT_ARGUMENT;
 return create_stun_part4(c,actor,4);
}

static int r102_gate_guard_effect_actor(const gh_title_player_view *actor)
{
 return actor&&actor->bytes&&actor->size>0x1650&&actor->original_vtable214==0x4479c8&&
    (w(actor->bytes+0x2fc)>>16)==0x202&&w(actor->bytes+0x564)==0x200&&
    (w(actor->bytes+0x640)&0xffffu)==0x10&&!actor->bytes[0x1650];
}

/* Targeted R102 EM0A that uses the same authored part-zero ground effect. */
static int r102_gate_em0a_ground_actor(const gh_title_player_view *actor)
{
 const uint8_t *p=actor?actor->bytes:NULL;
 if(!p||actor->size<=0x1650||actor->original_vtable214!=0x4479c8||
    (w(p+0x2fc)>>16)!=0x202||w(p+0x564)!=0x20a||(w(p+0x640)&0xffffu)!=2||
    p[0x1650]!=5)return 0;
 return p[0x2f4]==2||(p[0x2f4]==1&&p[0x2f5]==8&&p[0x2f6]==3&&p[0x2f7]==2);
}

static int r102_em03_death_actor(const gh_title_player_view *actor)
{
 const uint8_t *p=actor?actor->bytes:NULL;
 return p&&actor->size>0x1650&&actor->original_vtable214==0x4479c8&&
    (w(p+0x2fc)>>16)==0x202&&w(p+0x564)==0x203&&(w(p+0x640)&0xffffu)==8&&
    !p[0x1650]&&p[0x2f4]==2;
}

gh_effect_status gh_effect_create_gate_guard_stun_part4_00306900(gh_effect_create_call *c,
 const gh_title_player_view *actor)
{
 if(!r102_gate_guard_effect_actor(actor))return GH_EFFECT_ARGUMENT;
 return create_stun_part4(c,actor,3);
}

gh_effect_status gh_effect_create_em0a_stun_part4_00306900(gh_effect_create_call *c,
 const gh_title_player_view *actor)
{
 if(!r103_em0a_effect_actor(actor))return GH_EFFECT_ARGUMENT;
 return create_stun_part4(c,actor,3);
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

static int r104_charge_actor(const gh_title_player_view *actor);

gh_effect_status gh_effect_create_enemy_knockdown_002a8608(gh_effect_create_call *c,
 const gh_title_player_view *actor,uint32_t index)
{
 const int recoil=index==4||index==8;
 if(!actor||!actor->bytes||actor->size<0x436||(!recoil&&index!=0x2f&&index!=0x30))
  return GH_EFFECT_ARGUMENT;
 if(recoil){
  const int arena=actor->size>0x1650&&actor->original_vtable214==0x4479c8&&
      actor->bytes[0x1650]==15&&(w(actor->bytes+0x2fc)>>16)==0x204&&
      w(actor->bytes+0x564)>=0x214&&w(actor->bytes+0x564)<=0x215;
  const int em03=r104_charge_actor(actor)&&actor->size>=0x17b8&&w(actor->bytes+0x17b4)==80;
  if((!arena&&!em03)||(w(actor->bytes+0x2f4)&0xffffu)!=0x0301)return GH_EFFECT_UNSUPPORTED;
 }
 /* 2A86A0 loads the same actor mirror bit for all of these effect IDs. */
 const int mirrored=(actor->bytes[0x434]&2)!=0;
 const gh_effect_create_request request={.bank=0x58,.index=index,.actor=actor,.mode=0,.selector=-1};
 gh_effect_status status=gh_effect_create_begin(c,&request);
 if(!status){
  c->configuration_death_part_zero=2;
  if(mirrored){
   /* Original 2A86A0 reads the motion mirror bit and applies actor scale. */
   c->configuration_nested_flags=0x80;
   c->configuration_nested_scale=w(actor->bytes+0x114);
  }
 }
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
 if(kind==0x363&&dependency==0x202&&stamp==18&&appearance==9)return 0x100;
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

gh_effect_status gh_effect_create_ev07_nested_002effec(gh_effect_create_call *call,uint32_t tag,const float position[3],
 const float rotation[3],const float color[4],float scale)
{
 uint32_t bits[3];
 if(!call||!position||!rotation||!color||!isfinite(scale))return GH_EFFECT_ARGUMENT;
 for(unsigned i=0;i<3;++i)if(!isfinite(rotation[i]))return GH_EFFECT_ARGUMENT;
 for(unsigned i=0;i<4;++i)if(!isfinite(color[i]))return GH_EFFECT_ARGUMENT;
 memcpy(bits,position,sizeof(bits));
 if(tag!=0x00020001u&&tag!=0x000d0006u)return GH_EFFECT_UNSUPPORTED;
 gh_effect_status status=positioned_begin(call,tag&0xffffu,tag>>16,bits,0);
 if(status)return status;
 call->configuration_esp10=1;
 call->configuration_nested_group=1;
 call->configuration_nested_flags=tag==0x00020001u?2:6;
 memcpy(&call->configuration_nested_scale,&scale,4);
 memcpy(call->configuration_esp10_rotation,rotation,12);
 memcpy(call->configuration_esp10_color,color,16);
 return GH_EFFECT_OK;
}

gh_effect_status gh_effect_create_r104_em03_ground_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0xa8,3,world_xyz,0);
}

gh_effect_status gh_effect_create_catapult_impact_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0,0x14,world_xyz,0);
}

gh_effect_status gh_effect_create_devil_projectile_impact_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0x58,0x41,world_xyz,0);
}

gh_effect_status gh_effect_create_em03_projectile_impact_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0xa8,0x10,world_xyz,0);
}

gh_effect_status gh_effect_create_om307_break_begin_00306838(gh_effect_create_call *call,uint32_t index)
{
 static const uint32_t origin[3];
 if(index<1||index>5)return GH_EFFECT_ARGUMENT;
 return positioned_begin(call,1,index,origin,0);
}

gh_effect_status gh_effect_create_om324_break_begin_001bfd28(gh_effect_create_call *call,
 const uint32_t world_xyz[3],uint32_t yaw_bits)
{
 return positioned_begin(call,0,0x1c,world_xyz,yaw_bits);
}

gh_effect_status gh_effect_create_enemy_blowaway_begin_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0,0xb,world_xyz,0);
}

gh_effect_status gh_effect_create_om346_break_begin_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0,7,world_xyz,0);
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

gh_effect_status gh_effect_create_enemy_boosted_impact_begin_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0,0x40,world_xyz,0);
}

gh_effect_status gh_effect_create_enemy_deflect_begin_00306838(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0x58,0x11,world_xyz,0);
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
gh_effect_status gh_effect_create_om331_break_begin_0019ca58(gh_effect_create_call *call,
 const uint32_t world_xyz[3])
{
 return positioned_begin(call,0,0xcc,world_xyz,0);
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
 if(em27_attack_actor(actor))return 1;
 if(!actor||!actor->bytes||actor->size<0x568||actor->original_vtable214!=0x4479c8)return 0;
 const unsigned kind=w(actor->bytes+0x2fc)>>16;
 const uint32_t dependency=w(actor->bytes+0x564);
 if(dependency==0x214||dependency==0x215)
  return kind==0x204&&actor->size>0x1650&&actor->bytes[0x1650]==15&&
      (w(actor->bytes+0x640)&0xffffu)==dependency-0x214;
 if(dependency!=0x200)return 0;
 if(kind==0x202)return 1;
 if(kind!=0x204||actor->size<=0x1650||actor->bytes[0x1650]!=24)return 0;
 const unsigned stamp=w(actor->bytes+0x640)&0xffffu;
 return stamp==2||stamp==3;
}

static int r103_paired_kick_actor(const gh_title_player_view *actor)
{
 if(!em00_family_attack_actor(actor))return 0;
 const uint32_t dependency=w(actor->bytes+0x564);
 return dependency==0x214||dependency==0x215;
}

static int r103_paired_kick_row(const uint8_t *row)
{
 if(row[0xbe]||row[0x128]||row[0x136])return 0;
 switch(row[0x133]){
 case 0:
 case 0x1a:
  return w(row)==0x1efe8010u&&w(row+4)==0x00c00000u&&
     row[0x12e]==(row[0x133]==0x1a?132:0);
 case 4:
  return w(row)==0x1efe0010u&&w(row+4)==0x00802000u&&row[0x12e]==5;
 default:
  return 0;
 }
}

static int em00_escape_effect_actor(const gh_title_player_view *actor)
{
 if(!actor||!actor->bytes||actor->size<0x644||actor->original_vtable214!=0x4479c8||
    w(actor->bytes+0x564)!=0x200)return 0;
 const unsigned kind=w(actor->bytes+0x2fc)>>16;
 if(kind==0x202)return (w(actor->bytes+0x640)&0xffffu)==20;
 return kind==0x204&&em00_family_attack_actor(actor);
}

static int16_t em0a_attack_room(const gh_title_player_view *actor)
{
 if(r103_em0a_effect_actor(actor))return 0x103;
 if(!actor||!actor->bytes||actor->size<=0x1650||actor->original_vtable214!=0x4479c8||
    (w(actor->bytes+0x2fc)>>16)!=0x202||w(actor->bytes+0x564)!=0x20a)return 0;
 const unsigned stamp=w(actor->bytes+0x640)&0xffffu;
 const unsigned appearance=actor->bytes[0x1650];
 if(!stamp&&appearance==3)return 0x100;
 if(stamp==2&&appearance==5)return 0x102;
 return 0;
}

static int r103_em0a_effect_actor(const gh_title_player_view *actor)
{
 if(!actor||!actor->bytes||actor->size<=0x1650||actor->original_vtable214!=0x4479c8||
    (w(actor->bytes+0x2fc)>>16)!=0x204||w(actor->bytes+0x564)!=0x20a||
    actor->bytes[0x1650]!=24)return 0;
 const unsigned stamp=w(actor->bytes+0x640)&0xffffu;
 return stamp==4||stamp==5;
}

static unsigned r103_em0a_slide_rows(unsigned group)
{
 return group==0x15?5:group==0x16||group==0x18?2:0;
}

/* Original selector19 CORE58 wind-up, travel and recovery SEQs. */
static int r103_em0a_slide_row(const uint8_t *row,unsigned group,unsigned index)
{
 if(index>=r103_em0a_slide_rows(group)||row[0x128]||row[0x136])return 0;
 if(group==0x16)
  return w(row)==(index?0x1ebe4010u:0x1ebec010u)&&w(row+4)==0x00c00000u&&
     !row[0xbe]&&row[0x133]==(index?0x1a:0)&&!row[0x12e];
 if(group==0x18){
  if(index)return w(row)==0x1e3e8010u&&w(row+4)==0x00c00000u&&
     !row[0xbe]&&row[0x133]==0x1a&&!row[0x12e];
  return w(row)==0x1e3e0010u&&w(row+4)==0x00800000u&&row[0xbe]==0xff&&
     row[0x133]==0x18&&!row[0x12e]&&row[0x18c]==5&&!row[0x18d]&&!w(row+0x15c);
 }
 static const uint32_t headers[]={0x1e3e8010u,0x1e3e0010u,0x1e3e8010u,
    0x1e3e0010u,0x1e3e8010u};
 static const uint8_t parts[]={4,4,4,0,25};
 static const uint8_t types[]={0,0,0,0x18,0};
 static const uint8_t texture_items[]={11,14,51,0,0};
 return w(row)==headers[index]&&w(row+4)==(index==4?0x00c00000u:0x00800000u)&&
    row[0xbe]==parts[index]&&row[0x133]==types[index]&&row[0x12e]==texture_items[index];
}

static int stamp8_attack_actor(const gh_title_player_view *actor)
{
 return em00_family_attack_actor(actor)&&actor->size>0x1650&&
        w(actor->bytes+0x564)==0x200&&(w(actor->bytes+0x640)&0xffffu)==8&&
        actor->bytes[0x1650]==5;
}

static int em00_attack_58_2b_actor(const gh_title_player_view *actor)
{
 if(stamp8_attack_actor(actor))return 1;
 if(!em00_family_attack_actor(actor)||actor->size<0x16c8||
    (w(actor->bytes+0x2fc)>>16)!=0x202)return 0;
 const unsigned stamp=w(actor->bytes+0x640)&0xffffu;
 const uint32_t home=w(actor->bytes+0x16c4);
 return (stamp==3&&actor->bytes[0x1650]==2&&home==6)||
    (stamp==31&&!actor->bytes[0x1650]&&!home);
}

static int em00_attack_trail_actor(const gh_title_player_view *actor,unsigned index)
{
 if(index==0x2b)return em00_attack_58_2b_actor(actor);
 if(index!=0x46||!em00_family_attack_actor(actor)||actor->size<0x16c8)return 0;
 const uint8_t *p=actor->bytes;
 const unsigned stamp=w(p+0x640)&0xffffu;
 return (w(p+0x2fc)>>16)==0x202&&w(p+0x16c4)==6&&
        ((stamp==20&&!p[0x1650])||
         (stamp==10&&p[0x1650]==5));
}

static int16_t wide_strike_effect_room(const gh_title_player_view *actor,unsigned index)
{
 if(em00_attack_trail_actor(actor,index))return 0x100;
 if(index!=0x2b)return 0;
 if(stamp8_attack_actor(actor))return 0x100;
 if(r103_paired_kick_actor(actor)&&w(actor->bytes+0x564)==0x214)return 0x103;
 if(!em00_family_attack_actor(actor)||w(actor->bytes+0x564)!=0x200)return 0;
 return (w(actor->bytes+0x2fc)>>16)==0x204?0x103:0;
}

static int wide_strike_effect_row(const uint8_t *row)
{
 const unsigned part=row[0xbe];
 if(row[0x128]||row[0x136]||w(row+4)!=0x00c00000u)return 0;
 if(row[0x133]==0x12)return (part==25||part==21)&&w(row)==0x1efe8010u;
 if(row[0x133]==0x20)return (part==24||part==20)&&w(row)==0x1e3e0010u;
 return row[0x133]==0x1a&&((part==24&&w(row)==0x1e3e8010u)||
                            (part==21&&w(row)==0x1efe8010u));
}

static int16_t em00_escape_effect_room(const gh_title_player_view *actor)
{
 if(!em00_family_attack_actor(actor)||actor->size<0x644||w(actor->bytes+0x564)!=0x200)return 0;
 const unsigned stamp=w(actor->bytes+0x640)&0xffffu;
 if(stamp==20)return 0x100;
 if(actor->size>0x1650&&((stamp==11&&actor->bytes[0x1650]==5)||
    (stamp==3&&actor->bytes[0x1650]==1)||
    (stamp==7&&actor->size>=0x16c8&&!actor->bytes[0x1650]&&w(actor->bytes+0x16c4)==2)))return 0x102;
 return 0;
}

static unsigned em0a_attack_effect_count(int16_t room,unsigned index)
{
 if(index==0x24)return room==0x100?4u:0;
 if(index==0x1d)return room==0x102||room==0x103?6u:0;
 return index==0x1e&&room?2u:0;
}

static int em0a_attack_effect_row(const uint8_t *row,unsigned index)
{
 if(row[0x128]||row[0x136])return 0;
 const unsigned part=row[0xbe],type=row[0x133];
 const uint32_t flags=w(row),flags4=w(row+4);
 if(index==0x1d&&!part&&!type)
  return (flags==0x1efee010u&&flags4==0x00800000u)||(flags==0x1efe8010u&&flags4==0x00c00000u);
 return (part==16||((index==0x1d||index==0x24)&&part==10))&&flags4==0x00c00000u&&
        ((type==9&&flags==0x1e3e8010u)||(type==0x12&&flags==0x1efe8010u));
}

static int devil_effect_actor(const gh_title_player_view *actor,const gh_title_player_view *selector);

static int devil_projectile_effect_actor(const gh_title_player_view *actor)
{
 return actor&&actor->bytes&&actor->size==0x630&&actor->original_vtable214==0x4290c8&&
    (w(actor->bytes+0x2fc)>>16)==0x3be&&w(actor->bytes+0x610)==3;
}

static int em03_projectile_effect_actor(const gh_title_player_view *actor)
{
 return actor&&actor->bytes&&actor->size==0x630&&actor->original_vtable214==0x4290c8&&
    (w(actor->bytes+0x2fc)>>16)==0x3be&&!w(actor->bytes+0x610);
}

static int em03_projectile_est_row(unsigned member,const uint8_t *row,unsigned index)
{
 static const uint8_t launch_textures[]={14,10,132};
 static const uint8_t impact_textures[]={14,51,10,132,7};
 if(w(row+4)!=0x00800000u||row[0x128]||row[0x133]||row[0x136])return 0;
 if(member==0xf){
  return index<sizeof(launch_textures)&&!row[0xbe]&&row[0x12e]==launch_textures[index]&&
    w(row)==(index?0x1efe8010u:0x1e3e8010u);
 }
 return member==0x10&&index<sizeof(impact_textures)&&row[0xbe]==0xff&&
    row[0x12e]==impact_textures[index]&&w(row)==(index<2?0x1e3e8010u:0x1efe8010u);
}

/* Original CORE58 emission groups for the four ranged EM20 sequences. */
static unsigned devil_ranged_effect_rows(uint32_t index)
{
 static const uint8_t counts[]={7,7,21,25};
 return index>=0x42&&index<=0x45?counts[index-0x42]:0;
}

/* The selector 16 sequences carry normal and heat-5 CORE58 kick rows. */
static unsigned devil_combo_effect_rows(uint32_t index)
{
 if(index==0x37)return 3;
 return (index>=0x35&&index<=0x36)||(index>=0x49&&index<=0x4b)?4:0;
}

static int devil_combo_effect_row(uint32_t index,const uint8_t *row,unsigned slot)
{
 static const uint32_t pair_flags[]={0x1eee0010u,0x1eee8010u,0x1eee0010u,0x1eee8010u};
 static const uint8_t pair_parts[]={15,16,9,10},pair_types[]={0,0x12,0,0x12};
 static const uint32_t finish_flags[]={0x1eee8010u,0x1e2e0010u,0x1eee8010u,0x1e2ea010u};
 static const uint8_t finish_parts[]={15,15,16,16},finish_types[]={0,0,0x12,9};
 if(slot>=devil_combo_effect_rows(index)||w(row+4)!=0x00c00000u||row[0x128]||row[0x136])
  return 0;
 if(index==0x37&&slot)++slot;
 if(index==0x37||index==0x4b)
  return w(row)==finish_flags[slot]&&row[0xbe]==finish_parts[slot]&&row[0x133]==finish_types[slot];
 return w(row)==pair_flags[slot]&&row[0xbe]==pair_parts[slot]&&row[0x133]==pair_types[slot];
}

static int devil_melee_effect_index(uint32_t index)
{
 return index>=0x4c&&index<=0x4e;
}

static int devil_melee_effect_row(const uint8_t *row,unsigned index)
{
 return index<2&&w(row)==(index?0x1eee8010u:0x1eee0010u)&&w(row+4)==0x00c00000u&&
    row[0xbe]==9+index&&!row[0x128]&&row[0x133]==(index?0x12:0)&&!row[0x136];
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

/* CORE035 is the original attached EM0A hold impact on PL00. */
static int em0a_grab_impact_row(const uint8_t *row,unsigned index)
{
 static const uint32_t flags[]={0x00802000u,0x00800000u,0x00c00000u,0x00c00000u};
 return index<4&&w(row)==(index<2?0x1e3e0010u:0x1efe8010u)&&w(row+4)==flags[index]&&
    row[0xbe]==(index<2?2:0)&&row[0x133]==(index<2?7:0x1a)&&
    row[0x12e]==(index==3?7:0)&&!row[0x128]&&!row[0x136];
}

static int player_heat_activation_row(const uint8_t *r,unsigned i)
{
 static const uint8_t parts[]={10,8,7,7,9,10,9,8,10,10,10,8,8,8,8,8,9,8,10,10,0};
 static const uint8_t types[]={0,0,0,9,9,9,0,0,0,0,0,0,0,0,4,4,4,4,7,7,0x15};
 static const uint8_t textures[]={14,14,14,24,24,24,76,76,76,0,11,24,24,24,9,8,8,7,0,0,0};
 static const uint32_t flags[]={0x800000,0x800000,0x800000,0xe00000,0xe00000,0xe00000,
     0xc00000,0xc00000,0xc00000,0xc00000,0x800000,0x802000,0xc02000,0x802000,
     0xc02000,0xc02000,0xc02000,0xc02000,0x800000,0x800000,0xc00000};
 return i<sizeof(parts)&&!r[0x128]&&!r[0x136]&&r[0xbe]==parts[i]&&r[0x133]==types[i]&&
     r[0x12e]==textures[i]&&w(r+4)==flags[i];
}

/* Authored PL00 Heat finish sequence uses all twelve CORE 0:28 rows. */
static int player_heat_finish_row(const uint8_t *r,unsigned i)
{
 static const uint8_t parts[]={10,8,7,7,9,10,9,9,10,9,10,10};
 static const uint8_t types[]={0,0,0,9,9,9,0,0,0,0,7,7};
 static const uint8_t textures[]={14,14,14,24,24,24,76,0,6,51,0,0};
 static const uint32_t headers[]={0x1e3e8010u,0x1e3e8010u,0x1e3e8010u,0x1ebea010u,0x1ebea010u,0x1ebea010u,0x1eeee010u,0x1eeee010u,0x1efee010u,0x1e3e8010u,0x1e3e0010u,0x1e3e8010u};
 static const uint32_t flags[]={0x00800000u,0x00800000u,0x00800000u,0x00e00000u,0x00e00000u,0x00e00000u,0x00c00000u,0x00c00000u,0x00800000u,0x00800000u,0x00800000u,0x00800000u};
 return i<sizeof(parts)&&!r[0x128]&&!r[0x136]&&r[0xbe]==parts[i]&&r[0x133]==types[i]&&
    r[0x12e]==textures[i]&&w(r)==headers[i]&&w(r+4)==flags[i];
}

/* Bank 0/21 is the live God Hand aura once the original Heat sequence ends. */
static int player_heat_active_row(const uint8_t *r,unsigned i)
{
 static const uint8_t parts[]={10,9,8,7,7,0xff,8,9,9,8};
 static const uint8_t types[]={0,0,0,0,0,3,0,0,0x1f,0x1f};
 static const uint8_t textures[]={14,14,14,14,14,29,10,10,12,12};
 static const uint32_t headers[]={0x1e3e8010,0x1e3e8010,0x1e3e8010,0x1e3e8010,0x1e3e8010,
     0x1e3e0010,0x1efee010,0x1efee010,0x1ebea010,0x1ebe8010};
 return i<sizeof(parts)&&!r[0x128]&&!r[0x136]&&r[0xbe]==parts[i]&&r[0x133]==types[i]&&
    r[0x12e]==textures[i]&&w(r)==headers[i]&&w(r+4)==(i<6?0x00802000u:0x00c02000u);
}

static int player_attack_boost_row(const uint8_t *row,unsigned i)
{
 static const uint8_t parts[]={10,10,2,3,2,2,2,2,2,0,0,2,4,2,0};
 static const uint8_t types[]={7,7,9,9,0,0,0,0,0,0x15,0,0,0,0,0};
 static const uint8_t textures[]={0,0,24,24,76,0,32,32,51,0,1,43,14,14,14};
 static const uint32_t flags[]={0x800000,0x800000,0xe00000,0xe00000,0xc00000,0xc00000,
    0x802000,0xc02000,0x800000,0xc00000,0x800000,0x800000,0x800000,0x800000,0x800000};
 return i<sizeof(parts)&&!row[0x128]&&!row[0x136]&&row[0xbe]==parts[i]&&
    row[0x133]==types[i]&&row[0x12e]==textures[i]&&w(row+4)==flags[i];
}


/* R103 paired throw's 204 effect bank publishes the 15 authored EST3B rows. */
static int r103_boss_throw_effect_row(const uint8_t *row,unsigned index)
{
 static const struct {uint32_t header,flags;uint8_t delay,part,texture,type;} rows[]={
  {0x1efe8010,0x00c00000,0,25,0x84,0},
  {0x1efe8010,0x00c00000,0,20,0x84,0},
  {0x1e3e0010,0x00800000,0,2,0,7},
  {0x1e3e0010,0x00800000,10,2,0,7},
  {0x1e3e8010,0x00800000,6,0,0,7},
  {0x1e3e0010,0x00800000,0,0,0,7},
  {0x1e3e0010,0x00800000,20,2,0,7},
  {0x1efe8010,0x00c00000,0,0,0x84,0},
  {0x1e3e8010,0x00c00000,4,0,0x33,0},
  {0x1e3e0010,0x00800000,0,0,4,0x14},
  {0x1efe0010,0x00800000,50,0,5,4},
  {0x1efe0010,0x00800000,50,0,5,4},
  {0x1efee010,0x00c00000,0,0,0,0x15},
  {0x1efe8010,0x00c00000,0,0,2,0x15},
  {0x1efe8010,0x00c00000,70,0,0,0},
 };
 return index<sizeof(rows)/sizeof(rows[0])&&w(row)==rows[index].header&&w(row+4)==rows[index].flags&&
    w(row+12)==rows[index].delay&&row[0xbe]==rows[index].part&&row[0x12e]==rows[index].texture&&
    row[0x133]==rows[index].type&&!row[0x128]&&!row[0x136];
}

/* CORE58 groups used by the original 3B/3C paired attack. */
static unsigned r103_pincer_effect_rows(uint32_t index)
{
 switch(index){
 case 0x28:case 0x4f:return 1;
 case 0x32:case 0x34:return 2;
 case 0x33:return 3;
 default:return 0;
 }
}

static int r103_pincer_boss(const gh_title_player_view *actor)
{
 if(!actor||!actor->bytes||actor->size<=0x1650||actor->original_vtable214!=0x4479c8)return 0;
 const uint8_t *p=actor->bytes;
 const uint32_t dependency=w(p+0x564);
 return (w(p+0x2fc)>>16)==0x204&&(dependency==0x214||dependency==0x215)&&
    (w(p+0x640)&0xffffu)==dependency-0x214&&p[0x1650]==15;
}

static int r103_pincer_effect_actor(const gh_title_player_view *actor,uint32_t index)
{
 if(!r103_pincer_boss(actor))return 0;
 const uint8_t *p=actor->bytes;
 const unsigned phase=(index==0x34||index==0x4f)?15:11;
 return !p[0x2f4]&&p[0x2f5]==(index==0x32?0x3b:0x3c)&&p[0x2f6]==phase&&!p[0x2f7];
}

static int r103_pincer_effect_row(const uint8_t *row,uint32_t index,unsigned ordinal)
{
 if(row[0x128]||row[0x136])return 0;
 if(index==0x34){
  if(ordinal==0)return w(row)==0x1e3e0010u&&w(row+4)==0x00800000u&&
      row[0xbe]==20&&row[0x133]==7&&!row[0x12e];
  return ordinal==1&&w(row)==0x1eb00010u&&w(row+4)==0x00c00000u&&
      row[0xbe]==21&&!row[0x133]&&!row[0x12e];
 }
 if(index==0x4f)return !ordinal&&w(row)==0x1efe8010u&&w(row+4)==0x00c00000u&&
     row[0xbe]==20&&row[0x133]==0x1a&&row[0x12e]==132;
 const int foot=index==0x28,dust=!foot&&ordinal+1==r103_pincer_effect_rows(index);
 const unsigned type=foot?0:dust?9:ordinal?0x12:0x1a;
 return w(row)==(foot?0x1efe0010u:dust?0x1e3e8010u:0x1efe8010u)&&
    w(row+4)==(dust?0x00c01000u:0x00c00000u)&&!row[0xbe]&&
    row[0x133]==type&&row[0x12e]==(dust?15u:132u);
}

static int forward_evade_effect_actor(const gh_title_player_view *actor)
{
 return actor&&actor->bytes&&actor->size>=0x300&&actor->original_vtable214==0x41d220&&
    (w(actor->bytes+0x2fc)>>16)==0x100&&!actor->bytes[0x2f4]&&actor->bytes[0x2f5]==7&&
    actor->bytes[0x2f6]==1&&!actor->bytes[0x2f7];
}

static int r104_charge_actor(const gh_title_player_view *actor)
{
 return actor&&actor->bytes&&actor->size>0x1651&&actor->original_vtable214==0x4479c8&&
    (w(actor->bytes+0x2fc)>>16)==0x203&&w(actor->bytes+0x564)==0x250&&
    w(actor->bytes+0x1644)==0x80000&&!actor->bytes[0x1650]&&!actor->bytes[0x1651];
}

static unsigned em03_sequence_effect_count(unsigned member)
{
 switch(member){
 case 2: case 8: return 10;
 case 0xb: return 3;
 case 0x14: return 13;
 default: return 0;
 }
}

static int em03_sequence_effect_row(unsigned member,const uint8_t *row,unsigned index)
{
 if(member==2||member==8){
  static const uint8_t parts[]={15,15,10,10,15,15,15,15,10,10};
  const int trail=!(index&1);
  return index<sizeof(parts)&&row[0xbe]==parts[index]&&row[0x12e]==(trail?15:0)&&
    !row[0x128]&&!row[0x136]&&row[0x133]==(trail?9:0x12)&&
    w(row)==(trail?0x1e3e8010u:0x1efe8010u)&&w(row+4)==0x00c00000u;
 }
 if(member==0xb){
  static const uint8_t approach_parts[]={0,21,25};
  return index<sizeof(approach_parts)&&row[0xbe]==approach_parts[index]&&
    !row[0x12e]&&!row[0x128]&&!row[0x136]&&row[0x133]==(index?0:7)&&
    w(row)==(index?0x1e360010u:0x1e3e0010u)&&w(row+4)==(index?0x00c00000u:0x00800000u);
 }
 static const uint8_t parts[]={16,10,8,16,14,10,16,0,0,0,0,0,20};
 static const uint8_t textures[]={0,14,14,14,14,14,14,8,10,14,10,14,132};
 if(member!=0x14||index>=sizeof(parts)||row[0xbe]!=parts[index]||row[0x12e]!=textures[index]||
    row[0x128]||row[0x136])return 0;
 const uint32_t properties=!index?0x00802000u:index==12?0x00800000u:0x00c00000u;
 const uint32_t kind=!index?0x1e3e0010u:index==10?0x1efe0010u:0x1efe8010u;
 const unsigned child=!index?7u:index==12?0x1au:0u;
 return w(row)==kind&&w(row+4)==properties&&row[0x133]==child;
}


static unsigned em03_melee_est_lines(unsigned member)
{
 switch(member){
 case 6:case 7:case 9:case 0xa:return 1;
 case 0x1a:case 0x1c:return 2;
 default:return 0;
 }
}

static int r104_held_impact_row(const uint8_t *row,unsigned index)
{
 static const uint32_t flags[]={0x1e3e0010,0x1efe8010,0x1efe0010,0x1efe0010,0x1efe8010,0x1efe0010};
 static const uint32_t attachment[]={0x00802000,0x00c00000,0x00c00000,0x00c00000,0x00c00000,0x00800000};
 static const uint8_t part[]={16,0,0,0,0,0xff};
 static const uint8_t child[]={7,0,0,0,0x1a,4};
 static const uint8_t texture[]={0,51,132,7,0,5};
 return index<sizeof(child)&&w(row)==flags[index]&&w(row+4)==attachment[index]&&
     row[0xbe]==part[index]&&!row[0x128]&&!row[0x136]&&
     row[0x133]==child[index]&&row[0x12e]==texture[index];
}

gh_effect_status gh_effect_create_actor_sequence_0014fb48(gh_effect_create_call *call,
 const gh_effect_create_request *request,uint32_t scale)
{
 if(!request||!request->actor||request->selector!=-1||!scale||
    (scale&0x80000000u)||(scale&0x7f800000u)==0x7f800000u)return GH_EFFECT_UNSUPPORTED;
 const gh_title_player_view *actor=request->actor;
 const int held_impact=request->bank==0xa8&&request->index==0x19&&request->mode==-1;
 if(held_impact){
  if(scale!=0x3f800000u||!actor->bytes||actor->size<0x300||
     actor->original_vtable214!=0x41d220||(w(actor->bytes+0x2fc)>>16)!=0x100||
     w(actor->bytes+0x2f4)!=0x00030901u)return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0&&request->index==0x39&&request->mode==0){
  if(!actor->bytes||actor->size<0x300||actor->original_vtable214!=0x41d220||
     (w(actor->bytes+0x2fc)>>16)!=0x100||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x23||
     actor->bytes[0x2f6]!=1||actor->bytes[0x2f7])return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0&&request->index>=0x29&&request->index<=0x2b&&!request->mode){
  if(!forward_evade_effect_actor(actor))return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0&&((request->index==0x3e&&request->mode==-1)||
                      (request->index==0x42&&!request->mode)||
                      (request->index==0x28&&!request->mode&&actor->bytes&&actor->size>0x1684&&
                       actor->bytes[0x2f4]==0&&actor->bytes[0x2f5]==9&&actor->bytes[0x2f6]==1&&
                       !actor->bytes[0x2f7]&&actor->bytes[0x1684]==1)||
                      (request->index==0x22&&request->mode==-1&&actor->bytes&&actor->size>0x1684&&
                       actor->bytes[0x2f4]==0&&actor->bytes[0x2f5]==8&&actor->bytes[0x2f6]==1&&
                       !actor->bytes[0x2f7]&&actor->bytes[0x1684]==1)||
                      (request->index==0x35&&request->mode==-1))){
  if(!actor->bytes||actor->size<0x300||actor->original_vtable214!=0x41d220||
     (w(actor->bytes+0x2fc)>>16)!=0x100)return GH_EFFECT_UNSUPPORTED;
  if(request->index==0x35&&(actor->bytes[0x2f4]!=1||actor->bytes[0x2f5]!=8||
     actor->bytes[0x2f6]!=3||actor->bytes[0x2f7]))return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x6c&&request->index==0x3b&&request->mode==-1){
  if(!actor->bytes||actor->size<0x300||actor->original_vtable214!=0x41d220||
     (w(actor->bytes+0x2fc)>>16)!=0x100||actor->bytes[0x2f4]!=1||actor->bytes[0x2f5]!=0xf||
     actor->bytes[0x2f6]!=1||actor->bytes[0x2f7]!=1)return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&(request->index==0x27||request->index==0x59)&&!request->mode){
  const int16_t escape_room=em00_escape_effect_room(actor);
  if((!escape_room&&!em00_escape_effect_actor(actor))||actor->bytes[0x2f4]!=1||
     actor->bytes[0x2f5]!=0x1d||(escape_room==0x102&&actor->bytes[0x2f7]!=1))
   return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0xa8&&em03_melee_est_lines(request->index)&&!request->mode){
  if(!r104_charge_actor(actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x15||
     actor->bytes[0x2f6]!=1||actor->bytes[0x2f7])return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0xa8&&request->index==0x13&&!request->mode){
  if(!r104_charge_actor(actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x21||
     actor->bytes[0x2f6]!=1||actor->bytes[0x2f7])return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&request->index==0x1a&&request->mode>=-1&&request->mode<=0){
  if(!r104_charge_actor(actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x1a||
     actor->bytes[0x2f6]>(request->mode==-1?0u:1u)||actor->bytes[0x2f7])
   return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0xa8&&em03_sequence_effect_count(request->index)&&!request->mode){
  const int approach=request->index==0xb;
  const unsigned move=approach?0xb:request->index==0x14?0x23:0x16;
  if(!r104_charge_actor(actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=move||
     actor->bytes[0x2f6]!=(approach?3:1)||actor->bytes[0x2f7]>(approach?1:0))
   return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&request->index==0x1c&&request->mode==-1){
  if(!r104_charge_actor(actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x1a||
     actor->bytes[0x2f6]!=4||actor->bytes[0x2f7])return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&(request->index==0x2b||request->index==0x46)&&!request->mode){
  if(!wide_strike_effect_room(actor,request->index)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x15)
   return GH_EFFECT_UNSUPPORTED;
  if(request->index==0x46&&(w(actor->bytes+0x640)&0xffffu)==10&&
     (actor->bytes[0x2f6]!=1||actor->bytes[0x2f7]))return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&r103_em0a_slide_rows(request->index)&&!request->mode){
  const unsigned phase=request->index==0x15?1:request->index==0x16?3:7;
  if(!r103_em0a_effect_actor(actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x19||
     actor->bytes[0x2f6]!=phase||actor->bytes[0x2f7])return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&(request->index==0x1d||request->index==0x1e||request->index==0x24)&&!request->mode){
  const int16_t room=em0a_attack_room(actor);
  if(!em0a_attack_effect_count(room,request->index)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x15||
     (request->index==0x24&&(actor->bytes[0x2f6]!=1||actor->bytes[0x2f7])))
   return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&request->index==0x50&&!request->mode){
  if(!r103_paired_kick_actor(actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x17||
     actor->bytes[0x2f6]!=1||actor->bytes[0x2f7]!=1)return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&r103_pincer_effect_rows(request->index)&&!request->mode){
  if(!r103_pincer_effect_actor(actor,request->index))return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&request->index==0x5d&&!request->mode){
  if(!em0f_route_actor(actor))return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&request->index==0x3f&&!request->mode){
  if(!devil_effect_actor(actor,actor))return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&devil_melee_effect_index(request->index)&&!request->mode){
  if(!devil_effect_actor(actor,actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x15||
     actor->bytes[0x2f6]!=(request->index==0x4e)||actor->bytes[0x2f7])return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&devil_combo_effect_rows(request->index)&&!request->mode){
  const unsigned phase=request->index>=0x49?request->index-0x49:request->index-0x35;
  if(!devil_effect_actor(actor,actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x16||
     actor->bytes[0x2f6]!=3+phase*2)return GH_EFFECT_UNSUPPORTED;
 }else if(request->bank==0x58&&devil_ranged_effect_rows(request->index)&&!request->mode){
  if(!devil_effect_actor(actor,actor)||actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x23||
     actor->bytes[0x2f7]!=2)return GH_EFFECT_UNSUPPORTED;
 }else{
  if(request->bank!=0x58||(request->index!=0x2d&&request->index!=0x2c&&request->index!=0x5a)||
     request->mode)return GH_EFFECT_UNSUPPORTED;
  if(request->index!=0x2d&&!em00_family_attack_actor(actor))return GH_EFFECT_UNSUPPORTED;
 }
 gh_effect_status status=gh_effect_create_begin(call,request);
 if(!status){
  call->configuration_nested_scale=scale;
  call->configuration_held_impact_part_zero=(uint8_t)held_impact;
  /* The explicit-part 306900 initializes config itself. */
  if(!held_impact&&actor->bytes&&actor->size>=0x436&&(actor->bytes[0x434]&2u))
   call->configuration_nested_flags=0x80;
 }
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
int gh_effect_ambient_row_is_bound(gh_game_effects *owner,uint32_t index,gh_effect_view row)
{
 if(!owner||!row.bytes||row.size!=0x2f0)return 0;
 gh_effect_create_call lookup={.owner=owner,.request={.index=index}};
 if(ambient_est(&lookup)||!lookup.est.bytes)return 0;
 for(unsigned i=0;i<lookup.count;i++)
  if(row.bytes==lookup.est.bytes+16+(size_t)i*0x2f0)return 1;
 return 0;
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

static int forward_evade_effect_row(unsigned index,const uint8_t *row)
{
 if(index<0x29||index>0x2b||row[0xbe]!=4||row[0x128]||row[0x136])return 0;
 if(row[0x133]==9)return row[0x12e]==15&&w(row+4)==0x00800000u&&
    w(row)==(index==0x2b?0x1e3ec010u:0x1e3e8010u);
 return row[0x133]==0x12&&!row[0x12e]&&w(row)==0x1efe8010u&&w(row+4)==0x00c00000u;
}
static int16_t door_break_actor_room(const gh_title_player_view *actor)
{
 if(!actor||!actor->bytes||actor->size<0x8f0||
    actor->original_vtable214!=w(actor->bytes+0x214))return 0;
 const unsigned kind=w(actor->bytes+0x2fc)>>16;
 if(kind==0x300&&actor->original_vtable214==0x422f90u)return 0x100;
 if(kind==0x313&&actor->original_vtable214==0x424128u)return 0x101;
 return 0;
}

int gh_effect_r200_catapult_part_is_bound(gh_effect_view row,
 const gh_title_player_view *actor,const gh_title_player_view *selector)
{
 if(!actor||!actor->bytes||actor->size!=0x640||actor->original_vtable214!=0x428c20||
    (w(actor->bytes+0x2fc)>>16)!=0x3bd||selector!=actor||!row.bytes||row.size!=0x2f0)
  return 0;
 const uint8_t *r=row.bytes;
 /* Three original CORE00:13 rows, all attached to the catapult model root. */
 return w(r)==0x1efe8010u&&w(r+4)==0x00c01000u&&!r[0xbe]&&!r[0x128]&&
     r[0x133]==1&&!r[0x136]&&(r[0x12e]==10||r[0x12e]==132);
}

int gh_effect_r101_door_break_part_is_bound(gh_effect_view row,
 const gh_title_player_view *actor,const gh_title_player_view *selector)
{
 if(!door_break_actor_room(actor)||selector!=actor||!row.bytes||row.size!=0x2f0)return 0;
 const uint8_t *r=row.bytes;
 return w(r)==0x1efe8010u&&w(r+4)==0x00c00000u&&
        !r[0xbe]&&!r[0x128]&&!r[0x133]&&!r[0x136]&&(r[0x12e]==132||r[0x12e]==2);
}

/* CORE58:29 attaches the EM02 capture burst to part 25. */
static int r100_em02_capture_actor(const gh_title_player_view *actor)
{
 if(!actor||!actor->bytes||actor->size<0x16c8||actor->original_vtable214!=0x4479c8||
    (w(actor->bytes+0x2fc)>>16)!=0x202)return 0;
 const uint8_t *p=actor->bytes;
 const unsigned stamp=w(p+0x640)&0xffffu;
 const uint32_t dependency=w(p+0x564),home=w(p+0x16c4);
 if(dependency==0x203&&stamp==19&&!p[0x1650])return 1;
 return dependency==0x202&&((stamp==4&&p[0x1650]==2&&home==6)||
    (!p[0x1650]&&((stamp==29&&home==2)||(stamp==32&&!home))));
}

static int r100_em02_capture_row(const uint8_t *row)
{
 if(row[0xbe]!=25||row[0x128]||row[0x136]||w(row+4)!=0x00c00000u)return 0;
 if(row[0x133]==0x1a)return w(row)==0x1e3e8010u;
 return !row[0x133]&&(w(row)==0x1efe6010u||w(row)==0x1efee010u);
}

/* A8:03 is the original parentless EM03 ground impact from SEQ 2E7FC0. */
static int r104_em03_ground_row(const uint8_t *row,unsigned index)
{
 static const uint32_t est_flags[]={0x1e3e0010,0x1e3e0010,0x1efe0010,0x1efe0010,
     0x1efe6010,0x1efee010,0x1efee010};
 static const uint32_t node_flags[]={0x800000,0x800000,0xa00000,0x800000,0xc00000,0xc00000,0xc00000};
 static const uint8_t child[]={24,20,20,20,21,21,21};
 return index<sizeof(child)&&w(row)==est_flags[index]&&w(row+4)==node_flags[index]&&
     row[0xbe]==0xff&&row[0x133]==child[index]&&!row[0x136];
}

static int r104_em03_ground_record(const gh_game_effects *effects,gh_effect_view record,unsigned start)
{
 gh_effect_view est;
 if(!record.bytes||record.size!=0x2f0||gh_effect_est_at_00306030(effects,0xa8,3,&est)||
    !est.bytes||est.size<16+7*0x2f0||w(est.bytes+4)!=7)return 0;
 for(unsigned i=start;i<7;++i)
  if(record.bytes==est.bytes+16+i*0x2f0)return r104_em03_ground_row(record.bytes,i);
 return 0;
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
 if(c->configuration_held_impact_part_zero){
  if(b->work->room!=0x104||c->count!=6||!b->model||b->model->work!=b->work||
     !b->model->parts_ready||b->model->root.bytes!=c->request.actor->bytes)
   return fail(c,0x306900,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;++i){
   if(!r104_held_impact_row(entry.bytes+16+i*0x2f0,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->request.bank==0xa8&&em03_sequence_effect_count(c->request.index)){
  const gh_title_player_view *actor=c->request.actor;
  const unsigned member=c->request.index;
  if(!r104_charge_actor(actor)||!c->configuration_nested_scale||c->request.mode||
     c->request.selector!=-1||c->count!=em03_sequence_effect_count(member)||b->work->room!=0x104||!b->model||
     b->model->work!=b->work||!b->model->parts_ready||b->model->root.bytes!=actor->bytes)
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;++i){
   if(!em03_sequence_effect_row(member,entry.bytes+16+i*0x2f0,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->request.bank==0&&c->request.index==0x1a&&door_break_actor_room(c->request.actor)){
  const gh_title_player_view *actor=c->request.actor;
  if(b->work->room!=door_break_actor_room(actor)||!b->model||b->model->work!=b->work||
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
 if(c->request.bank==0&&c->request.index>=0x29&&c->request.index<=0x2b&&
    c->configuration_nested_scale){
  const gh_title_player_view *actor=c->request.actor;
  if(b->work->room!=0x103||!forward_evade_effect_actor(actor)||
     c->request.mode||c->request.selector!=-1||c->count!=2)
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<2;++i){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(!forward_evade_effect_row(c->request.index,row)||row[0x133]!=(i?0x12:9))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->request.bank==0&&c->request.index==0x22&&c->configuration_nested_scale){
  if((b->work->room!=0x100&&b->work->room!=0x102&&b->work->room!=0x103)||c->count!=21)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;++i)
   if(!player_heat_activation_row(entry.bytes+16+i*0x2f0,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }
 if(c->request.bank==0&&c->request.index==0x28&&c->configuration_nested_scale){
  if((b->work->room!=0x100&&b->work->room!=0x102&&b->work->room!=0x103)||c->count!=12)
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;++i)
   if(!player_heat_finish_row(entry.bytes+16+i*0x2f0,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }
 if(c->request.bank==0&&c->request.index==0x21){
  const gh_title_player_view *actor=c->request.actor;
  if(!actor||!actor->bytes||actor->size<0x15f8||actor->original_vtable214!=0x41d220||
     (w(actor->bytes+0x2fc)>>16)!=0x100||c->request.mode!=3||c->request.selector!=-1||
     actor->bytes[0x5f2]!=3||(w(actor->bytes+0x15f4)&0x20u)||c->count!=10||
     (b->work->room!=0x100&&b->work->room!=0x102&&b->work->room!=0x103&&
      !(b->work->room==0x104&&!actor->bytes[0x2f4]&&actor->bytes[0x2f5]==0x23&&
        actor->bytes[0x2f6]==1&&!actor->bytes[0x2f7])))
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;++i)
   if(!player_heat_active_row(entry.bytes+16+i*0x2f0,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }
 if(c->request.bank==0&&c->request.index==0x4e){
  const gh_title_player_view *actor=c->request.actor;
  if(b->work->room!=0x102||!actor||!actor->bytes||actor->size<0x600||
     actor->original_vtable214!=0x41d220||(w(actor->bytes+0x2fc)>>16)!=0x100||
     w(actor->bytes+0x5fc)!=600||c->request.mode!=5||c->request.selector!=-1||c->count!=15)
   return fail(c,0x12c708,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;++i)
   if(!player_attack_boost_row(entry.bytes+16+i*0x2f0,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }
 if(c->configuration_2fa158)c->config_flags|=0xe;
 if(c->configuration_05288)c->config_flags|=2;
 c->config_flags|=c->configuration_nested_flags;
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
 if(c->request.bank==0x58&&c->request.index==0x50&&c->configuration_nested_scale){
  static const uint8_t original_types[]={0x1a,0,4,4,0x1a,0};
  if(b->work->room!=0x103||c->count!=sizeof(original_types)||!b->model||
     b->model->work!=b->work||b->model->root.bytes!=c->request.actor->bytes)
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(row[0x133]!=original_types[i]||!r103_paired_kick_row(row))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->request.bank==0x58&&(c->request.index==0x2b||c->request.index==0x46)&&c->configuration_nested_scale){
  static const uint8_t parts[]={25,24,21,20,24,21};
  static const uint8_t children[]={0x12,0x20,0x12,0x20,0x1a,0x1a};
  const int16_t room=wide_strike_effect_room(c->request.actor,c->request.index);
  if(c->count!=6||!room||!b->work||b->work->room!=room)
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<6;i++){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(!wide_strike_effect_row(row)||row[0xbe]!=parts[i]||row[0x133]!=children[i])
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 const unsigned pincer_rows=c->request.bank==0x58?r103_pincer_effect_rows(c->request.index):0;
 if(pincer_rows&&c->configuration_nested_scale){
  if(c->count!=pincer_rows||b->work->room!=0x103||!b->model||b->model->work!=b->work||
     !r103_pincer_effect_actor(c->request.actor,c->request.index)||
     b->model->root.bytes!=c->request.actor->bytes)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;i++)
   if(!r103_pincer_effect_row(entry.bytes+16+i*0x2f0,c->request.index,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }
 const unsigned slide_rows=c->request.bank==0x58?r103_em0a_slide_rows(c->request.index):0;
 if(slide_rows&&c->configuration_nested_scale){
  if(b->work->room!=0x103||c->count!=slide_rows)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;i++)
   if(!r103_em0a_slide_row(entry.bytes+16+i*0x2f0,c->request.index,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }
 if(c->request.bank==0x58&&(c->request.index==0x1d||c->request.index==0x1e||c->request.index==0x24)&&c->configuration_nested_scale){
  const unsigned index=c->request.index;
  static const uint8_t parts[]={16,16,10,10,0,0};
  static const uint8_t types[]={9,0x12,9,0x12,0,0};
  const int16_t room=em0a_attack_room(c->request.actor);
  if(!b->work||room!=b->work->room||c->count!=em0a_attack_effect_count(room,index))
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;++i){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(row[0xbe]!=parts[i]||row[0x133]!=types[i]||!em0a_attack_effect_row(row,index))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
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
 if(c->request.bank==0xa8&&(c->request.index==0xf||c->request.index==0x10)){
  const unsigned member=c->request.index;
  const gh_title_player_view *actor=c->request.actor;
  if(b->work->room!=0x104||c->request.selector!=-1||
     (member==0xf?(!em03_projectile_effect_actor(actor)||c->request.mode||
        c->configuration_positioned||c->count!=3||!b->model||
        b->model->work!=b->work||!b->model->parts_ready||b->model->root.bytes!=actor->bytes):
       (actor||c->request.mode!=-1||!c->configuration_positioned||c->count!=5)))
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;++i){
   if(!em03_projectile_est_row(member,entry.bytes+16+i*0x2f0,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->request.bank==0x58&&c->request.index==0x29){
  const gh_title_player_view *actor=c->request.actor;
  if(b->work->room!=0x100||!r100_em02_capture_actor(actor)||
     actor->bytes[0x2f4]||actor->bytes[0x2f5]!=0x18||actor->bytes[0x2f6]!=1||
     c->request.mode||c->request.selector!=-1||c->count!=3)
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;++i){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(!r100_em02_capture_row(row)||row[0x133]!=(i==1?0x1a:0))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->request.bank==0x58&&devil_melee_effect_index(c->request.index)&&c->configuration_nested_scale){
  if(c->count!=2||b->work->room!=0x100)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<2;++i)
   if(!devil_melee_effect_row(entry.bytes+16+i*0x2f0,i))return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }
 const unsigned devil_combo=c->request.bank==0x58&&c->configuration_nested_scale?
    devil_combo_effect_rows(c->request.index):0;
 if(devil_combo){
  if(c->count!=devil_combo||b->work->room!=0x100)
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<devil_combo;++i)
   if(!devil_combo_effect_row(c->request.index,entry.bytes+16+i*0x2f0,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
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
 if(c->request.bank==0x6c&&c->request.index==0x3b&&c->configuration_nested_scale){
  if(b->work->room!=0x103||c->count!=15)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;i++)
   if(!r103_boss_throw_effect_row(entry.bytes+16+i*0x2f0,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }
 if(c->request.bank==0&&c->request.index==0x35&&c->configuration_nested_scale){
  if(b->work->room!=0x103||c->count!=4)return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;i++)
   if(!em0a_grab_impact_row(entry.bytes+16+i*0x2f0,i))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
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
 if(c->configuration_esp10){
  const int cut12=c->request.bank==6;
  if(b->work->room!=2||!c->owner->event_registered_41c||!c->configuration_positioned||
     (cut12?(c->request.index!=13||c->count!=7):(c->request.bank!=1||c->request.index!=2||c->count!=4))||
     c->request.actor||c->request.mode!=-1||c->request.selector!=-1||
     c->configuration_nested_group!=1||c->configuration_nested_flags!=(cut12?6u:2u))
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  const uint8_t *r=entry.bytes+16+0x2f0;
  if(w(r)!=0x1ece0010u||w(r+4)!=0x00800000u||w(r+0xbc)!=0x00ff0301u||
     w(r+0xc0)!=0x00010000u||w(r+0x130)!=1u||w(r+0x134)!=0x00010000u||
     w(r+0x2a0)||r[0x128]||r[0x133])
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
 }else if(c->configuration_positioned&&c->request.bank==1){
  static const uint8_t counts[]={0,9,7,7,9,5};
  const unsigned index=c->request.index;
  if(b->work->room!=0x100||index<1||index>5||c->count!=counts[index]||c->request.actor||
     c->request.mode!=-1||c->request.selector!=-1||c->configuration_positioned_yaw)
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;++i){
   const uint8_t *r=entry.bytes+16+i*0x2f0;
   const int model=i+2>=c->count;
   if(w(r+4)!=0x00800000u||r[0xbe]!=0xff||r[0x128]||r[0x136]||
      r[0x133]!=(model?4:0)||
      (model?(w(r)!=0x1e3e0010u||r[0x12e]!=2||w(r+0x16c)!=2||w(r+0x170)):
       (w(r)!=0x1e3e8010u&&w(r)!=0x1efe8010u)))
    return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  }
 }
 if(c->configuration_positioned&&c->request.bank==0xa8&&c->request.index==3){
  if(b->work->room!=0x104||!b->model||!r104_charge_actor(&b->model->root)||
     b->model->root.bytes[0x2f5]!=0x45||c->count!=7)
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<c->count;++i)
   if(!r104_em03_ground_row(entry.bytes+16+i*0x2f0,i))return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
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
 if(c->configuration_positioned&&c->request.bank==0&&c->request.index==0xcc){
  static const uint8_t textures[]={0,0,2,203};
  if(!b->model||!b->model->root.bytes||b->model->root.size<0x300||
     b->model->root.original_vtable214!=0x425c78u||c->count!=sizeof(textures))
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  const unsigned kind=w(b->model->root.bytes+0x2fc)>>16;
  if(!((b->work->room==0x102&&kind==0x331)||(b->work->room==0x104&&kind==0x332)))
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<sizeof(textures);++i){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(w(row)!=(i==3?0x1ef46010u:0x1efee010u)||w(row+4)!=0x00c00000u||
      row[0xbe]!=0xff||row[0x128]||row[0x133]||row[0x136]||row[0x12e]!=textures[i])
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
 if(c->configuration_positioned&&c->request.bank==0&&c->request.index==0x40){
  static const uint8_t texture[]={11,57,51,1,76,15};
  static const uint32_t flags[]={0x1efe8010,0x1efe8010,0x1efee010,0x1efee010,0x1efee010,0x1e3ea010};
  if((b->work->room!=0x100&&b->work->room!=0x102&&b->work->room!=0x103)||c->count!=sizeof(texture))
   return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
  for(unsigned i=0;i<sizeof(texture);++i){
   const uint8_t *row=entry.bytes+16+i*0x2f0;
   if(w(row)!=flags[i]||w(row+4)!=(i==3?0x00a00000u:0x00800000u)||row[0xbe]!=0xff||
      row[0x128]||row[0x136]||row[0x133]!=(i==5?9:0)||row[0x12e]!=texture[i])
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
 if(c->configuration_part4_stun){
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
 if(c->configuration_death_part_zero==2){
  c->config_flags=c->configuration_nested_flags;
  if(c->request.index==4||c->request.index==8){
   const int dust=c->request.index==4;
   if((b->work->room!=0x103&&!(b->work->room==0x104&&r104_charge_actor(c->request.actor)))||
      c->count!=(dust?1u:2u))return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
   for(unsigned i=0;i<c->count;++i){
    const uint8_t *row=entry.bytes+16+i*0x2f0;
    if(row[0xbe]||row[0x128]||row[0x136]||row[0x133]!=(dust?0x1a:9)||
       w(row)!=(dust?0x1efe6010u:0x1e3e8010u)||w(row+4)!=(dust?0x00c00000u:0x00e00000u))
     return fail(c,0x3066d0,GH_EFFECT_UNSUPPORTED);
   }
  }
 }
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
 if(c->configuration_death_part_zero||c->configuration_held_impact_part_zero)p[0x1ec]=0;
 if(c->configuration_part4_stun)p[0x1ec]=4;
 if(c->configuration_esp10||(!c->configuration_nested_group&&c->configuration_nested_scale))
  ww(p+0x1e0,c->configuration_nested_scale);
 if(c->configuration_esp10)memcpy(p+0x1d0,c->configuration_esp10_color,16);
 if(!r[0x28e]){if(!b->random)return fail(c,0x31cfe0,GH_EFFECT_UNSUPPORTED);if(c->random_identity&&c->random_identity!=b->random)return GH_EFFECT_ARGUMENT;c->random_identity=b->random;uint32_t seed;if(gh_game_mt_next_0030f348(b->random,&seed))return fail(c,0x31cfe0,GH_EFFECT_ARGUMENT);ww(p+0x220,seed);}else ww(p+0x220,r[0x28e]);c->stage=3;return GH_EFFECT_OK;
}
static gh_effect_status initialize_positioned_transform(gh_effect_create_call *call,
 const gh_effect_create_bindings *bindings,uint8_t *payload)
{
 if(!bindings->atan_coeff)return fail(call,0x147c88,GH_EFFECT_UNSUPPORTED);
 if(call->configuration_positioned_atan_coeff&&call->configuration_positioned_atan_coeff!=bindings->atan_coeff)
  return fail(call,0x147c88,GH_EFFECT_ARGUMENT);
 float config_euler[3]={0},euler[3],rotation[16],translation[16],local[16],world[16];
 if(call->configuration_esp10)memcpy(config_euler,call->configuration_esp10_rotation,12);
 else memcpy(config_euler+1,&call->configuration_positioned_yaw,4);
 identity((uint8_t *)translation);
 memcpy(translation+12,call->configuration_positioned_position,12);
 identity((uint8_t *)world);
 /* Room bank 1 supplies the world offsets in EST, with identity config. */
 if(call->request.bank==1)memcpy(world+12,payload+0x40,12);

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
 int part=c->configuration_part4_stun?4:c->configuration_held_impact_part_zero?0:(int8_t)r[0xbe];if(part==-3){/* Original303A30 false -> original3005D8 marks pendingfree. */unsigned off=(c->slot>>5)*4;ww(b->storage.emitters+0x26020+off,w(b->storage.emitters+0x26020+off)|(0x80000000u>>(c->slot&31)));c->original_return=0;c->complete=1;return GH_EFFECT_OK;}
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
  for(unsigned k=0;k<3;k++){
   if(w(p+0x30+4*k)!=0)return fail(c,0x2b61b0,GH_EFFECT_UNSUPPORTED);
   if(!(c->configuration_positioned&&c->request.bank==1)&&w(p+0x40+4*k))
    return fail(c,0x2b61b0,GH_EFFECT_UNSUPPORTED);
  }
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
 if(part==-1&&c->request.bank==0&&c->request.index==0x21&&player_heat_active_row(r,5)){
  if(!b->model||!b->model->parts_ready||b->model->root.bytes!=m->actor->bytes)
   return fail(c,0x303a30,GH_EFFECT_UNSUPPORTED);
  m->heat_model=b->model;
 }
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
 return gh_esp03_destroy_002e3500(&view->heat);
}
void gh_effect_create_forget_on_owner_destroy(gh_game_effects*s){if(!s||!s->native)return;if(s->native->owner!=s)return;for(unsigned i=0;i<1024;i++)(void)release_trail(s->native->esps+i);free(s->native);s->native=NULL;}

int gh_effect_player_heat_root_emitter_is_bound(const gh_effect_emitter_view *view)
{
 if(!view||view->model||view->part||!view->actor||view->actor!=view->selector||
    !view->actor->bytes||view->actor->size<0x300||view->actor->original_vtable214!=0x41d220||
    (w(view->actor->bytes+0x2fc)>>16)!=0x100||!view->record.bytes||view->record.size!=0x2f0||
    !view->payload||view->payload_size!=0x260||w(view->payload+0x18c)!=0x00210000u||
    w(view->payload+0x110)!=3)return 0;
 return player_heat_active_row(view->record.bytes,5);
}

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
 const int16_t escape_room=em00_escape_effect_room(actor);
 if(actor==view->selector&&view->model&&
    (escape_room==0x100||(view->model->work&&
     ((escape_room==0x102&&view->model->work->room==0x102)||
      (view->model->work->room==0x103&&em00_escape_effect_actor(actor)))))&&
    view->model->root.bytes==actor->bytes&&view->record.size==0x2f0&&
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
 if((r102_gate_guard_effect_actor(actor)||r103_em0a_effect_actor(actor))&&actor==view->selector&&view->model&&view->model->work&&
    ((view->model->work->room==0x102&&r102_gate_guard_effect_actor(actor))||
     (view->model->work->room==0x103&&r103_em0a_effect_actor(actor)))&&view->model->root.bytes==actor->bytes&&view->model->part_count>4&&
    view->part==view->model->parts+4&&payload&&view->payload_size==0x260&&
    w(payload+0x18c)==0x00040000u&&w(payload+0x110)==3&&payload[0x1ec]==4)
  return gh_effect_player_stun_row(view->record);
 if(!actor||actor!=view->selector||!actor->bytes||actor->size<0x300||
    actor->original_vtable214!=0x41d220||(w(actor->bytes+0x2fc)>>16)!=0x100||
    !view->model||view->model->root.bytes!=actor->bytes||view->record.size!=0x2f0||
    !record||view->payload_size!=0x260||!payload)return 0;
 const uint32_t profile=w(payload+0x18c);
 if(profile==0x001900a8u&&w(payload+0x110)==UINT32_MAX&&view->model->work&&
    view->model->work->room==0x104&&view->model->part_count&&
    view->part==view->model->parts&&!payload[0x1ec]){
  for(unsigned i=0;i<6;++i)if(r104_held_impact_row(record,i))return 1;
  return 0;
 }
 if(profile==0x00210000u&&w(payload+0x110)==3&&view->model->work&&
    (view->model->work->room==0x100||view->model->work->room==0x102||
     view->model->work->room==0x103||view->model->work->room==0x104)){
  const unsigned part=record[0xbe];
  if(part>=view->model->part_count||view->part!=view->model->parts+part)return 0;
  for(unsigned i=0;i<10;++i)if(player_heat_active_row(record,i))return 1;
  return 0;
 }
 if(profile==0x00220000u&&w(payload+0x110)==UINT32_MAX&&view->model->work&&
    (view->model->work->room==0x100||view->model->work->room==0x102||view->model->work->room==0x103)){
  const unsigned part=record[0xbe];
  if(part>=view->model->part_count||view->part!=view->model->parts+part)return 0;
  for(unsigned i=0;i<21;++i)if(player_heat_activation_row(record,i))return 1;
  return 0;
 }
 if(profile==0x00280000u&&!w(payload+0x110)&&view->model->work&&
    (view->model->work->room==0x100||view->model->work->room==0x102||view->model->work->room==0x103)){
  const unsigned part=record[0xbe];
  if(part>=view->model->part_count||view->part!=view->model->parts+part)return 0;
  for(unsigned i=0;i<12;++i)if(player_heat_finish_row(record,i))return 1;
  return 0;
 }
 if(profile==0x004e0000u&&w(payload+0x110)==5&&view->model->work&&view->model->work->room==0x102){
  const unsigned part=record[0xbe];
  if(part>=view->model->part_count||view->part!=view->model->parts+part)return 0;
  for(unsigned i=0;i<15;++i)if(player_attack_boost_row(record,i))return 1;
  return 0;
 }
 if(profile==0x00250000u&&w(payload+0x110)==3&&view->model->work&&
    (view->model->work->room==0x100||view->model->work->room==0x102||view->model->work->room==0x103)){
  const unsigned part=record[0xbe],type=record[0x133];
  const uint32_t flags=w(record+4),header=w(record);
  if(part>=view->model->part_count||view->part!=view->model->parts+part||
     record[0x128]||record[0x136])return 0;
  /* CORE 25's original Heat-ready aura binds all eleven rows to Gene. */
  if(type==9)return (part==8||part==10)&&header==0x1ebea010u&&
     flags==0x00e02000u&&record[0x12e]==24;
  return !type&&record[0x12e]==14&&
     ((part==9&&header==0x1efee010u&&flags==0x00802000u)||
      ((part==7||part==8||part==10)&&header==0x1e3e8010u&&
       (flags==0x00800000u||flags==0x00802000u)));
 }
 /* PL00 114608 emits CORE0:0E from Gene's model part 9. The two
  * original class-0 rows use type-0 sprites and texture 11. */
 if(profile==0x000e0000u&&w(payload+0x110)==UINT32_MAX&&view->model->work&&
    (view->model->work->room==0x103||view->model->work->room==0x104)&&
    view->model->part_count>9&&view->part==view->model->parts+9&&
    record[0xbe]==9&&!record[0x128]&&!record[0x133]&&!record[0x136]&&
    record[0x12e]==11&&w(record+4)==0x00802000u)
  return (w(record)==0x1e320010u&&w(record+0xbc)==0xff090002u)||
         (w(record)==0x1e3e8010u&&w(record+0xbc)==0xff090001u);
 /* 129578's original CORE19 charge and 129630's CORE2E release
  * retain the canonical PL00 model and the authored positive parts. */
 if((profile==0x00190000u||profile==0x002e0000u)&&w(payload+0x110)==UINT32_MAX&&
    view->model->work&&(view->model->work->room==0x103||view->model->work->room==0x104)&&record[0xbe]<view->model->part_count&&
    view->part==view->model->parts+record[0xbe]&&!record[0x128]&&!record[0x136]){
  const unsigned part=record[0xbe],type=record[0x133],texture=record[0x12e];
  const uint32_t flags=w(record+4);
  if(profile==0x00190000u){
   if(type==7)return part==10&&!texture&&flags==0x00802000u;
   if(type==4)return (part==8||part==9)&&(texture==7||texture==8||texture==9)&&flags==0x00c02000u;
   if(type==0)return flags==0x00802000u&&
      ((part==9&&(texture==24||texture==20||texture==6))||(part==8&&(texture==1||texture==51)));
  }else{
   if(type==7)return part==10&&!texture&&flags==0x00800000u;
   if(type==9)return (part==8||part==9)&&texture==24&&flags==0x00e00000u;
   if(type==0)return (part==8||part==9||part==10)&&
      (texture==14||texture==0||texture==51||texture==6||texture==11);
  }
  return 0;
 }
 /* PL00 motion 0450 uses CORE39's original roulette launch sequence.
  * Positive part attachments use the existing typed part transform. */
 if(profile==0x00390000u&&!w(payload+0x110)&&view->model->work&&
    (view->model->work->room==0x103||view->model->work->room==0x104)&&
    record[0xbe]<view->model->part_count&&
    view->part==view->model->parts+record[0xbe]&&!record[0x128]&&!record[0x136]){
  const unsigned part=record[0xbe],type=record[0x133],texture=record[0x12e];
  if(type==7)return part==10&&!texture&&w(record+4)==0x00802000u&&
     (w(record)==0x1e3e8010u||w(record)==0x1e3e0010u);
  if(type==9)return (part==7||part==9||part==10)&&texture==24&&
     w(record)==0x1ebea010u&&w(record+4)==0x00e02000u;
  if(type==0)return (part==10&&(texture==43||texture==51)&&w(record+4)==0x00802000u)||
     (!part&&(texture==14||texture==18)&&w(record+4)==0x00c02000u);
  return !part&&((type==0x1a&&w(record+4)==0x00c02000u)||
                (type==0x14&&w(record+4)==0x00802000u));
 }
 if(!(profile&0xffffu)&&!w(payload+0x110)&&view->model->work&&
    view->model->work->room==0x103&&forward_evade_effect_row(profile>>16,record))
  return view->model->part_count>4&&view->part==view->model->parts+4;
 if(!(profile&0xffffu)&&(profile>>16)>=0x15&&(profile>>16)<=0x18&&
    w(payload+0x110)==UINT32_MAX){
  const unsigned part=record[0xbe];
  return player_dodge_effect_row(profile>>16,record)&&part<view->model->part_count&&
     view->part==view->model->parts+part;
 }
 if(w(payload+0x18c)==0x00040000u&&w(payload+0x110)==4&&payload[0x1ec]==4)
  return view->model->part_count>4&&view->part==view->model->parts+4&&gh_effect_player_stun_row(view->record);
 if(profile==0x003b006cu&&w(payload+0x110)==UINT32_MAX){
  const unsigned part=record[0xbe];
  if(!view->model->work||view->model->work->room!=0x103||
     part>=view->model->part_count||view->part!=view->model->parts+part)return 0;
  for(unsigned i=0;i<15;i++)if(r103_boss_throw_effect_row(record,i))return 1;
  return 0;
 }
 if(w(payload+0x18c)==0x00350000u&&w(payload+0x110)==UINT32_MAX){
  const unsigned part=record[0xbe];
  if(!view->model->work||view->model->work->room!=0x103||
     part>=view->model->part_count||view->part!=view->model->parts+part)return 0;
  for(unsigned i=0;i<4;i++)if(em0a_grab_impact_row(record,i))return 1;
  return 0;
 }
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
 if(!actor||!actor->bytes||actor->size<0x644)return 0;
 const unsigned kind=w(actor->bytes+0x2fc)>>16;
 const unsigned stamp=w(actor->bytes+0x640)&0xffffu;
 const uint32_t dependency=w(actor->bytes+0x564);
 if(kind==0x204)return dependency==0x203&&(stamp==6||stamp==7)&&actor->size>0x1650&&
     actor->bytes[0x1650]==24&&actor->original_vtable214==0x4479c8;
 if(kind!=0x202)return 0;
 const int appearance0=actor->size>0x1650&&actor->original_vtable214==0x4479c8&&!actor->bytes[0x1650];
 const int west_formation=actor->size>=0x16c8&&actor->original_vtable214==0x4479c8&&
     actor->bytes[0x1650]==2&&w(actor->bytes+0x16c4)==6&&((dependency==0x202&&stamp==4)||(dependency==0x203&&stamp==5));
 return west_formation||(dependency==0x203&&(stamp==19||((stamp==1||stamp==8)&&appearance0)))||
        (dependency==0x202&&appearance0&&
         (stamp==0x17||stamp==32||(stamp==0x19&&actor->size>=0x16c8&&w(actor->bytes+0x16c4)==2)));
}

int gh_effect_stamp19_attack_part_is_bound(const gh_effect_emitter_view *v)
{
 if(!v||!em02_em03_attack_actor(v->actor)||!v->model)return 0;
 /* The original EM02 and EM03 use 58:2D on part 15. */
 const unsigned stamp=w(v->actor->bytes+0x640)&0xffffu;
 int16_t room=0x102;
 if(stamp==1)room=0x105;
 else if(stamp==4||stamp==5||stamp==32)room=0x100;
 else if(stamp==6||stamp==7)room=0x103;
 if(stamp!=19&&(!v->model->work||v->model->work->room!=room))return 0;
 return v->selector==v->actor&&
        v->model->root.bytes==v->actor->bytes&&v->model->part_count>15&&
        v->part==v->model->parts+15&&v->record.bytes&&v->record.size==0x2f0&&
        v->payload&&v->payload_size==0x260&&w(v->payload+0x18c)==0x002d0058u&&
        v->record.bytes[0xbe]==15&&
        !v->record.bytes[0x128]&&!v->record.bytes[0x136]&&
        (v->record.bytes[0x133]==0x12||v->record.bytes[0x133]==9)&&
        !w(v->payload+0x110)&&v->payload[0x1ec]==0xff;
}

int gh_effect_r103_slide_camera_emitter_is_bound(const gh_effect_emitter_view *v)
{
 /* 303A30 keeps the actor selection but leaves the part absent for -1. */
 return v&&v->payload&&v->payload_size==0x260&&v->initialized&&
    r103_em0a_effect_actor(v->actor)&&v->actor==v->selector&&!v->model&&!v->part&&
    v->record.bytes&&v->record.size==0x2f0&&!w(v->payload+0x110)&&v->payload[0x1ec]==0xff&&
    w(v->payload+0x18c)==0x00180058u&&r103_em0a_slide_row(v->record.bytes,0x18,0);
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
 if(profile==0x00290058u)
  return v->model->work&&v->model->work->room==0x100&&r100_em02_capture_actor(v->actor)&&
     r100_em02_capture_row(v->record.bytes)&&v->model->part_count>25&&v->part==v->model->parts+25;
 if(profile==0x00500058u)
  return v->model->work&&v->model->work->room==0x103&&r103_paired_kick_actor(v->actor)&&
     v->model->part_count&&v->part==v->model->parts&&r103_paired_kick_row(v->record.bytes);
 const unsigned pincer_group=profile>>16;
 const unsigned pincer_rows=(profile&0xffffu)==0x58?r103_pincer_effect_rows(pincer_group):0;
 if(pincer_rows){
  const unsigned part=v->record.bytes[0xbe];
  if(!r103_pincer_boss(v->actor)||!v->model->work||v->model->work->room!=0x103||
     part>=v->model->part_count||v->part!=v->model->parts+part)return 0;
  for(unsigned i=0;i<pincer_rows;i++)
   if(r103_pincer_effect_row(v->record.bytes,pincer_group,i))return 1;
  return 0;
 }
 if(profile==0x002b0058u||profile==0x00460058u){
  const unsigned part=v->record.bytes[0xbe];
  const int16_t room=wide_strike_effect_room(v->actor,profile>>16);
  return room&&v->model->work&&v->model->work->room==room&&
         wide_strike_effect_row(v->record.bytes)&&part<v->model->part_count&&
         v->part==v->model->parts+part;
 }
 const unsigned slide_group=profile>>16;
 const unsigned slide_rows=(profile&0xffffu)==0x58?r103_em0a_slide_rows(slide_group):0;
 if(slide_rows){
  const unsigned part=v->record.bytes[0xbe];
  if(!v->model->work||v->model->work->room!=0x103||!r103_em0a_effect_actor(v->actor)||
     part>=v->model->part_count||v->part!=v->model->parts+part)return 0;
  for(unsigned i=0;i<slide_rows;i++)if(r103_em0a_slide_row(v->record.bytes,slide_group,i))return 1;
  return 0;
 }
 if(profile==0x001d0058u||profile==0x001e0058u||profile==0x00240058u){
  const int16_t room=em0a_attack_room(v->actor);
  const unsigned part=v->record.bytes[0xbe],index=profile>>16;
  return v->model->work&&room==v->model->work->room&&em0a_attack_effect_count(room,index)&&
         part<v->model->part_count&&v->part==v->model->parts+part&&
         em0a_attack_effect_row(v->record.bytes,index);
 }
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

static int roulette_light_row(const gh_game_effects *effects,gh_effect_view row,
 const gh_title_player_view *actor,const gh_title_player_view *selector)
{
 if((!r103_pincer_boss(actor)&&!r104_charge_actor(actor))||actor!=selector||!row.bytes||row.size!=0x2f0)return 0;
 const uint8_t *r=row.bytes;
 gh_effect_view est;
 return !gh_effect_est_at_00306030(effects,0,0x61,&est)&&est.bytes&&est.size>=16+0x2f0&&
    w(est.bytes+4)==1&&r==est.bytes+16&&w(r)==0x1e3e0010u&&w(r+4)==0x00800000u&&
    !r[0xbe]&&!r[0x128]&&r[0x133]==5&&!r[0x136]&&w(r+0x16c)==1&&w(r+0x170)==100&&
    w(r+0x13c)==0x3dcccccdu&&w(r+0x190)==0x00010001u&&w(r+0x194)==0x001e0000u;
}

int gh_effect_enemy_light_emitter_is_bound(const gh_game_effects *owner,
 const gh_effect_emitter_view *v)
{
 if(v&&v->initialized&&v->payload&&v->payload_size==0x260&&
    w(v->payload+0x18c)==0x00610000u&&w(v->payload+0x110)==6){
  return roulette_light_row(owner,v->record,v->actor,v->selector)&&
      v->model&&v->model->work&&
      ((v->model->work->room==0x103&&r103_pincer_boss(v->actor))||
       (v->model->work->room==0x104&&r104_charge_actor(v->actor)))&&v->model->root.bytes==v->actor->bytes&&v->model->part_count&&v->part==v->model->parts;
 }
 return v&&v->initialized&&v->payload&&v->payload_size==0x260&&
     (w(v->payload+0x18c)==0x000b0058u||w(v->payload+0x18c)==0x000c0058u)&&
     w(v->payload+0x110)==4&&
     enemy_light_row(owner,v->record,v->actor,v->selector)&&
     v->model&&v->model->work&&v->model->work->room==0x100&&
     v->model->root.bytes==v->actor->bytes&&v->model->part_count>4&&v->part==v->model->parts+4;
}

int gh_effect_enemy_light_esp_is_bound(const gh_game_effects *owner,const gh_effect_esp_view *v)
{
 if(v&&v->type==5&&v->payload&&v->payload_size==0x400&&
    w(v->payload+0x23a)==0x00610000u&&w(v->payload+0x10c)==6)
  return roulette_light_row(owner,v->record,v->actor,v->selector);
 return v&&v->type==0&&v->payload&&v->payload_size==0x400&&
     enemy_light_row(owner,v->record,v->actor,v->selector);
}

static int r100_west_death_owner(const gh_title_player_view *actor,const gh_boot_work *work)
{
 if(!work||work->room!=0x100||!actor||!actor->bytes||actor->size<0x16c8||
    actor->original_vtable214!=0x4479c8)return 0;
 const uint8_t *p=actor->bytes;
 const uint32_t dependency=w(p+0x564);
 const unsigned stamp=w(p+0x640)&0xffffu;
 const int formation=w(p+0x16c4)==6&&p[0x1650]==2&&
    ((dependency==0x200&&stamp==3)||(dependency==0x202&&stamp==4)||
     (dependency==0x203&&stamp==5));
 const int wave=w(p+0x16c4)==2&&!p[0x1650]&&
    ((dependency==0x202&&stamp==29)||(dependency==0x203&&stamp==30));
 /* AT03 stamp32 uses the same attached CORE58 knockdown rows from its
  * original EM02 death sequence, with no AT02 home mode. */
 const int at3=!p[0x1650]&&!w(p+0x16c4)&&
    ((dependency==0x202&&stamp==32)||(dependency==0x200&&stamp==31));
 const int north=dependency==0x203&&stamp==11&&p[0x1650]==5&&w(p+0x16c4)==6;
 return (w(p+0x2fc)>>16)==0x202&&(formation||wave||at3||north)&&
    (w(p+0x548)>>16)==0&&(p[0x2f4]==1||p[0x2f4]==2);
}

int gh_effect_knockdown_emitter_is_bound(const gh_effect_emitter_view *v)
{
 if(!v||!v->payload||v->payload_size!=0x260)return 0;
 const uint32_t program=w(v->payload+0x18c);
 const int recoil=program==0x00040058u||program==0x00080058u;
 if((!recoil&&program!=0x002f0058u&&program!=0x00300058u)||!v->initialized||
    !v->actor||!v->actor->bytes||v->actor->size<0x644||v->actor!=v->selector||
    !v->model||v->model->root.bytes!=v->actor->bytes||
    !v->record.bytes||v->record.size!=0x2f0||v->record.bytes[0x128]||v->record.bytes[0x136]||
    w(v->payload+0x110)||v->payload[0x1ec])return 0;
 const unsigned stamp=w(v->actor->bytes+0x640)&0xffffu;
 const uint32_t dependency=w(v->actor->bytes+0x564);
 const unsigned kind=w(v->actor->bytes+0x2fc)>>16;
 const int arena=kind==0x204&&v->actor->size>0x1650&&v->actor->bytes[0x1650]==24&&
    v->actor->original_vtable214==0x4479c8&&v->model->work&&v->model->work->room==0x103&&
    ((dependency==0x200&&(stamp==2||stamp==3))||(dependency==0x203&&(stamp==6||stamp==7))||
     r103_em0a_effect_actor(v->actor));
 /* Original 253DE8 attaches CORE 58:2F/30 to the same boss skeleton. */
 const int arena_boss=r103_pincer_boss(v->actor)&&v->model->work&&
    v->model->work->room==0x103;
 /* R104's 253DE8 uses the same attached rows as the arena boss.
  * Keep the original EM03 owner across the normal and mirrored entries. */
 const int r104_boss=v->model->work&&v->model->work->room==0x104&&r104_charge_actor(v->actor);
 if(recoil&&!arena_boss&&!r104_boss)return 0;
 if(!arena&&!arena_boss&&!r104_boss&&(kind!=0x202||
    !((stamp==19&&dependency==0x203)||(stamp==20&&dependency==0x200)||
      (program==0x002f0058u&&v->model->work&&v->model->work->room==0x100&&
       stamp8_attack_actor(v->actor))||r100_west_death_owner(v->actor,v->model->work))))return 0;
 const unsigned part=v->record.bytes[0xbe];
 return part<v->model->part_count&&v->part==v->model->parts+part;
}
/* The 240548 recovery and west 230F20 attack SEQ share the original
 * bank 58/index2 part-0 emitter with the knockdown/death paths. */
static int enemy_landing_emitter(const gh_effect_emitter_view *v)
{
 const gh_title_player_view *actor=v->actor;
 const gh_boot_work *work=v->model->work;
 if(!work||actor->size<=0x1650||actor->original_vtable214!=0x4479c8)return 0;
 const unsigned stamp=w(actor->bytes+0x640)&0xffffu,appearance=actor->bytes[0x1650];
 const uint32_t kind=w(actor->bytes+0x564),object=w(actor->bytes+0x2fc)>>16;
 if(work->room==0x100)
  return object==0x202&&kind==0x200&&stamp==3&&appearance==2&&actor->size>=0x16c8&&
     w(actor->bytes+0x16c4)==6&&w(v->payload+0x110)==UINT32_MAX&&
     v->record.bytes[0x133]==0x1a&&w(v->record.bytes+4)==0x00c00000u;
 if(work->room==0x103){
  const int boss=appearance==15&&stamp<2&&kind==0x214u+stamp;
  const int reinforcement=appearance==24&&
     ((kind==0x200&&(stamp==2||stamp==3))||(kind==0x203&&(stamp==6||stamp==7))||
      (kind==0x20a&&(stamp==4||stamp==5)));
  return object==0x204&&(boss||reinforcement)&&w(v->payload+0x110)==UINT32_MAX;
 }
 if(work->room==0x101&&object==0x202&&appearance==6){
  /* The first native side-arena wave uses the original 240548 dust SEQ. */
  return ((stamp==0x16&&kind==0x200)||(stamp==0x20&&kind==0x203))&&
     v->record.bytes[0x133]==0x1a&&w(v->record.bytes+4)==0x00c00000u&&
     w(v->payload+0x110)==UINT32_MAX;
 }
 if(work->room==0x104)
  return r104_charge_actor(actor)&&w(v->payload+0x110)==UINT32_MAX&&
     v->record.bytes[0x133]==0x1a&&w(v->record.bytes+4)==0x00c00000u;
 if(work->room!=0x102||object!=0x202)return 0;
 if(!appearance)return stamp==7&&kind==0x200;
 static const uint16_t dependency[]={0x202,0x200,0x203};
 return appearance==6&&stamp>=0x20&&stamp<=0x22&&kind==dependency[stamp-0x20];
}

int gh_effect_stamp19_death_part_is_bound(const gh_effect_emitter_view *v)
{
 if(!v||!v->actor||!v->actor->bytes||v->actor->size<0x644||v->selector!=v->actor||
    !v->model||v->model->root.bytes!=v->actor->bytes||!v->part||v->part!=v->model->parts||
    !v->record.bytes||v->record.size!=0x2f0||!v->payload||v->payload_size!=0x260||
    w(v->payload+0x18c)!=0x00020058u||v->payload[0x1ec]||
    v->record.bytes[0xbe]||v->record.bytes[0x128]||v->record.bytes[0x136])return 0;
 if(enemy_landing_emitter(v)||r100_west_death_owner(v->actor,v->model->work))return 1;

 const uint8_t *actor=v->actor->bytes;
 if((w(actor+0x2fc)>>16)!=0x202)return 0;
 const uint32_t dependency=w(actor+0x564);
 const unsigned stamp=w(actor+0x640)&0xffffu;
 const int gate_death=v->model->work&&v->model->work->room==0x102&&
    ((r102_gate_guard_effect_actor(v->actor)&&actor[0x2f4]==2)||
     em00_escape_effect_room(v->actor)==0x102||r102_gate_em0a_ground_actor(v->actor)||
     r102_em03_death_actor(v->actor))&&
    w(v->payload+0x110)==UINT32_MAX&&v->record.bytes[0x133]==0x1a&&
    w(v->record.bytes+4)==0x00c00000u;
 return (dependency==0x203&&stamp==19)||(dependency==0x200&&stamp==20)||
    (v->model->work&&v->model->work->room==0x100&&stamp8_attack_actor(v->actor))||gate_death||
    (dependency==0x20a&&!stamp&&v->actor->size>0x1650&&actor[0x1650]==3);
}

gh_effect_status gh_effect_create_emitter_at(const gh_game_effects*s,unsigned slot,gh_effect_emitter_view*out){
 if(!s||!out||slot>=256||!s->native||s->native->owner!=s)return GH_EFFECT_ARGUMENT;
 const struct gh_effect_native*n=s->native;const emitter_metadata*m=&n->slots[slot];uint32_t mask=0x80000000u>>(slot&31);if(!m->associated||m->quiesced||!(w(n->emitters+0x26000+(slot>>5)*4)&mask))return GH_EFFECT_UNSUPPORTED;
 const gh_actor_part*part=NULL;if(m->model){if(gh_actor_model_part_at(m->model,m->part_index,&part)||part->handle!=m->part_handle)return GH_EFFECT_UNSUPPORTED;}
 *out=(gh_effect_emitter_view){.record={m->record,0x2f0},.actor=m->actor,.selector=m->selector,.model=m->model,.part=part,.serial=m->serial,.initialized=m->initialized,.payload=n->emitters+(size_t)slot*0x260,.payload_size=0x260,.first_esp_child=m->first_esp_child};return GH_EFFECT_OK;
}

static int om307_esp04_row(const gh_game_effects *owner,uint32_t profile,gh_effect_view row)
{
 const unsigned index=profile>>16;
 if((profile&0xffffu)!=1||index<1||index>5||!row.bytes||row.size!=0x2f0)return 0;
 gh_effect_view est;
 if(lookup_est(owner,1,index,&est)||!est.bytes||est.size<16||w(est.bytes)!=0x545345u)return 0;
 const unsigned count=w(est.bytes+4);
 if(count<2||count>(est.size-16)/0x2f0||
    (row.bytes!=est.bytes+16+(count-2)*0x2f0&&row.bytes!=est.bytes+16+(count-1)*0x2f0))return 0;
 const uint8_t *r=row.bytes;
 return w(r)==0x1e3e0010u&&w(r+4)==0x00800000u&&r[0xbe]==0xff&&!r[0x128]&&
    r[0x12e]==2&&r[0x133]==4&&!r[0x136]&&w(r+0x16c)==2&&!w(r+0x170);
}

int gh_effect_r100_om307_esp04_is_bound(const gh_game_effects *owner,const gh_effect_esp_view *view)
{
 return owner&&view&&view->type==4&&!view->actor&&!view->selector&&view->payload&&
    view->payload_size==0x400&&w(view->payload+0x10c)==UINT32_MAX&&
    om307_esp04_row(owner,w(view->payload+0x23a),view->record);
}

static uint32_t esp_vtable(unsigned type){
 switch(type){
 case 0: return 0x44fad0u;
 case 1: return 0x44e090u;
 case 3: return GH_ESP03_VTABLE;
 case 4: return GH_ESP04_VTABLE;
 case 5: return 0x44e640u;
 case 6: return 0x44e6d8u;
 case 7: return 0x44e770u;
 case 0x1f: return 0x44f448u;
 case 8: return 0x44e820u;
 case 9: return 0x44e8c8u;
 case 0x10: return 0x44eab8u;
 case 0x12: return 0x44eb70u;
 case 0x13: return 0x44ec68u;
 case 0x14: return GH_ESP14_VTABLE;
 case 0x15: return 0x44ef08u;
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
/* The verified EV05 EL10 sprite rows differ in data, while all use the
 * same original 302BB4 or 302B58 attachment selected by the EST flag. */
static int ev05_el10_sprite_row(const uint8_t *record,unsigned part)
{
 static const struct {
  uint32_t header,flags,texture,timing,children;
  unsigned part;
 } known[]={
  {0x1eee8010u,0xc00000u,0x40701u,0x180000u,0x2u,4},
  {0x1e308010u,0x800000u,0xff1d0001u,0x10000u,0x1u,29},
  {0x1e3e8010u,0x800000u,0xff1d0001u,0x10000u,0x1u,29},
  {0x1eee0010u,0xc00000u,0x61d0801u,0x0u,0x2u,29},
  {0x1eee0010u,0x800000u,0x1d0801u,0x0u,0x2u,29},
  {0x1eee0010u,0xc00000u,0x40801u,0x73000fu,0x2u,4},
  {0x1eee0010u,0xc00000u,0x40801u,0xeb0000u,0x2u,4},
  {0x1eee0010u,0xc00000u,0x40002u,0x3d0037u,0x2u,4},
  {0x1eee0010u,0xc00000u,0x40201u,0x3c0032u,0x2u,4},
  {0x1eee0010u,0x800000u,0x40201u,0x3e0032u,0x2u,4},
  {0x1eae0010u,0xc00000u,0x11d0601u,0x0u,0x2u,29},
  {0x1ee60010u,0xc00000u,0x11d0101u,0x40000u,0x2u,29},
  {0x1eee0010u,0xc00000u,0x11d0101u,0x140012u,0x2u,29},
  {0x1e3e0010u,0x800000u,0xff1d0003u,0x30002u,0x1u,29},
  {0x1e3e8010u,0x800000u,0xff1d0002u,0x20001u,0x1u,29},
  {0x1e3e8010u,0x800000u,0xff1d0001u,0x40003u,0x1u,29},
  {0x1e308010u,0x800000u,0xff1d0001u,0x790078u,0x1u,29},
  {0x1e3e8010u,0x800000u,0xff1d0001u,0x790078u,0x1u,29},
  {0x1ebe8010u,0x800000u,0x1d0201u,0x7c0078u,0x1u,29},
  {0x1eee0010u,0x800000u,0x31d0801u,0xffff0085u,0x2u,29},
  {0x1eee8010u,0x800000u,0x61d0201u,0xffff0000u,0x2u,29},
  {0x1e3e8010u,0x800000u,0xff1d0002u,0x10000u,0x1u,29},
  {0x1eee0010u,0x800000u,0x41d0201u,0xffff0000u,0x2u,29},
  {0x1eee8010u,0x800000u,0x21d0201u,0x560030u,0x2u,29},
  {0x1eee8010u,0x800000u,0x61d0401u,0xffff0000u,0x2u,29},
 };
 if(w(record+0x134))return 0;
 for(unsigned i=0;i<sizeof(known)/sizeof(known[0]);++i){
  if(known[i].part==part&&known[i].header==w(record)&&known[i].flags==w(record+4)&&
     known[i].texture==w(record+0xbc)&&known[i].timing==w(record+0xc0)&&
     known[i].children==w(record+0x130))return 1;
 }
 return 0;
}

/* EV06 uses the authored EL10 and EL05 attachment points. The two
 * screen-space rows have no actor and are handled independently. */
static int authored_event_attached_row(const uint8_t *record,unsigned part,unsigned source)
{
 static const struct {
  uint32_t header,flags,texture,timing,children;
  unsigned part;
  uint16_t source;
 } rows[]={
  {0x1e308010u, 0x00800000u, 0xff1d0001u, 0x00010000u, 0x00000001u, 29, 0x610},
  {0x1e3e8010u, 0x00800000u, 0xff1d0001u, 0x00010000u, 0x00000001u, 29, 0x610},
  {0x1eee0010u, 0x00800000u, 0x001d0801u, 0xffff0000u, 0x00000002u, 29, 0x610},
  {0x1efe8010u, 0x00800000u, 0xff0a0001u, 0x00270026u, 0x00000001u, 10, 0x610},
  {0x1efe8010u, 0x00800000u, 0xff080001u, 0x00270026u, 0x00000001u, 8, 0x610},
  {0x1efe8010u, 0x00c00000u, 0x030a0102u, 0x002d0026u, 0x00000001u, 10, 0x610},
  {0x1efe8010u, 0x00800000u, 0xff0a0201u, 0xffff0026u, 0x00000001u, 10, 0x610},
  {0x1efe8010u, 0x00800000u, 0xff100001u, 0x00290028u, 0x00000001u, 16, 0x610},
  {0x1efe8010u, 0x00800000u, 0xff0e0001u, 0x00290028u, 0x00000001u, 14, 0x610},
  {0x1efe8010u, 0x00c00000u, 0x03100102u, 0x0032002bu, 0x00000001u, 16, 0x610},
  {0x1efe8010u, 0x00800000u, 0xff100201u, 0xffff002du, 0x00000001u, 16, 0x610},
  {0x1efe8010u, 0x00800000u, 0xff0a0001u, 0x00010000u, 0x00000001u, 10, 0x610},
  {0x1efe8010u, 0x00800000u, 0xff070001u, 0x00010000u, 0x00000001u, 7, 0x610},
  {0x1efe8010u, 0x00800000u, 0x050a0101u, 0x00180000u, 0x00000001u, 10, 0x610},
  {0x1efe8010u, 0x00800000u, 0xff0a0201u, 0xffff0000u, 0x00000001u, 10, 0x610},
  {0x1efe8010u, 0x00c00000u, 0xff0a0001u, 0x00010000u, 0x00000001u, 10, 0x610},
  {0x1e3e8010u, 0x00800000u, 0xff0a0001u, 0x00060005u, 0x09000001u, 10, 0x610},
  {0x1efe8010u, 0x00800000u, 0xff100001u, 0x00010000u, 0x00000001u, 16, 0x610},
  {0x1efe8010u, 0x00800000u, 0xff0d0001u, 0x00010000u, 0x00000001u, 13, 0x610},
  {0x1efe8010u, 0x00800000u, 0x05100101u, 0x00180000u, 0x00000001u, 16, 0x610},
  {0x1efe8010u, 0x00800000u, 0xff100201u, 0xffff0000u, 0x00000001u, 16, 0x610},
  {0x1efe8010u, 0x00c00000u, 0xff100001u, 0x00010000u, 0x00000001u, 16, 0x610},
  {0x1e3e8010u, 0x00800000u, 0xff100001u, 0x00060005u, 0x09000001u, 16, 0x610},
  {0x1eee0010u, 0x00800000u, 0x041d0601u, 0xffff0009u, 0x00000002u, 29, 0x610},
  {0x1efe8010u, 0x00800000u, 0x050a0101u, 0x00120000u, 0x00000001u, 10, 0x610},
  {0x1efe8010u, 0x00800000u, 0x05100301u, 0x00180000u, 0x00000001u, 16, 0x610},
  {0x1e3ea010u, 0x00800000u, 0xff1d0001u, 0x00060005u, 0x1f000001u, 29, 0x610},
  {0x1eee0010u, 0x00800000u, 0x001d0401u, 0xffff000au, 0x00000002u, 29, 0x610},
  {0x1eee0010u, 0x00c00000u, 0x001d0401u, 0xffff0000u, 0x00000002u, 29, 0x610},
  {0x1e3e8010u, 0x00800000u, 0xff150001u, 0x00090008u, 0x00000001u, 21, 0x605},
  {0x1e3e8010u, 0x00800000u, 0xff140001u, 0x00090008u, 0x00000001u, 20, 0x605},
  {0x1e3e8010u, 0x00800000u, 0xff130001u, 0x00090008u, 0x00000001u, 19, 0x605},
  {0x1ebea010u, 0x00e02000u, 0xff150a01u, 0x0028000au, 0x09000001u, 21, 0x605},
  {0x1ebea010u, 0x00e02000u, 0xff141901u, 0x0028000au, 0x09000001u, 20, 0x605},
  {0x1ebea010u, 0x00e02000u, 0xff130501u, 0x0028000au, 0x09000001u, 19, 0x605},
  {0x1ebea010u, 0x00e02000u, 0xff120401u, 0x0028000au, 0x09000001u, 18, 0x605},
  {0x1efe8010u, 0x00800000u, 0x00000102u, 0x000a0000u, 0x00000001u, 0, 0x610},
  {0x1e308010u, 0x00800000u, 0xff1d0001u, 0x00010000u, 0x00000001u, 29, 0x610},
  {0x1e3e8010u, 0x00800000u, 0xff1d0001u, 0x00010000u, 0x00000001u, 29, 0x610},
  {0x1eee0010u, 0x00800000u, 0x001d0401u, 0xffff000au, 0x00000002u, 29, 0x610},
  {0x1eee0010u, 0x00c00000u, 0x00040004u, 0x00730070u, 0x00000002u, 4, 0x610},
  {0x1eee0010u, 0x00c00000u, 0x00040001u, 0x00740070u, 0x00000002u, 4, 0x610},
  {0x1e3e8010u, 0x00800000u, 0xff1d0001u, 0x004f004eu, 0x00000001u, 29, 0x610},
  {0x1eee0010u, 0x00800000u, 0x001d0301u, 0xffff000au, 0x00000002u, 29, 0x610},
  {0x1eee0010u, 0x00800000u, 0x001d0201u, 0x01630154u, 0x00000002u, 29, 0x610},
  {0x1e3e0010u, 0x00802000u, 0xff150001u, 0x00290028u, 0x07000001u, 21, 0x605},
  {0x1efe8010u, 0x00c00000u, 0x0014000cu, 0x00310030u, 0x1a000001u, 20, 0x605},
  {0x1e3e0010u, 0x00802000u, 0xff000001u, 0x000d000cu, 0x07000001u, 0, 0x610},
  /* EV08 EST06 rows bind EL00 part 1 and EL59 part 0. */
  {0x1efe8010u, 0x00800000u, 0x00010801u, 0x001e0001u, 0x1a000001u, 1, 0x600},
  {0x1e3e0010u, 0x00800000u, 0xff000001u, 0x00010000u, 0x07000001u, 0, 0x659},
  {0x1e3e8010u, 0x00800000u, 0x00000001u, 0x00070006u, 0x00000001u, 0, 0x659},
  /* EV07 original cuts 5, 6 and 8. */
  {0x1ebea010u, 0x00800000u, 0xff1d0001u, 0x00180017u, 0x1f000001u, 29, 0x610},
  {0x1efe8010u, 0x00800000u, 0x00040201u, 0x003c0014u, 0x00000001u, 4, 0x610},
  {0x1ebe8010u, 0x00c00000u, 0x001d0400u, 0x00000000u, 0x00000001u, 29, 0x612},
  {0x1ebea010u, 0x00800000u, 0xff1d0001u, 0x001f001eu, 0x1f000001u, 29, 0x610},
  {0x1ebea010u, 0x00800000u, 0xff1d0001u, 0x00510050u, 0x1f000001u, 29, 0x610},
  {0x1ebea010u, 0x00800000u, 0xff1d0001u, 0x00250024u, 0x1f000001u, 29, 0x610},
  {0x1ece0010u, 0x00800000u, 0x00040004u, 0x004f004cu, 0x00000002u, 4, 0x610},
  {0x1e3e8010u, 0x00800000u, 0xff1d0001u, 0x00310030u, 0x00000001u, 29, 0x610},
  /* EV07 original cuts 12, 14, 15 and 18. */
  {0x1ebfa010u, 0x00800000u, 0xff1d1e01u, 0xffff0000u, 0x1f000001u, 29, 0x610},
  {0x1efe8010u, 0x00800000u, 0x00190002u, 0x004f004eu, 0x00000001u, 25, 0x615},
  {0x1efe8010u, 0x00800000u, 0x00190001u, 0x004f004eu, 0x00000001u, 25, 0x615},
  {0x1e3e0010u, 0x00800000u, 0xff0a0001u, 0x000a0009u, 0x07000001u, 10, 0x620},
  {0x1efe8010u, 0x00800000u, 0x00040401u, 0x0046000au, 0x00000001u, 4, 0x610},
  {0x1efe8010u, 0x00800000u, 0x00040401u, 0x001e0000u, 0x00000001u, 4, 0x610},
 };
 if(w(record+0x134))return 0;
 for(unsigned i=0;i<sizeof(rows)/sizeof(rows[0]);++i){
  if(rows[i].source==source&&rows[i].part==part&&rows[i].header==w(record)&&rows[i].flags==w(record+4)&&
     rows[i].texture==w(record+0xbc)&&rows[i].timing==w(record+0xc0)&&
     rows[i].children==w(record+0x130))return 1;
 }
 return 0;
}

/* EST06's part -1 keeps the event actor for lifetime but uses the
 * unattached cNode transform, as in 303A30 and 302B24. */
int gh_effect_ev05_partless_is_bound(gh_effect_view row,const gh_title_player_view *actor,
 const gh_title_player_view *selector)
{
 return row.bytes&&row.size==0x2f0&&actor&&!selector&&actor->bytes&&actor->size>=0x300&&
    actor->original_vtable214==0x44b528u&&(w(actor->bytes+0x2fc)>>16)==0x610u&&
    w(row.bytes)==0x1e3e8010u&&w(row.bytes+4)==0x00800000u&&
    w(row.bytes+0xbc)==0xffff0001u&&w(row.bytes+0xc0)==0x00070003u&&
    w(row.bytes+0x128)==0xff000100u&&w(row.bytes+0x130)==1u&&!w(row.bytes+0x134)&&
    w(row.bytes+0x2a0)==0x06100000u;
}

int gh_effect_event_part_is_bound(gh_effect_view row,const gh_title_player_view *actor,
 const gh_title_player_view *selector){
 if(!row.bytes||row.size!=0x2f0||!actor||(selector&&selector!=actor)||!actor->bytes||actor->size<0x300||
    actor->original_vtable214!=0x44b528u||row.bytes[0x136])return 0;
 unsigned kind=actor->bytes[0x2fe]|(unsigned)actor->bytes[0x2ff]<<8;
 const uint32_t row_actor=w(row.bytes+0x2a0);
 const unsigned source=row_actor>>16;
 const unsigned part=row.bytes[0xbe];
 /* The original event lookup retries EL05 as EL00 when EL05 is absent. */
 if(!selector&&!(row_actor&0xffffu)&&((source==0x605&&(kind==0x605||kind==0x600))||
     ((source==0x600||source==0x610||source==0x612||source==0x615||source==0x620||source==0x659)&&source==kind))&&authored_event_attached_row(row.bytes,part,source))return 1;
 if(row_actor!=(uint32_t)kind<<16)return 0;
 const uint32_t flags=w(row.bytes+4);
 /* R002 EFF bank 1 entry 2 rows 0, 2 and 3 bind EV07 EL10 part 29.
  * The other authored row uses group 1 and is excluded by SetEffect. */
 if(kind==0x610&&part==29&&flags==0x00800000u&&!w(row.bytes+0x134)){
  const uint32_t header=w(row.bytes),texture=w(row.bytes+0xbc),timing=w(row.bytes+0xc0);
  if(header==0x1e3e0010u&&texture==0x001d0301u&&timing==0xffff0000u&&
     w(row.bytes+0x130)==0x10000001u)return 1;
  if(timing==0x00010000u&&w(row.bytes+0x130)==1u&&
     ((header==0x1e008010u&&texture==0xff1d0004u)||
      (header==0x1e0e0010u&&texture==0xff1d0001u)))return 1;
 }
 if(selector)return 0;
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
  if(kind==0x610&&ev05_el10_sprite_row(row.bytes,part))return 1;
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
     !w(row.bytes+0x134)&&(timing==0x000a0009u||timing==0x00070006u||timing==0x00060005u||timing==0x001f001eu||timing==0x00010000u);
 }
 /* EV05 EST05/06 retain the original ESP31 trail on EL10 part29. */
 if(kind==0x610&&part==29&&flags==0x00800000u&&row.bytes[0x133]==0x1f&&
    w(row.bytes)==0x1ebea010u&&w(row.bytes+0xbc)==0xff1d0001u&&
    w(row.bytes+0x130)==0x1f000001u&&!w(row.bytes+0x134)){
  const uint32_t timing=w(row.bytes+0xc0);
  return timing==0x001f001eu||timing==0x004c004bu||timing==0x00010000u;
 }
 /* EV05 EST03 row 6 begins at authored cut 2/frame 242. */
 if(kind==0x610&&part==2&&row.bytes[0x133]==7&&flags==0x00802000u)
  return w(row.bytes)==0x1e3e0010u&&w(row.bytes+0xbc)==0xff020001u&&
     w(row.bytes+0xc0)==0x00f300f2u&&w(row.bytes+0x130)==0x07000001u&&!w(row.bytes+0x134);
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
 const unsigned combo=devil_combo_effect_rows(index);
 const unsigned expected=index==3?8:index==0x3f?23:index==0x40?4:
    devil_melee_effect_index(index)?2:combo?combo:devil_ranged_effect_rows(index);
 if((tag&0xffffu)!=0x58||!expected||!row.bytes||row.size!=0x2f0||
    row.bytes[0x128]||row.bytes[0x136])return 0;
 gh_effect_view est;
 if(gh_effect_est_at_00306030(owner,0x58,index,&est)||!est.bytes||est.size<16)return 0;
 const unsigned count=w(est.bytes+4);
 if(count!=expected||count>(est.size-16)/0x2f0)return 0;
 for(unsigned i=0;i<count;i++)if(row.bytes==est.bytes+16+i*0x2f0)
  return combo?devil_combo_effect_row(index,row.bytes,i):
      !devil_melee_effect_index(index)||devil_melee_effect_row(row.bytes,i);
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

/* Original 2022D0 creates A8:1 on the R104 stamp-zero boss. */
static int em03_spawn_row(const gh_game_effects *owner,gh_effect_view row,
 const gh_title_player_view *actor,const gh_title_player_view *selector)
{
 if(!actor||actor!=selector||!actor->bytes||actor->size<0x1652||
    actor->original_vtable214!=0x4479c8||(w(actor->bytes+0x2fc)>>16)!=0x203||
    w(actor->bytes+0x564)!=0x250||w(actor->bytes+0x1644)!=0x80000||
    actor->bytes[0x1650]||actor->bytes[0x1651]||!row.bytes||row.size!=0x2f0||
    w(row.bytes+4)!=0x00800000u||row.bytes[0xbe]!=29||
    row.bytes[0x133]||row.bytes[0x136])return 0;
 gh_effect_view est;
 if(gh_effect_est_at_00306030(owner,0xa8,1,&est)||!est.bytes||
    est.size<16+3*0x2f0||w(est.bytes+4)!=3)return 0;
 for(unsigned i=0;i<3;i++)
  if(row.bytes==est.bytes+16+i*0x2f0)return 1;
 return 0;
}

static int em03_charge_row(const gh_game_effects *owner,gh_effect_view row,
 const gh_title_player_view *actor,const gh_title_player_view *selector,unsigned index)
{
 if(actor!=selector||!r104_charge_actor(actor)||!row.bytes||row.size!=0x2f0)return 0;
 gh_effect_view est;
 return !gh_effect_est_at_00306030(owner,0x58,index,&est)&&est.bytes&&
    est.size>=16+0x2f0&&w(est.bytes+4)==1&&row.bytes==est.bytes+16&&
    w(row.bytes)==0x1e3e8010&&w(row.bytes+4)==0x00c00000&&!row.bytes[0xbe]&&
    row.bytes[0x133]==0x1a&&!row.bytes[0x128]&&!row.bytes[0x136];
}

static int em03_sequence_effect_bound(const gh_game_effects *owner,gh_effect_view row,
 const gh_title_player_view *actor,const gh_title_player_view *selector,uint32_t tag)
{
 const unsigned member=tag>>16,count=em03_sequence_effect_count(member);
 if((tag&0xffffu)!=0xa8||!count||actor!=selector||!r104_charge_actor(actor)||
    !row.bytes||row.size!=0x2f0)return 0;
 gh_effect_view est;
 if(gh_effect_est_at_00306030(owner,0xa8,member,&est)||!est.bytes||
    est.size<16+count*0x2f0||w(est.bytes+4)!=count)return 0;
 for(unsigned i=0;i<count;++i){
  if(row.bytes==est.bytes+16+i*0x2f0)return em03_sequence_effect_row(member,row.bytes,i);
 }
 return 0;
}

static int em03_projectile_row_bound(const gh_game_effects *owner,gh_effect_view row,
 const gh_title_player_view *actor,const gh_title_player_view *selector)
{
 if(actor!=selector||!em03_projectile_effect_actor(actor)||!row.bytes||row.size!=0x2f0)return 0;
 gh_effect_view est;
 if(gh_effect_est_at_00306030(owner,0xa8,0xf,&est)||!est.bytes||
    est.size<16+3*0x2f0||w(est.bytes+4)!=3)return 0;
 for(unsigned i=0;i<3;++i){
  if(row.bytes==est.bytes+16+i*0x2f0)return em03_projectile_est_row(0xf,row.bytes,i);
 }
 return 0;
}

static int em03_melee_row(const gh_game_effects *owner,gh_effect_view row,
 const gh_title_player_view *actor,const gh_title_player_view *selector,unsigned member)
{
 const unsigned count=em03_melee_est_lines(member);
 if(actor!=selector||!r104_charge_actor(actor)||!row.bytes||row.size!=0x2f0||!count)return 0;
 gh_effect_view est;
 if(gh_effect_est_at_00306030(owner,0xa8,member,&est)||!est.bytes||
    est.size<16+count*0x2f0||w(est.bytes+4)!=count)return 0;
 const uint8_t *p=row.bytes;
 if(w(p)!=0x1efe8010u||w(p+4)!=0x00c00000u||
    w(p+0x128)!=0xff000000u||w(p+0x130)!=0x12000001u||w(p+0x134))return 0;
 if(member==6)
  return p==est.bytes+16&&w(p+0xbc)==0x00150001u&&w(p+0xc0)==0x00120010u;
 if(member==7)
  return p==est.bytes+16&&w(p+0xbc)==0x000a0001u&&w(p+0xc0)==0x00060004u;
 if(member==9)
  return p==est.bytes+16&&w(p+0xbc)==0x000a0001u&&w(p+0xc0)==0x000a0008u;
 if(member==0xa)
  return p==est.bytes+16&&w(p+0xbc)==0x000a0201u&&w(p+0xc0)==0x00190012u;
 if(member==0x1a)
  return (p==est.bytes+16&&w(p+0xbc)==0x000a0201u&&w(p+0xc0)==0x000c000au)||
         (p==est.bytes+16+0x2f0&&w(p+0xbc)==0x00100201u&&w(p+0xc0)==0x00130011u);
 return (p==est.bytes+16&&p[0xbe]==10&&w(p+0xc0)==0x000e000cu)||
        (p==est.bytes+16+0x2f0&&p[0xbe]==16&&w(p+0xc0)==0x001c001au);
}

/* em03.dat SEQ 2DBBC0 frame 1 selects four original type-0 EST lines. */
static int em03_selector21_row(const gh_game_effects *owner,gh_effect_view row,
 const gh_title_player_view *actor,const gh_title_player_view *selector)
{
 if(actor!=selector||!r104_charge_actor(actor)||!row.bytes||row.size!=0x2f0)return 0;
 gh_effect_view est;
 if(gh_effect_est_at_00306030(owner,0xa8,0x13,&est)||!est.bytes||
    est.size<16+4*0x2f0||w(est.bytes+4)!=4)return 0;
 const uint8_t *p=row.bytes;
 static const uint32_t clip[]={0x00040003u,0x0026000du,0x0029000du,0x0021000du};
 for(unsigned i=0;i<4;i++){
  if(p!=est.bytes+16+i*0x2f0)continue;
  return w(p)==(i?0x1efe8010u:0x1e3e8010u)&&w(p+4)==(i?0x00c00000u:0x00800000u)&&
      w(p+0xbc)==(i?0x05040001u:0xff1d0001u)&&w(p+0xc0)==clip[i]&&
      w(p+0x128)==(i?0xff000000u:0xff000100u)&&w(p+0x130)==1u&&!w(p+0x134)&&
      p[0x12e]==(i?132u:14u)&&!w(p+0x268)&&w(p+0x290)==0x01010102u;
 }
 return 0;
}

int gh_effect_em03_emitter_is_bound(const gh_game_effects *owner,const gh_effect_emitter_view *v)
{
 if(!v||!v->initialized||!v->payload||v->payload_size!=0x260)return 0;
 const uint32_t tag=w(v->payload+0x18c),mode=w(v->payload+0x110);
 if(((tag==0x001a0058u&&(mode==UINT32_MAX||mode==0))||
     (tag==0x001c0058u&&mode==UINT32_MAX))&&
    em03_charge_row(owner,v->record,v->actor,v->selector,tag>>16))
  return v->model&&v->model->root.bytes==v->actor->bytes&&v->model->part_count&&
    v->part==v->model->parts;
 if(!mode&&(em03_sequence_effect_bound(owner,v->record,v->actor,v->selector,tag)||
    ((tag&0xffffu)==0xa8&&em03_melee_row(owner,v->record,v->actor,v->selector,tag>>16))||
    (tag==0x001300a8u&&em03_selector21_row(owner,v->record,v->actor,v->selector)))){
  const unsigned part=v->record.bytes[0xbe];
  return v->model&&v->model->root.bytes==v->actor->bytes&&v->model->part_count>part&&
    v->part==v->model->parts+part;
 }
 if(tag==0x000f00a8u&&!mode&&em03_projectile_row_bound(owner,v->record,v->actor,v->selector))
  return v->model&&v->model->root.bytes==v->actor->bytes&&v->model->part_count&&
    v->part==v->model->parts;
 return tag==0x000100a8u&&w(v->payload+0x110)==1&&
    em03_spawn_row(owner,v->record,v->actor,v->selector)&&
    v->model&&v->model->root.bytes==v->actor->bytes&&v->model->part_count>29&&
    v->part==v->model->parts+29;
}

int gh_effect_em03_esp_is_bound(const gh_game_effects *owner,const gh_effect_esp_view *v)
{
 return v&&((v->type==0&&(em03_spawn_row(owner,v->record,v->actor,v->selector)||
                         em03_selector21_row(owner,v->record,v->actor,v->selector)))||
    (v->type==0x1a&&(em03_charge_row(owner,v->record,v->actor,v->selector,0x1a)||
                    em03_charge_row(owner,v->record,v->actor,v->selector,0x1c)))||
    (v->type==0&&em03_projectile_row_bound(owner,v->record,v->actor,v->selector))||
    (v->payload&&v->payload_size==0x400&&!w(v->payload+0x10c)&&
     em03_sequence_effect_bound(owner,v->record,v->actor,v->selector,w(v->payload+0x23a))&&
     v->type==v->record.bytes[0x133]));
}

static int sprite_binding(gh_effect_view row,const gh_title_player_view *actor,
 const gh_title_player_view *selector){
 return gh_esp14_nested_sprite_is_bound(row,actor,selector)||gh_effect_event_part_is_bound(row,actor,selector)||
    gh_effect_ev05_partless_is_bound(row,actor,selector)||
        gh_effect_r101_door_break_part_is_bound(row,actor,selector)||
        gh_effect_r200_catapult_part_is_bound(row,actor,selector);
}

static int authenticated_esp(const gh_game_effects *e,const gh_effect_esp_view *v,int player_attachment)
{
 if(!e||!e->native||e->native->owner!=e||!v||!v->payload||
    (v->type!=0&&v->type!=9&&v->type!=0x1a&&!(player_attachment&&(v->type==3||v->type==4||v->type==7||v->type==0x12||v->type==0x14||v->type==0x15||v->type==0x1f))))return 0;
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
 if(!v||!v->payload||v->payload_size!=0x400||v->actor!=v->selector||
    !v->record.bytes||v->record.size!=0x2f0)return 0;
 const uint32_t profile=w(v->payload+0x23a);
 if(profile==0x00500058u&&r103_paired_kick_actor(v->actor)){
  gh_effect_view est;
  if(gh_effect_est_at_00306030(owner,0x58,0x50,&est)||!est.bytes||
     est.size<16+6*0x2f0||w(est.bytes+4)!=6)return 0;
  for(unsigned i=0;i<6;i++)
   if(v->record.bytes==est.bytes+16+i*0x2f0)
    return v->type==v->record.bytes[0x133]&&r103_paired_kick_row(v->record.bytes);
  return 0;
 }
 const unsigned pincer_group=profile>>16;
 const unsigned pincer_rows=(profile&0xffffu)==0x58?r103_pincer_effect_rows(pincer_group):0;
 if(pincer_rows&&r103_pincer_boss(v->actor)){
  gh_effect_view est;
  if(gh_effect_est_at_00306030(owner,0x58,pincer_group,&est)||
     !est.bytes||est.size<16+pincer_rows*0x2f0||w(est.bytes+4)!=pincer_rows)return 0;
  for(unsigned i=0;i<pincer_rows;i++)
   if(v->record.bytes==est.bytes+16+i*0x2f0)
    return v->type==v->record.bytes[0x133]&&r103_pincer_effect_row(v->record.bytes,pincer_group,i);
  return 0;
 }
 const unsigned slide_group=profile>>16;
 const unsigned slide_rows=(profile&0xffffu)==0x58?r103_em0a_slide_rows(slide_group):0;
 if(slide_rows&&r103_em0a_effect_actor(v->actor)){
  gh_effect_view est;
  if(gh_effect_est_at_00306030(owner,0x58,slide_group,&est)||
     !est.bytes||est.size<16+slide_rows*0x2f0||w(est.bytes+4)!=slide_rows)return 0;
  for(unsigned i=0;i<slide_rows;i++)
   if(v->record.bytes==est.bytes+16+i*0x2f0)
    return v->type==v->record.bytes[0x133]&&r103_em0a_slide_row(v->record.bytes,slide_group,i);
  return 0;
 }
 const int capture=profile==0x00290058u&&
     r100_em02_capture_actor(v->actor)&&(v->type==0||v->type==0x1a)&&
     r100_em02_capture_row(v->record.bytes);
 if(!capture&&!em00_family_attack_actor(v->actor)&&!em0a_attack_room(v->actor))return 0;
 const unsigned index=v->payload[0x23c]|((unsigned)v->payload[0x23d]<<8);
 if(!capture&&v->type!=9&&v->type!=0x12&&v->type!=0x1a&&v->type!=0x20&&(index!=0x1d||v->type))return 0;
 gh_effect_emitter_view emitter;
 if(!gh_effect_create_emitter_at(owner,v->emitter_slot,&emitter)&&emitter.serial==v->emitter_serial)
    return emitter.actor==v->actor&&emitter.selector==v->selector&&
       emitter.record.bytes==v->record.bytes&&gh_effect_em00_attack_emitter_is_bound(&emitter);
 if(capture){
  gh_effect_view est;
  if(w(v->payload+0x23a)!=0x00290058u||gh_effect_est_at_00306030(owner,0x58,0x29,&est)||!est.bytes||
     est.size<16+3*0x2f0||w(est.bytes+4)!=3)return 0;
  for(unsigned i=0;i<3;++i)
   if(v->record.bytes==est.bytes+16+i*0x2f0)
    return v->type==v->record.bytes[0x133]&&r100_em02_capture_row(v->record.bytes);
  return 0;
 }
 if((profile&0xffffu)==0x58&&wide_strike_effect_room(v->actor,index)){
  gh_effect_view est;
  if(gh_effect_est_at_00306030(owner,0x58,index,&est)||!est.bytes||
     est.size<16+6*0x2f0||w(est.bytes+4)!=6)return 0;
  for(unsigned i=0;i<6;i++)
   if(v->record.bytes==est.bytes+16+i*0x2f0)
    return v->record.bytes[0x133]==v->type&&wide_strike_effect_row(v->record.bytes);
  return 0;
 }
 if(index==0x1d||index==0x1e||index==0x24){
  const unsigned count=em0a_attack_effect_count(em0a_attack_room(v->actor),index);
  gh_effect_view est;
  if(!count||gh_effect_est_at_00306030(owner,0x58,index,&est)||
     !est.bytes||est.size<16+count*0x2f0||w(est.bytes+4)!=count)return 0;
  for(unsigned i=0;i<count;++i)
   if(v->record.bytes==est.bytes+16+i*0x2f0)
    return v->record.bytes[0x133]==v->type&&em0a_attack_effect_row(v->record.bytes,index);
  return 0;
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
 if(gh_effect_em00_attack_esp_is_bound(owner,v)||stamp19_attack_esp_bound(owner,v)||
    r101_weapon_esp_bound(owner,v)||
    (v&&(v->type==9||v->type==0x12)&&gh_effect_em03_esp_is_bound(owner,v)))return 1;
 if(!v||v->type!=0x12||!v->payload||v->payload_size!=0x400)return 0;
 const uint32_t profile=w(v->payload+0x23a);
 return (profile&0xffffu)==0xa8&&em03_melee_row(owner,v->record,v->actor,v->selector,profile>>16);
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
 const uint8_t *r=source.record.bytes;
 const unsigned child=r[0x133];
 const uint32_t attack_profile=w(source.payload+0x18c);
 const int attack=(((attack_profile&0xffffu)==0x58&&
    (r103_em0a_slide_rows(attack_profile>>16)||r103_pincer_effect_rows(attack_profile>>16)))||
    child==9||child==0x12||child==0x1a||child==0x20||
    attack_profile==0x00500058u||(!child&&(attack_profile==0x001d0058u||attack_profile==0x00290058u)))&&gh_effect_em00_attack_emitter_is_bound(&source);
 const int knock=(r[0x133]==0||r[0x133]==9||
    (r[0x133]==0x1a&&(((r[0xbe]==25||r[0xbe]==21)&&attack_profile==0x002f0058u)||
     (r[0xbe]==25&&attack_profile==0x00300058u)||
     (!r[0xbe]&&attack_profile==0x00040058u))))&&
    gh_effect_knockdown_emitter_is_bound(&source);
 const int death_ground=r[0x133]==0x1a&&gh_effect_stamp19_death_part_is_bound(&source);
 const int player_attachment=gh_effect_player_attachment_emitter_is_bound(&source);
 const int player_heat_root=gh_effect_player_heat_root_emitter_is_bound(&source);
 const int stamp19=gh_effect_stamp19_attack_part_is_bound(&source);
 const int weapon=gh_effect_r101_weapon_emitter_is_bound(&source);
 const int emad=gh_effect_emad_emitter_is_bound(e,&source)||
     gh_effect_em03_emitter_is_bound(e,&source);
 const int enemy_light=gh_effect_enemy_light_emitter_is_bound(e,&source);
 const int event_part=enemy_light||emad||stamp19||weapon||player_attachment||
    attack||knock||death_ground||gh_effect_event_part_is_bound(source.record,source.actor,source.selector)||
    gh_effect_r101_door_break_part_is_bound(source.record,source.actor,source.selector)||
    gh_effect_r200_catapult_part_is_bound(source.record,source.actor,source.selector);
 const int devil=gh_effect_devil_emitter_is_bound(e,&source);
 const int bound=devil||enemy_light||emad||stamp19||weapon||player_attachment||attack||knock||death_ground||player_heat_root||gh_effect_r103_slide_camera_emitter_is_bound(&source)||
    sprite_binding(source.record,source.actor,source.selector);
 if((!bound&&(source.actor||source.selector))||((event_part||devil)?(!source.model||!source.part):(source.model||source.part))){*f=0x2faa88;return GH_EFFECT_UNSUPPORTED;}
 unsigned i;for(i=0;i<1024;i++)if(!(w(b->esp+0x100000+(i>>5)*4)&(UINT32_C(0x80000000)>>(i&31))))break;if(i==1024)return GH_EFFECT_OK;
 unsigned type=source.record.bytes[0x133];uint32_t vtable=esp_vtable(type);
 const int event_trail=type==0x1f&&(attack_profile==0x00050006u||attack_profile==0x00060006u||
    attack_profile==0x00070006u||attack_profile==0x00090006u||attack_profile==0x000d0006u)&&
    gh_effect_event_part_is_bound(source.record,source.actor,source.selector);
 const int catapult=attack_profile==0x00130000u&&!w(source.payload+0x110)&&
     gh_effect_r200_catapult_part_is_bound(source.record,source.actor,source.selector);
 int catapult_impact_gradient=0;
 if(type==1&&attack_profile==0x00140000u&&w(source.payload+0x110)==UINT32_MAX&&
    !source.actor&&!source.selector&&r[0xbe]==0xff){
  gh_effect_view est;
  if(!gh_effect_est_at_00306030(e,0,0x14,&est)&&est.size>=16+9*0x2f0&&w(est.bytes+4)==9){
   const uint8_t *rows=est.bytes+16;
   catapult_impact_gradient=r==rows+2*0x2f0||r==rows+3*0x2f0||
       r==rows+4*0x2f0||r==rows+8*0x2f0;
  }
 }
 if(type==1&&!catapult&&!catapult_impact_gradient&&!(attack_profile==0x00010003u&&w(source.payload+0x110)==0x2201u&&
    !source.actor&&!source.selector&&r[0xbe]==0xff&&r[0x12e]==132&&
    (w(r)&~0x8000u)==0x1efe0010u&&w(r+4)==0x00800000u&&
    gh_effect_ambient_row_is_bound(e,1,source.record))){
  *f=0x2faca8;return GH_EFFECT_UNSUPPORTED;
 }
 if(!vtable||(type==3&&!player_heat_root)||(type==0x1f&&!devil&&!event_trail&&!(player_attachment&&attack_profile==0x00210000u&&
    (player_heat_active_row(r,8)||player_heat_active_row(r,9))))){*f=0x2faca8;return GH_EFFECT_UNSUPPORTED;}
 const int paired_kick=attack&&attack_profile==0x00500058u&&r103_paired_kick_row(r);
 const int roulette_charge=player_attachment&&w(source.payload+0x18c)==0x00190000u&&
    w(source.payload+0x110)==UINT32_MAX&&(r[0xbe]==8||r[0xbe]==9);
 const int om307_child=type==4&&!source.actor&&!source.selector&&
     w(source.payload+0x110)==UINT32_MAX&&om307_esp04_row(e,attack_profile,source.record);
 const int held_impact=player_attachment&&attack_profile==0x001900a8u&&r104_held_impact_row(r,5);
 if(type==4&&!paired_kick&&!roulette_charge&&!om307_child&&!held_impact){
  const uint32_t tag=w(source.payload+0x18c);
  const int paired_throw=tag==0x003b006cu&&
      (r103_boss_throw_effect_row(r,10)||r103_boss_throw_effect_row(r,11));
  int heat_activation=0;
  if(tag==0x00220000u)
   for(unsigned row=14;row<18;++row)heat_activation|=player_heat_activation_row(r,row);
  if(!player_attachment||w(source.payload+0x110)!=UINT32_MAX||(!paired_throw&&!heat_activation)){
   *f=0x2faca8;return GH_EFFECT_UNSUPPORTED;
  }
 }
 if(type==0x15){
  const uint32_t tag=w(source.payload+0x18c),mode=w(source.payload+0x110);
  const int boost=tag==0x004e0000u&&mode==5&&player_attack_boost_row(r,9);
  const int paired_throw=tag==0x003b006cu&&mode==UINT32_MAX&&
    (r103_boss_throw_effect_row(r,12)||r103_boss_throw_effect_row(r,13));
  const int heat_activation=tag==0x00220000u&&mode==UINT32_MAX&&
    player_heat_activation_row(r,20)&&w(r+0x18c)==15;
  const int em03_ground=tag==0x000300a8u&&mode==UINT32_MAX&&!source.actor&&!source.selector&&
    r104_em03_ground_record(e,source.record,4);
  if(!em03_ground&&(!player_attachment||(!boost&&!paired_throw&&!heat_activation))){
   *f=0x2faca8;return GH_EFFECT_UNSUPPORTED;
  }
 }
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
 uint8_t*p=b->esp+(size_t)i*0x400;memset(p,0,0x400);construct_esp_base(p,type,vtable);if(type==3||type==9||type==0x20||type==0x1f)ww(p+0x2d4,0x3f800000);if(type==0x1f)ww(p+0x30c,0x3f800000);if(type==4)vf0(p+0x2c0);if(type==5){vf0(p+0x2f0);vf0(p+0x300);vf0(p+0x370);vf0(p+0x380);vf0(p+0x390);}if(type==0x14)gh_esp14_construct_tail_002ffd88(p);if(type==0x15){vf0(p+0x2c0);vf0(p+0x2e0);}uint32_t id=++e->native->serial;esp_metadata*m=&e->native->esps[i];*m=(esp_metadata){.record=source.record,.actor=source.actor,.selector=source.selector,.light_part=type==5?source.part:NULL,.serial=id,.emitter_serial=source_serial,.emitter_slot=source_slot,.associated=1,.type=(uint8_t)type,.parent_emitter=-1,.previous=-1,.next=-1,.knockdown=(uint8_t)(knock||death_ground),.player_attachment=(uint8_t)(player_attachment||player_heat_root),.heat_model=e->native->slots[source_slot].heat_model,.target120_known_null=(uint8_t)((bound||(!source.actor&&!source.selector))&&!w(source.payload+0x218))};unsigned off=(i>>5)*4;uint32_t mask=UINT32_C(0x80000000)>>(i&31);ww(b->esp+0x100000+off,w(b->esp+0x100000+off)|mask);ww(b->esp+0x100080+off,w(b->esp+0x100080+off)&~mask);ww(b->esp+0x100100+off,w(b->esp+0x100100+off)&~mask);*slot=i;*serial=id;*allocated=1;return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_at(const gh_game_effects*e,unsigned slot,gh_effect_esp_view*out){
 if(!e||!out||slot>=1024||!e->native||e->native->owner!=e)return GH_EFFECT_ARGUMENT;const esp_metadata*m=&e->native->esps[slot];if(!m->associated||m->quiesced||!(w(e->native->esp+0x100000+(slot>>5)*4)&(UINT32_C(0x80000000)>>(slot&31))))return GH_EFFECT_UNSUPPORTED;
 *out=(gh_effect_esp_view){.record=m->record,.texture=m->texture,.secondary_texture=m->secondary_texture,.actor=m->actor,.selector=m->selector,.light_part=m->light_part,.payload=e->native->esp+(size_t)slot*0x400,.payload_size=0x400,.serial=m->serial,.emitter_serial=m->emitter_serial,.emitter_slot=m->emitter_slot,.initialized=m->initialized,.type=m->type,.globals=e->native->frame_globals,.scene_actors=e->native->frame_actors,.parent_emitter=m->parent_emitter,.previous=m->previous,.next=m->next,.parent_serial=m->parent_serial,.target120_known_null=m->target120_known_null,.retired_actor=m->retired_actor};return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_heat_aura_init_002e35c0(gh_game_effects *e,unsigned slot,
 uint32_t serial,uint32_t work594,uint32_t *frontier)
{
 if(!frontier)return GH_EFFECT_ARGUMENT;
 *frontier=0x2e35c0;
 gh_effect_esp_view view;
 if(gh_effect_create_esp_at(e,slot,&view)||view.serial!=serial||view.type!=3||view.initialized||
    w(view.payload+0x23a)!=0x00210000u||w(view.payload+0x10c)!=3||
    !gh_effect_player_attachment_esp_is_bound(e,&view)||!player_heat_active_row(view.record.bytes,5))
  return GH_EFFECT_UNSUPPORTED;
 esp_metadata *meta=e->native->esps+slot;
 gh_effect_status status=gh_esp03_initialize_002e35c0(&meta->heat,e->native->trail_pools,work594,
    view.payload,view.record.bytes,frontier);
 if(!status)meta->initialized=1;
 return status;
}

gh_effect_status gh_effect_create_esp_heat_aura_move_002e4280(gh_game_effects *e,unsigned slot,
 uint32_t serial,gh_static_transform_policy policy,uint32_t *frontier)
{
 if(!frontier)return GH_EFFECT_ARGUMENT;
 *frontier=0x2e4280;
 gh_effect_esp_view view;
 if(gh_effect_create_esp_at(e,slot,&view)||view.serial!=serial||view.type!=3||!view.initialized||
    !gh_effect_player_attachment_esp_is_bound(e,&view))return GH_EFFECT_ARGUMENT;
 esp_metadata *meta=e->native->esps+slot;
 if(!meta->heat_model||!view.actor||meta->heat_model->root.bytes!=view.actor->bytes)
  return GH_EFFECT_UNSUPPORTED;
 return gh_esp03_move_002e4280(&meta->heat,meta->heat_model,policy,frontier);
}

const gh_esp03_active_state *gh_effect_create_esp_heat_aura_state(
 const gh_game_effects *e,unsigned slot,uint32_t serial)
{
 gh_effect_esp_view view;
 if(gh_effect_create_esp_at(e,slot,&view)||view.serial!=serial||view.type!=3||!view.initialized||
    !gh_effect_player_attachment_esp_is_bound(e,&view))return NULL;
 return &e->native->esps[slot].heat;
}

gh_effect_status gh_effect_create_esp_bind_textures(gh_game_effects*e,unsigned slot,uint32_t serial,gh_effect_view texture,gh_effect_view secondary){gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial)return GH_EFFECT_ARGUMENT;e->native->esps[slot].texture=texture;e->native->esps[slot].secondary_texture=secondary;return GH_EFFECT_OK;}

static gh_effect_status abort_esp_native(gh_game_effects*e,unsigned slot,uint32_t serial,int ready_allowed){gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||(!ready_allowed&&v.initialized))return GH_EFFECT_ARGUMENT;if((v.actor||v.selector)&&!sprite_binding(v.record,v.actor,v.selector)&&!gh_effect_attack_esp_is_bound(e,&v)&&!gh_effect_knockdown_esp_is_bound(e,&v)&&!gh_effect_player_attachment_esp_is_bound(e,&v)&&!gh_effect_devil_esp_is_bound(e,&v)&&!gh_effect_enemy_light_esp_is_bound(e,&v)&&(!gh_effect_emad_esp_is_bound(e,&v)&&!gh_effect_em03_esp_is_bound(e,&v)))return GH_EFFECT_ARGUMENT;uint32_t vtable=esp_vtable(v.type);if(!vtable||w(v.payload+0xf0)!=vtable)return GH_EFFECT_UNSUPPORTED;if(v.parent_emitter>=0){status=gh_effect_create_esp_unlink_002b5ec0(e,slot,serial);if(status)return status;}if(release_trail(e->native->esps+slot))return GH_EFFECT_ARGUMENT;unsigned off=(slot>>5)*4;uint32_t keep=~(UINT32_C(0x80000000)>>(slot&31));for(unsigned bank=0;bank<3;bank++)ww(e->native->esp+0x100000+bank*0x80+off,w(e->native->esp+0x100000+bank*0x80+off)&keep);e->native->esps[slot]=(esp_metadata){0};return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_abort_partial(gh_game_effects*e,unsigned slot,uint32_t serial){return abort_esp_native(e,slot,serial,0);}

gh_effect_status gh_effect_create_bind_frame_globals(gh_game_effects*e,struct gh_effects_frame_globals*g){if(!e||!e->native||e->native->owner!=e||!g||!g->initialized)return GH_EFFECT_ARGUMENT;if(e->native->frame_globals&&e->native->frame_globals!=g)return GH_EFFECT_ARGUMENT;e->native->frame_globals=g;return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_commit_type5(gh_game_effects *e,unsigned slot,uint32_t serial)
{
 gh_effect_esp_view view;
 if(gh_effect_create_esp_at(e,slot,&view)||view.serial!=serial||view.initialized||
    !view.light_part||!view.globals||!view.globals->world_light||
    w(view.payload+0xf0)!=esp_vtable(5)||!gh_effect_enemy_light_esp_is_bound(e,&view))
  return GH_EFFECT_ARGUMENT;
 e->native->esps[slot].initialized=1;
 return GH_EFFECT_OK;
}

gh_effect_status gh_effect_create_esp_commit_base(gh_game_effects*e,unsigned slot,uint32_t serial){
 gh_effect_esp_view view;
 gh_effect_status status=gh_effect_create_esp_at(e,slot,&view);
 if(status||view.serial!=serial||view.type>1||view.initialized||
    w(view.payload+0xf0)!=esp_vtable(view.type)||!view.globals||!view.globals->initialized||
    (view.parent_emitter>=0&&!(w(view.payload+0x11c)&0x01000000u)))return GH_EFFECT_ARGUMENT;
 if((view.actor||view.selector)&&!sprite_binding(view.record,view.actor,view.selector)&&!gh_effect_attack_esp_is_bound(e,&view)&&!gh_effect_knockdown_esp_is_bound(e,&view)&&!gh_effect_player_attachment_esp_is_bound(e,&view)&&!gh_effect_devil_esp_is_bound(e,&view)&&
    !gh_effect_enemy_light_esp_is_bound(e,&view)&&(!gh_effect_emad_esp_is_bound(e,&view)&&!gh_effect_em03_esp_is_bound(e,&view)))
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
 if(tag==0x000b00a8u||tag==0x001400a8u)
  return !w(v->payload+0x10c)&&em03_sequence_effect_bound(e,v->record,v->actor,v->selector,tag);
 if(tag==0x001900a8u){
  gh_effect_view est;
  return w(v->payload+0x10c)==UINT32_MAX&&gh_effect_player_attachment_esp_is_bound(e,v)&&
     r104_held_impact_row(v->record.bytes,0)&&
     !gh_effect_est_at_00306030(e,0xa8,0x19,&est)&&est.size>=16+6*0x2f0&&
     w(est.bytes+4)==6&&v->record.bytes==est.bytes+16;
 }
 if(tag==0x004e0000u){
  gh_effect_view est;
  return v->actor&&v->selector==v->actor&&w(v->payload+0x10c)==5&&
     gh_effect_player_attachment_esp_is_bound(e,v)&&
     !gh_effect_est_at_00306030(e,0,0x4e,&est)&&est.size>=16+15*0x2f0&&
     w(est.bytes+4)==15&&(v->record.bytes==est.bytes+16||v->record.bytes==est.bytes+16+0x2f0);
 }
 if(tag==0x00090000u){
  gh_effect_view est;
  return !v->actor&&!v->selector&&w(v->payload+0x10c)==1&&v->record.bytes[0xbe]==0xfcu&&
    !gh_effect_est_at_00306030(e,0,9,&est)&&est.bytes&&est.size>=16+4*0x2f0&&
    w(est.bytes+4)==4&&(v->record.bytes==est.bytes+16+0x2f0||v->record.bytes==est.bytes+16+2*0x2f0);
 }
 if(tag==0x000f0000u||tag==0x00140000u){
  gh_effect_view est;
  const uint8_t *r=v->record.bytes;
  const unsigned index=tag>>16;
  const unsigned count=index==0xf?2:9;
  /* Original 292E80 and catapult 1B8EC4 use unattached CORE row zero. */
  return !v->actor&&!v->selector&&w(v->payload+0x10c)==UINT32_MAX&&
     w(r)==0x1e3e0010u&&w(r+4)==0x00800000u&&w(r+0xbc)==0x00ff0001u&&
     w(r+0x130)==0x07000001u&&!w(r+0x134)&&
     (index==0xf||(w(r+0x13c)==0x41400000u&&!w(r+0x140)&&
        w(r+0x144)==0x40200000u&&w(r+0x16c)==30&&w(r+0x170)==2))&&
     !gh_effect_est_at_00306030(e,0,index,&est)&&est.size>=16+count*0x2f0&&
     w(est.bytes+4)==count&&r==est.bytes+16;
 }
 if((tag&0xffffu)==6&&e&&e->event_registered_41c&&w(v->payload+0x10c)==(tag>>16)+0x24ffu){
  static const struct {unsigned index,count,row;uint32_t timing;} ev07[]={
   {9,4,0,0x00010000u},{10,1,0,0x00550054u},{13,7,6,0x004f004eu},
   {14,1,0,0x00830082u},{15,2,0,0x000a0009u}
  };
  const unsigned index=tag>>16;
  const uint8_t *r=v->record.bytes;
  const int attached=index==15;
  if((attached?gh_effect_event_part_is_bound(v->record,v->actor,v->selector):!v->actor&&!v->selector)&&
     w(r)==0x1e3e0010u&&w(r+4)==0x00800000u&&w(r+0xbc)==(attached?0xff0a0001u:0x00ff0001u)&&
     w(r+0x130)==0x07000001u&&!w(r+0x134)){
   for(unsigned i=0;i<sizeof(ev07)/sizeof(*ev07);++i){
    if(index!=ev07[i].index||w(r+0xc0)!=ev07[i].timing)continue;
    gh_effect_view est;
    if(!gh_effect_est_at_00306030(e,6,index,&est)&&est.bytes&&
       est.size>=16+ev07[i].count*0x2f0&&w(est.bytes+4)==ev07[i].count&&
       r==est.bytes+16+ev07[i].row*0x2f0)return 1;
   }
  }
 }
 if(tag==0x00080006u&&e&&e->event_registered_41c&&w(v->payload+0x10c)==0x2507u){
  gh_effect_view est;
  /* EV08 cut 7 attaches its first EST row to EL59 part 0. */
  if(gh_effect_event_part_is_bound(v->record,v->actor,v->selector)&&
     !gh_effect_est_at_00306030(e,6,8,&est)&&est.bytes&&est.size>=16+2*0x2f0&&
     w(est.bytes+4)==2&&v->record.bytes==est.bytes+16)return 1;
 }
 if(tag==0x00080006u||tag==0x00090006u){
  gh_effect_view est;
  const unsigned index=tag>>16,count=index==8?10:6,row=index==8?8:0;
  if(e&&e->event_registered_41c&&gh_effect_event_part_is_bound(v->record,v->actor,v->selector)&&
     !gh_effect_est_at_00306030(e,6,index,&est)&&est.size>=16+count*0x2f0&&
     w(est.bytes+4)==count&&v->record.bytes==est.bytes+16+row*0x2f0)return 1;
 }
 if(tag==0x00030006u){
  gh_effect_view est;
  /* Original EV05 cut 2/frame 242 distortion attaches to EL10 part 2. */
  return e&&e->event_registered_41c&&gh_effect_event_part_is_bound(v->record,v->actor,v->selector)&&
     !gh_effect_est_at_00306030(e,6,3,&est)&&est.size>=16+8*0x2f0&&
     w(est.bytes+4)==8&&v->record.bytes==est.bytes+16+6*0x2f0;
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
 if(tag==0x00340058u)return gh_effect_em00_attack_esp_is_bound(e,v);
 if(tag==0x003f0058u||((tag&0xffffu)==0x58&&
    (devil_ranged_effect_rows(tag>>16)||devil_combo_effect_rows(tag>>16))))
  return gh_effect_devil_esp_is_bound(e,v);
 if(tag==0x003b006cu||tag==0x00350000u||tag==0x00420000u||tag==0x00270058u||tag==0x00590058u||
    tag==0x005d0058u||tag==0x00170000u||tag==0x00220000u||tag==0x00280000u||tag==0x00190000u||
    tag==0x002e0000u||tag==0x00390000u)
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
unsigned gh_effect_esp_type15_copies(const gh_game_effects *e,const gh_effect_esp_view *v)
{
 if(!v||v->type!=0x15||v->payload_size!=0x400||v->record.size!=0x2f0)return 0;
 const uint32_t tag=w(v->payload+0x23a),mode=w(v->payload+0x10c);
 if(tag==0x000300a8u&&mode==UINT32_MAX&&!v->actor&&!v->selector&&
    r104_em03_ground_record(e,v->record,4))return 16;
 if(!gh_effect_player_attachment_esp_is_bound(e,v))return 0;
 const uint8_t *row=v->record.bytes;
 if(tag==0x004e0000u&&mode==5&&player_attack_boost_row(row,9))return 16;
 if(tag==0x00220000u&&mode==UINT32_MAX&&player_heat_activation_row(row,20)&&w(row+0x18c)==15)return 16;
 if(tag==0x003b006cu&&mode==UINT32_MAX){
  if(r103_boss_throw_effect_row(row,12))return 13;
  if(r103_boss_throw_effect_row(row,13))return 16;
 }
 return 0;
}

gh_effect_status gh_effect_create_esp_commit_15(gh_game_effects *e,unsigned slot,uint32_t serial)
{
 gh_effect_esp_view v;
 if(gh_effect_create_esp_at(e,slot,&v)||v.serial!=serial||v.type!=0x15||v.initialized||
    w(v.payload+0xf0)!=esp_vtable(0x15)||!v.globals||!v.globals->initialized||
    !gh_effect_esp_type15_copies(e,&v))
  return GH_EFFECT_ARGUMENT;
 e->native->esps[slot].initialized=1;
 return GH_EFFECT_OK;
}

int gh_effect_model_esp_actor_is_bound(const gh_game_effects *e,const gh_effect_esp_view *view)
{
 return gh_effect_player_attachment_esp_is_bound(e,view)||
        (view&&view->type==4&&gh_effect_em00_attack_esp_is_bound(e,view));
}

gh_effect_status gh_effect_create_esp_commit_14(gh_game_effects *e,unsigned slot,uint32_t serial){
 gh_effect_esp_view view;
 gh_effect_status status=gh_effect_create_esp_at(e,slot,&view);
 if(status||view.serial!=serial||(view.type!=4&&view.type!=0x14)||view.initialized||
    w(view.payload+0xf0)!=esp_vtable(view.type)||!view.globals||!view.globals->initialized||
    ((view.actor||view.selector)&&!gh_effect_model_esp_actor_is_bound(e,&view)))return GH_EFFECT_ARGUMENT;
 e->native->esps[slot].initialized=1;
 return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_commit_10(gh_game_effects *e,unsigned slot,uint32_t serial)
{
 gh_effect_esp_view view;
 gh_effect_status status=gh_effect_create_esp_at(e,slot,&view);
 gh_effect_view est;
 /* Original 2FAE14 creates only this bounded R002 child here.
  * Refuse unrelated uses before marking the host owner initialized. */
 if(status||view.serial!=serial||view.type!=0x10||view.initialized||
    w(view.payload+0xf0)!=esp_vtable(0x10)||!view.globals||!view.globals->initialized||
    !gh_effect_event_part_is_bound(view.record,view.actor,view.selector))
  return GH_EFFECT_ARGUMENT;
 const uint32_t tag=w(view.payload+0x23a);
 const int cut12=tag==0x000d0006u;
 const unsigned count=cut12?7:4;
 if((!cut12&&tag!=0x00020001u)||w(view.payload+0x10c)!=(cut12?0x250cu:2u)||
    gh_effect_est_at_00306030(e,tag&0xffffu,tag>>16,&est)||!est.bytes||est.size<16+count*0x2f0||
    w(est.bytes+4)!=count||view.record.bytes!=est.bytes+16)
  return GH_EFFECT_ARGUMENT;
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
    ((view.actor||view.selector)&&!r101_weapon_esp_bound(e,&view)&&
     !gh_effect_em00_attack_esp_is_bound(e,&view)))return GH_EFFECT_ARGUMENT;
 e->native->esps[slot].initialized=1;
 return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_commit_1a(gh_game_effects *e,unsigned slot,uint32_t serial)
{
 gh_effect_esp_view v;
 gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);
 if(status||v.serial!=serial||v.type!=0x1a||v.initialized||
    w(v.payload+0xf0)!=0x44f270u||!v.globals||!v.globals->initialized||!v.scene_actors)
  return GH_EFFECT_ARGUMENT;
 if((v.actor||v.selector)&&
    !gh_esp14_nested_sprite_is_bound(v.record,v.actor,v.selector)&&
    !gh_effect_event_part_is_bound(v.record,v.actor,v.selector)&&
    !gh_effect_attack_esp_is_bound(e,&v)&&!gh_effect_knockdown_esp_is_bound(e,&v)&&
    !gh_effect_player_attachment_esp_is_bound(e,&v)&&!gh_effect_devil_esp_is_bound(e,&v)&&
    !gh_effect_em03_esp_is_bound(e,&v))
  return GH_EFFECT_ARGUMENT;
 e->native->esps[slot].initialized=1;
 return GH_EFFECT_OK;
}
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
 if(v->type==0x1f&&v->payload&&v->payload_size==0x400){
  const uint32_t tag=w(v->payload+0x23a);
  if((tag==0x00050006u||tag==0x00060006u||tag==0x00070006u||tag==0x00090006u||tag==0x000d0006u)&&
     e&&e->event_registered_41c&&
     gh_effect_event_part_is_bound(v->record,v->actor,v->selector)){
   gh_effect_view est;
   /* EV07 authentic trail row offsets in the registered event EST. */
   const unsigned index=tag>>16;
   const unsigned count=index==6?2:index==7?3:index==9?4:index==13?7:0;
   if(count&&!gh_effect_est_at_00306030(e,6,index,&est)&&est.bytes&&
      est.size>=16+count*0x2f0&&w(est.bytes+4)==count){
    if(index==6)return v->record.bytes==est.bytes+16;
    if(index==13)return v->record.bytes==est.bytes+16+2*0x2f0;
    return v->record.bytes==est.bytes+16+0x2f0||
        (index==7&&v->record.bytes==est.bytes+16+2*0x2f0);
   }
   /* EV06 cut4's EL10 attached ESP31 is EST05 row 2 of 18. */
   if(tag==0x00050006u&&!gh_effect_est_at_00306030(e,6,5,&est)&&
      est.size>=16+18*0x2f0&&w(est.bytes+4)==18u)
    return v->record.bytes==est.bytes+16+2*0x2f0;
   if(!gh_effect_est_at_00306030(e,6,tag>>16,&est)&&est.size>=16+8*0x2f0&&w(est.bytes+4)==8){
    if(tag==0x00050006u)
     return v->record.bytes==est.bytes+16+4*0x2f0||v->record.bytes==est.bytes+16+7*0x2f0;
    return v->record.bytes==est.bytes+16+2*0x2f0;
   }
  }
 }
 if(v->type==0x1f)return gh_effect_devil_esp_is_bound(e,v)||
    (gh_effect_player_attachment_esp_is_bound(e,v)&&w(v->payload+0x23a)==0x00210000u&&
     (player_heat_active_row(v->record.bytes,8)||player_heat_active_row(v->record.bytes,9)));
 if(v->type==0x20&&gh_effect_em00_attack_esp_is_bound(e,v))return 1;
 gh_effect_view est;
 if(v->type==9){
  if(v->payload&&v->payload_size==0x400&&e&&e->event_registered_41c&&
     gh_effect_event_part_is_bound(v->record,v->actor,v->selector)){
   const uint32_t tag=w(v->payload+0x23a);
   const unsigned index=tag>>16;
   /* EV06's EST02 and EST03 have identical EL10 ESP09 row positions. */
   if((tag&0xffffu)==6&&(index==2||index==3)&&
      !gh_effect_est_at_00306030(e,6,index,&est)&&est.bytes&&
      est.size>=16+15*0x2f0&&w(est.bytes+4)==15u)
    return v->record.bytes==est.bytes+16+8*0x2f0||v->record.bytes==est.bytes+16+14*0x2f0;
   if(tag==0x00050006u&&!gh_effect_est_at_00306030(e,6,5,&est)&&est.bytes&&
      est.size>=16+18*0x2f0&&w(est.bytes+4)==18u)
    return v->record.bytes==est.bytes+16+11*0x2f0||v->record.bytes==est.bytes+16+17*0x2f0;
   if(tag==0x00080006u&&!gh_effect_est_at_00306030(e,6,8,&est)&&est.bytes&&
      est.size>=16+10*0x2f0&&w(est.bytes+4)==10u){
    for(unsigned i=4;i<=7;++i)
     if(v->record.bytes==est.bytes+16+i*0x2f0)return 1;
   }
   if((tag&0xffffu)==6&&index>=14&&index<=16&&
      !gh_effect_est_at_00306030(e,6,index,&est)&&est.bytes&&
      est.size>=16+(index==15?7:5)*0x2f0&&w(est.bytes+4)==(index==15?7u:5u)){
    if(index==15)return v->record.bytes==est.bytes+16+4*0x2f0;
    return v->record.bytes==est.bytes+16+2*0x2f0||v->record.bytes==est.bytes+16+3*0x2f0;
   }
  }
  if(gh_effect_attack_esp_is_bound(e,v)||gh_effect_knockdown_esp_is_bound(e,v)||
     gh_effect_player_attachment_esp_is_bound(e,v)||gh_effect_devil_esp_is_bound(e,v))return 1;
  if(v->payload&&v->payload_size==0x400&&w(v->payload+0x23a)==0x00400000u&&
     !v->actor&&!v->selector&&!gh_effect_est_at_00306030(e,0,0x40,&est)&&
     est.size>=16+6*0x2f0&&w(est.bytes+4)==6&&v->record.bytes==est.bytes+16+5*0x2f0&&
     v->record.size==0x2f0&&w(v->record.bytes)==0x1e3ea010u&&
     w(v->record.bytes+4)==0x00800000u&&w(v->record.bytes+0x18c)==0x00000801u) return 1;
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
    !gh_effect_esp_trail_is_bound(e,&v)||v.parent_emitter<0)return GH_EFFECT_ARGUMENT;
 const uint8_t *r=v.record.bytes;
 uint8_t *p=v.payload;
 /* The EM20, PL00 and EV06 EL10 rows share the original spring and Y acceleration. */
 const int ev07=w(p+0x23a)==0x000d0006u&&w(r)==0x1ebfa010u;
 /* 2F57B0 uses 16 when the original authored history byte is zero. */
 const unsigned count=ev07?16u:r[0x18d];
 if((ev07?(r[0x18d]||r[0x18e]||r[0x197]!=1):
     ((count!=15&&count!=20&&count!=25)||r[0x18e]!=5||r[0x197]))||
    w(r+0x13c)||w(r+0x140)!=0x3f800000||w(r+0x144)!=0x3e4ccccd||
    w(r+0x148)||w(r+0x14c)!=0x3c23d70a||w(r+0x150)||
    r[0x191]||r[0x192]||r[0x193]||r[0x194]||w(p+0x2e0)||w(p+0x2dc))
  return GH_EFFECT_UNSUPPORTED;
 float position[4],zero[4]={0,0,0,1};
 gh_effect_status status=gh_effect_node_world_position_native(e,0,slot,serial,position,policy,frontier);
 if(status)return status;
 status=initialize_trail(e,slot,&v,work594,position,zero,count,frontier);
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
 p[0x32c]=(uint8_t)(r[0x18e]+2);
 return GH_EFFECT_OK;
}

gh_effect_status gh_effect_create_esp_devil_trail_move_002f5bf8(gh_game_effects *e,
 unsigned slot,uint32_t serial,const gh_boot_work *work,gh_static_transform_policy policy,uint32_t *frontier)
{
 if(!frontier||!work)return GH_EFFECT_ARGUMENT;
 *frontier=0x2f5bf8;
 gh_effect_esp_view v;
 if(gh_effect_create_esp_at(e,slot,&v)||v.serial!=serial||!v.initialized||v.type!=0x1f||
    !gh_effect_esp_trail_is_bound(e,&v))return GH_EFFECT_ARGUMENT;
 esp_metadata *m=e->native->esps+slot;
 uint8_t *p=v.payload;
 const unsigned count=p[0x249];
 const size_t bytes=count*sizeof(float[4]);
 const int ev07=w(p+0x23a)==0x000d0006u&&w(v.record.bytes)==0x1ebfa010u;
 if(!m->trail_pools||!m->trail[2].pointer||!m->trail[3].pointer||
    (ev07?(count!=16||v.record.bytes[0x18d]):
     ((count!=15&&count!=20&&count!=25)||count!=v.record.bytes[0x18d]))||
    m->trail[2].bytes!=bytes||m->trail[3].bytes!=bytes||
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
 float point[25][4],velocity[25][4];
 memcpy(point,m->trail[2].pointer,bytes);
 memcpy(velocity,m->trail[3].pointer,bytes);
 for(unsigned i=count-1;i>0;--i)for(unsigned axis=0;axis<3;axis++)
  velocity[i][axis]=devil_trail_add(velocity[i][axis],
      devil_trail_add(point[i-1][axis],-point[i][axis]));
 for(unsigned i=0;i<count-1;++i)for(unsigned axis=0;axis<3;axis++)
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
 for(unsigned i=1;i<count;++i)for(unsigned axis=0;axis<3;axis++){
  if(axis==1)velocity[i][axis]=devil_trail_add(velocity[i][axis],gravity);
  velocity[i][axis]=devil_trail_mul(velocity[i][axis],0.8f);
  point[i][axis]=devil_trail_add(point[i][axis],velocity[i][axis]);
  if(!isfinite(point[i][axis]))return GH_EFFECT_UNSUPPORTED;
 }
 memcpy(point[0],position,12);
 memcpy(m->trail[2].pointer,point,bytes);
 memcpy(m->trail[3].pointer,velocity,bytes);
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
 /* 2EBF98 uses sixteen points for zero in the authored history byte. */
 const unsigned count=r[0x18d]?r[0x18d]:16u;
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
 const uint32_t profile=w(p+0x23a);
 const int wide_strike=(profile&0xffffu)==0x58&&wide_strike_effect_room(view.actor,profile>>16)&&
    gh_effect_em00_attack_esp_is_bound(e,&view);
 const unsigned count=wide_strike?r[0x18d]:16;
 const uint32_t trail_fields=w(r+0x18c);
 if(w(r+0x16c)||w(r+0x170)||w(r+0x148)||p[0x245]!=1||
    (wide_strike?(p[0x246]!=1||(trail_fields!=0x701&&trail_fields!=0x601)):
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
 ww(p+0x2e0,wide_strike?4:5);
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

gh_effect_status gh_effect_create_esp_pause_002fbe50(gh_game_effects*e,unsigned slot,uint32_t serial,uint32_t*f){if(!f)return GH_EFFECT_ARGUMENT;*f=0;gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||!v.initialized)return GH_EFFECT_ARGUMENT;if(!esp_vtable(v.type)||((v.actor||v.selector)&&!sprite_binding(v.record,v.actor,v.selector)&&!gh_effect_r200_catapult_part_is_bound(v.record,v.actor,v.selector)&&!gh_effect_attack_esp_is_bound(e,&v)&&!gh_effect_knockdown_esp_is_bound(e,&v)&&!gh_effect_player_attachment_esp_is_bound(e,&v)&&!gh_effect_devil_esp_is_bound(e,&v)&&!gh_effect_enemy_light_esp_is_bound(e,&v)&&(!gh_effect_emad_esp_is_bound(e,&v)&&!gh_effect_em03_esp_is_bound(e,&v)))){*f=0x2fbe50;return GH_EFFECT_UNSUPPORTED;}
 /* Supported constructors create no ESPchildren and the only supportedattach
  * API accepts emitterparents. Unknown rawchild/parent tokens remainstrict. */
 if(w(v.payload+4)||w(v.payload+8)||w(v.payload+0xc)||w(v.payload+0x10)){*f=0x2b5cf0;return GH_EFFECT_UNSUPPORTED;}status=gh_effect_create_esp_unlink_002b5ec0(e,slot,serial);if(status){*f=0x2b5ec0;return status;}unsigned off=(slot>>5)*4;uint32_t bit=UINT32_C(0x80000000)>>(slot&31);ww(e->native->esp+0x100080+off,w(e->native->esp+0x100080+off)|bit);return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_destroy_hidden_native(gh_game_effects*e,unsigned slot,uint32_t serial,uint32_t*f){if(!f)return GH_EFFECT_ARGUMENT;*f=0;gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||!v.initialized)return GH_EFFECT_ARGUMENT;unsigned off=(slot>>5)*4;uint32_t bit=UINT32_C(0x80000000)>>(slot&31);if(!(w(e->native->esp+0x100100+off)&bit))return GH_EFFECT_ARGUMENT;if(v.type==4||v.type==0x14){*f=v.type==4?0x2e6368:0x2f27e8;return GH_EFFECT_UNSUPPORTED;}if(!esp_vtable(v.type)||((v.actor||v.selector)&&!sprite_binding(v.record,v.actor,v.selector)&&!gh_effect_r200_catapult_part_is_bound(v.record,v.actor,v.selector)&&!gh_effect_attack_esp_is_bound(e,&v)&&!gh_effect_knockdown_esp_is_bound(e,&v)&&!gh_effect_player_attachment_esp_is_bound(e,&v)&&!gh_effect_devil_esp_is_bound(e,&v)&&!gh_effect_enemy_light_esp_is_bound(e,&v)&&(!gh_effect_emad_esp_is_bound(e,&v)&&!gh_effect_em03_esp_is_bound(e,&v)))||v.parent_emitter!=-1||v.previous!=-1||v.next!=-1||w(v.payload+4)||w(v.payload+8)||w(v.payload+0xc)||w(v.payload+0x10)){*f=0x2fbe28;return GH_EFFECT_UNSUPPORTED;}if(w(v.payload+0xf0)!=esp_vtable(v.type)){*f=0x2fbe28;return GH_EFFECT_UNSUPPORTED;}
 if((v.type==3||v.type==9||v.type==0x1f||v.type==0x20)&&release_trail(e->native->esps+slot)){
  *f=v.type==3?0x2e3500:0x2ebc50;return GH_EFFECT_ARGUMENT;
 }
 if(v.type==5&&(!v.globals||!v.actor||gh_game_world_light_remove_owned_002d9158(
      v.globals->world_light,(uint16_t)(w(v.payload+0x2b0)>>16),v.actor->bytes))){
  *f=0x2d9158;return GH_EFFECT_UNSUPPORTED;
 }
 /* Actualvirtual dtor(flags2): concreteVT ->44FAD0 ->44B310; bit1clear
  * means original30F5A0 is not called. Caller clearsactive onlyafterreturn. */
 ww(v.payload+0xf0,0x44fad0u);ww(v.payload+0xf0,0x44b310u);e->native->esps[slot].associated=0;return GH_EFFECT_OK;}

gh_effect_status gh_effect_create_esp_check_hidden_14(gh_game_effects *e,unsigned slot,uint32_t serial){
 gh_effect_esp_view v;
 if(gh_effect_create_esp_at(e,slot,&v)||v.serial!=serial||!v.initialized||(v.type!=4&&v.type!=0x14))
  return GH_EFFECT_ARGUMENT;
 unsigned off=(slot>>5)*4;
 uint32_t mask=UINT32_C(0x80000000)>>(slot&31);
 if(!(w(e->native->esp+0x100100+off)&mask))return GH_EFFECT_ARGUMENT;
 if(((v.actor||v.selector)&&!gh_effect_model_esp_actor_is_bound(e,&v))||
    v.parent_emitter!=-1||v.previous!=-1||v.next!=-1||
    w(v.payload+4)||w(v.payload+8)||w(v.payload+0xc)||w(v.payload+0x10)||
    w(v.payload+0xf0)!=esp_vtable(v.type))return GH_EFFECT_UNSUPPORTED;
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
 const int player=actor->original_vtable214==0x41d220u&&w(actor->bytes+0x2fc)>>16==0x100;
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
  const int heat_ready=player&&w(view.payload+0x18c)==0x00250000u&&
    gh_effect_player_attachment_emitter_is_bound(&view);
  const uint32_t profile=w(view.payload+0x18c);
  const int heat_active=player&&profile==0x00210000u&&
    (gh_effect_player_attachment_emitter_is_bound(&view)||
     gh_effect_player_heat_root_emitter_is_bound(&view));
  const int pincer=(profile&0xffffu)==0x58&&r103_pincer_effect_rows(profile>>16)&&
    gh_effect_em00_attack_emitter_is_bound(&view);
  if(!devil&&!heat_ready&&!heat_active&&(!gh_effect_emad_emitter_is_bound(effects,&view)&&!gh_effect_em03_emitter_is_bound(effects,&view))&&
     !gh_effect_enemy_light_emitter_is_bound(effects,&view)&&!pincer)
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
  const int heat_ready=player&&w(view.payload+0x23a)==0x00250000u&&
    (view.type==0||view.type==9)&&gh_effect_player_attachment_esp_is_bound(effects,&view);
  const uint32_t profile=w(view.payload+0x23a);
  const int heat_active=player&&profile==0x00210000u&&
    (view.type==0||view.type==3||view.type==0x1f)&&
    gh_effect_player_attachment_esp_is_bound(effects,&view);
  const int pincer=(profile&0xffffu)==0x58&&r103_pincer_effect_rows(profile>>16)&&
    gh_effect_attack_esp_is_bound(effects,&view);
  if(!devil&&!heat_ready&&!heat_active&&(!gh_effect_emad_esp_is_bound(effects,&view)&&!gh_effect_em03_esp_is_bound(effects,&view))&&
     !gh_effect_enemy_light_esp_is_bound(effects,&view)&&!pincer)return GH_EFFECT_UNSUPPORTED;
  const int known_trail=((devil||heat_active)&&view.type==0x1f)||(heat_ready&&view.type==9);
  if(m->trail_pools&&(!known_trail||m->trail_pools!=native->trail_pools))
   return GH_EFFECT_UNSUPPORTED;
  if(m->heat.pools&&(!heat_active||view.type!=3||m->heat.pools!=native->trail_pools))
   return GH_EFFECT_UNSUPPORTED;
  particles[particle_count++]=i;
 }
 /* 305B90 has already stopped the emitter and unlinked its particles.
  * State5 clears original arena 754240 wholesale. Release the paused
  * trails' native allocations before that bounded arena is destroyed. */
 for(unsigned j=0;j<particle_count;j++){
  esp_metadata *m=native->esps+particles[j];
  if((m->trail_pools||m->heat.pools)&&release_trail(m))return GH_EFFECT_ARGUMENT;
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
 const int opening=actor->size>=0x644&&actor->original_vtable214==0x4479c8&&
    (w(actor->bytes+0x2fc)>>16)==0x202&&w(actor->bytes+0x564)==0x203&&
    (w(actor->bytes+0x640)&0xffffu)==19;
 const int r102_ground=(r102_gate_guard_effect_actor(actor)||em00_escape_effect_room(actor)==0x102||
    r102_gate_em0a_ground_actor(actor)||r102_em03_death_actor(actor))&&actor->bytes[0x2f4]==2;
 const int em03_retained=r104_charge_actor(actor)&&actor->bytes[0x2f4]==2&&
    actor->bytes[0x2f5]==2&&actor->bytes[0x2f6]==6&&actor->bytes[0x2f7]==0&&
    (w(actor->bytes+0x250)&0xe0010000u)==0xe0010000u;
 if((!opening&&!r102_ground&&!em03_retained)||(w(actor->bytes+0x250)&0xa0008000u)!=0xa0000000u)
  return GH_EFFECT_OK;
 gh_effect_view est={0};
 unsigned rows=0;
 for(unsigned i=0;i<1024;i++){
  esp_metadata *m=n->esps+i;
  const uint32_t bit=UINT32_C(0x80000000)>>(i&31);
  if(!(w(n->esp+0x100000+(i>>5)*4)&bit)||!m->associated||!m->initialized||
     m->actor!=actor||m->selector!=actor||m->parent_emitter!=-1||m->previous!=-1||m->next!=-1)
   continue;
  const uint8_t *payload=n->esp+(size_t)i*0x400;
  if((w(payload+0x11c)&0x80u)||w(payload+0x120))continue;
  if(em03_retained){
   const uint32_t tag=w(payload+0x23a);
   const int spawn=m->type==0&&tag==0x000100a8u&&em03_spawn_row(e,m->record,actor,actor);
   /* 24F638 also leaves unlinked, ordinarily timed 58:04 recoil dust. */
   const int recoil_dust=m->type==0x1a&&tag==0x00040058u&&m->knockdown;
   if(!spawn&&!recoil_dust)continue;
  }else{
   if(m->type!=0x1a||!m->knockdown||w(payload+0x23a)!=0x00020058u||
      w(payload+0x10c)!=UINT32_MAX)continue;
   if(!est.bytes){
    if(gh_effect_est_at_00306030(e,0x58,2,&est)||!est.bytes||est.size<16||w(est.bytes)!=0x545345u)
     return GH_EFFECT_UNSUPPORTED;
    rows=w(est.bytes+4);
    if(rows>(est.size-16)/0x2f0)return GH_EFFECT_BOUNDS;
   }
   unsigned row=0;
   while(row<rows&&m->record.bytes!=est.bytes+16+(size_t)row*0x2f0)row++;
   if(row==rows)continue;
  }
  /* 2FBED0 pauses on actor death only for a linked particle. These
   * unlinked sprites retain their original update and lifetime. Copy the
   * last 2FB0E8/2FEC38 actor inputs before 30F5A0 frees that actor. */
  const uint8_t *p=actor->bytes;
  float tint;
  memcpy(&tint,p+0x24c,4);
  if(!isfinite(tint))return GH_EFFECT_UNSUPPORTED;
  m->retired_actor=(gh_effect_retired_actor){
   .flags=w(p+0x250),.tint_alpha=tint,
   .opacity={p[0x2b6],p[0x2b7],p[0x2bc]},.valid=1};
  m->actor=m->selector=NULL;
 }
 return GH_EFFECT_OK;
}

/* EV07 EventEffect::Quit stopped the room bank 1 emitters and unlinked
 * their particles before original 312778 deletes EL10. The next native
 * effect ticks still own the stopped nodes and their R002 EST source. */
gh_effect_status gh_effect_create_detach_ev07_stopped_effects(gh_game_effects *e,
 const gh_title_player_view *actor)
{
 if(!e||!actor||!actor->bytes||actor->size<0x300||actor->original_vtable214!=0x44b528u||
    (w(actor->bytes+0x2fc)>>16)!=0x610)return GH_EFFECT_ARGUMENT;
 if(!e->native)return GH_EFFECT_OK;
 struct gh_effect_native *n=e->native;
 if(n->owner!=e)return GH_EFFECT_ARGUMENT;
 gh_effect_view est;
 if(gh_effect_est_at_00306030(e,1,2,&est)||!est.bytes||
    est.size<16+4*0x2f0||w(est.bytes+4)!=4)return GH_EFFECT_ARGUMENT;
 unsigned emitters[256],particles[1024],emitter_count=0,particle_count=0;
 for(unsigned i=0;i<256;i++){
  const unsigned off=(i>>5)*4;
  const uint32_t bit=UINT32_C(0x80000000)>>(i&31);
  emitter_metadata *m=n->slots+i;
  if(!(w(n->emitters+0x26000+off)&bit)||!attaches_actor(m->actor,m->selector,actor))continue;
  const uint8_t *p=n->emitters+i*0x260;
  if(!m->associated||!m->initialized||m->quiesced||m->first_esp_child!=-1||
     !(w(p+0x114)&0x40000000u)||w(p+0x118)!=2u||w(p+0x18c)!=0x00020001u||
     w(p+0x110)!=2u||w(p+0xf0)!=0x44fd58u||w(p+4)||w(p+8)||w(p+0x10)||
     (m->record!=est.bytes+16&&m->record!=est.bytes+16+2*0x2f0&&m->record!=est.bytes+16+3*0x2f0))
   return GH_EFFECT_UNSUPPORTED;
  emitters[emitter_count++]=i;
 }
 for(unsigned i=0;i<1024;i++){
  const unsigned off=(i>>5)*4;
  const uint32_t bit=UINT32_C(0x80000000)>>(i&31);
  esp_metadata *m=n->esps+i;
  if(!(w(n->esp+0x100000+off)&bit)||!attaches_actor(m->actor,m->selector,actor))continue;
  const uint8_t *p=n->esp+i*0x400;
  if(!m->associated||!m->initialized||m->quiesced||m->type||
     !((w(n->esp+0x100080+off)|w(n->esp+0x100100+off))&bit)||
     m->parent_emitter!=-1||m->previous!=-1||m->next!=-1||
     w(p+0x104)!=2u||w(p+0x23a)!=0x00020001u||w(p+0x10c)!=2u||
     w(p+4)||w(p+8)||w(p+0xc)||w(p+0x10)||
     (m->record.bytes!=est.bytes+16+2*0x2f0&&m->record.bytes!=est.bytes+16+3*0x2f0))
   return GH_EFFECT_UNSUPPORTED;
  particles[particle_count++]=i;
 }
 for(unsigned i=0;i<emitter_count;i++){
  emitter_metadata *m=n->slots+emitters[i];
  m->actor=NULL;m->selector=NULL;m->model=NULL;
  m->part_index=0;m->part_handle=0;
 }
 for(unsigned i=0;i<particle_count;i++){
  esp_metadata *m=n->esps+particles[i];
  m->actor=NULL;m->selector=NULL;
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
