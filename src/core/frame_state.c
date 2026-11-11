/* Actual patched ELF31CEB8,30F518,31CFE0,31D020 and30AC40 leaves.
 * Original MT implementation and attribution: game_init.c/LICENSE.upstream.
 * Numerical policy is a bounded implementation reference, not hardware proof. */
#include "frame_state.h"
#include <string.h>

static void put_word(gh_boot_work *work,size_t offset,uint32_t value)
{memcpy((unsigned char*)work+offset,&value,4);}
static uint32_t word(const gh_boot_work *work,size_t offset)
{uint32_t value;memcpy(&value,(const unsigned char*)work+offset,4);return value;}
static unsigned high_bit(uint32_t value)
{unsigned top=0;while(value>>=1)++top;return top;}
static uint32_t encode_chop(uint32_t magnitude,unsigned exponent_offset)
{
    if(!magnitude)return 0;
    unsigned top=high_bit(magnitude);
    uint32_t significand=top>23?magnitude>>(top-23):magnitude<<(23-top);
    return ((top+exponent_offset)<<23)|(significand&UINT32_C(0x7fffff));
}
static uint32_t converted_integer(uint32_t value)
{
    if(!value)return 0;
    unsigned top=high_bit(value);
    return top>23?(value>>(top-23))<<(top-23):value;
}
gh_frame_error gh_frame_seed_0031ceb8(gh_frame_random *random,gh_game_mt *mt,gh_frame_numeric_policy policy)
{
    if(!random||!mt||policy!=GH_FRAME_CHOP_REFERENCE)return GH_FRAME_ARGUMENT;
    random->auxiliary_word=0x3124;random->lcg.word=0x66b;random->policy=policy;
    gh_game_mt_seed_0030f2b0(mt,UINT32_C(0xd8a9523d));return GH_FRAME_OK;
}
gh_frame_error gh_frame_random_byte_0031cef8(gh_frame_random *random,uint8_t *out)
{
    if(!random||!out||random->policy!=GH_FRAME_CHOP_REFERENCE)return GH_FRAME_ARGUMENT;
    uint32_t product=(random->auxiliary_word&0xffffu)*5u;
    random->auxiliary_word=((product&0xffu)<<8)|((product>>8)&0xffu);
    *out=(uint8_t)random->auxiliary_word;
    return GH_FRAME_OK;
}
uint32_t gh_frame_unit_bits_0030f518_word(uint32_t value)
{return encode_chop(value>>1,96);}
uint32_t gh_frame_jitter_bits_0031d020_word(uint32_t value)
{
    /* CVT.S.W loses integer low bits first. SUB.S then chops the exact
     * difference; ADD.S doubles it exactly. Collapsing before CVT is wrong. */
    uint32_t converted=converted_integer(value>>1);
    int64_t difference=(int64_t)converted-INT64_C(0x40000000);
    uint32_t magnitude=(uint32_t)(difference<0?-difference:difference);
    return encode_chop(magnitude,97)|(difference<0?UINT32_C(0x80000000):0);
}
gh_frame_error gh_frame_rate_policy(gh_boot_work *work)
{
    if(!work)return GH_FRAME_ARGUMENT;
    int half=work->phase569==1;
    put_word(work,0x55c,half?UINT32_C(0x3f000000):UINT32_C(0x3f800000));
    put_word(work,0x560,half?2:4);
    /* Actual SB: +569 is a distinct mode byte, never part of the FPS store. */
    ((unsigned char*)work)[0x568]=(uint8_t)(half?60:30);
    put_word(work,0x564,(uint32_t)half);return GH_FRAME_OK;
}
gh_frame_error gh_frame_counter_advance(gh_boot_work *work)
{if(!work)return GH_FRAME_ARGUMENT;put_word(work,0x210,word(work,0x210)+1u);return GH_FRAME_OK;}
gh_frame_error gh_frame_random_before_tasks_0030b008(gh_frame_random *random,gh_game_mt *mt,gh_boot_work *work)
{
    uint32_t discarded,value;
    if(!random||!mt||!work||random->policy!=GH_FRAME_CHOP_REFERENCE)return GH_FRAME_ARGUMENT;
    if(gh_game_mt_next_0030f348(mt,&discarded)!=GH_INIT_OK)return GH_FRAME_RANDOM;
    if(gh_game_mt_next_0030f348(mt,&value)!=GH_INIT_OK)return GH_FRAME_RANDOM;
    put_word(work,0x604,gh_frame_jitter_bits_0031d020_word(value));return GH_FRAME_OK;
}
