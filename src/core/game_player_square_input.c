#include "game_player_square_input.h"
#include <math.h>
#include <string.h>

static uint16_t half(const uint8_t *p){uint16_t value;memcpy(&value,p,2);return value;}
static uint32_t word(const uint8_t *p){uint32_t value;memcpy(&value,p,4);return value;}
static void store_half(uint8_t *p,uint16_t value){memcpy(p,&value,2);}
static float number(const uint8_t *p){float value;memcpy(&value,p,4);return value;}

/* 12C000's two Square entries. 1FB830 returns zero on absent CoreSave
 * and otherwise reads a signed word at 1D0 + 24h * mode. */
static gh_player_frame_status combo_gate(const uint8_t *p,gh_core_save_view save,
    const gh_game_init_state *init,unsigned mode,int *allows,uint32_t *frontier)
{
    *allows=0;
    const uint16_t current=half(p+0x5e2);
    if((!mode&&init->heat[0x10])||current!=mode||!save.bytes)return GH_PLAYER_FRAME_OK;
    const size_t offset=0x1d0+mode*0x24;
    if(save.size<offset+4){
        *frontier=0x1fb830;
        return GH_PLAYER_FRAME_ARGUMENT;
    }
    int32_t maximum;
    memcpy(&maximum,save.bytes+offset,4);
    *allows=(int32_t)half(p+0x5e0)<maximum;
    return GH_PLAYER_FRAME_OK;
}

gh_player_frame_status gh_player_square_input_00123fc8(
    const gh_title_player_view *player,gh_core_save_view save,const gh_game_init_state *init,
    int caller_mode,int *original_return,uint32_t *frontier)
{
    if(!original_return||!frontier)return GH_PLAYER_FRAME_ARGUMENT;
    *original_return=0;
    *frontier=0;
    if(!player||!player->bytes||player->size<0x15f8||!init||!player->bytes[0x621]){
        *frontier=0x123fc8;
        return GH_PLAYER_FRAME_ARGUMENT;
    }
    uint8_t *p=player->bytes;
    float angle=0;
    if(p[0x61f]){
        angle=number(p+0x608);
        if(!isfinite(angle)){
            *frontier=0x123ff0;
            return GH_PLAYER_FRAME_NUMERIC;
        }
    }
    unsigned substate=0x0b;
    if(angle>2.35619449615478515625f){
        int allows;
        gh_player_frame_status status=combo_gate(p,save,init,3,&allows,frontier);
        if(status)return status;
        if(allows&&caller_mode&&!(half(p+0x3ac)&0x20))return GH_PLAYER_FRAME_OK;
        store_half(p+0x5e2,3);
        store_half(p+0x5e0,0);
    }else if(word(p+0x15f4)&0x8000){
        substate=0x0c;
    }else{
        int allows;
        gh_player_frame_status status=combo_gate(p,save,init,0,&allows,frontier);
        if(status)return status;
        if(allows&&caller_mode&&!(half(p+0x3ac)&0x20))return GH_PLAYER_FRAME_OK;
        if(half(p+0x5e2)){
            store_half(p+0x5e2,0);
            store_half(p+0x5e0,0);
        }
    }
    p[0x2f4]=0;
    p[0x2f5]=(uint8_t)substate;
    p[0x2f6]=0;
    p[0x2f7]=0;
    *original_return=1;
    return GH_PLAYER_FRAME_OK;
}

gh_player_frame_status gh_player_triangle_input_00123f44(
    const gh_title_player_view *player,gh_core_save_view save,int caller_mode,
    int *original_return,uint32_t *frontier)
{
    if(!original_return||!frontier)return GH_PLAYER_FRAME_ARGUMENT;
    *original_return=0;
    *frontier=0;
    if(!player||!player->bytes||player->size<0x15f8||!player->bytes[0x622]){
        *frontier=0x123f44;
        return GH_PLAYER_FRAME_ARGUMENT;
    }
    uint8_t *p=player->bytes;
    if(p[0x61f]||(word(p+0x15f4)&0x38000u)){
        *frontier=p[0x61f]?0x123ecc:0x123f44;
        return GH_PLAYER_FRAME_UNSUPPORTED;
    }
    /* 12C000(set1) returns true for the same set at/above its limit. */
    if(half(p+0x5e2)==1&&caller_mode&&!(half(p+0x3ac)&0x20)){
        int32_t maximum=0;
        if(save.bytes){
            if(save.size<0x1f8){*frontier=0x1fb830;return GH_PLAYER_FRAME_ARGUMENT;}
            memcpy(&maximum,save.bytes+0x1f4,4);
        }
        if((int32_t)half(p+0x5e0)>=maximum)return GH_PLAYER_FRAME_OK;
    }
    store_half(p+0x5e2,1);
    store_half(p+0x5e0,0);
    p[0x2f4]=p[0x2f6]=p[0x2f7]=0;
    p[0x2f5]=0x0b;
    *original_return=1;
    return GH_PLAYER_FRAME_OK;
}
