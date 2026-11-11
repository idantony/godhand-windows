#include "game_loaded_room.h"
#include <string.h>
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static gh_scene_aux_status stop(gh_game_scene_aux*s,uint32_t f){s->frontier=f;return GH_AUX_UNSUPPORTED;}
gh_scene_aux_status gh_scene_door_step_001ffe10(gh_game_scene_aux*s,uint16_t*wait,const gh_boot_work*w){
 if(!s||!wait||!w)return GH_AUX_ARGUMENT;s->frontier=0;
 if(!s->door_stage){if(*wait)*wait=(uint16_t)(*wait-1u);s->door_stage=1;}
 if(word((const uint8_t*)w+0x5e4)&0x02000000u)return stop(s,0x2000f8);
 s->door_stage=0;return GH_AUX_OK;
}
void gh_scene_background_mode_0030e5c0(gh_boot_work*w,int mode,uint32_t a,uint32_t b,uint32_t c){
 if(!w||(mode!=0&&mode!=1))return;
 uint8_t*p=(uint8_t*)w;uint64_t x,y;memcpy(&x,p+0x3e0,8);memcpy(&y,p+0x270,8);
 uint64_t low=mode?0x16:0x10;x=(x&UINT64_C(0xffffffffffff8000))|low;y=(y&UINT64_C(0xffffffffffff8000))|low;
 memcpy(p+0x3e0,&x,8);memcpy(p+0x270,&y,8);
 if(mode){uint32_t config=(a&255)|((b&255)<<8)|((c&255)<<16);memcpy(p+0x60c,&config,4);}
}
gh_scene_aux_status gh_scene_cursor_open_load_002bf268(gh_game_scene_aux *s,gh_boot_work *w)
{
 if(!s||!w)return GH_AUX_ARGUMENT;
 /* The original availability check 002BF700 returns one. 002BF170 clears
  * only the listed cursor fields before openLoad selects mode two. */
 uint8_t *busy=(uint8_t*)w+0x5e8;
 uint32_t flags=word(busy)|UINT32_C(0x40000000);
 memset(s->save_load+1,0,7);
 memset(s->save_load+9,0,2);
 memset(s->save_load+0xc,0,2);
 memcpy(busy,&flags,sizeof(flags));
 s->save_load[0]=2;
 s->load_flags_saved=0;
 return GH_AUX_OK;
}

static gh_scene_aux_status cursor_load_step(gh_game_scene_aux *s,gh_boot_work *w,
 gh_scene_start_load start,void *context)
{
 uint8_t *cursor=s->save_load;
 uint8_t *work=(uint8_t*)w;
 if(!cursor[1]){
  /* 002BF3D0 calls 002BF708 then 002BF790 before launching the MC task. */
  if(!s->load_flags_saved){
   memcpy(cursor+0x70,work+0x5e0,4);
   memcpy(cursor+0x74,work+0x5d8,8);
   uint32_t mask=word(work+0x5e0)|UINT32_C(0xfff00000);
   uint32_t flags=(word(work+0x5d8)&UINT32_C(0xfdffffff))|UINT32_C(0xd8862000);
   memcpy(work+0x5e0,&mask,4);
   memcpy(work+0x5d8,&flags,4);
   s->load_flags_saved=1;
  }
  if(!start||start(context))return stop(s,0x2ae7f0);
  cursor[1]=1;
  cursor[2]=cursor[3]=0;
 }else if(cursor[1]==1 && !(word(work+0x5e8)&UINT32_C(0x80000000))){
  /* 002BF420 waits for the task, then uses the original successful-load bit. */
  cursor[0]=5;
  if(w->flags&0x40){cursor[0]=4;cursor[0xc]=1;}
  cursor[1]=cursor[2]=cursor[3]=0;
 }
 return GH_AUX_OK;
}

gh_scene_aux_status gh_scene_cursor_step_002bf2d0(gh_game_scene_aux*s,gh_boot_work*w,
 gh_scene_start_load start,void *context){
 static const uint32_t targets[]={0x2bf478,0x2bf540,0x2bf568};
 if(!s||!w)return GH_AUX_ARGUMENT;s->frontier=0;
 /* Predecessor 002BF380 is JR RA/NOP. */
 if(!(word((const uint8_t*)w+0x5e8)&0x40000000u))return GH_AUX_OK;
 gh_scene_background_mode_0030e5c0(w,1,0,0,0);
 uint8_t kind=s->save_load[0];
 if(kind==2)return cursor_load_step(s,w,start,context);
 if(kind==4){
  /* 002BF540 advances to the common cursor exit without changing +0C. */
  if(!s->save_load[1]){
   s->save_load[0]=5;
   s->save_load[1]=s->save_load[2]=s->save_load[3]=0;
  }
  return GH_AUX_OK;
 }
 return kind>=3&&kind<=5?stop(s,targets[kind-3]):GH_AUX_OK;
}
gh_scene_aux_status gh_scene_effects_step_0030f228(gh_game_scene_aux*s,const gh_boot_work*w,gh_scene_aux_clock clock,void*context){
 if(!s||!w||!clock)return GH_AUX_ARGUMENT;s->frontier=0;
 if(!s->effect_stage){s->effect_profile_752d00=clock(context);s->effect_count_3c3bb8=0;s->effect_stage=1;}
 if(!(word((const uint8_t*)w+0x5d8)&0x10000000u))return stop(s,0x300628);
 s->effect_profile_752d00-=clock(context);s->effect_stage=0;return GH_AUX_OK;
}

static void put_word(uint8_t *destination,uint32_t value)
{memcpy(destination,&value,4);}
static uint16_t read_half(const uint8_t *bytes)
{return (uint16_t)((uint16_t)bytes[0]|(uint16_t)bytes[1]<<8);}

