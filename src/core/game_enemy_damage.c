#include "game_enemy_damage.h"
#include "game_enemy_row2.h"
#include "game_camera.h"
#include "frame_state.h"
#include "game_hitarea_frame.h"
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
        half(actor + 0x640) == 5 && word(actor + 0x564) == 0x203 &&
        actor[0x1650] == 2 && word(actor + 0x16c4) == 6;
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

static int room_unarmed_punch(const gh_enemy_actor *enemy, const uint8_t *actor, uint16_t effect)
{
    const uint32_t dependency=word(actor+0x564);
    return effect==7&&enemy->entry.b.work&&
        ((dependency==0x200&&enemy->entry.b.work->room==0x102)||r103_arena_punch_target(enemy,dependency));
}

static int r102_close_em00_punch(const gh_enemy_actor *enemy, const uint8_t *actor, uint16_t effect) {
    if(!room_unarmed_punch(enemy,actor,effect)||enemy->entry.b.work->room!=0x102)return 0;
    const unsigned stamp=half(actor+0x640);
    return (stamp==0x10&&!actor[0x1650])||(stamp==0x0b&&actor[0x1650]==5);
}

static int r102_stamp2(const gh_enemy_actor *enemy, const uint8_t *actor) {
    return enemy->entry.object.view.size > 0x1650 && word(actor + 0x564) == 0x20a && half(actor + 0x640) == 2 &&
        actor[0x1650] == 5 && enemy->entry.b.work && enemy->entry.b.work->room == 0x102;
}

static int r102_stamp2_punch(const gh_enemy_actor *enemy, const uint8_t *actor, uint16_t effect) {
    return effect == 7 && r102_stamp2(enemy, actor);
}

