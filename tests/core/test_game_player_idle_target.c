#include "../../src/core/game_player_idle_target.h"
#include "../../src/core/game_enemy_2a2.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { if(!(expr)){fprintf(stderr,"line %d: %s\n",__LINE__,#expr);return 1;} } while(0)
static uint32_t word(const void *p){uint32_t v;memcpy(&v,p,4);return v;}

int main(int argc,char **argv)
{
    gh_disc *disc;
    CHECK(argc==2&&!gh_disc_open(argv[1],&disc));
    uint8_t elf[8];
    CHECK(!gh_disc_read_boot_elf(disc,0x4479c8-0xff000+0x68,elf,8)&&
          word(elf)==0&&word(elf+4)==0x275978);
    CHECK(!gh_disc_read_boot_elf(disc,0x448ca0-0xff000+0x68,elf,8)&&
          word(elf)==0&&word(elf+4)==0x2a93a8);
    CHECK(!gh_disc_read_boot_elf(disc,0x448ca0-0xff000+0x80,elf,8)&&
          word(elf)==0&&word(elf+4)==0x2a9458);
    gh_disc_close(disc);

    uint8_t player_bytes[GH_TITLE_PLAYER_BYTES]={0},enemy_bytes[GH_EM202_BYTES]={0};
    uint8_t em2a2_bytes[GH_EM2A2_BYTES]={0};
    gh_title_player_view player;
    gh_player_target enemy={.handle=254},em2a2={.handle=255};
    CHECK(!gh_title_player_construct_00107e40(player_bytes,sizeof(player_bytes),&player));
    CHECK(!gh_enemy202_construct_00201fa0(enemy_bytes,sizeof(enemy_bytes),&enemy.view));
    CHECK(!gh_enemy2a2_construct_00284a28(em2a2_bytes,sizeof(em2a2_bytes),&em2a2.view));
    memcpy(enemy_bytes+0x2fe,&(uint16_t){0x202},2);
    memcpy(enemy_bytes+0x564,&(uint32_t){0x202},4);
    memcpy(em2a2_bytes+0x2fe,&(uint16_t){0x2a2},2);
    memcpy(enemy_bytes+0x3ac,&(uint16_t){8},2);
    memcpy(em2a2_bytes+0x3ac,&(uint16_t){8},2);
    memcpy(player_bytes+0x54a,&(uint16_t){50},2);
    memcpy(enemy.view.alias_f0,&(float[3]){4,.5f,5},12);
    gh_game_init_state *init=calloc(1,sizeof(*init));
    CHECK(init);
    gh_game_action_buttons buttons={0};
    gh_game_action_buttons_reset_001f72c0(&buttons);
    gh_player_idle_target_result result;
    uint32_t frontier;

    CHECK(!gh_player_idle_target_00123bf0(&player,&enemy,init,&buttons,0,0,&result,&frontier)&&
          !result.transitioned&&!result.prompt_emitted&&!frontier&&
          !memcmp(enemy_bytes+0x1680,enemy.view.alias_f0,12));
    memcpy(enemy.view.alias_f0+4,&(float){.499f},4);
    memcpy(enemy_bytes+0x2f4,&(uint16_t){0x202},2);
    CHECK(!gh_player_idle_target_00123bf0(&player,&enemy,init,&buttons,0,0,&result,&frontier)&&
          !result.prompt_emitted);
    memcpy(enemy_bytes+0x2f4,&(uint16_t){0x101},2);
    CHECK(!gh_player_idle_target_00123bf0(&player,&em2a2,init,&buttons,0,0,&result,&frontier)&&
          !result.prompt_emitted);
    CHECK(!gh_player_idle_target_00123bf0(&player,&enemy,init,&buttons,1,0,&result,&frontier)&&
          !result.prompt_emitted);
    memcpy(init->enemies+0x514,&(int32_t){1},4);
    CHECK(!gh_player_idle_target_00123bf0(&player,&enemy,init,&buttons,0,0,&result,&frontier)&&
          !result.prompt_emitted);
    memset(init->enemies+0x514,0,4);
    CHECK(!gh_player_idle_target_00123bf0(&player,&enemy,init,&buttons,0,0,&result,&frontier)&&
          result.prompt_emitted&&!result.transitioned&&!frontier);
    CHECK(buttons.entries[0].callback_id10==0x2dbca0&&buttons.entries[0].callback_argument==enemy_bytes&&
          buttons.entries[0].life==4&&buttons.entries[0].priority==10&&buttons.entries[0].kind==0);
    player_bytes[0x622]=1;player_bytes[0x61f]=1;player_bytes[0x2f7]=9;
    memcpy(player_bytes+0x608,&(float){.25f},4);
    CHECK(!gh_player_idle_target_00123bf0(&player,&enemy,init,&buttons,0,0,&result,&frontier)&&
          !result.prompt_emitted&&result.transitioned&&result.target_640.view.bytes==enemy_bytes&&
          result.target_640.handle==254&&player_bytes[0x648]==60&&player_bytes[0x2f5]==14&&
          player_bytes[0x2f7]==0);
    free(init);
    puts("PASS actual ISO 0202/02A2 VT height slots, 0.5 boundary, downed pair gate, native prompt, and typed attack14 transition");
    return 0;
}
