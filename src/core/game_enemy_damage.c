#include "game_enemy_damage.h"
#include "game_enemy_row2.h"
#include "game_camera.h"
#include "frame_state.h"
#include "game_hitarea_frame.h"
#include "game_player_targets.h"
#include <fenv.h>
#include <math.h>
#include <string.h>

static uint16_t half(const uint8_t *p) {
    return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}

static uint32_t word(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static gh_actor_motion_status stop(uint32_t *frontier, uint32_t entry) {
    *frontier = entry;
    return GH_ACTOR_MOTION_UNSUPPORTED;
}

static int r100_stamp19(const gh_enemy_actor *enemy, const uint8_t *actor) {
    return word(actor + 0x564) == 0x203 && half(actor + 0x640) == 19 &&
        enemy->entry.b.work && (uint16_t)enemy->entry.b.work->room == 0x100;
}

static int r100_at2_em03(const gh_enemy_actor *enemy, const uint8_t *actor) {
    return enemy->entry.b.work && enemy->entry.b.work->room == 0x100 &&
        word(actor + 0x564) == 0x203 && word(actor + 0x16c4) == 6 &&
        ((half(actor + 0x640) == 5 && actor[0x1650] == 2) ||
         (half(actor + 0x640) == 11 && actor[0x1650] == 5));
}

static int r100_west_em00(const gh_enemy_actor *enemy,const uint8_t *actor)
{
    if(!enemy->entry.b.work||enemy->entry.b.work->room!=0x100||
       enemy->entry.object.view.size<0x16c8||word(actor+0x564)!=0x200)return 0;
    const unsigned stamp=half(actor+0x640);
    const uint32_t home_mode=word(actor+0x16c4);
    return (stamp==3&&actor[0x1650]==2&&home_mode==6)||
        (stamp==31&&!actor[0x1650]&&!home_mode);
}

static int r100_west_em02_em03(const gh_enemy_actor *enemy, const uint8_t *actor) {
    if (!enemy->entry.b.work || enemy->entry.b.work->room != 0x100 ||
        enemy->entry.object.view.size < 0x16c8)
        return 0;
    const unsigned stamp = half(actor + 0x640);
    const uint32_t dependency = word(actor + 0x564);
    const uint32_t mode = word(actor + 0x16c4);
    const unsigned appearance = actor[0x1650];
    const int approach = mode == 2 && appearance == 0 &&
        ((stamp == 29 && dependency == 0x202) || (stamp == 30 && dependency == 0x203));
    const int formation = mode == 6 && appearance == 2 &&
        ((stamp == 4 && dependency == 0x202) || (stamp == 5 && dependency == 0x203));
    const int at3 = mode == 0 && appearance == 0 && stamp == 32 && dependency == 0x202;
    return approach || formation || at3;
}

static int r100_stamp20(const gh_enemy_actor *enemy, const uint8_t *actor) {
    return word(actor + 0x564) == 0x200 && half(actor + 0x640) == 20 &&
        enemy->entry.b.work && (uint16_t)enemy->entry.b.work->room == 0x100;
}

static int r100_stamp8(const gh_enemy_actor *enemy, const uint8_t *actor) {
    return word(actor + 0x564) == 0x200 && half(actor + 0x640) == 8 &&
        enemy->entry.b.work && (uint16_t)enemy->entry.b.work->room == 0x100;
}

static int r103_first_wave_em00(const gh_enemy_actor *enemy)
{
    return enemy->entry.b.work && enemy->entry.request.dependency_kind == 0x200 &&
        gh_enemy204_is_r103_entry(&enemy->entry.request, enemy->entry.b.work->room);
}

static int r103_arena_punch_target(const gh_enemy_actor *enemy,uint32_t dependency)
{
    return enemy->entry.b.work && enemy->entry.request.dependency_kind==dependency &&
        (dependency==0x200||dependency==0x203) &&
        gh_enemy204_is_r103_entry(&enemy->entry.request,enemy->entry.b.work->room);
}

static int r103_arena_em0a(const gh_enemy_actor *enemy,const uint8_t *actor)
{
    return enemy->entry.b.work&&word(actor+0x564)==0x20a&&
        enemy->entry.request.dependency_kind==0x20a&&
        gh_enemy204_is_r103_entry(&enemy->entry.request,enemy->entry.b.work->room);
}

static int r103_boss_player_strike_target(const gh_enemy_actor *enemy,const uint8_t *actor)
{
    const uint32_t dependency=word(actor+0x564);
    return enemy->entry.b.work&&dependency>=0x214&&dependency<=0x215&&
        enemy->entry.request.dependency_kind==dependency&&enemy->entry.request.entry_number==dependency-0x214&&
        gh_enemy204_is_r103_entry(&enemy->entry.request,enemy->entry.b.work->room);
}

static int r101_side_arena_punch(const gh_enemy_actor *enemy,const uint8_t *actor,uint16_t effect)
{
    const unsigned stamp=half(actor+0x640);
    const uint32_t dependency=word(actor+0x564);
    return effect==7&&enemy->entry.b.work&&enemy->entry.b.work->room==0x101&&
        enemy->entry.object.view.size>0x1650&&actor[0x1650]==6&&
        enemy->entry.request.entry_number==stamp&&enemy->entry.request.dependency_kind==dependency&&
        ((dependency==0x200&&(stamp==0x16||stamp==0x18))||
         (stamp==0x27&&dependency==0x202)||(dependency==0x203&&(stamp==0x20||stamp==0x28))||
         (dependency==0x20f&&(stamp==0x1a||stamp==0x1b||stamp==0x22||stamp==0x23)));
}

static int room_unarmed_punch(const gh_enemy_actor *enemy, const uint8_t *actor, uint16_t effect)
{
    const uint32_t dependency=word(actor+0x564);
    return effect==7&&enemy->entry.b.work&&
        ((dependency==0x200&&enemy->entry.b.work->room==0x102)||r103_arena_punch_target(enemy,dependency)||
         r101_side_arena_punch(enemy,actor,effect)||r103_boss_player_strike_target(enemy,actor));
}

static int r102_close_em00_punch(const gh_enemy_actor *enemy, const uint8_t *actor, uint16_t effect) {
    if(!room_unarmed_punch(enemy,actor,effect)||enemy->entry.b.work->room!=0x102)return 0;
    const unsigned stamp=half(actor+0x640);
    return (stamp==0x10&&!actor[0x1650])||(stamp==0x0b&&actor[0x1650]==5);
}

static int r102_em03_punch(const gh_enemy_actor *enemy, const uint8_t *actor, uint16_t effect) {
    return effect == 7 && enemy->entry.object.view.size > 0x1650 &&
        word(actor + 0x564) == 0x203 && half(actor + 0x640) == 8 && !actor[0x1650] &&
        enemy->entry.b.work && enemy->entry.b.work->room == 0x102;
}

static int r102_at0b_em02_punch(const gh_enemy_actor *enemy,const uint8_t *actor,uint16_t effect)
{
    return effect==7&&word(actor+0x564)==0x202&&half(actor+0x640)==0x17&&
        !actor[0x1650]&&word(actor+0x16c4)==2&&enemy->entry.b.work&&enemy->entry.b.work->room==0x102;
}

static int r101_key_carrier_punch(const gh_enemy_actor *enemy,const uint8_t *actor,uint16_t effect)
{
    return effect==7&&enemy->entry.object.view.size>0x1650&&enemy->entry.b.work&&
        enemy->entry.b.work->room==0x101&&word(actor+0x564)==0x227&&
        half(actor+0x640)==9&&actor[0x1650]==4&&word(actor+0x560)==0x3e3;
}

static int r101_wave4_em27(const gh_enemy_actor *enemy,const uint8_t *actor)
{
    const unsigned stamp=half(actor+0x640);
    return enemy->entry.b.work&&enemy->entry.b.work->room==0x101&&
        enemy->entry.object.view.size>0x1650&&enemy->entry.object.view.original_vtable214==0x4479c8&&
        word(actor+0x564)==0x227&&half(actor+0x2fe)==0x202&&actor[0x1650]==6&&
        (stamp==0x1e||stamp==0x26);
}

static int original_em27_punch(const gh_enemy_actor *enemy, const uint8_t *actor, uint16_t effect) {
    if(effect!=7||enemy->entry.object.view.size<=0x1650||
       word(actor+0x564)!=0x227||!enemy->entry.b.work)return 0;
    const unsigned room=enemy->entry.b.work->room;
    const unsigned stamp=half(actor+0x640);
    return r101_key_carrier_punch(enemy,actor,effect)||r101_wave4_em27(enemy,actor)||
        (room==0x102&&stamp==4&&!actor[0x1650]);
}

static int r102_stamp2(const gh_enemy_actor *enemy, const uint8_t *actor) {
    return enemy->entry.object.view.size > 0x1650 && word(actor + 0x564) == 0x20a && half(actor + 0x640) == 2 &&
        actor[0x1650] == 5 && enemy->entry.b.work && enemy->entry.b.work->room == 0x102;
}

static int r102_stamp2_punch(const gh_enemy_actor *enemy, const uint8_t *actor, uint16_t effect) {
    return effect == 7 && r102_stamp2(enemy, actor);
}

static int r104_em03_damage_receiver(const gh_enemy_actor *enemy,const uint8_t *actor)
{
    return gh_enemy203_is_r104_entry(&enemy->entry.request,enemy->entry.b.work->room)&&
        half(actor+0x2fe)==0x203&&word(actor+0x564)==0x250;
}

/* The original amplified Give12 chooses the EM03 fall and doubles its power. */
static int r104_amplified_triangle_fall(const gh_enemy_actor *enemy,const uint8_t *p)
{
    return r104_em03_damage_receiver(enemy,p)&&half(p+0x2f4)==0x0701&&
        !p[0x2f7]&&word(p+0x17b4)==24;
}

static int r200_em02_triangle(const gh_enemy_actor *enemy,const uint8_t *actor)
{
    return enemy&&enemy->entry.b.work&&enemy->entry.b.work->room==0x200&&
        enemy->entry.object.view.original_vtable214==0x4479c8&&half(actor+0x2fe)==0x201&&
        word(actor+0x564)==0x202&&half(actor+0x640)==1&&!actor[0x1650];
}

static int r200_em02_triangle_recoil(const gh_enemy_actor *enemy,const uint8_t *actor)
{
    return r200_em02_triangle(enemy,actor)&&word(actor+0x2f4)==0x0101u&&
        word(actor+0x17b4)==24;
}

static int r200_em1a_triangle(const gh_enemy_actor *enemy,const uint8_t *actor)
{
    return enemy&&enemy->entry.b.work&&enemy->entry.b.work->room==0x200&&
        enemy->entry.object.view.original_vtable214==0x4479c8&&half(actor+0x2fe)==0x201&&
        word(actor+0x564)==0x21a&&!half(actor+0x640)&&!actor[0x1650];
}

static int triangle_take_supported(const gh_enemy_actor *enemy, const uint8_t *actor, uint16_t effect) {
    return effect==0x12&&(r100_stamp19(enemy,actor)||r100_stamp20(enemy,actor)||
        r100_stamp8(enemy,actor)||r100_at2_em03(enemy,actor)||r104_em03_damage_receiver(enemy,actor)||
        r100_west_em00(enemy,actor)||r100_west_em02_em03(enemy,actor)||
        r103_arena_punch_target(enemy,word(actor+0x564))||r103_arena_em0a(enemy,actor)||
        r103_boss_player_strike_target(enemy,actor)||r200_em02_triangle(enemy,actor)||r200_em1a_triangle(enemy,actor));
}

/* PL00 114120 submits Give2A with the original shape17 attachment. */
static int r103_heat_activation_contact(const gh_enemy_actor *enemy,
    const gh_title_player_view *player,uint16_t effect,int32_t power)
{
    const uint8_t *target=enemy->entry.object.view.bytes;
    const int side_em02=enemy->entry.b.work&&enemy->entry.b.work->room==0x101&&
        enemy->entry.request.appearance==6&&enemy->entry.request.entry_number==0x27&&
        enemy->entry.request.dependency_kind==0x202&&word(target+0x564)==0x202&&half(target+0x640)==0x27;
    return effect==0x2a&&power==5&&(r103_first_wave_em00(enemy)||r103_boss_player_strike_target(enemy,target)||side_em02||r101_wave4_em27(enemy,target))&&
        player->size>0x1684&&half(player->bytes+0x2f4)==0x0800&&
        player->bytes[0x1684]==1&&enemy->entry.b.init->heat[0x10]&&
        (word(player->bytes+0x15f4)&0x10u);
}

static int back_circle_contact(const gh_enemy_actor *enemy,
    const gh_title_player_view *player, uint16_t effect)
{
    const uint8_t *target=enemy->entry.object.view.bytes;
    return effect==0x0f&&half(player->bytes+0x2f4)==0x0b00&&
        word(player->bytes+0x5e8)==52&&
        (r103_boss_player_strike_target(enemy,target)||r104_em03_damage_receiver(enemy,target));
}

/* 1142E0 sends the same Give2A at the start of the exhaustion clip.
 * The Heat manager has already cleared active mode through 1239E4 -> 2A9CC8. */
static int r103_heat_exhaustion_contact(const gh_enemy_actor *enemy,
    const gh_title_player_view *player,uint16_t effect,int32_t power)
{
    const uint8_t *target=enemy->entry.object.view.bytes;
    const int side_em00=enemy->entry.b.work&&enemy->entry.b.work->room==0x101&&
        enemy->entry.request.appearance==6&&enemy->entry.request.entry_number==0x18&&
        enemy->entry.request.dependency_kind==0x200&&word(target+0x564)==0x200&&half(target+0x640)==0x18;
    const int side_em03=enemy->entry.b.work&&enemy->entry.b.work->room==0x101&&
        enemy->entry.request.appearance==6&&enemy->entry.request.entry_number==0x28&&
        enemy->entry.request.dependency_kind==0x203&&word(target+0x564)==0x203&&half(target+0x640)==0x28;
    return effect==0x2a&&power==5&&(r103_boss_player_strike_target(enemy,target)||side_em00||side_em03)&&player->size>0x1684&&
        word(player->bytes+0x2f4)==0x00010900u&&player->bytes[0x1684]==1&&
        !enemy->entry.b.init->heat[0x10];
}

static int r103_heat_transition_contact(const gh_enemy_actor *enemy,
    const gh_title_player_view *player,uint16_t effect,int32_t power)
{
    return r103_heat_activation_contact(enemy,player,effect,power)||
        r103_heat_exhaustion_contact(enemy,player,effect,power);
}

static int r100_amplified_triangle_receiver(const gh_enemy_actor *enemy,const uint8_t *actor)
{
    return r100_stamp20(enemy,actor)||r100_stamp8(enemy,actor)||
        (word(actor+0x564)==0x202&&r100_west_em02_em03(enemy,actor))||
        (r100_stamp19(enemy,actor)&&half(actor+0x2f4)==0x0101&&!actor[0x2f6]);
}

static gh_actor_motion_status take_player_power_0028fb88(uint8_t *,const gh_title_player_view *,
    const gh_game_init_state *,int32_t,uint32_t *);

static int r104_turn_is_exposed(const uint8_t *actor)
{
    return half(actor+0x2f4)==0x0800&&actor[0x2f6]==1&&actor[0x2f7]==1&&
        !(half(actor+0x3ac)&0x100u)&&(word(actor+0x16d4)&1u);
}

static int r104_brace_is_exposed(const uint8_t *actor)
{
    return half(actor+0x2f4)==0x2100&&actor[0x2f6]==1&&
        !(half(actor+0x3ac)&0x100u)&&(word(actor+0x16d0)&0x80e020u)==0x800000u;
}

static int r104_player_punch(const gh_enemy_actor *enemy,const uint8_t *actor,uint16_t effect)
{
    if(effect!=7||!gh_enemy203_is_r104_entry(&enemy->entry.request,enemy->entry.b.work->room)||
       half(actor+0x2fe)!=0x203||word(actor+0x564)!=0x250)return 0;
    const uint16_t state=half(actor+0x2f4);
    const unsigned phase=actor[0x2f6],variant=actor[0x2f7];
    const uint16_t flags=half(actor+0x3ac);
    const int amplified=(flags&0x100u)!=0;
    if(state==0x0800)
        return r104_turn_is_exposed(actor);
    return !variant&&((state==0x1a00&&(phase==3||phase==5))||
        (state==0x1500&&phase==1&&(amplified||flags==0x800))||
        (state==0x1600&&phase==1&&amplified)||
        r104_brace_is_exposed(actor));
}

gh_actor_motion_status gh_enemy202_damage_0028fb88(
    gh_enemy_actor *enemy, const gh_title_player_view *player, int32_t power, uint32_t *frontier) {
    if (!frontier) return GH_ACTOR_MOTION_ARGUMENT;
    *frontier = 0;
    if (!enemy || !player || !player->bytes || player->size < 0x600)
        return GH_ACTOR_MOTION_ARGUMENT;
    gh_title_player_view *view = &enemy->entry.object.view;
    if (!view->bytes || view->size < 0x642 || view->original_vtable214 != 0x4479c8 ||
        word(view->bytes + 0x214) != view->original_vtable214 ||
        (half(view->bytes+0x2fe)!=0x202&&!r103_arena_punch_target(enemy,word(view->bytes+0x564))&&
         !r103_arena_em0a(enemy,view->bytes)&&!r103_boss_player_strike_target(enemy,view->bytes)&&
         !r104_player_punch(enemy,view->bytes,7)&&
         !(power==25&&back_circle_contact(enemy,player,0x0f))&&
         !(r200_em1a_triangle(enemy,view->bytes)&&power==12&&
           word(player->bytes+0x2f4)==0x00010b00u)&&
         !(r200_em02_triangle(enemy,view->bytes)&&word(player->bytes+0x2f4)==0x00010b00u&&
           (power==24||(power==12&&(word(view->bytes+0x2f4)==0x00010101u||
             word(view->bytes+0x2f4)==0x00011500u))))&&
         !(r104_em03_damage_receiver(enemy,view->bytes)&&word(player->bytes+0x5e8)==29&&
           power==((half(view->bytes+0x3ac)&0x100u)?24:12))) ||
        !enemy->hitareas.created[0])
        return stop(frontier, 0x20a5dc);
    const gh_hitarea_object *body = &enemy->hitareas.units[0];
    gh_title_player_view *owner;
    const gh_title_player_view *source;
    if (body->fallback || body->group != 2 || body->size < 0x64 ||
        body->state != enemy->entry.b.init || gh_hitarea_object_actor(body, &owner) ||
        owner != view || gh_hitarea_object_last_hit_actor(body, &source) ||
        source != player || half(player->bytes + 0x2fe) != 0x100 ||
        !(word(body->bytes + 0x60) & 1) ||
        (half(body->bytes + 0x46) != 7 &&
            !triangle_take_supported(enemy, view->bytes, half(body->bytes + 0x46)) &&
            !back_circle_contact(enemy,player,half(body->bytes+0x46)) &&
            !r103_heat_transition_contact(enemy,player,half(body->bytes+0x46),power)))
        return stop(frontier, 0x20a5dc);

    return take_player_power_0028fb88(view->bytes,player,body->state,power,frontier);
}

static gh_actor_motion_status take_player_power_0028fb88(uint8_t *actor,const gh_title_player_view *player,
    const gh_game_init_state *init,int32_t power,uint32_t *frontier)
{
    const uint32_t kind = word(actor + 0x564);
    if (kind == 0x21f || kind == 0x23b) return stop(frontier, 0x28fd38);
    fenv_t saved;
    if (feholdexcept(&saved)) return stop(frontier, 0x28fcf8);
    if (fesetround(FE_TOWARDZERO)) {
        (void)fesetenv(&saved);
        return stop(frontier, 0x28fcf8);
    }
    volatile float multiplier = word(player->bytes + 0x5fc) ? 1.5f : 1.0f;
    if (init->heat[0x10]) {
        if (actor[0x622] == 1) multiplier = multiplier * 1.5f;
        else multiplier = multiplier + multiplier;
    }
    volatile float input = (float)power;
    volatile float product = input * multiplier;
    volatile float rounded = product + 0.5f;
    if (fesetenv(&saved) || rounded < -2147483648.0f || rounded >= 2147483648.0f)
        return stop(frontier, 0x28fd14);
    int32_t damage = (int32_t)rounded;
    if (damage > 32000) damage = 32000;
    uint16_t hp = (uint16_t)((uint32_t)half(actor + 0x54a) - (uint32_t)damage);
    if (!hp || hp >= 0x8000) hp = 0;
    memcpy(actor + 0x54a, &hp, sizeof(hp));
    return GH_ACTOR_MOTION_OK;
}


static float number(const uint8_t *p) {
    float value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void put(uint8_t *p, uint32_t value) {
    memcpy(p, &value, sizeof(value));
}

static void put_half(uint8_t *p, uint16_t value) {
    memcpy(p, &value, sizeof(value));
}

static gh_actor_motion_status gain_heat_002a9978(const gh_enemy_entry_bindings *b,float amount,uint32_t *frontier) {
    uint32_t pc=0;
    const gh_init_status status=gh_game_heat_gain_002a9978(b->init,b->work,b->save,amount,&pc);
    return status?stop(frontier,pc):GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_counter_heat_002a9978(gh_enemy_actor *enemy, uint32_t *frontier) {
    if (!enemy || !enemy->entry.b.init || !enemy->entry.object.view.bytes ||
        enemy->entry.object.view.size < 0x642)
        return stop(frontier, 0x2a9978);
    const gh_title_player_view *view = &enemy->entry.object.view;
    const uint8_t *p = view->bytes;
    const unsigned stamp = word(p + 0x640) & 0xffffu;
    const gh_boot_work *work = enemy->entry.b.work;
    const int story = work && work->room == 0x102 && view->size > 0x1650 &&
        view->original_vtable214 == 0x4479c8 &&
        ((stamp == 11 && p[0x1650] == 5) || (stamp == 3 && p[0x1650] == 1) ||
         (stamp == 7 && view->size >= 0x16c8 && !p[0x1650] && word(p + 0x16c4) == 2)) &&
        p[0x2f4] == 1 &&
        p[0x2f5] == 0x1d && p[0x2f6] == 1 && p[0x2f7] == 1;
    const int wave4 = work && work->room == 0x101 && view->size > 0x1650 &&
        view->original_vtable214 == 0x4479c8 && word(p + 0x564) == 0x227 &&
        half(p + 0x2fe) == 0x202 && p[0x1650] == 6 && (stamp == 30 || stamp == 38) &&
        p[0x2f4] == 1 && p[0x2f5] == 0x1d && p[0x2f6] == 1 && p[0x2f7] == 1;
    if (!wave4 && (word(p + 0x564) != 0x200 || (stamp != 20 && !story && !r103_first_wave_em00(enemy))))
        return stop(frontier, 0x2a9978);
    return gain_heat_002a9978(&enemy->entry.b, 10.0f, frontier);
}
static gh_actor_motion_status first_punch_wall(gh_game_scene_tables *actors,const uint8_t *p,uint32_t *frontier)
{
    if(!actors)return stop(frontier,0x131e10);
    gh_scene_ray_report wall = {0};
    gh_scene_ray_request ray = {.mask = 1, .report_limit = 1, .segmented = 1,
        .native_reports = &wall};
    fenv_t saved;
    if (feholdexcept(&saved)) return stop(frontier, 0x30a2b0);
    if (fesetround(FE_TOWARDZERO)) {
        (void)fesetenv(&saved);
        return stop(frontier, 0x30a2b0);
    }
    for (unsigned i = 0; i < 3; ++i) {
        volatile float y = number(p + 0x90 + 4*i) * 0.5f;
        volatile float z = number(p + 0xa0 + 4*i) * -0.8f;
        const float translation = number(p + 0xb0 + 4*i);
        volatile float yz = y + z;
        ray.from[i] = y + translation;
        ray.to[i] = yz + translation;
    }
    ray.from[3] = ray.to[3] = 1;
    if (fesetenv(&saved)) return stop(frontier, 0x30a2b0);
    int hit;
    if (gh_scene_ray_query_00131e10(actors, &ray, &hit, frontier))
        return GH_ACTOR_MOTION_UNSUPPORTED;
    if (hit) {
        if (!wall.attributes || wall.attribute_bytes < 8) return stop(frontier, 0x24bf20);
        if (word(wall.attributes + 4) & 0x120u) return stop(frontier, 0x1f8180);
    }
    return GH_ACTOR_MOTION_OK;
}

static int r101_scenery_reaction_enemy(const gh_enemy_actor *enemy, const uint8_t *actor)
{
    return word(actor + 0x564) == 0x227 && half(actor + 0x640) == 9 &&
           actor[0x1650] == 4 && enemy->entry.b.work && enemy->entry.b.work->room == 0x101;
}

static int scenery_reaction_enemy(const gh_enemy_actor *enemy, const uint8_t *actor) {
    const uint32_t dependency = word(actor + 0x564);
    /* Original 20A178 accepts effect 2E from scenery and maps the live
     * receiver to selector 11, including the exchanged R101 actor. */
    const int r102_em27=dependency==0x227&&half(actor+0x640)==4&&!actor[0x1650]&&
        enemy->entry.b.work&&enemy->entry.b.work->room==0x102;
    const uint32_t impact_power=word(actor+0x17b4);
    const int west_em0a=dependency==0x20a&&!half(actor+0x640)&&actor[0x1650]==3&&
        half(actor+0x2f4)==0x1101&&!actor[0x2f6]&&!actor[0x2f7]&&
        (impact_power==0||impact_power==20)&&enemy->entry.b.work&&enemy->entry.b.work->room==0x100;
    return dependency == 0x200 || dependency == 0x202 || dependency == 0x203 ||
           r101_scenery_reaction_enemy(enemy, actor) || r102_em27 || west_em0a;
}

static uint32_t scenery_hit_vtable(unsigned kind)
{
    return kind == 0x303 ? 0x423230u : kind == 0x304 ? 0x423338u : 0;
}

static int r100_counter_collateral(const gh_enemy_actor *enemy,const uint8_t *actor,
    const gh_title_player_view *source, uint32_t power)
{
    return source&&source!=&enemy->entry.object.view&&source->bytes&&source->size>=0x642&&
        source->original_vtable214==0x4479c8&&word(source->bytes+0x214)==0x4479c8&&
        half(source->bytes+0x2fe)==0x202&&r100_stamp19(enemy,actor)&&
        word(source->bytes+0x564)==0x200&&half(source->bytes+0x640)==20&&
        source->bytes[0x2f4]==1&&source->bytes[0x2f5]==0x1d&&power==10;
}
/* OM3BE publishes the same environmental Give to every valid cEm00 receiver. */
static int r100_devil_contact(const gh_enemy_actor *enemy,
    const gh_title_player_view *source,uint32_t power)
{
    return enemy->entry.b.work&&enemy->entry.b.work->room==0x100&&power==20&&
        source&&source->bytes&&source->size==0x630&&
        source->original_vtable214==0x4290c8&&word(source->bytes+0x214)==0x4290c8&&
        half(source->bytes+0x2fe)==0x3be&&word(source->bytes+0x610)==3;
}

static int r103_counter_collateral(const gh_enemy_actor *enemy,const gh_title_player_view *source,
    uint32_t power)
{
    if(!r103_first_wave_em00(enemy)||!source||source==&enemy->entry.object.view||
       !source->bytes||source->size<=0x1650||source->original_vtable214!=0x4479c8||power!=10)return 0;
    const uint8_t *p=source->bytes;
    if(word(p+0x214)!=0x4479c8)return 0;
    const unsigned stamp=half(p+0x640);
    return half(p+0x2fe)==0x204&&word(p+0x564)==0x200&&p[0x1650]==24&&
        (stamp==2||stamp==3)&&p[0x2f4]==1&&p[0x2f5]==0x1d;
}

/* The 24F638 heavy recoil enables the body's original 262750 Give2E.
 * Both twin slots are original 0204 actors with the shared Take vtable. */
static int r103_heavy_recoil_collateral(const gh_enemy_actor *enemy,
    const gh_title_player_view *source,uint32_t power)
{
    const uint8_t *target=enemy->entry.object.view.bytes;
    if(!r103_boss_player_strike_target(enemy,target)||!source||
       source==&enemy->entry.object.view||!source->bytes||source->size<0x642||
       source->original_vtable214!=0x4479c8||power!=10)return 0;
    const uint8_t *p=source->bytes;
    return word(p+0x214)==0x4479c8&&half(p+0x2fe)==0x204&&
        word(p+0x564)==0x215&&half(p+0x640)==1&&
        word(target+0x564)==0x214&&half(target+0x640)==0&&
        half(p+0x2f4)==0x0301&&(p[0x2f6]==1||p[0x2f6]==2)&&!p[0x2f7];
}

static gh_actor_motion_status enemy_receive_sound(gh_enemy202_hit_call *call,
    gh_enemy_actor *enemy, int16_t bank, uint16_t program, uint32_t *frontier)
{
    const gh_actor_motion_bindings *b = &enemy->motion.motion.b;
    gh_motion_sound_request *request = &call->pressure_sound;
    if (!b->sound || !b->model) return stop(frontier, 0x2cba48);
    if (!request->model) {
        *request = (gh_motion_sound_request){
            .model = b->model, .actor = &enemy->entry.object.view, .bank = bank,
        };
        memcpy(&request->program, &program, sizeof(program));
    }
    uint32_t handle;
    gh_actor_motion_status status = b->sound(b->sound_context, request, &handle, frontier);
    if (status) return status;
    *request = (gh_motion_sound_request){0};
    return GH_ACTOR_MOTION_OK;
}

static int r200_catapult_contact(const gh_enemy_actor *enemy,const gh_hitarea_object *body,
 const gh_title_player_view *source)
{
    if(!enemy||!body||!body->bytes||body->size<0x50||!source||!source->bytes||source->size!=0x640)
        return 0;
    const gh_enemy_entry *entry=&enemy->entry;
    const uint8_t *p=entry->object.view.bytes;
    return gh_enemy201_is_r200_entry(&entry->request,entry->b.work->room)&&
        word(p+0x564)==0x21a&&!half(p+0x640)&&source->original_vtable214==0x428c20&&
        half(source->bytes+0x2fe)==0x3bd&&word(source->bytes+0x2f4)==0x00000300u&&
        !half(body->bytes+0x44)&&half(body->bytes+0x46)==0x52&&word(body->bytes+0x4c)==20;
}

/* Original scenery Give paths set the environment flag in 20A5DC's VT+AC call. */
static int r101_grenade_contact(const gh_enemy_actor *enemy,const gh_title_player_view *source,
    uint16_t effect,uint32_t power)
{
    if(effect!=0x30||power!=50||!source||!source->bytes||source->size!=0x680||
       source->original_vtable214!=0x4282f0||word(source->bytes+0x214)!=0x4282f0||
       half(source->bytes+0x2fe)!=0x364||source->bytes[0x2f4]!=3||
       !enemy->entry.b.work||enemy->entry.b.work->room!=0x101)return 0;
    const uint8_t *actor=enemy->entry.object.view.bytes;
    const unsigned stamp=half(actor+0x640);
    return word(actor+0x564)==0x20f&&actor[0x1650]==6&&
        (stamp==0x1a||stamp==0x1b||stamp==0x22||stamp==0x23);
}

static gh_actor_motion_status environment_hit(gh_enemy202_hit_call *call,
    const gh_enemy_entry *entry, const gh_hitarea_object *body, gh_enemy_actor *enemy,
    const gh_title_player_view *source, unsigned rank, uint32_t *frontier)
{
    const gh_title_player_view *actor = &entry->object.view;
    uint8_t *p = actor->bytes;
    if (!source || !source->bytes || source->size < 0x300)
        return stop(frontier, 0x20a178);
    enum { PRESSURE_CORE_SOUND = 40, PRESSURE_LOCAL_SOUND };
    const unsigned source_kind = half(source->bytes + 0x2fe);
    const uint32_t power = word(body->bytes + 0x4c);
    const uint16_t effect = half(body->bytes + 0x46);
    const int barrel = enemy && effect == 0x52 && source_kind == 0x310 && source->size >= 0x920 &&
        r102_stamp2(enemy, p) && power == 80 && !half(body->bytes + 0x44) &&
        !word(source->bytes + 0x2f4);
    const int grenade=r101_grenade_contact(enemy,source,effect,power);
    const int explosive=barrel||grenade;
    /* R200's catapult impact sphere gives the original effect 52 with power 20. */
    const int catapult = r200_catapult_contact(enemy,body,source);
    const int recoil = enemy && r103_heavy_recoil_collateral(enemy,source,power);
    const int arena_counter = recoil || (enemy && r103_counter_collateral(enemy,source,power));
    const int devil_contact = enemy && r100_devil_contact(enemy,source,power);
    const uint16_t receiver_state=half(p+0x2f4);
    const int west_em0a = enemy && entry->b.work && entry->b.work->room==0x100 &&
        word(p+0x564)==0x20a && !half(p+0x640) && p[0x1650]==3 && p[0x2f6]==1 &&
        ((devil_contact && !receiver_state && !p[0x2f7]) ||
         (source_kind==0x303 && !power && ((receiver_state==0x0700 && p[0x2f7]==1) ||
                                           (receiver_state==0x1101 && !p[0x2f7]))));
    /* The summoned R100 EM20 shares the scenery Take handler and resource
     * family. Its runtime supplies the real OM303 body owner. */
    const int devil_scenery = !enemy && entry->b.work && entry->b.work->room == 0x100 &&
        half(p + 0x2fe) == 0x220 && word(p + 0x564) == 0x220 &&
        half(p + 0x640) == 0xff && !power && source_kind == 0x303;
    const uint32_t source_vtable = catapult ? 0x428c20u : grenade ? 0x4282f0u : barrel ? 0x423e48u :
        source_kind == 0x202 || arena_counter ? 0x4479c8u : devil_contact ? 0x4290c8u : scenery_hit_vtable(source_kind);
    if ((effect != 0x2e && !explosive && !catapult) || !source_vtable || source->original_vtable214 != source_vtable ||
        word(source->bytes + 0x214) != source_vtable ||
        actor->original_vtable214 != 0x4479c8 || word(p + 0x214) != 0x4479c8 ||
        (!devil_scenery && (!enemy || (half(p + 0x2fe) != 0x202 && !arena_counter && !catapult) ||
          (!explosive && !catapult && !recoil && !west_em0a && !scenery_reaction_enemy(enemy,p)))) ||
        word(body->bytes + 0x40) != UINT32_MAX)
        return stop(frontier, 0x20a178);
    if (source_kind == 0x202) {
        if (!r100_counter_collateral(enemy,p,source,power))
            return stop(frontier, 0x20a178);
    } else if (power && !arena_counter && !explosive && !catapult && !devil_contact) {
        return stop(frontier, 0x20a178);
    }

    if (call->phase == 0) {
        const float drain = number(p + 0x770), cooldown = number(p + 0x54c);
        const float block = number(p + 0x1740), armor = number(p + 0x1734);
        const int16_t hp = (int16_t)half(p + 0x54a);
        uint32_t behavior = word(p + 0x16d0);
        uint16_t flags = half(p + 0x3ac);
        if (!isfinite(drain) || !isfinite(cooldown) || !isfinite(block) || !isfinite(armor) ||
            (drain > 0 && hp > 0))
            return stop(frontier, 0x20a178);
        if (cooldown > 0) return GH_ACTOR_MOTION_OK;
        call->effect = effect;
        if (half(p + 0x17ca)) flags &= (uint16_t)~0x100u;
        if ((behavior & 0x40000u) || block > 0 || armor > 0) {
            flags |= 0x100;
            put(p + 0x16d4, word(p + 0x16d4) | 0x8000u);
        }
        put_half(p + 0x3ac, flags);
        put_half(p + 0x17ca, 10);
        if (hp <= 0 || (behavior & 0x1000u)) {
            call->phase = 5;
            return GH_ACTOR_MOTION_OK;
        }
        if (flags & 0x100) {
            behavior &= ~0x800000u;
            put(p + 0x16d0, behavior);
        }
        put(p + 0x17ac, (uint32_t)hp);
        /* The non-player branch of 28FB88 subtracts the original Give power. */
        put_half(p + 0x54a, (uint16_t)((uint32_t)hp>power?(uint32_t)hp-power:0));
        put(p + 0x17a4, word(p + 0x17a4)+power);
        put(p + 0x17a8, word(p + 0x17a8)+power);
        put(p + 0x17b4, power);
        if (barrel) {
            put(p + 0x17c4, 1);
            put_half(p + 0x17c8, 0x2a);
        }
        put(p + 0x1940, 0xbd4ccccd);
        put(p + 0x54c, 0x41700000);
        put(p + 0x16dc, 0);
        call->phase = 1;
    }
    if (call->phase == 1) {
        if ((p[0x17bd] == 1 && rank >= 3) || (p[0x17bd] == 2 && rank >= 2)) {
            uint32_t points = word(p + 0x16e8);
            if ((int32_t)word(p + 0x16ec) < 1) points += barrel ? 20 : 5;
            if ((int32_t)points > 200) {
                if (!enemy) return stop(frontier, 0x26ecd0);
                const int em03 = word(p + 0x564) == 0x203 && half(p + 0x640) == 13 &&
                    !p[0x1650] && enemy->entry.b.work && enemy->entry.b.work->room == 0x101;
                const unsigned stamp=half(p+0x640), state=half(p+0x2f4);
                const uint32_t dependency=word(p+0x564);
                const int west_patrol=entry->b.work->room==0x100&&effect==0x2e&&
                    source_kind==0x303&&!power&&p[0x1650]==5&&p[0x2f6]==1&&!p[0x2f7]&&
                    ((dependency==0x200&&stamp==10&&(state==0x1101||state==0x1300))||
                     (dependency==0x203&&stamp==11&&state==0x1300));
                if (!west_patrol && !em03 && !r101_scenery_reaction_enemy(enemy, p))
                    return stop(frontier, 0x26ecd0);
                call->phase = PRESSURE_CORE_SOUND;
            }
            put(p + 0x16e8, points);
        }
        if (!rank && (p[0x17bd] == 1 || p[0x17bd] == 2))
            return stop(frontier, 0x1fa010);
        if (call->phase == 1) call->phase = 2;
    }
    /* 26ECD0 plays both authored cues before clearing pressure. A stalled
     * sound retains its request and the points already added by 20A178. */
    if (call->phase == PRESSURE_CORE_SOUND) {
        gh_actor_motion_status status = enemy_receive_sound(call, enemy, 0, 0xf7, frontier);
        if (status) return status;
        call->phase = PRESSURE_LOCAL_SOUND;
    }
    if (call->phase == PRESSURE_LOCAL_SOUND) {
        const uint16_t program = (uint16_t)(word(p + 0x424) + 0x14u); /* 1595F0 */
        gh_actor_motion_status status = enemy_receive_sound(call, enemy, 1, program, frontier);
        if (status) return status;
        put(p + 0x16ec, rank >= 3 && rank <= 5 ? 300u : 150u);
        put(p + 0x16e8, 0);
        put(p + 0x16dc, 0); /* 26EE40(actor, 0, 0) leaves the manager timer alone. */
        call->phase = 2;
    }
    if (call->phase == 2) {
        const uint16_t flags = half(p + 0x3ac);
        const uint32_t behavior = word(p + 0x16d0), status = word(p + 0x16d4);
        /* 20B3D4 branches away from the ordinary route only for the
         * outer1/selector16 reaction before phase4. Variant is not read. */
        const int early_reaction = p[0x2f4] == 1 && p[0x2f5] == 0x16 && p[0x2f6] < 4;
        const int grounded_barrel=barrel&&(flags&8u)&&p[0x2f4]==1&&p[0x2f5]==8&&
            p[0x2f6]==3&&p[0x2f7]==2;
        if ((status & (grounded_barrel?0x180040u:0x80040u)) || (behavior & 0x806020u) ||
            (flags & (grounded_barrel?0x24u:0x2cu)) || early_reaction)
            return stop(frontier, 0x20b394);

        if(grounded_barrel){
            /* 430FC8 selects one MT & 7 for EM0A's grounded response.
             * 20B668 adds a separate rank3+ draw before the Heat gate. */
            uint32_t random;
            gh_game_mt mt=body->state->random;
            if(gh_game_mt_next_0030f348(&mt,&random))
                return stop(frontier,0x31cfe0);
            int counter=(random&7u)==0;
            if(rank>=3){
                if(gh_game_mt_next_0030f348(&mt,&random))
                    return stop(frontier,0x31cfe0);
                if(random%3u==0)counter=1;
            }
            body->state->random=mt;
            if(counter&&!body->state->heat[0x10]&&(int32_t)word(p+0x17ac)>0&&number(p+0x17cc)<=0){
                const uint16_t maximum=half(p+0x548);
                const uint32_t before=word(p+0x17ac);
                put_half(p+0x54a,(uint16_t)(before<maximum?before:maximum));
                p[0x1866]=0;
                put(p+0x54c,0x40a00000u);
                put(p+0x17c4,UINT32_MAX);
                put_half(p+0x17c8,0);
                p[0x2f4]=1;p[0x2f5]=0x14;p[0x2f6]=p[0x2f7]=0;
                call->phase=5;
                return GH_ACTOR_MOTION_OK;
            }
        }

        /* Dead 2E uses 20BB80 while dead 52 uses the 20BC44 knockdown. */
        const int dead=!half(p+0x54a);
        if(power&&dead&&!explosive&&!catapult){
            p[0x2f4]=2;
            p[0x2f5]=(flags&8u)?1:0;
            p[0x2f6]=p[0x2f7]=0;
            call->reacted=1;
            call->phase=5;
            return GH_ACTOR_MOTION_OK;
        }

        /* For original 2E/52, only 20CAC8 draws once for selector 1.
         * The remaining counter tables reject these effects before RNG. */
        if (((effect != 0x52 && !grenade) || !dead) && p[0x17bc] == 1) {
            uint32_t random;
            if (gh_game_mt_next_0030f348(&body->state->random, &random))
                return stop(frontier, 0x31cfe0);
        }
        p[0x2f4] = 1;
        /* Live kind52 selects 20C74C, the variant2 knockdown. */
        p[0x2f5] = (effect == 0x52 || grenade) ? 8 : 0x11;
        p[0x2f6] = 0;
        p[0x2f7] = (effect == 0x52 || grenade) ? 2 : 0;
        call->reacted = 1;
        call->phase = 5;
    }
    return GH_ACTOR_MOTION_OK;
}

static void restore_counter_hp(uint8_t *p)
{
    const int32_t before=(int32_t)word(p+0x17ac);
    const int16_t maximum=(int16_t)half(p+0x548);
    put_half(p+0x54a,(uint16_t)(before<maximum?before:maximum));
}

static void counter_mode(uint8_t *p,unsigned mode,int extra_cooldown)
{
    restore_counter_hp(p);
    if(extra_cooldown)put(p+0x54c,0x40000000);
    put(p+0x17c4,UINT32_MAX);
    put_half(p+0x17c8,0);
    p[0x2f5]=(uint8_t)mode;
    p[0x2f4]=p[0x2f6]=p[0x2f7]=0;
}

/* Called after the actual 20CAC8 draw. Effect 12 with these two
 * dependency/selector pairs reaches only the bounded 20D398 and 20CD00
 * paths below. A successful counter returns Take's original false. */
static unsigned triangle_counter(uint8_t *p,const gh_title_player_view *player,
    gh_game_init_state *init,unsigned rank,int stamp20,gh_game_mt *mt,uint32_t *pc)
{
    if(rank<2||number(p+0x1740)>0||number(p+0x17cc)>0||
       (int32_t)word(p+0x17ac)<1||(half(p+0x3ac)&0x12cu))return 0;
    const uint32_t status=word(p+0x16d4);
    const int ignore_heat=(status&0x4000000u)!=0;
    const int player_power=word(player->bytes+0x5fc)!=0;
    const float facing=number(p+0x760);
    const float speed=number(p+0x5a8);
    if(!isfinite(facing)||!isfinite(speed)){
        *pc=0x20d398;
        return 0;
    }
    uint32_t random;
    if(!player_power&&(ignore_heat||!init->heat[0x10])&&
       ((status&0x40000u)||facing<=1.5707964f)){
        if(gh_game_mt_next_0030f348(mt,&random)){
            *pc=0x31cfe0;
            return 0;
        }
        if(!(random&3u))return 0x70;
    }
    if(stamp20&&(ignore_heat||(!init->heat[0x10]&&!player_power))&&
       facing<=2.3561945f&&speed>=1){
        if(gh_game_mt_next_0030f348(mt,&random)){
            *pc=0x31cfe0;
            return 0;
        }
        if((random&31u)==3)return 10;
    }
    return 0;
}

static gh_actor_motion_status r101_box_reaction(gh_enemy202_hit_call *call,
 gh_enemy_actor *enemy, unsigned rank, uint32_t *frontier)
{
 uint8_t *p=enemy->entry.object.view.bytes;
 const uint32_t f0=word(p+0x16d0),f4=word(p+0x16d4);
 const uint16_t sequence=half(p+0x3ac);
 if((int16_t)half(p+0x54a)<1||(f0&0x806020u)||(f4&0x180040u)||
    (sequence&0x2cu)||(p[0x2f4]==1&&p[0x2f5]==0x16&&p[0x2f6]<4))
  return stop(frontier,0x20b394);

 /* 26EE40(0,0) resets only this actor's attack lock, at every rank. */
 put(p+0x16dc,0);
 const unsigned rage=p[0x17bd];
 if((rage==1&&rank>=3)||(rage==2&&rank>=2)){
  const int32_t old=(int32_t)word(p+0x16e8);
  const uint32_t increment=(int32_t)word(p+0x16ec)<1?20:0;
  if((int64_t)old+increment>200)return stop(frontier,0x26ecd0);
  put(p+0x16e8,(uint32_t)old+increment);
 }

 /* 20D138 returns zero. The four counter probes exclude effect36.
  * 20CAC8's initial mode1 RNG draw precedes the effect test. */
 if(p[0x17bc]==1){
  uint32_t draw;
  if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
   return stop(frontier,0x31cfe0);
 }
 p[0x2f4]=1;
 p[0x2f5]=1;
 p[0x2f6]=0;
 p[0x2f7]=0;
 call->phase=5;
 call->reacted=1;
 return GH_ACTOR_MOTION_OK;
}

/* 20A178 kind36 from the live R101 Wood Box. For source kind030A
 * and flags4/5 both zero, 28FB88 neither scales damage nor shares HP. */
static gh_actor_motion_status r101_box_hit(gh_enemy202_hit_call *call,
 gh_enemy_actor *enemy,const gh_title_player_view *source,unsigned rank,uint32_t *frontier)
{
 enum { BOX_DAMAGE_COMMITTED=32 };
 uint8_t *p=enemy->entry.object.view.bytes;
 const gh_hitarea_object *body=&enemy->hitareas.units[0];
 if(!source||!source->bytes||source->size<0x648||source->original_vtable214!=0x4238d8||
    word(source->bytes+0x214)!=0x4238d8||half(source->bytes+0x2fe)!=0x30a||
    half(source->bytes+0x2f4)!=0x0100||!enemy->entry.b.work||
    enemy->entry.b.work->room!=0x101||word(p+0x564)!=0x227||half(p+0x640)!=9||
    enemy->entry.object.view.original_vtable214!=0x4479c8)
  return stop(frontier,0x20a178);
 if(call->effect==0x36&&call->phase==BOX_DAMAGE_COMMITTED)
  return r101_box_reaction(call,enemy,rank,frontier);
 if(call->effect==0x36&&call->phase>=5&&call->reacted)
  return GH_ACTOR_MOTION_OK;
 if(call->phase)return stop(frontier,0x20a178);

 const float drain=number(p+0x770),cooldown=number(p+0x54c);
 const float block=number(p+0x1740),armor=number(p+0x1734);
 const int16_t previous=(int16_t)half(p+0x54a);
 if(!isfinite(drain)||drain>0||!isfinite(cooldown)||cooldown>0||
    !isfinite(block)||!isfinite(armor)||previous<=0)
  return stop(frontier,0x20a178);
 uint32_t power=word(body->bytes+0x4c);
 if(power>16000)return stop(frontier,0x28fb88);
 uint16_t flags=half(p+0x3ac);
 if(half(p+0x17ca))flags&=0xfeffu;
 if((word(p+0x16d0)&0x40000u)||block>0||armor>0){
  flags|=0x100u;
  put(p+0x16d4,word(p+0x16d4)|0x8000u);
 }
 put_half(p+0x3ac,flags);
 put_half(p+0x17ca,10);
 /* This effect's Heat value is zero, even with armor active. */
 put(p+0x17ac,(uint32_t)previous);
 if(flags&0x100u){
  power<<=1;
  put(p+0x16d0,word(p+0x16d0)&~0x800000u);
 }
 put_half(p+0x54a,power>=(uint32_t)previous?0:(uint16_t)((uint32_t)previous-power));
 put(p+0x17a4,word(p+0x17a4)+power);
 put(p+0x17a8,word(p+0x17a8)+power);
 put(p+0x17b4,power);
 put(p+0x1940,0xbd4ccccdu);
 put(p+0x54c,0x40400000u);
 call->effect=0x36;
 call->phase=BOX_DAMAGE_COMMITTED;
 return r101_box_reaction(call,enemy,rank,frontier);
}

enum { EM0A_COUNTER_START=15, EM0A_COUNTER_SOUND=42, EM0A_COUNTER_FINISH };

static gh_actor_motion_status em0a_counter_00274238(gh_enemy202_hit_call *call,
    gh_enemy_actor *enemy,uint8_t *p,unsigned rank,uint32_t *frontier)
{
    if((!r103_arena_em0a(enemy,p)&&!r103_boss_player_strike_target(enemy,p))||!rank||rank>5)
        return stop(frontier,0x274238);
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_game_mt *random=&enemy->entry.b.init->random;
    if(call->phase==EM0A_COUNTER_START){
        if(!(word(p+0x16d0)&0x2000u)){
            static const float cooldown[]={0,40,30,25,25,15};
            float delay=cooldown[rank];
            if(rank<5){
                gh_game_mt next=*random;
                uint32_t draw;
                if(gh_game_mt_next_0030f348(&next,&draw))return stop(frontier,0x31d000);
                fenv_t env;
                if(feholdexcept(&env))return stop(frontier,0x274448);
                if(fesetround(FE_TOWARDZERO)){(void)fesetenv(&env);return stop(frontier,0x274448);}
                volatile float source=(float)(draw>>1);
                volatile float scaled=source*0x1p-29f;
                volatile float adjusted=delay+scaled;
                delay=adjusted;
                if(fesetenv(&env))return stop(frontier,0x274448);
                *random=next;
            }
            memcpy(p+0x173c,&delay,4);
        }
        p[0x2f4]=0;p[0x2f5]=0x69;p[0x2f6]=0;p[0x2f7]=1;
        call->pressure_sound=(gh_motion_sound_request){.model=motion->b.model,
            .actor=&enemy->entry.object.view,.bank=0,.program=0x7c};
        call->phase=EM0A_COUNTER_SOUND;
    }
    if(call->phase==EM0A_COUNTER_SOUND){
        if(!motion->b.sound)return stop(frontier,0x2cba48);
        uint32_t handle=0;
        gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,
            &call->pressure_sound,&handle,frontier);
        if(status)return status;
        call->phase=EM0A_COUNTER_FINISH;
    }
    uint32_t draw;
    if(gh_game_mt_next_0030f348(random,&draw))return stop(frontier,0x31cfe0);
    put(p+0x17c4,UINT32_MAX);put_half(p+0x17c8,0);
    p[0x17bf]=(uint8_t)(draw%3u);
    call->phase=5;
    return GH_ACTOR_MOTION_OK;
}

/* 20A178 Give27 uses the projectile as the damage source. Its unscaled
 * VT+AC call, heat gain, and recovery enter 20CA90's 01/03 state. */
static gh_actor_motion_status roulette_shockwave_hit(gh_enemy202_hit_call *call,
    gh_enemy_actor *enemy, const gh_title_player_view *source, unsigned rank,
    uint32_t *frontier)
{
    uint8_t *p=enemy->entry.object.view.bytes;
    const gh_hitarea_object *body=&enemy->hitareas.units[0];
    const uint32_t dependency=word(p+0x564);
    const uint32_t power=word(body->bytes+0x4c);
    if (!source || !source->bytes || source->size!=0x630 ||
        source->original_vtable214!=0x4290c8 || word(source->bytes+0x214)!=0x4290c8 ||
        half(source->bytes+0x2fe)!=0x3be || word(source->bytes+0x610)!=0x2c ||
        enemy->entry.request.dependency_kind!=dependency || !enemy->entry.b.work ||
        word(p+0x214)!=0x4479c8 || power!=80 || !rank || rank>5)
        return stop(frontier,0x20a178);
    const int arena=(dependency==0x214||dependency==0x215)&&p[0x1650]==15&&
        gh_enemy204_is_r103_entry(&enemy->entry.request,enemy->entry.b.work->room);
    const unsigned move=half(p+0x2f4);
    const int charge_recovery=move==0x1a00&&p[0x2f6]==5&&!p[0x2f7];
    /* 20A178 takes the same projectile hit for either authored turn side. */
    const int turn=move==0x0800&&p[0x2f6]==1&&p[0x2f7]<=1;
    const int em03=r104_em03_damage_receiver(enemy,p)&&(call->phase||
        ((charge_recovery||turn)&&rank==1));
    if(!arena&&!em03)return stop(frontier,0x20a178);
    if (call->phase==0) {
        const float drain=number(p+0x770), cooldown=number(p+0x54c);
        const float block=number(p+0x1740), armor=number(p+0x1734);
        const uint16_t hp=half(p+0x54a);
        if (!isfinite(drain)||drain>0||!isfinite(cooldown)||!isfinite(block)||
            !isfinite(armor)||hp>=0x8000)
            return stop(frontier,0x20a178);
        uint16_t flags=half(p+0x3ac);
        if (half(p+0x17ca)) flags&=(uint16_t)~0x100u;
        if ((word(p+0x16d0)&0x40000u)||block>0||armor>0) {
            flags|=0x100u;
            put(p+0x16d4,word(p+0x16d4)|0x8000u);
        }
        put_half(p+0x3ac,flags);
        put_half(p+0x17ca,10);
        call->effect=0x27;
        call->phase=1;
    }
    if (call->phase==1) {
        if (half(p+0x54a)) {
            gh_actor_motion_status status=gain_heat_002a9978(&enemy->entry.b,3.0f,frontier);
            if (status) return status;
        }
        call->phase=2;
    }
    if (call->phase==2) {
        uint16_t hp=half(p+0x54a);
        if ((half(p+0x3ac)&0x100u)&&hp) {
            gh_actor_motion_status status=gain_heat_002a9978(&enemy->entry.b,rank==5?1.0f:4.0f,frontier);
            if (status) return status;
        }
        if (half(p+0x3ac)&0x100u) put(p+0x16d0,word(p+0x16d0)&~0x800000u);
        put(p+0x17ac,hp);
        put_half(p+0x54a,hp>power?(uint16_t)(hp-power):0);
        put(p+0x17a4,word(p+0x17a4)+power);
        put(p+0x17a8,word(p+0x17a8)+power);
        put(p+0x17b4,power);
        put(p+0x1940,0xbd4ccccd);
        put(p+0x54c,0x41700000);
        put(p+0x16dc,0);
        call->phase=3;
    }
    if (call->phase==3) {
        if (word(p+0x17ac)&&((p[0x17bd]==1&&rank>=3)||(p[0x17bd]==2&&rank>=2))) {
            const uint32_t points=word(p+0x16e8)+((int32_t)word(p+0x16ec)<1?20u:0u);
            if ((int32_t)points>200) return stop(frontier,0x26ecd0);
            put(p+0x16e8,points);
        }
        p[0x2f4]=1;
        p[0x2f5]=3;
        p[0x2f6]=p[0x2f7]=0;
        call->reacted=1;
        call->phase=5;
    }
    return GH_ACTOR_MOTION_OK;
}

enum { WEST_COUNTER_SOUND = 33, WEST_COUNTER_RESET };

static gh_actor_motion_status finish_west_counter(gh_enemy202_hit_call *call,
    gh_enemy_actor *enemy,uint32_t *frontier)
{
    uint8_t *p=enemy->entry.object.view.bytes;
    if(call->phase==WEST_COUNTER_SOUND){
        const gh_actor_motion_status status=enemy_receive_sound(call,enemy,0,0x7c,frontier);
        if(status)return status;
        put_half(p+0x17c8,0);
        put(p+0x17c4,UINT32_MAX);
        call->phase=WEST_COUNTER_RESET;
    }
    uint32_t reset;
    if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&reset))
        return stop(frontier,0x31cfe0);
    p[0x17bf]=(uint8_t)(reset%3u);
    call->phase=5;
    return GH_ACTOR_MOTION_OK;
}

