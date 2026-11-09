#include "../../src/core/frame_state.h"
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const gh_boot_work *w,size_t offset)
{uint32_t n;memcpy(&n,(const unsigned char*)w+offset,4);return n;}
static void numeric(void)
{
    const uint32_t vectors[][3]={
        {0,0,0xbf800000},{1,0,0xbf800000},{2,0x30000000,0xbf7fffff},
        {0x40000000,0x3e800000,0xbf000000},{0x7ffffffe,0x3effffff,0xb3800000},
        {0x80000000,0x3f000000,0},{0x80000002,0x3f000000,0},
        {0xc0000000,0x3f400000,0x3f000000},{0xffffffff,0x3f7fffff,0x3f7ffffe}};
    for(size_t i=0;i<sizeof(vectors)/sizeof(*vectors);++i){
        CHECK(gh_frame_unit_bits_0030f518_word(vectors[i][0])==vectors[i][1]);
        CHECK(gh_frame_jitter_bits_0031d020_word(vectors[i][0])==vectors[i][2]);
    }
}
int main(void)
{
    int original_rounding=fegetround();const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
    for(unsigned i=0;i<4;++i){CHECK(!fesetround(modes[i]));numeric();CHECK(fegetround()==modes[i]);}
    CHECK(!fesetround(original_rounding));
    gh_frame_random random={0};gh_game_mt mt,reference;gh_boot_work work,before;
    CHECK(gh_frame_seed_0031ceb8(&random,&mt,0)==GH_FRAME_ARGUMENT);
    CHECK(gh_frame_seed_0031ceb8(&random,&mt,GH_FRAME_CHOP_REFERENCE)==GH_FRAME_OK);
    CHECK(random.auxiliary_word==0x3124&&random.lcg.word==0x66b&&mt.index==624);
    reference=mt;memset(&work,0xa5,sizeof(work));before=work;
    for(unsigned phase=0;phase<4;phase++){
        work=before;work.phase569=(uint8_t)phase;
        CHECK(gh_frame_rate_policy(&work)==GH_FRAME_OK);
        CHECK(word(&work,0x55c)==(phase==1?0x3f000000u:0x3f800000u));
        CHECK(word(&work,0x560)==(phase==1?2u:4u)&&word(&work,0x564)==(phase==1));
        CHECK(((unsigned char*)&work)[0x568]==(phase==1?60u:30u)&&work.phase569==phase);
        CHECK(!memcmp(&work,&before,0x55c)&&!memcmp((unsigned char*)&work+0x56a,(unsigned char*)&before+0x56a,sizeof(work)-0x56a));
    }
    memset(&work,0,sizeof(work));uint32_t maximum=UINT32_MAX;memcpy((unsigned char*)&work+0x210,&maximum,4);
    CHECK(gh_frame_counter_advance(&work)==GH_FRAME_OK&&word(&work,0x210)==0);
    for(unsigned i=0;i<1000;++i){uint32_t discarded,value;
        CHECK(gh_game_mt_next_0030f348(&reference,&discarded)==GH_INIT_OK&&gh_game_mt_next_0030f348(&reference,&value)==GH_INIT_OK);
        CHECK(gh_frame_random_before_tasks_0030b008(&random,&mt,&work)==GH_FRAME_OK);
        CHECK(!memcmp(&mt,&reference,sizeof(mt))&&word(&work,0x604)==gh_frame_jitter_bits_0031d020_word(value));
    }
    CHECK(gh_frame_rate_policy(NULL)==GH_FRAME_ARGUMENT&&gh_frame_counter_advance(NULL)==GH_FRAME_ARGUMENT);
    puts("PASS original frame rate/counter/seed and two-MT advance; finite chop vectors independent of host rounding");return 0;
}
