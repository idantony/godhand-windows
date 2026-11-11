#include "game_player_idle_target.h"
#include "game_action_button_set.h"
#include <fenv.h>
#include <math.h>
#include <string.h>

static uint16_t half(const void *p){uint16_t value;memcpy(&value,p,2);return value;}
static uint32_t word(const void *p){uint32_t value;memcpy(&value,p,4);return value;}
static float number(const void *p){float value;memcpy(&value,p,4);return value;}
static gh_player_target_status stop(uint32_t *f,uint32_t entry,gh_player_target_status status)
{
    *f=entry;
    return status;
}

static gh_player_target_status height_vector(const gh_title_player_view *target,
    const uint8_t **position,uint32_t *f)
{
    if(!target->alias_f0||target->size<0x568||
       target->original_vtable214!=word(target->bytes+0x214))
        return stop(f,0x123c08,GH_PLAYER_TARGET_ARGUMENT);
    switch(target->original_vtable214){
    case 0x4479c8: {
        float xyz[3];
        gh_player_target_status result=gh_enemy_target_position_00275978(target,xyz);
        if(result)return stop(f,0x275978,result);
        *position=target->bytes+0x1680;
        return GH_PLAYER_TARGET_OK;
    }
    case 0x42aa98:
    case 0x448ca0:
    case 0x44a660:
        /* The original 2A93A8 chain resolves VT+80/+84 to 2A9458. */
        *position=target->alias_f0;
        return GH_PLAYER_TARGET_OK;
    default:return stop(f,0x123c08,GH_PLAYER_TARGET_UNSUPPORTED);
    }
}

gh_player_target_status gh_player_idle_target_00123bf0(
    const gh_title_player_view *player,const gh_player_target *nearby,
    const gh_game_init_state *init,gh_game_action_buttons *buttons,
    int aux_6a0_present,int aux_698_present,
    gh_player_idle_target_result *out,uint32_t *f)
{
    if(!out||!f)return GH_PLAYER_TARGET_ARGUMENT;
    *out=(gh_player_idle_target_result){0};
    *f=0;
    if(!nearby||!nearby->handle)return GH_PLAYER_TARGET_OK;
    if(!player||!player->bytes||player->size<0x15f8||!player->alias_f0||
       !nearby->view.bytes)return GH_PLAYER_TARGET_ARGUMENT;

    const uint8_t *height;
    gh_player_target_status status=height_vector(&nearby->view,&height,f);
    if(status)return status;
    fenv_t env;
    if(feholdexcept(&env))return stop(f,0x123c20,GH_PLAYER_TARGET_NUMERIC);
    if(fesetround(FE_TONEAREST)){
        (void)fesetenv(&env);
        return stop(f,0x123c20,GH_PLAYER_TARGET_NUMERIC);
    }
    volatile float difference=number(player->alias_f0+4)-number(height+4);
    if(fesetenv(&env)||!isfinite(difference))
        return stop(f,0x123c20,GH_PLAYER_TARGET_NUMERIC);

    const uint8_t *target=nearby->view.bytes;
    unsigned kind=half(target+0x2fe);
    if(kind<0x200||kind>=0x2a0||!(half(target+0x3ac)&8)||
       half(target+0x2f4)==0x0202||fabsf(difference)>=.5f)
        return GH_PLAYER_TARGET_OK;

    uint8_t *p=player->bytes;
    uint32_t angle_bits=0x3f490fdb;
    if(p[0x622]&&p[0x61f]&&number(p+0x608)<number(&angle_bits)){
        out->target_640=*nearby;
        p[0x648]=60;
        p[0x2f4]=0;
        p[0x2f5]=14;
        p[0x2f6]=0;
        p[0x2f7]=0;
        out->transitioned=1;
        return GH_PLAYER_TARGET_OK;
    }
    if(aux_6a0_present||aux_698_present)return GH_PLAYER_TARGET_OK;
    if(!init)return stop(f,0x2dbc10,GH_PLAYER_TARGET_ARGUMENT);
    int16_t hp;
    int32_t busy;
    memcpy(&hp,p+0x54a,2);
    memcpy(&busy,init->enemies+0x514,4);
    if(hp<=0||(word(p+0x15f4)&0x400000)||busy>0||init->enemies[0x53c])
        return GH_PLAYER_TARGET_OK;

    const gh_action_set_request request={.priority=10,.life=4,
        .callback_id=0x2dbca0,.argument=nearby->view.bytes};
    unsigned slot;
    if(gh_action_button_set_001f7858(buttons,&request,&out->prompt_emitted,&slot,f)){
        if(!*f)*f=0x1f7858;
        return GH_PLAYER_TARGET_CALLEE;
    }
    return GH_PLAYER_TARGET_OK;
}