static int triangle_take_supported(const gh_enemy_actor *enemy, const uint8_t *actor, uint16_t effect) {
    return effect==0x12&&(r100_stamp19(enemy,actor)||r100_stamp20(enemy,actor)||
        r100_stamp8(enemy,actor)||r100_at2_em03(enemy,actor)||
        r103_arena_punch_target(enemy,word(actor+0x564))||r103_arena_em0a(enemy,actor));
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
         !r103_arena_em0a(enemy,view->bytes)) ||
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
            !triangle_take_supported(enemy, view->bytes, half(body->bytes + 0x46))))
        return stop(frontier, 0x20a5dc);

    uint8_t *actor = view->bytes;
    const uint32_t kind = word(actor + 0x564);
    if (kind == 0x21f || kind == 0x23b) return stop(frontier, 0x28fd38);
    fenv_t saved;
    if (feholdexcept(&saved)) return stop(frontier, 0x28fcf8);
    if (fesetround(FE_TOWARDZERO)) {
        (void)fesetenv(&saved);
        return stop(frontier, 0x28fcf8);
    }
    volatile float multiplier = word(player->bytes + 0x5fc) ? 1.5f : 1.0f;
    if (body->state->heat[0x10]) {
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

static gh_actor_motion_status gain_heat_002a9978(gh_enemy_actor *enemy, float amount, uint32_t *frontier) {
    const gh_enemy_entry_bindings *b = &enemy->entry.b;
    gh_core_save_view save = b->save;
    uint8_t *heat = b->init->heat;
    if (!save.bytes || save.size < 0x20 || !b->work)
        return stop(frontier, 0x2a9978);
    if (heat[0x10]) return GH_ACTOR_MOTION_OK;

    const float maximum = number(heat), current = number(heat + 4), peak = number(heat + 8);
    if (!isfinite(maximum) || !isfinite(current) || !isfinite(peak))
        return stop(frontier, 0x2a9978);
    fenv_t saved;
    if (feholdexcept(&saved)) return stop(frontier, 0x2a9978);
    if (fesetround(FE_TOWARDZERO)) {
        (void)fesetenv(&saved);
        return stop(frontier, 0x2a9978);
    }
    const int easy = !(word(save.bytes + 0x14) & 0x4000000u) && !save.bytes[0x1f];
    if (easy) {
        volatile float increased = amount * 1.25f;
        amount = increased;
    }
    if ((uint16_t)b->work->room == 0x801) {
        volatile float increased = amount * 1.25f;
        amount = increased;
    }
    volatile float sum = current + amount;
    const float next = maximum < sum ? maximum : sum;
    volatile float delta = next - current;
    volatile float next_peak_sum = peak + delta;
    const float next_peak = maximum < next_peak_sum ? maximum : next_peak_sum;
    volatile float ratio = maximum != 0 ? next / maximum : 0;
    if (fesetenv(&saved) || !isfinite(ratio)) return stop(frontier, 0x2a9c98);
    memcpy(heat + 4, &next, 4);
    memcpy(heat + 8, &next_peak, 4);
    put(heat + 0x14, ratio < 0.5f ? 0u : ratio <= 0.75f ? 1u : 2u);
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_counter_heat_002a9978(gh_enemy_actor *enemy, uint32_t *frontier) {
    if (!enemy || !enemy->entry.b.init || !enemy->entry.object.view.bytes ||
        enemy->entry.object.view.size < 0x642)
        return stop(frontier, 0x2a9978);
    const gh_title_player_view *view = &enemy->entry.object.view;
    const uint8_t *p = view->bytes;
    const unsigned stamp = word(p + 0x640) & 0xffffu;
    const gh_boot_work *work = enemy->entry.b.work;
    const int story = work && work->room == 0x102 && stamp == 11 && view->size > 0x1650 &&
        view->original_vtable214 == 0x4479c8 && p[0x1650] == 5 && p[0x2f4] == 1 &&
        p[0x2f5] == 0x1d && p[0x2f6] == 1 && p[0x2f7] == 1;
    if (word(p + 0x564) != 0x200 || (stamp != 20 && !story && !r103_first_wave_em00(enemy)))
        return stop(frontier, 0x2a9978);
    return gain_heat_002a9978(enemy, 10.0f, frontier);
}
static gh_actor_motion_status first_punch_wall(gh_enemy_actor *enemy, uint32_t *frontier) {
    if (!enemy->b.actors) return stop(frontier, 0x131e10);
    const uint8_t *p = enemy->entry.object.view.bytes;
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
    if (gh_scene_ray_query_00131e10(enemy->b.actors, &ray, &hit, frontier))
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
    return dependency == 0x200 || dependency == 0x202 || dependency == 0x203 ||
           r101_scenery_reaction_enemy(enemy, actor);
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

static gh_actor_motion_status environment_pressure_sound(gh_enemy202_hit_call *call,
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

/* Original scenery Give paths set the environment flag in 20A5DC's VT+AC call. */
static gh_actor_motion_status environment_hit(gh_enemy202_hit_call *call,
    gh_enemy_actor *enemy, const gh_title_player_view *source, unsigned rank,
    uint32_t *frontier)
{
    gh_title_player_view *actor = &enemy->entry.object.view;
    uint8_t *p = actor->bytes;
    const gh_hitarea_object *body = &enemy->hitareas.units[0];
    if (!source || !source->bytes || source->size < 0x300)
        return stop(frontier, 0x20a178);
    enum { PRESSURE_CORE_SOUND = 40, PRESSURE_LOCAL_SOUND };
    const unsigned source_kind = half(source->bytes + 0x2fe);
    const uint32_t power = word(body->bytes + 0x4c);
    const uint16_t effect = half(body->bytes + 0x46);
    const int barrel = effect == 0x52 && source_kind == 0x310 && source->size >= 0x920 &&
        r102_stamp2(enemy, p) && power == 80 && !half(body->bytes + 0x44) &&
        !word(source->bytes + 0x2f4);
    const int arena_counter = r103_counter_collateral(enemy,source,power);
    const uint32_t source_vtable = barrel ? 0x423e48u :
        source_kind == 0x202 || arena_counter ? 0x4479c8u : scenery_hit_vtable(source_kind);
    if ((effect != 0x2e && !barrel) || !source_vtable || source->original_vtable214 != source_vtable ||
        word(source->bytes + 0x214) != source_vtable ||
        actor->original_vtable214 != 0x4479c8 || word(p + 0x214) != 0x4479c8 ||
        (half(p + 0x2fe) != 0x202 && !arena_counter) ||
        (!barrel && !scenery_reaction_enemy(enemy,p)) ||
        word(body->bytes + 0x40) != UINT32_MAX)
        return stop(frontier, 0x20a178);
    if (source_kind == 0x202) {
        if (!r100_counter_collateral(enemy,p,source,power))
            return stop(frontier, 0x20a178);
    } else if (power && !arena_counter && !barrel) {
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
        /* Both non-player sources use 28FB88's unscaled environment hit. */
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
                const int em03 = word(p + 0x564) == 0x203 && half(p + 0x640) == 13 &&
                    !p[0x1650] && enemy->entry.b.work && enemy->entry.b.work->room == 0x101;
                if (!em03 && !r101_scenery_reaction_enemy(enemy, p))
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
        gh_actor_motion_status status = environment_pressure_sound(call, enemy, 0, 0xf7, frontier);
        if (status) return status;
        call->phase = PRESSURE_LOCAL_SOUND;
    }
    if (call->phase == PRESSURE_LOCAL_SOUND) {
        const uint16_t program = (uint16_t)(word(p + 0x424) + 0x14u); /* 1595F0 */
        gh_actor_motion_status status = environment_pressure_sound(call, enemy, 1, program, frontier);
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
        if(power&&dead&&!barrel){
            p[0x2f4]=2;
            p[0x2f5]=(flags&8u)?1:0;
            p[0x2f6]=p[0x2f7]=0;
            call->reacted=1;
            call->phase=5;
            return GH_ACTOR_MOTION_OK;
        }

        /* For original 2E/52, only 20CAC8 draws once for selector 1.
         * The remaining counter tables reject these effects before RNG. */
        if ((!barrel || !dead) && p[0x17bc] == 1) {
            uint32_t random;
            if (gh_game_mt_next_0030f348(&body->state->random, &random))
                return stop(frontier, 0x31cfe0);
        }
        p[0x2f4] = 1;
        /* Live kind52 selects 20C74C, the variant2 knockdown. */
        p[0x2f5] = barrel ? 8 : 0x11;
        p[0x2f6] = 0;
        p[0x2f7] = barrel ? 2 : 0;
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
    if (effect == 0x2e || effect == 0x52)
        return environment_hit(call, enemy, source, rank, frontier);
    if(effect==0x36)return r101_box_hit(call,enemy,source,rank,frontier);
    const int triangle = triangle_take_supported(enemy, p, effect);
    if (source != player || half(player->bytes + 0x2fe) != 0x100 || (effect != 7 && !triangle))
        return stop(frontier, 0x20a178);

    /* The R100 EM03 receivers have the same original 0203 reaction probes.
     * The west formation enters through the actual Triangle effect. */
    const uint32_t dependency = word(p + 0x564);
    const int opening_203 = r100_stamp19(enemy,p)||(triangle&&
        (r100_at2_em03(enemy,p)||(dependency==0x203&&r103_arena_punch_target(enemy,dependency))));
    const int opening_200 = r100_stamp20(enemy,p)||
        (triangle&&(r100_stamp8(enemy,p)||r103_first_wave_em00(enemy)));
    const int opening_pair = opening_203 || opening_200;
    const int ordinary_punch = room_unarmed_punch(enemy, p, effect);
    const int hit_probe_selector = opening_pair || ordinary_punch;
    const int counter_selector = opening_200 || (ordinary_punch && dependency==0x200);
    const int continuing_knockdown = triangle && dependency == 0x203 &&
        r103_arena_punch_target(enemy, dependency) && !half(p + 0x54a) &&
        half(p + 0x2f4) == 0x0701 && (half(p + 0x3ac) & 0x2cu) == 4 &&
        !(word(p + 0x16d4) & 0x100040u);

    if (call->phase == 0) {
        const float drain = number(p + 0x770), cooldown = number(p + 0x54c);
        const float block = number(p + 0x1740), armor = number(p + 0x1734);
        const uint16_t hp = half(p + 0x54a);
        uint16_t flags = half(p + 0x3ac);
        if ((!hit_probe_selector && dependency != 0x20a) || (!hp && !continuing_knockdown) || hp >= 0x8000 ||
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
        if (triangle) call->effect = effect;
        call->phase = 1;
    }
    if (call->phase == 1) {
        if (half(p + 0x54a)) {
            gh_actor_motion_status status = gain_heat_002a9978(enemy, triangle ? 2.0f : 0.8f, frontier);
            if (status) return status;
        }
        call->phase = 2;
    }
    enum { amplified_power_ready = 16, stamp2_counter_cue = 18, stamp2_counter_reset = 19 };
    if (call->phase == 2 && (half(p + 0x3ac) & 0x100u)) {
        const int amplified_triangle = triangle && (r100_stamp20(enemy, p) || r100_stamp8(enemy, p) ||
            r103_first_wave_em00(enemy));
        const int r102_em00 = r102_close_em00_punch(enemy, p, effect);
        const int verified_motion = ((amplified_triangle || r102_em00 ||
            (r102_stamp2_punch(enemy, p, effect) && p[0x2f6] == 1)) && p[0x2f5] == 0x15) ||
            (r102_em00 && p[0x2f5] == 0x4b && p[0x2f6] == 1);
        if (!verified_motion || p[0x2f4] || p[0x2f7] || !rank)
            return stop(frontier, 0x20a4e8);
        /* 20A4CC adds a separate Heat gain before switching on the hit kind. */
        gh_actor_motion_status status = gain_heat_002a9978(enemy, rank == 5 ? 1.0f : 4.0f, frontier);
        if (status) return status;
        call->phase = amplified_power_ready;
    }
    if (call->phase == 2 || call->phase == amplified_power_ready) {
        put(p + 0x17ac, half(p + 0x54a));
        uint32_t power_bits = word(body->bytes + 0x4c);
        if (call->phase == amplified_power_ready) {
            /* Effects 07 and 12 are below the 20A550 table range and take its default. */
            power_bits <<= 1;
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
        put(p + 0x54c, 0x40400000);
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
    if (call->phase == 15) return stop(frontier,0x274238);
    if (call->phase == 3) {
        const uint16_t flags = half(p + 0x3ac);
        const uint32_t behavior = word(p + 0x16d0), status = word(p + 0x16d4);
        /* 20DA18 returns zero for previous HP <= 0. The airborne
         * effect-12 branch then returns 1 without resetting 01/07. */
        if (continuing_knockdown) {
            call->reacted = 1;
            call->phase = 5;
            return GH_ACTOR_MOTION_OK;
        }
        if (!triangle && p[0x17bd] == 1 && rank >= 3) return stop(frontier, 0x26ecd0);
        if (r102_stamp2_punch(enemy, p, effect) &&
            !p[0x2f4] && p[0x2f5] == 0x4b && p[0x2f6] == 1 && !p[0x2f7] &&
            (behavior & 0x882000u) == 0x800000u && half(p + 0x54a) &&
            !body->state->heat[0x10]) {
            /* 20AF20 goes directly to 20B390 with no block timer flag.
             * Live HP and Heat inactive reach 26A338 without resetting Move. */
            put(p + 0x16fc, p[0x1700] & 1 ? 0x3e860a92u : 0xbe860a92u);
            p[0x1700]++;
            call->reacted = 1;
            call->phase = 5;
            return GH_ACTOR_MOTION_OK;
        }
        /* 20BB58 dispatches effect 7 to the original 20BB80 dead-HP arm. */
        const int r102_em00 = r102_close_em00_punch(enemy, p, effect);
        if ((opening_pair || r102_em00) && !half(p + 0x54a) && rank &&
            (triangle||rank < 3) && p[0x17bd] == 1 &&
            !(flags & 0x2cu) && !(behavior & 0x806020u) &&
            !(status & 0x180040u) && !(p[0x2f4] == 1 && p[0x2f5] == 0x16)) {
            p[0x2f4] = triangle ? 1 : 2;
            p[0x2f5] = triangle ? 7 : 0;
            p[0x2f6] = p[0x2f7] = 0;
            call->reacted = 1;
            call->phase = 5;
            return GH_ACTOR_MOTION_OK;
        }
        if (!rank || p[0x17bd] != 1 || !half(p + 0x54a) || (flags & 0x2cu) ||
            (behavior & (opening_pair ? 0x806020u : 0x802020u)) ||
            (status & 0x80040u) || (p[0x2f4] == 1 && p[0x2f5] == 0x16))
            return stop(frontier, 0x20a7e0);
        if (p[0x17bc] != (unsigned)hit_probe_selector || p[0x17ba] != (unsigned)hit_probe_selector ||
            p[0x17b9] != (unsigned)counter_selector || p[0x17bb] != 1)
            return stop(frontier, 0x20cac8);
        /* 20D138 returns 0. With selector 1, 20CAC8 draws once before
         * rejecting effect 7. 20D398 requires selector 3 for effect 7,
         * and 20CD00 rejects selector 0. Selector 1 returns at rank 1.
         * The rank2+ selector1 branch has an additional counter draw. */
        if (!triangle && counter_selector && rank >= 2)
            return stop(frontier, 0x20cd00);
        if (hit_probe_selector) {
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
        /* 20D218 returns before the counter RNG on the amplified effect-7 Take. */
        const int skip_counter = (flags & 0x100u) && (r102_close_em00_punch(enemy, p, effect) ||
            r102_stamp2_punch(enemy, p, effect));
        if (!skip_counter && !word(player->bytes + 0x5fc) && block <= 0 &&
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
                if(!triangle&&!r102_em00&&!stamp2&&!r103_arena_punch_target(enemy,dependency)){
                    call->phase = 6;
                    return stop(frontier, 0x20d2a0);
                }
                uint32_t retry;
                gh_game_mt random_state = body->state->random;
                if (gh_game_mt_next_0030f348(&random_state,&retry))
                    return stop(frontier,0x31cfe0);
                if(!(retry&3u)&&number(p+0x760)<=1.5707964f){
                    if(r102_em00||stamp2){
                        /* 274238's ordinary dependency arm and 20D358's counter reset
                         * commit only once after the original ordered RNG draws. */
                        float duration=0;
                        const int refresh_timer=!(word(p+0x16d0)&0x2000u);
                        if(refresh_timer){
                            static const float delay[]={0,40,30,20,20,15};
                            if(rank>5)return stop(frontier,0x274238);
                            duration=delay[rank];
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
                        if(stamp2){
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
    if(call->phase==stamp2_counter_cue){
        gh_actor_motion_status status=environment_pressure_sound(call,enemy,0,0x7c,frontier);
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
        gh_actor_motion_status status = first_punch_wall(enemy, frontier);
        if (status) return status;
        p[0x2f4] = 1;
        const int stunned = number(p + 0x17cc) > 0;
        /* 20BD94 sends amplified 020A effect 7 to 20BDE0. */
        const int stamp2_amplified = r102_stamp2_punch(enemy, p, effect) && (half(p + 0x3ac) & 0x100u);
        p[0x2f5] = triangle ? (stunned ? 7 : 1) : (stunned || stamp2_amplified ? 1 : 0);
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

gh_actor_motion_status gh_enemy202_hit_rank_points_001fa280(
    gh_enemy_actor *enemy, const uint8_t *thresholds, unsigned points, uint32_t *frontier) {
    if (!frontier) return GH_ACTOR_MOTION_ARGUMENT;
    *frontier = 0;
    if (!enemy || !thresholds || points > INT16_MAX || !enemy->entry.b.work || !enemy->entry.b.init)
        return GH_ACTOR_MOTION_ARGUMENT;
    const gh_enemy_entry_bindings *b = &enemy->entry.b;
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

gh_actor_motion_status gh_enemy202_hit_rank_001fa280(
    gh_enemy_actor *enemy, const uint8_t *thresholds, uint32_t *frontier)
{
    return gh_enemy202_hit_rank_points_001fa280(enemy, thresholds, 4, frontier);
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
    const int triangle = triangle_take_supported(enemy, p, effect);
    const int amplified = (half(p + 0x3ac) & 0x100u) != 0;
    if (!isfinite(timer) || (word(p + 0x564) != 0x20a && !r100_stamp19(enemy, p) &&
         !r100_stamp20(enemy, p) && !triangle && !room_unarmed_punch(enemy, p, effect)) ||
        (amplified && !(triangle && (r100_stamp20(enemy, p) || r100_stamp8(enemy, p) ||
        r103_first_wave_em00(enemy))) &&
         !r102_close_em00_punch(enemy, p, effect) && !r102_stamp2_punch(enemy, p, effect)))
        return stop(frontier, 0x208f34);
    unsigned increment = 10;
    if (triangle || effect == 0x52) {
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
    const int thrown_box=call->effect==0x36&&dependency==0x227&&half(p+0x640)==9&&
        enemy->entry.b.work&&enemy->entry.b.work->room==0x101;
    if (manager_active) {
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
        if (!scenery && !box && !r100_counter_collateral(enemy,p,source,power) && !arena_counter)
            return stop(frontier, 0x271810);
        /* The positive manager arm always creates CORE0:C. Only active
         * Heat or original 12C748 (player+5FC != 0) adds attached CORE121. */
        if (init->heat[0x10] || word(player->bytes+0x5fc))
            return stop(frontier, 0x271df4);
    }
    const int amplified = (half(p + 0x3ac) & 0x100u) != 0;
    const int amplified_triangle = call->effect == 0x12 &&
        (r100_stamp20(enemy, p) || r100_stamp8(enemy, p) || r103_first_wave_em00(enemy));
    if (amplified && (manager_active ||
        (!amplified_triangle && !r102_close_em00_punch(enemy, p, half(body->bytes + 0x46)) &&
         !r102_stamp2_punch(enemy, p, half(body->bytes + 0x46)))))
        return stop(frontier, 0x271cf8);
    if (amplified && ((word((const uint8_t *)enemy->entry.b.work + 0x590) & 0x800u) ||
                      (word(p + 0x16d4) & 0x8000u)))
        return stop(frontier, 0x271d7c);
    if ((!thrown_box && (call->effect == 0x2e ? !scenery_reaction_enemy(enemy,p) :
            dependency != 0x20a && !r100_stamp19(enemy, p) && !r100_stamp20(enemy, p) &&
            !triangle_take_supported(enemy,p,call->effect) &&
            !room_unarmed_punch(enemy, p, half(body->bytes + 0x46)))) ||
        init->heat[0x10] || word(player->bytes + 0x5fc) ||
        (!manager_active && (!save.bytes || save.size < 0xaf)))
        return stop(frontier, 0x271810);
    const unsigned costume = manager_active ? 0 : gh_core_save_costume(save);
    if (costume == 1 || costume == 3 || costume == 5 || costume == 7)
        return stop(frontier, 0x271e58);
    gh_actor_motion_status position_status=gh_enemy202_impact_position_00271518(enemy,call->impact.position,frontier);
    if(position_status)return position_status;
    call->impact.bank=manager_active ? 0 : 0x58;
    call->impact.index=manager_active ? 0xc : amplified ? 0x12 : 0x10;
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
