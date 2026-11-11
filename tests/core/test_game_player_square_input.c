#include "../../src/core/game_player_square_input.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(expr) do { if(!(expr)){fprintf(stderr,"line %d: %s\n",__LINE__,#expr);return 1;} } while(0)
static uint32_t word(const void *p){uint32_t v;memcpy(&v,p,4);return v;}
static void put(void *p,uint32_t v){memcpy(p,&v,4);}

int main(int argc,char **argv)
{
    gh_disc *disc;
    C(argc==2&&!gh_disc_open(argv[1],&disc));
    uint8_t descriptors[16],combo[24];
    C(!gh_disc_read_boot_elf(disc,0x3bd0f0-0xff000+11*8,descriptors,sizeof(descriptors)));
    C(word(descriptors)==0xffff0000&&word(descriptors+4)==0x10d3a8&&
      word(descriptors+8)==0xffff0000&&word(descriptors+12)==0x10de70);
    C(!gh_disc_read_boot_elf(disc,0x41d1a0-0xff000,combo,sizeof(combo)));
    C(word(combo)==0x12c034&&word(combo+12)==0x12c078);
    gh_disc_close(disc);

    uint8_t player_bytes[GH_TITLE_PLAYER_BYTES]={0},save_bytes[GH_CORE_SAVE_BYTES];
    gh_title_player_view player;
    gh_boot_work work={0};
    gh_game_init_state *init=calloc(1,sizeof(*init));
    gh_core_save_view save={save_bytes,sizeof(save_bytes)};
    C(init&&!gh_title_player_construct_00107e40(player_bytes,sizeof(player_bytes),&player));
    C(!gh_core_save_system_init(save)&&!gh_core_save_game_init(save,&work));
    C(word(save_bytes+0x1d0)==4&&word(save_bytes+0x1b0)==0);
    player_bytes[0x621]=3;
    put(player_bytes+0x2f4,0x03020100);
    int result;
    uint32_t frontier;
    C(!gh_player_square_input_00123fc8(&player,save,init,0,&result,&frontier)&&result==1&&
      !frontier&&word(player_bytes+0x2f4)==0x00000b00&&!word(player_bytes+0x5e0)&&
      player_bytes[0x621]==3);
    put(player_bytes+0x2f4,0x03020100);
    C(!gh_player_square_input_00123fc8(&player,save,init,1,&result,&frontier)&&result==0&&
      word(player_bytes+0x2f4)==0x03020100);
    init->heat[0x10]=1;
    C(!gh_player_square_input_00123fc8(&player,save,init,1,&result,&frontier)&&result==1&&
      word(player_bytes+0x2f4)==0x00000b00);
    init->heat[0x10]=0;
    put(player_bytes+0x5e0,0x00020003);
    C(!gh_player_square_input_00123fc8(&player,save,init,1,&result,&frontier)&&result==1&&
      !word(player_bytes+0x5e0));

    player_bytes[0x61f]=1;
    put(player_bytes+0x608,0x4016cbe4); /* Exact 135-degree boundary is neutral. */
    put(player_bytes+0x15f4,0x8000);
    C(!gh_player_square_input_00123fc8(&player,save,init,0,&result,&frontier)&&result==1&&
      word(player_bytes+0x2f4)==0x00000c00);
    put(player_bytes+0x608,0x4016cbe5);
    C(!gh_player_square_input_00123fc8(&player,save,init,0,&result,&frontier)&&result==1&&
      word(player_bytes+0x2f4)==0x00000b00&&word(player_bytes+0x5e0)==0x00030000);
    C(!gh_player_square_input_00123fc8(&player,save,init,1,&result,&frontier)&&result==0);
    player_bytes[0x3ac]=0x20;
    C(!gh_player_square_input_00123fc8(&player,save,init,1,&result,&frontier)&&result==1&&
      word(player_bytes+0x5e0)==0x00030000);
    free(init);
    puts("PASS actual ISO mode table and Move dispatch, NewGame Square combo0, heat/caller gates, backwards boundary and alternate state");
    return 0;
}
