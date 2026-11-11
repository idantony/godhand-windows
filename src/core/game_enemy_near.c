/* Patched SLUS_215.03 list and numeric behavior. Upstream symbols under LICENSE.upstream. */
#include "game_enemy_near.h"
#include <fenv.h>
#include <math.h>
#include <string.h>

#pragma STDC FENV_ACCESS ON

enum near_kind { EMA2_NEAR, ACTIVE_NEAR, OTHER_NEAR };
typedef struct {
    const gh_game_title_player *targets[64];
    uint8_t slots[64],count;
} near_matches;

static uint32_t word(const uint8_t *p)
{
    uint32_t value;
    memcpy(&value,p,4);
    return value;
}

static uint16_t half(const uint8_t *p)
{
    uint16_t value;
    memcpy(&value,p,2);
    return value;
}

static int hp_alive(uint16_t hp)
{
    return hp && hp<0x8000;
}

static gh_actor_motion_status stop(uint32_t *frontier,uint32_t pc,gh_actor_motion_status status)
{
    *frontier=pc;
    return status;
}

static const uint8_t *elf_bytes_at(const void *image,size_t size,uint32_t address,size_t extent)
{
    const uint8_t *elf=image;
    if(!elf||size<52||memcmp(elf,"\177ELF\1\1\1",7)||half(elf+18)!=8)return NULL;
    uint32_t headers=word(elf+28);
    unsigned stride=half(elf+42),count=half(elf+44);
    if(stride<32||headers>size||count>(size-headers)/stride)return NULL;
    for(unsigned i=0;i<count;i++){
        const uint8_t *p=elf+headers+(size_t)i*stride;
        uint32_t base=word(p+8),file_bytes=word(p+16);
        if(word(p)!=1||address<base||(uint64_t)address+extent>(uint64_t)base+file_bytes)
            continue;
        uint64_t offset=(uint64_t)word(p+4)+address-base;
        if(offset>size||extent>size-offset)return NULL;
        return elf+(size_t)offset;
    }
    return NULL;
}

static unsigned first_slot(const gh_game_init_state *manager)
{
    if(!manager->enemy_top)return word(manager->enemies+0x500)?64:UINT8_MAX;
    for(unsigned i=0;i<64;i++)
        if(manager->enemy_top==manager->enemies+(size_t)i*20)return i;
    return 64;
}

static int xz(const gh_title_player_view *view,float out[2])
{
    if(!view||!view->bytes||!view->alias_f0)return 0;
    uintptr_t base=(uintptr_t)view->bytes,position=(uintptr_t)view->alias_f0;
    if(position<base||position-base>view->size||12>view->size-(position-base))return 0;
    memcpy(out,view->alias_f0,4);
    memcpy(out+1,view->alias_f0+8,4);
    return isfinite(out[0])&&isfinite(out[1]);
}

/* 1F7EB0: VSUB, VMUL, VADDz.x. Y never enters the distance. */
static float distance_xz(const float a[2],const float b[2])
{
    volatile float dx=a[0]-b[0],dz=a[1]-b[1];
    volatile float xx=dx*dx,zz=dz*dz;
    volatile float sum=xx+zz;
    return sum;
}