gh_scene_aux_status gh_scene_cursor_loaded_room_002bf568(gh_game_scene_aux *aux,
    gh_scene_loaded_room *state,const gh_scene_loaded_room_bindings *bindings)
{
    if(!aux||!state||!bindings||!bindings->door_bindings||
       !bindings->door_bindings->work||!bindings->door_bindings->fade)return GH_AUX_ARGUMENT;
    const gh_game_door_bindings *door_bindings=bindings->door_bindings;
    gh_boot_work *work=door_bindings->work;
    uint8_t *raw_work=(uint8_t*)work;
    uint8_t *cursor=aux->save_load;
    aux->frontier=0;
    if(!state->stage){
        if(cursor[0]!=5||!(word(raw_work+0x5e8)&UINT32_C(0x40000000)))return GH_AUX_ARGUMENT;
        const uint8_t fade=gh_screen_fade_status_00307718(door_bindings->fade);
        if(fade&&!(fade&4))return GH_AUX_OK;
        if(!aux->load_flags_saved)return stop(aux,0x2bf800);
        if(cursor[0xc]==1&&work->room!=0x20)return stop(aux,0x2bf568);

        cursor[0xa]=0;
        put_word(raw_work+0x5e8,word(raw_work+0x5e8)&~UINT32_C(0x40000000));
        memcpy(raw_work+0x5d8,cursor+0x74,8); /* 2BF800 */
        memcpy(raw_work+0x5e0,cursor+0x70,4); /* 2BF780 */
        memset(cursor,0,4);
        aux->load_flags_saved=0;
        if(cursor[0xc]!=1)return GH_AUX_OK;
        state->room=read_half(raw_work+0x5b4);
        work->flags|=0x40;
        state->stage=1;
    }
    if(state->stage==1){
        if(!bindings->bgm||!bindings->sound)return stop(aux,0x2d1768);
        gh_bgm_status bgm=gh_bgm_event_fade_002d1768(bindings->bgm,0,10,0,-1);
        if(bgm){
            uint32_t frontier=gh_bgm_requests_frontier(bindings->bgm);
            return stop(aux,frontier?frontier:0x2d1768);
        }
        bindings->sound->flags_ac|=0x200000;
        put_word(raw_work+0x5e4,word(raw_work+0x5e4)&~UINT32_C(0x4000));
        bindings->sound->flags_b0|=0x200000;
        state->stage=2;
    }

    const gh_checkpoint_bindings *checkpoint=bindings->checkpoint_bindings;
    const gh_game_title_player *player=door_bindings->player;
    if(!player||!player->ready||!player->view.bytes||player->view.size<0x54c||
       player->view.alias_f0!=player->view.bytes+0xb0)return stop(aux,0x2db6b0);
    if(state->room==0x100&&state->stage<4){
        if(!bindings->checkpoint||!checkpoint||checkpoint->work!=work||
           checkpoint->player!=&player->view||checkpoint->prepare!=door_bindings->prepare||
           !checkpoint->init||!checkpoint->save.bytes||checkpoint->save.size<GH_CORE_SAVE_BYTES||
           !checkpoint->destination||checkpoint->destination_size<GH_CHECKPOINT_BYTES)
            return stop(aux,0x2c0840);
        uint8_t *payload=checkpoint->destination;
        uint8_t *core=checkpoint->save.bytes;
        if(state->stage==2){
            /* 2C0840 retains only these original selected CoreSave fields. */
            state->clear_count=read_half(payload+0x10);
            state->difficulty=payload[0x1f];
            state->preserved_b90=word(payload+0xb90);
            state->output_mode=word(payload+0x170);
            state->world_active=word(payload+0x174);
            state->global_toggle=word(payload+0x178);
            if(gh_core_save_system_init(checkpoint->save))return stop(aux,0x1f9848);
            if(state->difficulty<3)core[0x1f]=state->difficulty;
            if(gh_core_save_game_init(checkpoint->save,work))return stop(aux,0x1f98d0);
            checkpoint->init->world_time=0;
            put_word(core+0xb8c,word(core+0xb8c)|1);
            state->stage=3;
        }
        gh_checkpoint_status capture=gh_game_checkpoint_002c05b0(bindings->checkpoint,checkpoint);
        if(capture)return stop(aux,bindings->checkpoint->frontier?bindings->checkpoint->frontier:0x2c05b0);
        if(gh_game_heat_reset_002a9908(checkpoint->init,work,checkpoint->save))return stop(aux,0x2a9908);
        payload[0xc]=0;payload[0xd]=1;
        memcpy(payload+0x10,&state->clear_count,2);
        payload[0x1f]=state->difficulty;
        put_word(payload+0xb90,state->preserved_b90);
        put_word(payload+0x170,state->output_mode);
        put_word(payload+0x174,state->world_active);
        put_word(payload+0x178,state->global_toggle);
        memcpy(core,payload,GH_CORE_SAVE_BYTES);
        state->stage=4;
    }

    uint32_t position[3];
    memcpy(position,player->view.alias_f0,sizeof(position));
    const int first_room=state->room==0x100;
    gh_door_status status=gh_game_door_request_001fff30(bindings->door,door_bindings,state->room,
        position,word(player->view.bytes+0x104),(uint8_t)first_room,UINT32_C(0x80000000),(uint8_t)first_room);
    if(status){
        uint32_t frontier=bindings->door?bindings->door->frontier:0;
        return stop(aux,frontier?frontier:0x1fff30);
    }
    state->stage=0;
    return GH_AUX_OK;
}