/* EM20 owns a distinct native runtime and shares the original 208BE0 Take
 * entry for the live scenery or direct player body contacts. */
gh_actor_motion_status gh_enemy_devil_hit_0020a178(gh_enemy202_hit_call *call,
    const gh_enemy_entry *entry,const gh_enemy_hitareas *hitareas,gh_game_scene_tables *actors,
    const gh_title_player_view *player,unsigned rank,uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0;
    if(!call||!entry||!entry->b.init||!entry->b.work||!hitareas||!player||!player->bytes||
       player->size<0x600)return GH_ACTOR_MOTION_ARGUMENT;
    const gh_title_player_view *view=&entry->object.view;
    uint8_t *p=view->bytes;
    const gh_hitarea_object *body=&hitareas->units[0];
    gh_title_player_view *owner=NULL;
    const gh_title_player_view *source=NULL;
    if(entry->b.work->room!=0x100||!p||view->size<0x1944||
       view->original_vtable214!=0x4479c8||word(p+0x214)!=0x4479c8||
       half(p+0x2fe)!=0x220||word(p+0x564)!=0x220||half(p+0x640)!=0xff||
       !hitareas->created[0]||!body->bytes||body->fallback||body->group!=2||body->size<0x64||
       body->state!=entry->b.init||gh_hitarea_object_actor(body,&owner)||owner!=view||
       gh_hitarea_object_last_hit_actor(body,&source)||!(word(body->bytes+0x60)&1u))
        return stop(frontier,0x20a178);
    const uint16_t effect=half(body->bytes+0x46);
    if(effect==0x2e)return environment_hit(call,entry,body,NULL,source,rank,frontier);
    if(source!=player||half(player->bytes+0x2fe)!=0x100||effect!=0x12||
       word(body->bytes+0x40)||half(body->bytes+0x44)!=10)
        return stop(frontier,0x20a178);
    if(!call->phase){
        const float cooldown=number(p+0x54c),drain=number(p+0x770);
        const float block=number(p+0x1740),armor=number(p+0x1734);
        if(!isfinite(cooldown)||!isfinite(drain)||!isfinite(block)||!isfinite(armor))
            return stop(frontier,0x20a178);
        if(cooldown>0)return GH_ACTOR_MOTION_OK;
        const uint16_t hp=half(p+0x54a);
        const uint32_t behavior=word(p+0x16d0),status=word(p+0x16d4);
        uint16_t flags=half(p+0x3ac);
        if(half(p+0x17ca))flags&=(uint16_t)~0x100u;
        if(!hp||hp>=0x8000||drain>0||block>0||armor>0||(flags&0x12cu)||
           (behavior&0x840020u)||(status&0x180040u))
            return stop(frontier,0x20a394);
        put_half(p+0x3ac,flags);
        put_half(p+0x17ca,10);
        call->effect=0x12;
        call->phase=1;
    }
    if(call->phase==1){
        gh_actor_motion_status status=gain_heat_002a9978(&entry->b,2.0f,frontier);
        if(status)return status;
        call->phase=2;
    }
    if(call->phase==2){
        put(p+0x17ac,half(p+0x54a));
        const uint32_t raw=word(body->bytes+0x4c);
        int32_t power;
        memcpy(&power,&raw,4);
        gh_actor_motion_status status=take_player_power_0028fb88(p,player,entry->b.init,power,frontier);
        if(status)return status;
        put(p+0x17a4,word(p+0x17a4)+raw);
        put(p+0x17a8,word(p+0x17a8)+raw);
        put(p+0x17b4,raw);
        put(p+0x1940,0xbd4ccccdu);
        put(p+0x54c,0x40400000u);
        put(p+0x16dc,0);
        call->phase=3;
    }
    if(call->phase==3){
        if(!rank||rank>5||p[0x17bd]!=1||(rank>=3&&!(int32_t)word(p+0x16ec)))
            return stop(frontier,0x20a7e0);
        const float resistance=number(p+0x17cc),face=number(p+0x760),rate=number(p+0x5a8);
        if(!half(p+0x54a)||!isfinite(resistance)||resistance>0||!isfinite(face)||!isfinite(rate)||
           p[0x17bc]||p[0x17ba]||p[0x17bb]!=1)
            return stop(frontier,0x20cac8);
        if(!entry->b.init->heat[0x10]&&!word(player->bytes+0x5fc)){
            gh_game_mt *random=&entry->b.init->random;
            uint32_t draw;
            if(face<=2.3561945f&&rate>=1.0f){
                if(gh_game_mt_next_0030f348(random,&draw))return stop(frontier,0x31cfe0);
                if((draw&7u)==3u){
                    counter_mode(p,0x12,0);
                    p[0x2f7]=1;
                    call->phase=5;
                    return GH_ACTOR_MOTION_OK;
                }
            }
            static const uint8_t threshold[]={0,5,4,3,3,2};
            if(gh_game_mt_next_0030f348(random,&draw))return stop(frontier,0x31cfe0);
            const unsigned needed=threshold[rank]+(draw&3u)-(p[0x17c1]?2u:0u);
            if(p[0x17bf]<100)++p[0x17bf];
            if(p[0x17bf]>=needed){
                if(gh_game_mt_next_0030f348(random,&draw))return stop(frontier,0x31cfe0);
                if(!(draw&3u)&&face<=1.5707964f){
                    restore_counter_hp(p);
                    call->phase=15;
                }
            }
        }
        if(call->phase!=15)call->phase=4;
    }
    if(call->phase==15)return stop(frontier,0x274238);
    if(call->phase==4){
        gh_actor_motion_status status=first_punch_wall(actors,p,frontier);
        if(status)return status;
        p[0x2f4]=1;
        p[0x2f5]=1;
        p[0x2f6]=p[0x2f7]=0;
        call->reacted=1;
        call->phase=5;
    }
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_hit_0020a178(
    gh_enemy202_hit_call *call, gh_enemy_actor *enemy, const gh_title_player_view *player,
    unsigned rank, uint32_t *frontier) {
    if (!frontier) return GH_ACTOR_MOTION_ARGUMENT;
    *frontier = 0;
    if (!call || !enemy || !enemy->entry.b.init || !player || !player->bytes || player->size < 0x600)
        return GH_ACTOR_MOTION_ARGUMENT;
    uint8_t *p = enemy->entry.object.view.bytes;
    const gh_hitarea_object *body = &enemy->hitareas.units[0];
    if (!p || enemy->entry.object.view.size < 0x1944 || !enemy->hitareas.created[0])
        return stop(frontier, 0x20a178);
    const gh_title_player_view *source;
    gh_title_player_view *owner;
    if (body->fallback || body->group != 2 || body->size < 0x64 ||
        body->state != enemy->entry.b.init || gh_hitarea_object_actor(body, &owner) ||
        owner != &enemy->entry.object.view || gh_hitarea_object_last_hit_actor(body, &source) ||
        !(word(body->bytes + 0x60) & 1))
        return stop(frontier, 0x20a178);

    /* 20A2F4 rejects a new contact while the existing hit is cooling. */
    if (!call->phase && number(p + 0x54c) > 0.0f)
        return GH_ACTOR_MOTION_OK;

    const uint16_t effect = half(body->bytes + 0x46);
    if (effect == 0x2e || effect == 0x30 || effect == 0x52)
        return environment_hit(call, &enemy->entry, body, enemy, source, rank, frontier);
    if(effect==0x36)return r101_box_hit(call,enemy,source,rank,frontier);
    if(effect==0x27)return roulette_shockwave_hit(call,enemy,source,rank,frontier);
    const int triangle = triangle_take_supported(enemy, p, effect);
    const int heat_contact=r103_heat_transition_contact(enemy,player,effect,(int32_t)word(body->bytes+0x4c));
    const int back_circle=back_circle_contact(enemy,player,effect);
    if (source != player || half(player->bytes + 0x2fe) != 0x100 ||
        (effect != 7 && !triangle && !heat_contact && !back_circle))
        return stop(frontier, 0x20a178);

    /* The R100 EM03 receivers have the same original 0203 reaction probes.
     * The west formation enters through the actual Triangle effect. */
    const uint32_t dependency = word(p + 0x564);
    const int opening_203 = r100_stamp19(enemy,p)||r102_em03_punch(enemy,p,effect)||(triangle&&
        (r100_at2_em03(enemy,p)||r100_west_em02_em03(enemy,p)||(dependency==0x203&&r103_arena_punch_target(enemy,dependency))));
    const int west_em00=triangle&&r100_west_em00(enemy,p);
    const int opening_200 = r100_stamp20(enemy,p)||
        (triangle&&(r100_stamp8(enemy,p)||r103_first_wave_em00(enemy)))||
        (heat_contact&&r103_first_wave_em00(enemy))||west_em00;
    const int opening_pair = opening_203 || opening_200;
    const int ordinary_punch = room_unarmed_punch(enemy, p, effect);
    const int side_em0f=dependency==0x20f&&r101_side_arena_punch(enemy,p,effect);
    const int em27_punch = original_em27_punch(enemy, p, effect);
    const int boss_strike=(effect==7||triangle||heat_contact||back_circle)&&r103_boss_player_strike_target(enemy,p);
    const int side_heat=heat_contact&&enemy->entry.b.work->room==0x101;
    const int r200_triangle=triangle&&r200_em02_triangle(enemy,p)&&
        word(body->bytes+0x4c)==12&&half(body->bytes+0x44)==0xa&&rank==1&&
        word(player->bytes+0x2f4)==0x00010b00u;
    const int hit_probe_selector = opening_pair || (ordinary_punch && !boss_strike) ||
        r102_at0b_em02_punch(enemy,p,effect)||r200_triangle||side_heat;
    const int r104_punch=r104_player_punch(enemy,p,effect)&&
        word(body->bytes+0x4c)==3&&rank==1&&!body->state->heat[0x10];
    /* The charge recovery, rear turn and early melee reach the same ordinary dep0250 probes. */
    const int r104_recovery=r104_punch&&(p[0x2f6]==5||p[0x2f5]==8||
        (p[0x2f5]==0x15&&half(p+0x3ac)==0x800));
    const int r104_braced=r104_punch&&p[0x2f5]==0x21;
    /* Give12 from default set1 ID29 uses the same original dep0250 probes
     * at the rear of the charge and during the melee ending contact. */
    const int em03_combo=half(p+0x2f4)==0x1600&&p[0x2f6]<=1;
    const int fast_melee=half(p+0x2f4)==0x1500&&p[0x2f6]==1&&half(p+0x3ac)==0x800&&
        (word(p+0x16d4)&0x400u);
    const int r104_triangle_recoil=half(p+0x2f4)==0x0101&&
        (p[0x2f6]==3||(p[0x2f6]==1&&!half(p+0x3ac)));
    const int falling=half(p+0x2f4)==0x0701&&p[0x2f6]==1&&
        (half(p+0x3ac)&0x12cu)==4&&
        !(word(p+0x16d0)&0x4000u)&&!(word(p+0x16d4)&0x100040u);
    const int idle_move=!half(p+0x2f4)&&p[0x2f6]==1&&!half(p+0x3ac);
    /* The angry grab can take original contact before its SEQ bit 800.
     * Its authored brace is already active on that earlier frame. */
    const int braced_grab=half(p+0x2f4)==0x4b00&&p[0x2f6]==1&&
        (word(p+0x16d4)&0x400u);
    const int grab_windup=braced_grab&&half(p+0x3ac)==0x800;
    const int angry_grab_windup=braced_grab&&!half(p+0x3ac)&&
        (int32_t)word(p+0x16ec)>0&&(word(p+0x16d0)&0x800000u);
    const int brace_windup=r104_brace_is_exposed(p);
    /* 20A178's phase-zero contact gate has no exclusion for the original
     * dep0250 recovery at 00/6C/01. The unbraced Give12 uses its standing probes. */
    const int reverse_recovery=half(p+0x2f4)==0x6c00&&p[0x2f6]==1&&!half(p+0x3ac)&&
        (word(p+0x16d0)&0x80e420u)==0x400u&&!(word(p+0x16d4)&0x4180440u);
    const int r104_back_circle=back_circle&&grab_windup&&!p[0x2f7]&&
        r104_em03_damage_receiver(enemy,p)&&word(body->bytes+0x4c)==25&&
        rank==1&&!body->state->heat[0x10];
    const int r104_triangle=triangle&&r104_em03_damage_receiver(enemy,p)&&
        (r104_turn_is_exposed(p)||(!p[0x2f7]&&
         ((half(p+0x2f4)==0x1a00&&p[0x2f6]==5)||r104_triangle_recoil||falling||
          em03_combo||fast_melee||idle_move||grab_windup||angry_grab_windup||brace_windup||reverse_recovery)))&&
        word(player->bytes+0x5e8)==29&&
        word(body->bytes+0x4c)==12&&rank==1&&!body->state->heat[0x10];
    const int boss_recovery=r104_recovery||r104_triangle||em27_punch;
    const int r200_em1a=triangle&&r200_em1a_triangle(enemy,p)&&word(body->bytes+0x4c)==12&&
        half(body->bytes+0x44)==0xa&&rank==1&&word(player->bytes+0x2f4)==0x00010b00u&&
        (word(p+0x2f4)==0x00010900u||word(p+0x2f4)==0x00010101u);
    const int counter_selector = opening_200 || (ordinary_punch && dependency==0x200) || boss_strike||r200_em1a;
    const int arena_em0a=triangle&&r103_arena_em0a(enemy,p);
    const int arena_stun_airborne=arena_em0a&&effect==0x12&&number(p+0x17cc)>0;
    const int dead_airborne=triangle&&!half(p+0x54a)&&
        ((dependency==0x203&&r103_arena_punch_target(enemy,dependency))||arena_em0a||
         r103_first_wave_em00(enemy)||boss_strike);
    const int continuing_knockdown=(dead_airborne||arena_stun_airborne)&&
        half(p+0x2f4)==0x0701&&(half(p+0x3ac)&0x2cu)==4&&
        !(word(p+0x16d4)&0x100040u);
    const int continued_fall=triangle&&(west_em00||r100_west_em02_em03(enemy,p))&&
        half(p+0x2f4)==0x0701&&p[0x2f6]==1&&!p[0x2f7]&&
        !half(p+0x54a)&&(half(p+0x3ac)&4u);

    if (call->phase == 0) {
        const float drain = number(p + 0x770), cooldown = number(p + 0x54c);
        const float block = number(p + 0x1740), armor = number(p + 0x1734);
        const uint16_t hp = half(p + 0x54a);
        uint16_t flags = half(p + 0x3ac);
        if ((!hit_probe_selector && !em27_punch && !boss_strike && !r104_punch && !r104_triangle && !r104_back_circle && !r200_em1a && dependency != 0x20a) ||
            (!hp && !continuing_knockdown && !continued_fall) || hp >= 0x8000 ||
            !isfinite(drain) || drain > 0 || !isfinite(cooldown) || cooldown > 0 ||
            !isfinite(block) || !isfinite(armor))
            return stop(frontier, 0x20a178);
        if (half(p + 0x17ca)) flags &= (uint16_t)~0x100u;
        if ((word(p + 0x16d0) & 0x40000u) || block > 0 || armor > 0) {
            flags |= 0x100;
            put(p + 0x16d4, word(p + 0x16d4) | 0x8000u);
        }
        put_half(p + 0x3ac, flags);
        put_half(p + 0x17ca, 10);
        if (triangle||heat_contact||back_circle) call->effect = effect;
        call->phase = 1;
    }
    if (call->phase == 1) {
        if ((half(p + 0x54a)||continued_fall)&&!heat_contact) {
            gh_actor_motion_status status = gain_heat_002a9978(&enemy->entry.b, back_circle ? 3.0f : triangle ? 2.0f : 0.8f, frontier);
            if (status) return status;
        }
        call->phase = 2;
    }
    enum { amplified_power_ready = 16, stamp2_counter_cue = 18, stamp2_counter_reset = 19 };
    if (call->phase == 2 && (half(p + 0x3ac) & 0x100u)) {
        const int amplified_triangle = triangle && (r100_amplified_triangle_receiver(enemy,p) ||
            r103_first_wave_em00(enemy) || r103_arena_em0a(enemy,p) ||
            (p[0x2f6]==1&&r103_boss_player_strike_target(enemy,p)));
        const int boss_square = boss_strike && effect==7 && p[0x2f6]==1;
        /* EM03's authored attack can carry the original 20A4CC amplified flag. */
        const int opening_attack_contact = triangle && r100_stamp19(enemy,p) &&
            p[0x2f6]==1 && (half(p+0x3ac)&0x80u);
        const int r102_em00 = r102_close_em00_punch(enemy, p, effect);
        const int r101_square=p[0x2f6]==1&&
            (r101_key_carrier_punch(enemy,p,effect)||(em27_punch&&enemy->entry.b.work->room==0x101)||r101_side_arena_punch(enemy,p,effect));
        const int formation_capture = amplified_triangle && dependency == 0x202 &&
            r100_west_em02_em03(enemy,p) && p[0x2f5] == 0x18 && p[0x2f6] == 1;
        const int boss_heat=heat_contact&&boss_strike&&p[0x2f6]==1&&
            (p[0x2f5]==0x4b||(dependency==0x214&&p[0x2f5]==0x16));
        const int verified_motion = (r200_triangle&&p[0x2f5]==0x15&&p[0x2f6]==1) || (r104_triangle&&em03_combo) || boss_heat || ((amplified_triangle || heat_contact || boss_square || opening_attack_contact || back_circle || r102_em00 || r101_square ||
            (r102_stamp2_punch(enemy, p, effect) && p[0x2f6] == 1)) && p[0x2f5] == 0x15) ||
            formation_capture || ((r102_em00 || r101_square) && p[0x2f5] == 0x4b && p[0x2f6] == 1) ||
            (effect==7&&r101_wave4_em27(enemy,p)&&p[0x2f5]==0x16&&p[0x2f6]==1);
        if ((!verified_motion&&(!r104_punch||r104_recovery||r104_braced)) || p[0x2f4] || p[0x2f7] || !rank)
            return stop(frontier, 0x20a4e8);
        /* 20A4CC adds a separate Heat gain before switching on the hit kind. */
        gh_actor_motion_status status = gain_heat_002a9978(&enemy->entry.b, rank == 5 ? 1.0f : 4.0f, frontier);
        if (status) return status;
        call->phase = amplified_power_ready;
    }
    if (call->phase == 2 || call->phase == amplified_power_ready) {
        put(p + 0x17ac, half(p + 0x54a));
        uint32_t power_bits = word(body->bytes + 0x4c);
        if (call->phase == amplified_power_ready) {
            /* Give2A jumps to 20A578 and skips the default power doubling. */
            if(!heat_contact)power_bits <<= 1;
            put(p + 0x16d0, word(p + 0x16d0) & ~0x800000u);
        }
        int32_t power;
        memcpy(&power, &power_bits, sizeof(power));
        gh_actor_motion_status status = gh_enemy202_damage_0028fb88(enemy, player, power, frontier);
        if (status) return status;
        put(p + 0x17a4, word(p + 0x17a4) + (uint32_t)power);
        put(p + 0x17a8, word(p + 0x17a8) + (uint32_t)power);
        put(p + 0x17b4, (uint32_t)power);
        put(p + 0x1940, 0xbd4ccccd);
        /* 20A6F0 queues the extra bank1 SE only for the original list,
         * whose range starts at dependency 20A. Stamp19 falls through. */
        if (dependency == 0x20a) {
            put(p + 0x17c4, 1);
            put_half(p + 0x17c8, 0x2a);
        }
        put(p + 0x54c, back_circle ? 0x41200000u : 0x40400000u);
        put(p + 0x16dc, 0); /* 26EE40(actor, 0, 0) */
        if(triangle&&word(p+0x17ac)&&p[0x17bd]==1&&rank>=3){
            const uint32_t increase=(int32_t)word(p+0x16ec)<1?(power>9?20u:5u):0u;
            const uint32_t total=word(p+0x16e8)+increase;
            if(increase)put(p+0x16e8,total);
            if((int32_t)total>200){
                call->phase=14;
                return stop(frontier,0x26ecd0);
            }
        }
        call->phase = 3;
    }
    if (call->phase == 14) return stop(frontier,0x26ecd0);
    if(call->phase==EM0A_COUNTER_START||call->phase==EM0A_COUNTER_SOUND||
       call->phase==EM0A_COUNTER_FINISH)return em0a_counter_00274238(call,enemy,p,rank,frontier);
    if (call->phase == WEST_COUNTER_SOUND || call->phase == WEST_COUNTER_RESET)
        return finish_west_counter(call,enemy,frontier);
    if (call->phase == 3) {
        const uint16_t flags = half(p + 0x3ac);
        const uint32_t behavior = word(p + 0x16d0), status = word(p + 0x16d4);
        const int r104_braced_triangle=r104_triangle&&(grab_windup||angry_grab_windup||brace_windup)&&
            (behavior&0x800000u)&&!body->state->heat[0x10];
        const int r104_braced_kick=r104_back_circle&&(behavior&0x800000u);
        if(r104_braced||r104_braced_triangle||r104_braced_kick){
            /* Original 20B394/20B8A8 calls 26A338 and preserves the live Move. */
            if((flags&0x12cu)||(status&0x04080040u)||!half(p+0x54a))
                return stop(frontier,0x20af20);
            put(p+0x16fc,p[0x1700]&1u?0x3e860a92u:0xbe860a92u);
            p[0x1700]++;
            call->reacted=1;
            call->phase=5;
            return GH_ACTOR_MOTION_OK;
        }
        if(r104_punch&&!r104_recovery){
            /* Give07 reaches 20BCE8 on the standing amplified Take. */
            if(!(flags&0x100u)||(flags&0x2cu)||(behavior&0x806020u)||
               (status&0x04140040u)||!half(p+0x54a))
                return stop(frontier,0x20bce8);
            if(p[0x17bc]==1){
                uint32_t draw;
                if(gh_game_mt_next_0030f348(&body->state->random,&draw))
                    return stop(frontier,0x31cfe0);
            }
            /* 20D398, 20CD00 and 20D140 decline with flag100. Original
             * 24BF20 returns zero for dep0250 and 20BDDC selects 01/01. */
            p[0x2f4]=p[0x2f5]=1;
            p[0x2f6]=p[0x2f7]=0;
            call->reacted=1;
            call->phase=5;
            return GH_ACTOR_MOTION_OK;
        }
        if(back_circle){
            /* Give0F reaches 20C4A0 after the standing counter probes decline.
             * Selector0 returns before consuming MT in 20CAC8. */
            const float stun=number(p+0x17cc);
            if(rank!=1||p[0x17bc]||p[0x17b9]>1||!half(p+0x54a)||
                p[0x2f4]||p[0x2f5]!=0x15||p[0x2f6]!=1||p[0x2f7]||
                (flags&0x2cu)||(behavior&0x804020u)||(status&0x04100040u)||
                !isfinite(stun))
                return stop(frontier,0x20b394);
            p[0x2f4]=1;
            p[0x2f5]=stun>0||flags&0x100u?4:3;
            p[0x2f6]=p[0x2f7]=0;
            call->reacted=1;
            call->phase=5;
            return GH_ACTOR_MOTION_OK;
        }
        if(heat_contact){
            /* 20CAC8 selector1 draws before the active-Heat early return.
             * 20C8D8 uses the standing Take on the admitted original contacts. */
            const int em14_activation=boss_strike&&dependency==0x214&&
                half(player->bytes+0x2f4)==0x0800;
            const int em15_expiry_evade=boss_strike&&dependency==0x215&&
                half(player->bytes+0x2f4)==0x0900&&p[0x2f5]==9&&p[0x2f6]==1&&p[0x2f7]==1;
            const uint8_t action=boss_strike?(em14_activation?0x16:0x4b):0x15;
            const uint32_t status_mask=boss_strike?0x01401000u:0x01401400u;
            const int running_em00=side_heat&&dependency==0x200;
            const int recoiling_em03=side_heat&&dependency==0x203&&half(p+0x2f4)==1&&
                p[0x2f6]==1&&!p[0x2f7];
            /* 20CAC8 selector 2 reaches the active-Heat return without an MT draw.
             * Give2A then uses the original 20C8D8 standing reaction for 0227. */
            const int wave4=r101_wave4_em27(enemy,p)&&half(p+0x2f4)==0x0900&&
                p[0x2f6]==1&&p[0x2f7]==1&&p[0x17b9]==2&&p[0x17ba]==2&&
                p[0x17bb]==1&&p[0x17bc]==2&&p[0x17bd]==2;
            const int ordinary=(p[0x17bd]==1&&p[0x17bc]==(boss_strike?0:1)&&
                (recoiling_em03||(!p[0x2f4]&&(p[0x2f5]==(running_em00?6:action)||em15_expiry_evade)))&&
                (!em14_activation||(p[0x2f6]==1&&!p[0x2f7]))&&
                (status&status_mask)==((running_em00||recoiling_em03)?0u:boss_strike?0x1000u:0x400u));
            if(rank!=1||(int32_t)word(p+0x17ac)<=0||!half(p+0x54a)||
                (flags&0x2cu)||(behavior&0x80e020u)||
                (wave4?(status&0x01401400u)!=0:!ordinary))
                return stop(frontier,0x20bce8);
            if(p[0x17bc]==1){
                uint32_t random;
                if(gh_game_mt_next_0030f348(&body->state->random,&random))
                    return stop(frontier,0x31cfe0);
            }
            p[0x2f4]=1;
            p[0x2f5]=1;
            p[0x2f6]=p[0x2f7]=0;
            call->reacted=1;
            call->phase=5;
            return GH_ACTOR_MOTION_OK;
        }
        if (r104_triangle && falling && !(flags & 0x100u)) {
            /* Give12 follows 20DA18 to selector 2's roll unless already
             * staggered or 12C748 reports the player's special mode. */
            const float stagger = number(p + 0x17cc);
            if (!isfinite(stagger)) return stop(frontier, 0x26f1d8);
            if (stagger <= 0.0f && (int32_t)word(p + 0x17ac) > 0 && !word(player->bytes + 0x5fc)) {
                uint32_t draw;
                if (p[0x17b8] != 2) return stop(frontier, 0x20db14);
                if (gh_game_mt_next_0030f348(&body->state->random, &draw)) return stop(frontier, 0x31cfe0);
                if (!(draw & 3u)) {
                    restore_counter_hp(p);
                    put(p + 0x54c, 0x40000000u);
                    put(p + 0x17c4, UINT32_MAX);
                    put_half(p + 0x17c8, 0);
                    p[0x1867] = 0;
                    p[0x2f4] = 1;
                    p[0x2f5] = 9;
                    p[0x2f6] = p[0x2f7] = 0;
                    call->phase = 5;
                    return GH_ACTOR_MOTION_OK;
                }
            }
            /* 20BA0C returns one and preserves the existing 01/07 fall. */
            call->reacted = 1;
            call->phase = 5;
            return GH_ACTOR_MOTION_OK;
        }
        /* 20DA18 declines a counter during stun or with previous HP <= 0. */
        if (continuing_knockdown) {
            call->reacted = 1;
            call->phase = 5;
            return GH_ACTOR_MOTION_OK;
        }
        if (!triangle && p[0x17bd] == 1 && rank >= 3) return stop(frontier, 0x26ecd0);
        /* 20B988 routes an active fall through 20DA18. Its saved HP
         * is zero here, so 20CA40 preserves 01/07 and returns one. */
        if(continued_fall&&!word(p+0x17ac)&&!(behavior&0x802000u)&&!(status&0x180040u)){
            call->reacted=1;
            call->phase=5;
            return GH_ACTOR_MOTION_OK;
        }
        if (((r102_stamp2_punch(enemy,p,effect)&&(behavior&0x882000u)==0x800000u)||
             (arena_em0a&&(behavior&0x800000u))) &&
            !p[0x2f4] && p[0x2f5] == 0x4b && p[0x2f6] == 1 && !p[0x2f7] &&
            half(p + 0x54a) &&
            !body->state->heat[0x10]) {
            /* 20AF20 goes directly to 20B390 with no block timer flag.
             * Live HP and Heat inactive reach 26A338 without resetting Move. */
            put(p + 0x16fc, p[0x1700] & 1 ? 0x3e860a92u : 0xbe860a92u);
            p[0x1700]++;
            call->reacted = 1;
            call->phase = 5;
            return GH_ACTOR_MOTION_OK;
        }
        const int at0b_counter=r102_at0b_em02_punch(enemy,p,effect)&&half(p+0x2f4)==0x6900&&
            p[0x2f6]==1&&p[0x2f7]==1&&(behavior&0x2000u)&&!(behavior&0x804020u)&&
            !(status&0x182040u)&&!(flags&0x12cu)&&p[0x17bc]==1&&rank&&rank<=2;
        if(at0b_counter&&!body->state->heat[0x10]&&!word(player->bytes+0x5fc)){
            const gh_enemy_entry_bindings *binding=&enemy->entry.b;
            const gh_title_player_view *receiver=&enemy->entry.object.view;
            float origin[3],target[3],angle;
            const float face=number(p+0x760),timer=number(p+0x173c);
            if(!isfinite(face)||!isfinite(timer)||!receiver->alias_f0||!player->alias_f0)
                return stop(frontier,0x20b140);
            memcpy(origin,receiver->alias_f0,sizeof(origin));
            memcpy(target,player->alias_f0,sizeof(target));
            if(gh_player_target_angle_002da038(binding->boot_elf,binding->boot_elf_bytes,
                origin,target,number(p+0x104),3.1415927f,&angle,frontier))return stop(frontier,0x2da038);
            if(fabsf(angle)<=1.5707964f&&face<1.5707964f){
                if(timer>0){
                    /* 20B140's original selector1 draw occurs before it rejects Give07. */
                    uint32_t draw;
                    if(gh_game_mt_next_0030f348(&body->state->random,&draw))
                        return stop(frontier,0x31cfe0);
                    restore_counter_hp(p);
                    p[0x2f4]=p[0x2f6]=0;
                    p[0x2f5]=0x69;
                    p[0x2f7]=1;
                    call->phase=stamp2_counter_cue;
                    goto counter_cue;
                }
                restore_counter_hp(p);
                p[0x2f4]=p[0x2f6]=p[0x2f7]=0;
                p[0x2f5]=0x6f;
                call->reacted=1;
                call->phase=5;
                return GH_ACTOR_MOTION_OK;
            }
        }
        /* The original dead-HP table sends effect 07 to 20BB80 and
         * effect 12 to 20BBB0. Both bosses use this cEm00 table. */
        const int r102_em00 = r102_close_em00_punch(enemy, p, effect);
        const int lethal_wave4=effect==7&&r101_wave4_em27(enemy,p)&&half(p+0x2f4)==0x0f01&&
            p[0x2f6]==1&&!p[0x2f7]&&word(p+0x17b4)==3&&p[0x17bd]==2;
        if ((opening_pair || r102_em00 || arena_em0a || boss_strike ||
             r101_side_arena_punch(enemy,p,effect) || lethal_wave4) &&
            !half(p + 0x54a) && rank &&
            (triangle||rank < 3) && (p[0x17bd] == 1 || lethal_wave4) &&
            !(flags & 0x2cu) && !(behavior & 0x806020u) &&
            !(status & 0x180040u) && !(p[0x2f4] == 1 && p[0x2f5] == 0x16)) {
            p[0x2f4] = triangle ? 1 : 2;
            p[0x2f5] = triangle ? 7 : 0;
            p[0x2f6] = p[0x2f7] = 0;
            call->reacted = 1;
            call->phase = 5;
            return GH_ACTOR_MOTION_OK;
        }
        const int guard_counter=triangle&&r100_west_em02_em03(enemy,p)&&
            !p[0x2f4]&&p[0x2f5]==0x69&&p[0x2f6]==1&&p[0x2f7]==1&&
            (behavior&0x2000u)&&!(status&0x80040u)&&!body->state->heat[0x10]&&
            !word(player->bytes+0x5fc);
        if (guard_counter) {
            /* HEADINGS computed 2DA038 against this same live Player. */
            const float angle=number(p+0x760),timer=number(p+0x173c);
            if(!isfinite(angle)||!isfinite(timer))return stop(frontier,0x20b394);
            if(angle<1.5707964f){
                if(status&0x2000u){
                    restore_counter_hp(p);
                    put(p+0x16d4,status|0x4000u);
                    call->reacted=1;
                    call->phase=5;
                    return GH_ACTOR_MOTION_OK;
                }
                if(timer>0){
                    /* 20CAC8 draws for probe 1, then rejects effect12 before
                     * the timed 274238 counter is restarted. */
                    gh_game_mt random=body->state->random;
                    uint32_t draw;
                    if(p[0x17bc]==1&&gh_game_mt_next_0030f348(&random,&draw))
                        return stop(frontier,0x31cfe0);
                    if(rank>=3){
                        if(gh_game_mt_next_0030f348(&random,&draw))return stop(frontier,0x31d000);
                        const uint32_t fraction=gh_frame_unit_bits_0030f518_word(draw);
                        float roll;
                        memcpy(&roll,&fraction,sizeof(roll));
                        if(roll<=0.5f)put(p+0x173c,0);
                    }
                    body->state->random=random;
                    restore_counter_hp(p);
                    p[0x2f4]=p[0x2f6]=0;
                    p[0x2f5]=0x69;
                    p[0x2f7]=1;
                    call->phase=WEST_COUNTER_SOUND;
                    return finish_west_counter(call,enemy,frontier);
                }
                restore_counter_hp(p);
                p[0x2f4]=p[0x2f6]=p[0x2f7]=0;
                p[0x2f5]=0x6f;
                call->reacted=1;
                call->phase=5;
                return GH_ACTOR_MOTION_OK;
            }
        }
        /* 20B140 accepts frontal Give07 and Give12 during the bosses' recovery.
         * 20CAC8 selector0 returns false without the earlier draw. */
        const int boss_recover=boss_strike&&p[0x2f7]<=1&&!p[0x17bc];
        const int em0a_recover=arena_em0a&&effect==0x12&&p[0x2f7]==1;
        const int side_recover=r101_side_arena_punch(enemy,p,effect)&&p[0x2f7]==1&&p[0x17bc]==1;
        const int recover=(boss_recover||em0a_recover||side_recover)&&half(p+0x2f4)==0x6900&&
            p[0x2f6]==1&&(behavior&0x2000u)&&!(behavior&0x804020u)&&
            !(status&0x180040u)&&!(flags&0x12cu)&&p[0x17bd]==1&&rank&&rank<=2;
        if(recover&&!body->state->heat[0x10]&&!word(player->bytes+0x5fc)){
            const gh_enemy_entry_bindings *binding=&enemy->entry.b;
            float from[3],to[3],angle;
            const float facing=number(p+0x760),timer=number(p+0x173c);
            if(!isfinite(facing)||!isfinite(timer)||!enemy->entry.object.view.alias_f0||!player->alias_f0)
                return stop(frontier,0x20b140);
            memcpy(from,enemy->entry.object.view.alias_f0,sizeof(from));
            memcpy(to,player->alias_f0,sizeof(to));
            if(gh_player_target_angle_002da038(binding->boot_elf,binding->boot_elf_bytes,from,to,
               number(p+0x104),3.1415927f,&angle,frontier))return stop(frontier,0x2da038);
            if(fabsf(angle)<=1.5707964f&&facing<1.5707964f){
                if(boss_recover&&(status&0x2000u)){
                    restore_counter_hp(p);
                    put(p+0x16d4,status|0x4000u);
                    call->reacted=1;
                    call->phase=5;
                    return GH_ACTOR_MOTION_OK;
                }
                if(timer>0){
                    /* 20CAC8 draws for selector1, then rejects effect12. */
                    uint32_t draw;
                    if(p[0x17bc]==1&&gh_game_mt_next_0030f348(&body->state->random,&draw))
                        return stop(frontier,0x31cfe0);
                    restore_counter_hp(p);
                    if(side_recover){
                        /* 20B1B0 restarts 274238's variant-1 cue without
                         * refreshing the timer while the 2000 flag is set. */
                        p[0x2f4]=p[0x2f6]=0;
                        p[0x2f5]=0x69;
                        p[0x2f7]=1;
                        call->phase=stamp2_counter_cue;
                        goto counter_cue;
                    }
                    call->phase=EM0A_COUNTER_START;
                    return em0a_counter_00274238(call,enemy,p,rank,frontier);
                }
                restore_counter_hp(p);
                p[0x2f4]=0;p[0x2f5]=0x6f;p[0x2f6]=p[0x2f7]=0;
                call->reacted=1;
                call->phase=5;
                return GH_ACTOR_MOTION_OK;
            }
        }
        const uint32_t ignored=(recover||guard_counter)?0x2000u:0u;
        /* 20A7D0 suppresses the ordinary pressure gain for EM27 at rank one. */
        if((em27_punch||side_em0f)&&p[0x17bd]==2&&rank>=2)return stop(frontier,0x26ecd0);
        if (!rank || p[0x17bd] != ((boss_recovery||side_em0f) ? 2u : 1u) || !half(p + 0x54a) || (flags & 0x2cu) ||
            (behavior & (opening_pair ? 0x806020u : 0x802020u) & ~ignored) ||
            (status & 0x80040u) || (p[0x2f4] == 1 && p[0x2f5] == 0x16))
            return stop(frontier, 0x20a7e0);
        /* Both bosses use 20CAC8 selector0, 20D398 selector2, and 20CD00 selector1.
         * At rank one the original probes decline Give07 and Give12 without MT. */
        const unsigned hit_probe=boss_recovery?2u:(boss_strike||side_em0f)?0u:(unsigned)hit_probe_selector;
        const unsigned dodge_probe=(boss_recovery||boss_strike)?2u:side_em0f?0u:(unsigned)hit_probe_selector;
        const unsigned counter_kind=boss_recovery?2u:(unsigned)counter_selector;
        if (p[0x17bc] != hit_probe || p[0x17ba] != dodge_probe ||
            p[0x17b9] != counter_kind || p[0x17bb] != (side_em0f?0u:1u))
            return stop(frontier, 0x20cac8);
        /* 20D138 returns 0. With selector 1, 20CAC8 draws once before
         * rejecting effect 7. 20D398 requires selector 3 for effect 7,
         * and 20CD00 rejects selector 0. Selector 1 returns at rank 1.
         * The rank2+ selector1 branch has an additional counter draw. */
        if (boss_strike && triangle && rank >= 2)
            return stop(frontier, 0x20d398);
        if (!triangle && counter_selector && rank >= 2)
            return stop(frontier, 0x20cd00);
        /* Selector 2 validates the east EM27 row without the selector 1 draw. */
        if (hit_probe_selector && !side_em0f) {
            gh_game_mt mt=body->state->random;
            uint32_t probe_random;
            if (gh_game_mt_next_0030f348(&mt, &probe_random))
                return stop(frontier, 0x31cfe0);
            if(triangle){
                uint32_t pc=0;
                const unsigned counter=triangle_counter(p,player,body->state,rank,opening_200,&mt,&pc);
                if(pc)return stop(frontier,pc);
                if(counter){
                    body->state->random=mt;
                    counter_mode(p,counter,counter==0x70);
                    call->phase=5;
                    return GH_ACTOR_MOTION_OK;
                }
            }
            body->state->random=mt;
        }
        const float block = number(p + 0x1740);
        if (!isfinite(block)) return stop(frontier, 0x20d140);
        if(boss_recovery){
            /* 20CAC8 selector2 and 20D398 Give 7 do not draw. 20CD00
             * selector2 uses the original 1/32 roll at rank one. */
            const float face=number(p+0x760),rate=number(p+0x5a8),stun=number(p+0x17cc);
            if(!isfinite(face)||!isfinite(rate)||!isfinite(stun))
                return stop(frontier,0x20cd00);
            if(((status&0x04000000u)||(!body->state->heat[0x10]&&!word(player->bytes+0x5fc)))&&
                !(flags&0x100u)&&(int32_t)word(p+0x17ac)>0&&face<=2.3561945f&&
                rate>=1.0f&&block<=0&&stun<=0){
                uint32_t draw;
                if(gh_game_mt_next_0030f348(&body->state->random,&draw))
                    return stop(frontier,0x31cfe0);
                if((draw&31u)==3){
                    counter_mode(p,0xa,0);
                    call->phase=5;
                    return GH_ACTOR_MOTION_OK;
                }
            }
        }
        /* 20D218 returns before the counter RNG on the amplified effect-7 Take. */
        const int skip_counter = (flags & 0x100u) && (r104_triangle||(boss_strike&&effect==7) || r102_close_em00_punch(enemy, p, effect) ||
            r102_stamp2_punch(enemy, p, effect));
        if (p[0x17bb] && !skip_counter && !word(player->bytes + 0x5fc) && block <= 0 &&
            !body->state->heat[0x10] && !(number(p + 0x17cc) > 0)) {
            uint32_t random;
            if (gh_game_mt_next_0030f348(&body->state->random, &random))
                return stop(frontier, 0x31cfe0);
            unsigned threshold = (random & 3) + (rank == 5 ? 2u : rank >= 3 ? 3u : rank == 2 ? 4u : 5u);
            if (p[0x17c1]) threshold -= 2;
            p[0x17bf]++;
            if (p[0x17bf] > 100) p[0x17bf] = 100;
            if (p[0x17bf] >= threshold) {
                const int r102_em00 = r102_close_em00_punch(enemy, p, effect);
                const int stamp2=r102_stamp2_punch(enemy,p,effect);
                const int east_em00=ordinary_punch&&dependency==0x200&&enemy->entry.b.work->room==0x102&&
                    half(p+0x640)==3&&p[0x1650]==1;
                const int at0b_em02=r102_at0b_em02_punch(enemy,p,effect);
                /* 20D348 uses variant 1 for the authored arena EM00/EM03,
                 * with 274238's default timer class for both dependencies. */
                const int key_carrier=r101_key_carrier_punch(enemy,p,effect);
                /* The original frontal 20D348 branch uses the same dep0250 timer
                 * for the ordinary Give07 in the turning clip and the grab windup. */
                const int r104_turn_counter=r104_recovery&&word(p+0x2f4)==0x01010800u&&
                    !half(p+0x3ac);
                const int r104_counter=(r104_triangle&&grab_windup)||r104_turn_counter;
                const int synchronous_counter=stamp2||east_em00||r102_em03_punch(enemy,p,effect)||at0b_em02||
                    r101_side_arena_punch(enemy,p,effect)||key_carrier||(em27_punch&&enemy->entry.b.work->room==0x101)||r104_counter;
                const int faces=number(p+0x760)<=1.5707964f;
                /* 20D2A0 always draws before the face test. A rear turn cannot enter 274238 here. */
                const int r104_rear_pressure=r104_recovery&&p[0x2f5]==8&&!faces;
                if(!triangle&&!r102_em00&&!synchronous_counter&&!r103_arena_punch_target(enemy,dependency)&&
                   !r104_rear_pressure){
                    call->phase = 6;
                    return stop(frontier, 0x20d2a0);
                }
                uint32_t retry=UINT32_MAX;
                gh_game_mt random_state = body->state->random;
                if ((!at0b_em02||faces)&&gh_game_mt_next_0030f348(&random_state,&retry))
                    return stop(frontier,0x31cfe0);
                if(!(retry&3u)&&faces){
                    if(arena_em0a||boss_strike){
                        body->state->random=random_state;
                        restore_counter_hp(p);
                        call->phase=EM0A_COUNTER_START;
                        return em0a_counter_00274238(call,enemy,p,rank,frontier);
                    }
                    const int west_counter=triangle&&r100_west_em02_em03(enemy,p);
                    if(r102_em00||synchronous_counter||west_counter){
                        /* 274238's ordinary dependency arm and 20D358's counter reset
                         * commit only once after the original ordered RNG draws. */
                        float duration=0;
                        const int refresh_timer=!(word(p+0x16d0)&0x2000u);
                        if(refresh_timer){
                            static const float delay[]={0,40,30,20,20,15};
                            static const float em27_delay[]={0,15,13,12,12,10};
                            if(rank>5)return stop(frontier,0x274238);
                            /* 447260 dispatches dep0250 to the 10-unit rank-one timer. */
                            duration=r104_counter?10.0f:(key_carrier||(em27_punch&&enemy->entry.b.work->room==0x101))?em27_delay[rank]:delay[rank];
                            if(rank<5){
                                uint32_t jitter;
                                fenv_t env;
                                if(gh_game_mt_next_0030f348(&random_state,&jitter))
                                    return stop(frontier,0x31d000);
                                if(feholdexcept(&env))return stop(frontier,0x274238);
                                if(fesetround(FE_TOWARDZERO)){
                                    (void)fesetenv(&env);
                                    return stop(frontier,0x274238);
                                }
                                const uint32_t unit_bits=gh_frame_unit_bits_0030f518_word(jitter);
                                float unit;
                                memcpy(&unit,&unit_bits,4);
                                volatile float delta=unit*4.0f;
                                volatile float result=duration+delta;
                                duration=result;
                                if(fesetenv(&env))return stop(frontier,0x274238);
                            }
                        }
                        if(west_counter){
                            /* 20D34C passes variant 1. CORE 7C plays before
                             * the queued cue is cleared and the reset draw. */
                            body->state->random=random_state;
                            restore_counter_hp(p);
                            if(refresh_timer)memcpy(p+0x173c,&duration,4);
                            p[0x2f4]=p[0x2f6]=0;
                            p[0x2f5]=0x69;
                            p[0x2f7]=1;
                            call->phase=WEST_COUNTER_SOUND;
                            return finish_west_counter(call,enemy,frontier);
                        }
                        if(synchronous_counter){
                            /* 20D348 selects variant 1 and its synchronous cue.
                             * 20D350 clears the pending bank only after the cue returns. */
                            body->state->random=random_state;
                            restore_counter_hp(p);
                            if(refresh_timer)memcpy(p+0x173c,&duration,4);
                            p[0x2f4]=p[0x2f6]=0;
                            p[0x2f5]=0x69;
                            p[0x2f7]=1;
                            call->phase=stamp2_counter_cue;
                        }else{
                            uint32_t reset;
                            if(gh_game_mt_next_0030f348(&random_state,&reset))return stop(frontier,0x31cfe0);
                            body->state->random=random_state;
                            counter_mode(p,0x69,0);
                            if(refresh_timer)memcpy(p+0x173c,&duration,4);
                            p[0x17bf]=(uint8_t)(reset%3u);
                            call->phase=5;
                            return GH_ACTOR_MOTION_OK;
                        }
                    }else{
                        body->state->random=random_state;
                        restore_counter_hp(p);
                        call->phase=15;
                        return stop(frontier,0x274238);
                    }
                }
                body->state->random=random_state;
            }
        }
        if(call->phase==3)call->phase=4;
    }
counter_cue:
    if(call->phase==stamp2_counter_cue){
        gh_actor_motion_status status=enemy_receive_sound(call,enemy,0,0x7c,frontier);
        if(status)return status;
        put(p+0x17c4,UINT32_MAX);
        put_half(p+0x17c8,0);
        call->phase=stamp2_counter_reset;
    }
    if(call->phase==stamp2_counter_reset){
        uint32_t reset;
        if(gh_game_mt_next_0030f348(&body->state->random,&reset))return stop(frontier,0x31cfe0);
        p[0x17bf]=(uint8_t)(reset%3u);
        call->phase=5;
    }
    if (call->phase == 4) {
        /* Both 214 and 215 point to the original 24C028 early return. */
        if(!boss_strike&&!r104_recovery&&!r104_triangle){
            gh_actor_motion_status status = first_punch_wall(enemy->b.actors,p,frontier);
            if (status) return status;
        }
        p[0x2f4] = 1;
        const int stunned = number(p + 0x17cc) > 0;
        /* 20BD94 sends amplified effect 7 for 020A and the twins to 20BDE0. */
        const int amplified_square = (half(p + 0x3ac) & 0x100u) &&
            ((boss_strike&&effect==7)||r102_stamp2_punch(enemy,p,effect));
        /* 20BF34 uses the fall for Give12 if stunned or amplified. */
        const int triangle_fall=stunned||(r104_triangle&&(half(p+0x3ac)&0x100u));
        p[0x2f5] = triangle ? (triangle_fall ? 7 : 1) : (stunned || amplified_square ? 1 : 0);
        p[0x2f6] = p[0x2f7] = 0;
        call->reacted = 1;
        call->phase = 5;
    }
    return GH_ACTOR_MOTION_OK;
}


static unsigned save_rank_for(int score, const uint8_t *thresholds) {
    unsigned bracket = 0;
    while (bracket < 5 && (int32_t)word(thresholds + bracket * 4) <= score) bracket++;
    return bracket < 3 ? bracket + 1 : 5;
}

static gh_actor_motion_status hit_rank_points_001fa280(const gh_enemy_entry_bindings *b,
    const uint8_t *thresholds,unsigned points,uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0;
    if(!b||!thresholds||points>INT16_MAX||!b->work||!b->init)return GH_ACTOR_MOTION_ARGUMENT;
    gh_core_save_view save = b->save;
    if (!save.bytes) return GH_ACTOR_MOTION_OK;
    if (save.size < 0x20) return stop(frontier, 0x1fa280);
    if ((word((const uint8_t *)b->work + 0x5e4) & 0x400000u) || save.bytes[0x1f] == 2)
        return GH_ACTOR_MOTION_OK;
    if (word(save.bytes + 0x14) & 0x4000000u)
        return stop(frontier, 0x1fa2ec); /* 568240's cheat-path input owner. */

    const int old = (int16_t)half(save.bytes + 0x1c);
    const unsigned old_rank = save_rank_for(old, thresholds);
    static const float gain[] = {1.5f, 1.2f, 1.0f, 0.7f, 0.5f};
    fenv_t saved;
    if (feholdexcept(&saved)) return stop(frontier, 0x1fa3fc);
    if (fesetround(FE_TOWARDZERO)) {
        (void)fesetenv(&saved);
        return stop(frontier, 0x1fa3fc);
    }
    volatile float multiplier = gain[old_rank-1];
    if ((uint16_t)b->init->continue_count > 1) multiplier = multiplier * 0.8f;
    volatile float gain_points = multiplier * (float)points;
    if (fesetenv(&saved)) return stop(frontier, 0x1fa3fc);

    int score = (int16_t)(uint16_t)(old + (int)gain_points);
    if (score < 0) score = 0;
    int32_t maximum = (int32_t)word(thresholds + (save.bytes[0x1f] == 0 ? 4 : 16));
    if (maximum < 1 || maximum > INT16_MAX) return stop(frontier, 0x1fa4bc);
    if (score >= maximum) score = maximum - 1;
    unsigned rank = save_rank_for(score, thresholds);
    if (rank + 1 < old_rank) rank = old_rank - 1;
    if (rank == 4) rank = 3;
    if (old_rank < rank) save.bytes[0x1e] = 1;
    if (rank < old_rank && save.bytes[0x1e]) {
        save.bytes[0x1e]--;
        score = (int16_t)half(thresholds + (rank - 1) * 4);
    }
    put_half(save.bytes + 0x1c, (uint16_t)score);
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_hit_rank_points_001fa280(gh_enemy_actor *enemy,
    const uint8_t *thresholds,unsigned points,uint32_t *frontier)
{
    return hit_rank_points_001fa280(enemy?&enemy->entry.b:NULL,thresholds,points,frontier);
}

gh_actor_motion_status gh_enemy202_hit_rank_001fa280(
    gh_enemy_actor *enemy, const uint8_t *thresholds, uint32_t *frontier)
{
    return gh_enemy202_hit_rank_points_001fa280(enemy, thresholds, 4, frontier);
}

static gh_actor_motion_status hit_stun_0028ed28(gh_game_init_state *init,uint8_t *p,
    float timer,int triangle,int amplified,uint32_t *frontier)
{
    unsigned increment=10;
    if(triangle){
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&init->random,&draw))return stop(frontier,0x31cfe0);
        increment=15u<<(draw&1u);
    }
    if(amplified)increment*=3;
    if(timer<=0){
        const int32_t sum=(int32_t)(word(p+0x5b4)+increment);
        put(p+0x5b4,sum<0?0u:sum>100000?100000u:(uint32_t)sum);
        put(p+0x5b8,0x42b40000u);
    }
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_hit_stun_0028ed28(gh_enemy_actor *enemy, uint32_t *frontier) {
    if (!frontier) return GH_ACTOR_MOTION_ARGUMENT;
    *frontier = 0;
    if (!enemy || !enemy->entry.object.view.bytes || enemy->entry.object.view.size < 0x17d0)
        return GH_ACTOR_MOTION_ARGUMENT;
    uint8_t *p = enemy->entry.object.view.bytes;
    const float timer = number(p + 0x17cc);
    const gh_hitarea_object *body = &enemy->hitareas.units[0];
    const uint16_t effect = body->bytes && body->size >= 0x48 ? half(body->bytes + 0x46) : 0;
    const gh_title_player_view *source=NULL;
    const int has_source=(effect==0x2a||effect==0x0f||effect==0x30)&&body->bytes&&body->size>=0x50&&
        !gh_hitarea_object_last_hit_actor(body,&source)&&source&&source->bytes&&source->size>=0x600;
    const int heat_contact=has_source&&
        r103_heat_transition_contact(enemy,source,effect,(int32_t)word(body->bytes+0x4c));
    const int back_circle=has_source&&back_circle_contact(enemy,source,effect);
    const int grenade=has_source&&r101_grenade_contact(enemy,source,effect,word(body->bytes+0x4c));
    const int triangle = triangle_take_supported(enemy, p, effect);
    const int amplified = (half(p + 0x3ac) & 0x100u) != 0;
    const int shockwave=effect==0x27&&(word(p+0x564)==0x214||word(p+0x564)==0x215||r104_em03_damage_receiver(enemy,p));
    const int boss_square=effect==7&&r103_boss_player_strike_target(enemy,p);
    const gh_title_player_view *catapult_source=NULL;
    const int catapult=effect==0x52&&!gh_hitarea_object_last_hit_actor(body,&catapult_source)&&
        r200_catapult_contact(enemy,body,catapult_source);
    const int r104_punch=effect==7&&word(body->bytes+0x4c)==3&&
        gh_enemy203_is_r104_entry(&enemy->entry.request,enemy->entry.b.work->room)&&
        (((half(p+0x2f4)==0x0101||half(p+0x2f4)==0x0001)&&!p[0x2f6]&&!p[0x2f7])||
         (half(p+0x2f4)==0x2100&&r104_player_punch(enemy,p,effect)));
    if (!isfinite(timer) || (!catapult && !grenade && !shockwave && !r104_punch && word(p + 0x564) != 0x20a && !r100_stamp19(enemy, p) &&
         !r100_stamp20(enemy, p) && !triangle && !heat_contact && !back_circle && !room_unarmed_punch(enemy, p, effect) &&
         !r102_em03_punch(enemy, p, effect) && !original_em27_punch(enemy, p, effect) &&
         !r102_at0b_em02_punch(enemy,p,effect)) ||
        (amplified && !grenade && !shockwave && !heat_contact && !back_circle && !boss_square && !r104_punch &&
         !r101_key_carrier_punch(enemy,p,effect) &&
         !(enemy->entry.b.work->room==0x101&&original_em27_punch(enemy,p,effect)) && !r101_side_arena_punch(enemy,p,effect) &&
         !(triangle && (r200_em02_triangle_recoil(enemy,p)||r100_amplified_triangle_receiver(enemy,p) ||
        r104_amplified_triangle_fall(enemy,p) || r103_first_wave_em00(enemy) || r103_arena_em0a(enemy,p) || r103_boss_player_strike_target(enemy,p))) &&
         !r102_close_em00_punch(enemy, p, effect) && !r102_stamp2_punch(enemy, p, effect)))
        return stop(frontier, 0x208f34);
    unsigned increment = 10;
    if (triangle || effect == 0x52 || grenade || heat_contact || shockwave || back_circle) {
        uint32_t random;
        if (gh_game_mt_next_0030f348(&enemy->entry.b.init->random, &random))
            return stop(frontier, 0x31cfe0);
        increment = 15u << (random & 1u);
    }
    /* 208F58 triples the selected stun increment for the same hit marker. */
    if (amplified) increment *= 3;
    if (timer <= 0) {
        const int32_t sum = (int32_t)(word(p + 0x5b4) + increment);
        put(p + 0x5b4, sum < 0 ? 0u : sum > 100000 ? 100000u : (uint32_t)sum);
        put(p + 0x5b8, 0x42b40000);
    }
    return GH_ACTOR_MOTION_OK;
}


static gh_actor_motion_status take_impact_00271810(gh_enemy202_hit_call *call,
    const gh_enemy_entry *entry,const gh_hitarea_object *body,gh_enemy_actor *enemy,
    const gh_title_player_view *player,uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0;
    if(!call||!entry||!body||!player||!player->bytes||player->size<0x600||
       !entry->b.init||!entry->object.view.bytes)return GH_ACTOR_MOTION_ARGUMENT;
    if(call->phase==10)return GH_ACTOR_MOTION_OK;
    if(call->phase!=9)return stop(frontier,0x271810);
    const gh_title_player_view *actor=&entry->object.view;
    const uint8_t *p=actor->bytes;
    gh_game_init_state *init=entry->b.init;
    gh_core_save_view save=entry->b.save;
    if (actor->size < 0x1944 || !actor->alias_f0 || !body->bytes || body->size < 0x50)
        return stop(frontier, 0x271810);
    const uint32_t dependency = word(p + 0x564);
    const int manager_active = (int32_t)word(init->enemies + 0x514) > 0;
    const int devil=!enemy&&dependency==0x220&&half(p+0x2fe)==0x220&&
        half(p+0x640)==0xff&&(call->effect==0x12||call->effect==0x2e)&&
        entry->b.work&&entry->b.work->room==0x100;
    const int thrown_box=call->effect==0x36&&dependency==0x227&&half(p+0x640)==9&&
        entry->b.work&&entry->b.work->room==0x101;
    if(manager_active){
        if(!enemy)return stop(frontier,0x271810);
        const gh_title_player_view *source = NULL;
        if ((call->effect != 0x2e && !thrown_box) ||
            gh_hitarea_object_last_hit_actor(body,&source) || !source || !source->bytes || source->size < 0x300)
            return stop(frontier, 0x271810);
        const uint32_t power = word(body->bytes + 0x4c);
        const uint32_t vtable = scenery_hit_vtable(half(source->bytes + 0x2fe));
        const int scenery = vtable && source->original_vtable214 == vtable &&
            word(source->bytes + 0x214) == vtable && !power && word(body->bytes + 0x40) == UINT32_MAX;
        /* 271CA8 branches on the enemy manager, including when a scene prop
         * supplied the already validated Take. */
        const int box=thrown_box&&half(source->bytes+0x2fe)==0x30a&&
            source->original_vtable214==0x4238d8&&word(source->bytes+0x214)==0x4238d8;
        const int arena_counter = word(body->bytes + 0x40) == UINT32_MAX &&
            r103_counter_collateral(enemy,source,power);
        const int devil_contact = word(body->bytes + 0x40) == UINT32_MAX &&
            r100_devil_contact(enemy,source,power);
        if (!scenery && !box && !r100_counter_collateral(enemy,p,source,power) &&
            !arena_counter && !devil_contact)
            return stop(frontier, 0x271810);
        /* The positive manager arm always creates CORE0:C. Only active
         * Heat or original 12C748 (player+5FC != 0) adds attached CORE121. */
        if (init->heat[0x10] || word(player->bytes+0x5fc))
            return stop(frontier, 0x271df4);
    }
    const int amplified = (half(p + 0x3ac) & 0x100u) != 0;
    const int amplified_triangle = enemy&&call->effect==0x12&&r100_amplified_triangle_receiver(enemy,p);
    if(amplified&&(!enemy||manager_active||
        (!amplified_triangle && !r102_close_em00_punch(enemy, p, half(body->bytes + 0x46)))))
        return stop(frontier, 0x271cf8);
    if (amplified && ((word((const uint8_t *)entry->b.work + 0x590) & 0x800u) ||
                      (word(p + 0x16d4) & 0x8000u)))
        return stop(frontier, 0x271d7c);
    if ((!devil&&(!enemy||(!thrown_box&&(call->effect==0x2e?!scenery_reaction_enemy(enemy,p):
            dependency != 0x20a && !r100_stamp19(enemy, p) && !r100_stamp20(enemy, p) &&
            !triangle_take_supported(enemy, p, call->effect) &&
            !room_unarmed_punch(enemy,p,half(body->bytes+0x46))))))||
        init->heat[0x10] || word(player->bytes + 0x5fc) ||
        (!manager_active && (!save.bytes || save.size < 0xaf)))
        return stop(frontier, 0x271810);
    const unsigned costume = manager_active ? 0 : gh_core_save_costume(save);
    if (costume == 1 || costume == 3 || costume == 5 || costume == 7)
        return stop(frontier, 0x271e58);
    const uintptr_t base = (uintptr_t)p, origin = (uintptr_t)actor->alias_f0;
    if (origin < base || origin - base > actor->size - 16)
        return stop(frontier, 0x271518);

    const float x = number(actor->alias_f0), z = number(actor->alias_f0 + 8);
    const float hit_x = number(body->bytes + 0x20), hit_y = number(body->bytes + 0x24);
    const float hit_z = number(body->bytes + 0x28);
    if (!isfinite(x) || !isfinite(z) || !isfinite(hit_x) || !isfinite(hit_y) || !isfinite(hit_z))
        return stop(frontier, 0x271518);
    fenv_t saved;
    if (feholdexcept(&saved)) return stop(frontier, 0x271518);
    if (fesetround(FE_TOWARDZERO)) {
        (void)fesetenv(&saved);
        return stop(frontier, 0x271518);
    }
    volatile float dx = hit_x - x, dz = hit_z - z;
    float angle, angles[3] = {0}, rotation[16];
    if (gh_game_angle_001f8180(dx, dz, &angle, GH_CAMERA_NATIVE_BINARY32_REFERENCE, frontier)) {
        (void)fesetenv(&saved);
        return GH_ACTOR_MOTION_UNSUPPORTED;
    }
    angles[1] = angle;
    if (gh_actor_matrix_rotation_00147728(angles, 0, rotation, GH_STATIC_BINARY32_REFERENCE, frontier)) {
        (void)fesetenv(&saved);
        return GH_ACTOR_MOTION_UNSUPPORTED;
    }
    uint32_t noise[3];
    for (unsigned i = 0; i < 3; ++i) {
        if (gh_game_mt_next_0030f348(&init->random, &noise[i])) {
            (void)fesetenv(&saved);
            return stop(frontier, 0x31d020);
        }
        noise[i] = gh_frame_jitter_bits_0031d020_word(noise[i]);
    }
    volatile float local_x = number((const uint8_t *)noise) * 0.2f;
    volatile float local_y = number((const uint8_t *)(noise + 1)) * 0.2f;
    volatile float jitter_z = number((const uint8_t *)(noise + 2)) * 0.1f;
    volatile float local_z = 0.4f + jitter_z;
    float world[3];
    for (unsigned i = 0; i < 3; ++i) {
        volatile float a = rotation[i] * local_x;
        volatile float b = rotation[4+i] * local_y;
        volatile float c = rotation[8+i] * local_z;
        volatile float ab = a + b;
        world[i] = ab + c;
    }
    volatile float out_x = x + world[0], out_z = z + world[2];
    if (fesetenv(&saved) || !isfinite(out_x) || !isfinite(out_z))
        return stop(frontier, 0x30a2b0);
    call->impact = (gh_enemy202_impact_request){
        .position = {out_x, hit_y, out_z, 1.0f},
        .bank = manager_active ? 0 : 0x58,
        .index = manager_active ? 0xc : amplified ? 0x12 : 0x10,
        .scale = 1.0f, .mode = -1,
    };
    /* 292F08 tests the original actor byte before the unbound effect. */
    if (amplified && p[0x538]) call->impact.index = 0;
    call->phase = 10;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_impact_00271810(
    gh_enemy202_hit_call *call, gh_enemy_actor *enemy, const gh_title_player_view *player,
    uint32_t *frontier) {
    if (!frontier) return GH_ACTOR_MOTION_ARGUMENT;
    *frontier = 0;
    if (!call || !enemy || !player || !player->bytes || player->size < 0x600 ||
        !enemy->entry.b.init || !enemy->entry.object.view.bytes)
        return GH_ACTOR_MOTION_ARGUMENT;
    if (call->phase == 10) return GH_ACTOR_MOTION_OK;
    if (call->phase != 9) return stop(frontier, 0x271810);
    gh_title_player_view *actor = &enemy->entry.object.view;
    const gh_hitarea_object *body = &enemy->hitareas.units[0];
    const uint8_t *p = actor->bytes;
    gh_game_init_state *init = enemy->entry.b.init;
    gh_core_save_view save = enemy->entry.b.save;
    if (actor->size < 0x1944 || !actor->alias_f0 || !body->bytes || body->size < 0x50)
        return stop(frontier, 0x271810);
    const uint32_t dependency = word(p + 0x564);
    const int manager_active = (int32_t)word(init->enemies + 0x514) > 0;
    const uint32_t boost_timer=word(player->bytes+0x5fc);
    const int boosted_scenery=call->effect==0x2e&&dependency==0x227&&half(p+0x640)==4&&
        half(p+0x2f4)==0x1101;
    const uint16_t take_effect=half(body->bytes+0x46);
    const int boosted_at0b=take_effect==7&&word(p+0x16c4)==2&&half(p+0x2f4)==1&&
        !p[0x2f6]&&!p[0x2f7]&&((dependency==0x200&&half(p+0x640)==7)||
        r102_at0b_em02_punch(enemy,p,take_effect));
    const int boosted_impact=!manager_active&&!init->heat[0x10]&&boost_timer&&boost_timer<=600&&
        !p[0x1650]&&enemy->entry.b.work&&enemy->entry.b.work->room==0x102&&
        (boosted_scenery||boosted_at0b);
    const gh_title_player_view *heat_source=NULL;
    const int32_t take_power=(int32_t)word(body->bytes+0x4c);
    const uint16_t reaction=half(p+0x2f4);
    const int triangle=take_effect==0x12&&call->effect==0x12&&take_power==12&&reaction==0x0101;
    const int em15_square=take_effect==7&&!call->effect&&take_power==3&&
        dependency==0x215&&reaction==0x0001;
    const gh_title_player_view *take_source=NULL;
    const int r104_braced_kick=r104_em03_damage_receiver(enemy,p)&&reaction==0x4b00&&
        p[0x2f6]==1&&!p[0x2f7]&&(word(p+0x16d0)&0x800000u);
    const int back_circle=call->effect==0x0f&&
        (reaction==0x0301||reaction==0x0401||r104_braced_kick)&&take_power==25&&
        back_circle_contact(enemy,player,take_effect)&&
        !gh_hitarea_object_last_hit_actor(body,&take_source)&&take_source==player;
    const int active_boss_strike=(triangle||em15_square||back_circle)&&r103_boss_player_strike_target(enemy,p)&&
        !p[0x2f6]&&!p[0x2f7]&&init->heat[0x10]&&player->size>=0x15f8&&
        (word(player->bytes+0x15f4)&0x10u);
    const int exhausted_impact=call->effect==0x2a&&reaction==0x0101&&
        r103_heat_exhaustion_contact(enemy,player,take_effect,take_power)&&
        !gh_hitarea_object_last_hit_actor(body,&heat_source)&&heat_source==player;
    const int active_side_square=!call->effect&&take_power==3&&
        (reaction==0x0001||(reaction==0x0002&&!half(p+0x54a)))&&
        (r101_side_arena_punch(enemy,p,take_effect)||
         (take_effect==7&&r101_wave4_em27(enemy,p)))&&!p[0x2f6]&&!p[0x2f7]&&
        init->heat[0x10]&&player->size>=0x15f8&&(word(player->bytes+0x15f4)&0x10u);
    const int heat_impact=(active_boss_strike||active_side_square||(call->effect==0x2a&&
        r103_heat_activation_contact(enemy,player,take_effect,take_power)))&&
        !gh_hitarea_object_last_hit_actor(body,&heat_source)&&heat_source==player;
    const gh_title_player_view *scenery_source=NULL;
    const int west_heat_scenery=init->heat[0x10]&&!manager_active&&!boost_timer&&
        enemy->entry.b.work&&enemy->entry.b.work->room==0x100&&dependency==0x203&&
        half(p+0x640)==11&&p[0x1650]==5&&word(p+0x16c4)==6&&reaction==0x1101&&
        !p[0x2f6]&&!p[0x2f7]&&call->effect==0x2e&&take_effect==0x2e&&!take_power&&
        word(body->bytes+0x40)==UINT32_MAX&&
        !gh_hitarea_object_last_hit_actor(body,&scenery_source)&&scenery_source&&
        scenery_source->bytes&&scenery_source->size>=0x300&&
        scenery_source->original_vtable214==scenery_hit_vtable(half(scenery_source->bytes+0x2fe))&&
        scenery_source->original_vtable214&&
        word(scenery_source->bytes+0x214)==scenery_source->original_vtable214;
    if(heat_impact||west_heat_scenery){
        /* 271CA8 keeps the manager path distinct. The unmanaged original
         * 271DA8 Heat arm uses CORE0:40 at the 271518 point and CORE121. */
        if(manager_active)return stop(frontier,0x271ca8);
        gh_actor_motion_status status=gh_enemy202_impact_position_00271518(
            enemy,call->impact.position,frontier);
        if(status)return status;
        const int critical=(half(p+0x3ac)&0x100u)&&
            !(word((const uint8_t *)enemy->entry.b.work+0x590)&0x800u)&&
            !(word(p+0x16d4)&0x8000u);
        call->impact.bank=critical?0x58:0;
        call->impact.index=critical?(p[0x538]?0:0x12):0x40;
        call->impact.scale=1;
        call->impact.mode=-1;
        call->impact.heat_after_critical=(uint8_t)critical;
        call->phase=10;
        return GH_ACTOR_MOTION_OK;
    }
    const int shockwave=call->effect==0x27&&(dependency==0x214||dependency==0x215||r104_em03_damage_receiver(enemy,p));
    const gh_title_player_view *grenade_source=NULL;
    const int grenade=call->effect==0x30&&take_effect==0x30&&
        !gh_hitarea_object_last_hit_actor(body,&grenade_source)&&
        r101_grenade_contact(enemy,grenade_source,take_effect,(uint32_t)take_power);
    const int thrown_box=call->effect==0x36&&dependency==0x227&&half(p+0x640)==9&&
        enemy->entry.b.work&&enemy->entry.b.work->room==0x101;
    const gh_title_player_view *recoil_source=NULL;
    const int recoil=call->effect==0x2e&&word(body->bytes+0x40)==UINT32_MAX&&
        !gh_hitarea_object_last_hit_actor(body,&recoil_source)&&
        r103_heavy_recoil_collateral(enemy,recoil_source,word(body->bytes+0x4c));
    const gh_title_player_view *catapult_source=NULL;
    const int catapult=call->effect==0x52&&reaction==0x0801&&!p[0x2f6]&&p[0x2f7]==2&&
        !gh_hitarea_object_last_hit_actor(body,&catapult_source)&&
        r200_catapult_contact(enemy,body,catapult_source);
    if (manager_active) {
        const gh_title_player_view *source = NULL;
        if ((call->effect != 0x2e && !thrown_box && !grenade && !shockwave && !catapult) ||
            gh_hitarea_object_last_hit_actor(body,&source) || !source || !source->bytes || source->size < 0x300)
            return stop(frontier, 0x271810);
        const uint32_t power = word(body->bytes + 0x4c);
        const uint32_t vtable = scenery_hit_vtable(half(source->bytes + 0x2fe));
        const int scenery = vtable && source->original_vtable214 == vtable &&
            word(source->bytes + 0x214) == vtable && !power && word(body->bytes + 0x40) == UINT32_MAX;
        /* 271CA8 branches on the enemy manager, including when a scene prop
         * supplied the already validated Take. */
        const int box=thrown_box&&half(source->bytes+0x2fe)==0x30a&&
            source->original_vtable214==0x4238d8&&word(source->bytes+0x214)==0x4238d8;
        const int arena_counter = word(body->bytes + 0x40) == UINT32_MAX &&
            r103_counter_collateral(enemy,source,power);
        if (!scenery && !box && !grenade && !shockwave && !catapult && !r100_counter_collateral(enemy,p,source,power) && !arena_counter && !recoil &&
            !(word(body->bytes + 0x40) == UINT32_MAX && r100_devil_contact(enemy,source,power)))
            return stop(frontier, 0x271810);
        /* The positive manager arm always creates CORE0:C. Only active
         * Heat or original 12C748 (player+5FC != 0) adds attached CORE121. */
        if (init->heat[0x10] || word(player->bytes+0x5fc))
            return stop(frontier, 0x271df4);
    }
    const int amplified = (half(p + 0x3ac) & 0x100u) != 0;
    const int amplified_triangle = call->effect == 0x12 &&
        (r200_em02_triangle_recoil(enemy,p)||r100_amplified_triangle_receiver(enemy,p) || r104_amplified_triangle_fall(enemy,p) ||
         r103_first_wave_em00(enemy) ||
         r103_arena_em0a(enemy,p) || r103_boss_player_strike_target(enemy,p));
    const int boss_square=take_effect==7&&r103_boss_player_strike_target(enemy,p);
    const int r104_punch=take_effect==7&&!call->effect&&take_power==3&&
        gh_enemy203_is_r104_entry(&enemy->entry.request,enemy->entry.b.work->room)&&
        (((reaction==0x0101||reaction==0x0001)&&!p[0x2f6]&&!p[0x2f7])||
         (reaction==0x2100&&r104_player_punch(enemy,p,take_effect)));
    if (amplified && (manager_active ||
        (!amplified_triangle && !boss_square && !r104_punch && !back_circle && !r101_key_carrier_punch(enemy,p,take_effect) &&
         !(enemy->entry.b.work->room==0x101&&original_em27_punch(enemy,p,take_effect)) &&
         !r101_side_arena_punch(enemy,p,take_effect) && !r102_close_em00_punch(enemy, p, half(body->bytes + 0x46)) &&
         !r102_stamp2_punch(enemy, p, half(body->bytes + 0x46)))))
        return stop(frontier, 0x271cf8);
    if (amplified && ((word((const uint8_t *)enemy->entry.b.work + 0x590) & 0x800u) ||
                      (word(p + 0x16d4) & 0x8000u)))
        return stop(frontier, 0x271d7c);
    if ((!thrown_box && !grenade && !shockwave && !catapult && !exhausted_impact && !r104_punch && (call->effect == 0x2e ? !recoil && !scenery_reaction_enemy(enemy,p) :
            dependency != 0x20a && !r100_stamp19(enemy, p) && !r100_stamp20(enemy, p) &&
            !triangle_take_supported(enemy,p,call->effect) && !back_circle &&
            !room_unarmed_punch(enemy, p, half(body->bytes + 0x46)) &&
            !r102_em03_punch(enemy, p, half(body->bytes + 0x46)) &&
            !original_em27_punch(enemy, p, half(body->bytes + 0x46)) &&
            !r102_at0b_em02_punch(enemy,p,take_effect))) ||
        init->heat[0x10] || (boost_timer&&!boosted_impact) ||
        (!manager_active && !boosted_impact && (!save.bytes || save.size < 0xaf)))
        return stop(frontier, 0x271810);
    const unsigned costume = manager_active||boosted_impact ? 0 : gh_core_save_costume(save);
    if (costume == 1 || costume == 3 || costume == 5 || costume == 7)
        return stop(frontier, 0x271e58);
    gh_actor_motion_status position_status=gh_enemy202_impact_position_00271518(enemy,call->impact.position,frontier);
    if(position_status)return position_status;
    call->impact.bank=manager_active||boosted_impact ? 0 : 0x58;
    call->impact.index=manager_active ? 0xc : boosted_impact ? 0x40 : amplified ? 0x12 : 0x10;
    call->impact.scale=1;
    call->impact.mode=-1;
    /* 292F08 tests the original actor byte before the unbound effect. */
    if (amplified && p[0x538]) call->impact.index = 0;
    call->phase = 10;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_impact_position_00271518(gh_enemy_actor *enemy,
    float result[4], uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0;
    if(!enemy||!result||!enemy->entry.b.init)return GH_ACTOR_MOTION_ARGUMENT;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_hitarea_object *body=&enemy->hitareas.units[0];
    gh_game_init_state *init=enemy->entry.b.init;
    const uint8_t *p=actor->bytes;
    const uintptr_t base = (uintptr_t)p, origin = (uintptr_t)actor->alias_f0;
    if (!p||actor->size<16||!body->bytes||body->size<0x2c||origin < base || origin - base > actor->size - 16)
        return stop(frontier, 0x271518);

    const float x = number(actor->alias_f0), z = number(actor->alias_f0 + 8);
    const float hit_x = number(body->bytes + 0x20), hit_y = number(body->bytes + 0x24);
    const float hit_z = number(body->bytes + 0x28);
    if (!isfinite(x) || !isfinite(z) || !isfinite(hit_x) || !isfinite(hit_y) || !isfinite(hit_z))
        return stop(frontier, 0x271518);
    fenv_t saved;
    if (feholdexcept(&saved)) return stop(frontier, 0x271518);
    if (fesetround(FE_TOWARDZERO)) {
        (void)fesetenv(&saved);
        return stop(frontier, 0x271518);
    }
    volatile float dx = hit_x - x, dz = hit_z - z;
    float angle, angles[3] = {0}, rotation[16];
    if (gh_game_angle_001f8180(dx, dz, &angle, GH_CAMERA_NATIVE_BINARY32_REFERENCE, frontier)) {
        (void)fesetenv(&saved);
        return GH_ACTOR_MOTION_UNSUPPORTED;
    }
    angles[1] = angle;
    if (gh_actor_matrix_rotation_00147728(angles, 0, rotation, GH_STATIC_BINARY32_REFERENCE, frontier)) {
        (void)fesetenv(&saved);
        return GH_ACTOR_MOTION_UNSUPPORTED;
    }
    uint32_t noise[3];
    for (unsigned i = 0; i < 3; ++i) {
        if (gh_game_mt_next_0030f348(&init->random, &noise[i])) {
            (void)fesetenv(&saved);
            return stop(frontier, 0x31d020);
        }
        noise[i] = gh_frame_jitter_bits_0031d020_word(noise[i]);
    }
    volatile float local_x = number((const uint8_t *)noise) * 0.2f;
    volatile float local_y = number((const uint8_t *)(noise + 1)) * 0.2f;
    volatile float jitter_z = number((const uint8_t *)(noise + 2)) * 0.1f;
    volatile float local_z = 0.4f + jitter_z;
    float world[3];
    for (unsigned i = 0; i < 3; ++i) {
        volatile float a = rotation[i] * local_x;
        volatile float b = rotation[4+i] * local_y;
        volatile float c = rotation[8+i] * local_z;
        volatile float ab = a + b;
        world[i] = ab + c;
    }
    volatile float out_x = x + world[0], out_z = z + world[2];
    if (fesetenv(&saved) || !isfinite(out_x) || !isfinite(out_z))
        return stop(frontier, 0x30a2b0);
    result[0]=out_x;
    result[1]=hit_y;
    result[2]=out_z;
    result[3]=1;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_devil_take_post_00208be0(gh_enemy202_hit_call *call,
    const gh_enemy_entry *entry,const gh_enemy_hitareas *hitareas,const gh_title_player_view *player,
    const uint8_t *thresholds,uint32_t *frontier)
{
    uint8_t *p=entry->object.view.bytes;
    if(call->phase==5){
        if(half(p+0x3ac)&0x100u)return stop(frontier,0x208ec0);
        p[0x17b0]=15;
        call->phase=7;
    }
    if(call->phase==7){
        gh_actor_motion_status status=hit_rank_points_001fa280(&entry->b,thresholds,16,frontier);
        if(status)return status;
        call->phase=8;
    }
    if(call->phase==8){
        if(call->effect==0x12){
            const float timer=number(p+0x17cc);
            if(!isfinite(timer))return stop(frontier,0x208f34);
            gh_actor_motion_status status=hit_stun_0028ed28(entry->b.init,p,timer,1,0,frontier);
            if(status)return status;
        }
        call->phase=9;
    }
    if(call->phase==9)return take_impact_00271810(call,entry,&hitareas->units[0],NULL,player,frontier);
    return GH_ACTOR_MOTION_OK;
}