static gh_actor_motion_status eligible(enum near_kind kind,const gh_title_player_view *view,
    const gh_title_player_view *skip,const void *elf,size_t elf_size,int *accept,uint32_t *frontier)
{
    const uint8_t *p=view->bytes;
    *accept=0;
    if(!p||view->size<0x300)return GH_ACTOR_MOTION_BOUNDS;
    const uint16_t id=half(p+0x2fe);
    if(kind==ACTIVE_NEAR){
        *accept=(id<0x2a0||id>=0x300)&&(p[0x2f4]==1||p[0x2f5]==2);
    }else if(kind==OTHER_NEAR){
        if((skip&&p==skip->bytes)||id<0x200||id>=0x250)return GH_ACTOR_MOTION_OK;
        if(view->size<0x16d4)return stop(frontier,0x26ecc0,GH_ACTOR_MOTION_BOUNDS);
        *accept=(word(p+0x16d0)&2u)!=0;
    }else{
        if(view->size<0x54c)return GH_ACTOR_MOTION_BOUNDS;
        static const char expected[]="cEma2";
        const uint8_t *literal=elf_bytes_at(elf,elf_size,0x44a7a8,sizeof(expected));
        const uint8_t *name=elf_bytes_at(elf,elf_size,word(p+0x4ac),sizeof(expected));
        if(!literal||memcmp(literal,expected,sizeof(expected))||!name)
            return stop(frontier,0x3a5678,GH_ACTOR_MOTION_UNSUPPORTED);
        *accept=!memcmp(name,literal,sizeof(expected))&&hp_alive(half(p+0x54a));
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status search(const gh_game_init_state *manager,const gh_title_player_view *origin,
    const gh_title_player_view *skip,float radius,enum near_kind kind,const void *elf,size_t elf_size,
    unsigned max_count,near_matches *matches,uint32_t *frontier,uint32_t entry)
{
    *frontier=entry;
    if(!manager||!origin||!isfinite(radius))return GH_ACTOR_MOTION_ARGUMENT;
    unsigned slot=first_slot(manager);
    if(slot==UINT8_MAX){*frontier=0;return GH_ACTOR_MOTION_OK;}
    fenv_t env;
    if(feholdexcept(&env))return GH_ACTOR_MOTION_UNSUPPORTED;
    if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ACTOR_MOTION_UNSUPPORTED;}
    volatile float range=radius*radius;
    float point[2];
    gh_actor_motion_status status=GH_ACTOR_MOTION_OK;
    uint64_t visited=0;
    if(!isfinite(range)||!xz(origin,point))status=stop(frontier,0x1f7eb0,GH_ACTOR_MOTION_BOUNDS);
    while(!status&&slot!=UINT8_MAX&&matches->count<max_count){
        if(slot>=64||(visited&(UINT64_C(1)<<slot))||manager->enemies[(size_t)slot*20+16]!=1){
            status=stop(frontier,entry,GH_ACTOR_MOTION_BUSY);
            break;
        }
        visited|=UINT64_C(1)<<slot;
        const gh_game_title_player *actor=manager->enemy_actors[slot];
        if(actor){
            int accept;
            status=eligible(kind,&actor->view,skip,elf,elf_size,&accept,frontier);
            if(!status&&accept){
                float target[2];
                if(!xz(&actor->view,target)){
                    status=stop(frontier,0x1f7eb0,GH_ACTOR_MOTION_BOUNDS);
                    break;
                }
                float distance=distance_xz(target,point);
                if(!isfinite(distance)){
                    status=stop(frontier,0x1f7eb0,GH_ACTOR_MOTION_UNSUPPORTED);
                    break;
                }
                if(kind==EMA2_NEAR?distance<=range:distance<range){
                    matches->targets[matches->count]=actor;
                    matches->slots[matches->count++]=(uint8_t)slot;
                }
            }
        }else if(word(manager->enemies+(size_t)slot*20+8)){
            status=stop(frontier,entry,GH_ACTOR_MOTION_BUSY);
        }
        slot=manager->enemy_next[slot];
    }
    if(fesetenv(&env))return stop(frontier,entry,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!status)*frontier=0;
    return status;
}

gh_actor_motion_status gh_enemy_find_ema2_near_00292dc8(const gh_game_init_state *manager,
    const gh_title_player_view *origin,float radius,const void *elf,size_t elf_bytes,
    const gh_game_title_player **found,uint32_t *frontier)
{
    if(!found||!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *found=NULL;
    near_matches matches={0};
    gh_actor_motion_status status=search(manager,origin,NULL,radius,EMA2_NEAR,elf,elf_bytes,
        1,&matches,frontier,0x292dc8);
    if(!status)*found=matches.targets[0];
    return status;
}

gh_actor_motion_status gh_enemy_active_near_002919f8(const gh_game_init_state *manager,
    const gh_title_player_view *origin,float radius,int *found,uint32_t *frontier)
{
    if(!found||!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *found=0;
    near_matches matches={0};
    gh_actor_motion_status status=search(manager,origin,NULL,radius,ACTIVE_NEAR,NULL,0,
        1,&matches,frontier,0x2919f8);
    if(!status)*found=matches.count!=0;
    return status;
}

gh_actor_motion_status gh_enemy_other_near_00291ac8(const gh_game_init_state *manager,
    const gh_title_player_view *origin,const gh_title_player_view *skip,float radius,
    int *found,uint32_t *frontier)
{
    if(!found||!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *found=0;
    near_matches matches={0};
    gh_actor_motion_status status=search(manager,origin,skip,radius,OTHER_NEAR,NULL,0,
        1,&matches,frontier,0x291ac8);
    if(!status)*found=matches.count!=0;
    return status;
}

static gh_actor_motion_status damage_amount(const gh_enemy_ema2_damage_bindings *b,
    uint16_t *amount,uint32_t *frontier)
{
    *amount=1;
    if(b->work->room!=0x501)return GH_ACTOR_MOTION_OK;
    if(!b->player||!b->player->bytes||b->player->size<0x604)
        return stop(frontier,0x2db6b0,GH_ACTOR_MOTION_BOUNDS);
    float power;
    memcpy(&power,b->player->bytes+0x600,4);
    fenv_t env;
    if(feholdexcept(&env))return stop(frontier,0x288f70,GH_ACTOR_MOTION_UNSUPPORTED);
    if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return stop(frontier,0x288f70,GH_ACTOR_MOTION_UNSUPPORTED);}
    volatile float scaled=power*10.0f;
    int valid=isfinite(scaled)&&scaled>= -2147483648.0f&&scaled<2147483648.0f;
    volatile uint16_t result=valid?(uint16_t)(int32_t)scaled:0;
    if(fesetenv(&env)||!valid)
        return stop(frontier,0x288f70,GH_ACTOR_MOTION_UNSUPPORTED);
    *amount=result;
    return GH_ACTOR_MOTION_OK;
}

static int same_bindings(const gh_enemy_ema2_damage_bindings *a,const gh_enemy_ema2_damage_bindings *b)
{
    return a->manager==b->manager&&a->origin==b->origin&&a->player==b->player&&a->work==b->work&&
        a->elf==b->elf&&a->elf_bytes==b->elf_bytes&&a->sound==b->sound&&a->sound_context==b->sound_context;
}

gh_actor_motion_status gh_enemy_damage_ema2_near_00292d18(gh_enemy_ema2_damage_call *call,
    const gh_enemy_ema2_damage_bindings *b,uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0x292d18;
    if(!call||!b||!b->work)return GH_ACTOR_MOTION_ARGUMENT;
    if(call->active&&(call->identity!=call||!same_bindings(&call->bindings,b)))
        return GH_ACTOR_MOTION_BUSY;
    if(!call->active){
        near_matches matches={0};
        gh_actor_motion_status status=search(b->manager,b->origin,NULL,2.0f,EMA2_NEAR,b->elf,b->elf_bytes,
            64,&matches,frontier,0x292d18);
        if(status)return status;
        for(unsigned i=0;i<matches.count;i++)
            if(matches.targets[i]->view.size<0x15b4)
                return stop(frontier,0x288f00,GH_ACTOR_MOTION_BOUNDS);
        uint16_t amount=0;
        if(matches.count){
            if(!b->sound)return stop(frontier,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
            status=damage_amount(b,&amount,frontier);
            if(status)return status;
        }
        *call=(gh_enemy_ema2_damage_call){.bindings=*b,.identity=call,.count=matches.count,
            .amount=amount,.active=1};
        memcpy(call->targets,matches.targets,matches.count*sizeof(matches.targets[0]));
        memcpy(call->slots,matches.slots,matches.count);
    }
    for(;call->cursor<call->count;call->cursor++){
        unsigned slot=call->slots[call->cursor];
        const gh_game_title_player *target=call->targets[call->cursor];
        if(b->manager->enemy_actors[slot]!=target||b->manager->enemies[(size_t)slot*20+16]!=1)
            return stop(frontier,0x292d18,GH_ACTOR_MOTION_BUSY);
        uint8_t *p=target->view.bytes;
        uint16_t hp=half(p+0x54a);
        if(!hp_alive(hp))continue;
        uint32_t pc=0;
        gh_actor_motion_status status=b->sound(b->sound_context,target,&pc);
        if(status)return stop(frontier,pc?pc:0x2cba48,status);
        hp=(uint16_t)(hp-call->amount);
        memcpy(p+0x54a,&hp,2);
        int alive=hp_alive(hp),react=(word(p+0x15b0)&2u)||(half(p+0x3ac)&2u);
        p[0x2f4]=(uint8_t)(alive?1:2);
        p[0x2f5]=(uint8_t)react;
        p[0x2f6]=0;
        p[0x2f7]=(uint8_t)(alive&&react);
    }
    call->complete=1;
    *frontier=0;
    return GH_ACTOR_MOTION_OK;
}
