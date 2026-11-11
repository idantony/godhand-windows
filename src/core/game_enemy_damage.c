#include "game_enemy_damage.h"
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

static int r100_stamp20(const gh_enemy_actor *enemy, const uint8_t *actor) {
    return word(actor + 0x564) == 0x200 && half(actor + 0x640) == 20 &&
        enemy->entry.b.work && (uint16_t)enemy->entry.b.work->room == 0x100;
}

static int opening_triangle(const gh_enemy_actor *enemy, const uint8_t *actor, uint16_t effect) {
    return effect == 0x12 && (r100_stamp19(enemy, actor) || r100_stamp20(enemy, actor));
}

gh_actor_motion_status gh_enemy202_damage_0028fb88(
    gh_enemy_actor *enemy, const gh_title_player_view *player, int32_t power, uint32_t *frontier) {
    if (!frontier) return GH_ACTOR_MOTION_ARGUMENT;
    *frontier = 0;
    if (!enemy || !player || !player->bytes || player->size < 0x600)
        return GH_ACTOR_MOTION_ARGUMENT;
    gh_title_player_view *view = &enemy->entry.object.view;
    if (!view->bytes || view->size < 0x623 || view->original_vtable214 != 0x4479c8 ||
        word(view->bytes + 0x214) != view->original_vtable214 ||
        half(view->bytes + 0x2fe) != 0x202 || !enemy->hitareas.created[0])
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
            !opening_triangle(enemy, view->bytes, half(body->bytes + 0x46))))
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

static gh_actor_motion_status first_punch_heat(gh_enemy_actor *enemy, int triangle, uint32_t *frontier) {
    const gh_enemy_entry_bindings *b = &enemy->entry.b;
    gh_core_save_view save = b->save;
    uint8_t *heat = b->init->heat;
    if (!save.bytes || save.size < 0x20 || !b->work || (uint16_t)b->work->room != 0x100)
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
    const float amount = triangle ? (easy ? 2.5f : 2.0f) : (easy ? 1.0f : 0.8f);
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

static int scenery_reaction_dependency(uint32_t dependency) {
    return dependency == 0x200 || dependency == 0x202 || dependency == 0x203;
}

/* The OM304 Give sphere selects effect 2E with zero power and the
 * environment flag set in 20A5DC's VT+AC call. */
static gh_actor_motion_status scenery_hit(gh_enemy202_hit_call *call,
    gh_enemy_actor *enemy, const gh_title_player_view *source, unsigned rank,
    uint32_t *frontier)
{
    gh_title_player_view *actor = &enemy->entry.object.view;
    uint8_t *p = actor->bytes;
    const gh_hitarea_object *body = &enemy->hitareas.units[0];
    if (!source || !source->bytes || source->size < 0x300)
        return stop(frontier, 0x20a178);
    const unsigned source_kind = half(source->bytes + 0x2fe);
    const uint32_t source_vtable = source_kind == 0x303 ? 0x423230u :
        source_kind == 0x304 ? 0x423338u : 0;
    if (!source_vtable || source->original_vtable214 != source_vtable ||
        word(source->bytes + 0x214) != source_vtable ||
        actor->original_vtable214 != 0x4479c8 || word(p + 0x214) != 0x4479c8 ||
        half(p + 0x2fe) != 0x202 || !scenery_reaction_dependency(word(p + 0x564)) ||
        word(body->bytes + 0x4c) || word(body->bytes + 0x40) != UINT32_MAX)
        return stop(frontier, 0x20a178);

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
        call->effect = 0x2e;
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
        /* 28FB88 rounds zero power to zero for this non-player source. */
        put_half(p + 0x54a, (uint16_t)hp);
        put(p + 0x17b4, 0);
        put(p + 0x1940, 0xbd4ccccd);
        put(p + 0x54c, 0x41700000);
        put(p + 0x16dc, 0);
        call->phase = 1;
    }
    if (call->phase == 1) {
        if ((p[0x17bd] == 1 && rank >= 3) || (p[0x17bd] == 2 && rank >= 2)) {
            uint32_t points = word(p + 0x16e8);
            if ((int32_t)word(p + 0x16ec) < 1) points += 5;
            if ((int32_t)points > 200) return stop(frontier, 0x26ecd0);
            put(p + 0x16e8, points);
        }
        if (!rank && (p[0x17bd] == 1 || p[0x17bd] == 2))
            return stop(frontier, 0x1fa010);
        call->phase = 2;
    }
    if (call->phase == 2) {
        const uint16_t flags = half(p + 0x3ac);
        const uint32_t behavior = word(p + 0x16d0), status = word(p + 0x16d4);
        /* 20B3D4 branches away from the ordinary route only for the
         * outer1/selector16 reaction before phase4. Variant is not read. */
        const int early_reaction = p[0x2f4] == 1 && p[0x2f5] == 0x16 && p[0x2f6] < 4;
        if ((status & 0x80040u) || (behavior & 0x806020u) ||
            (flags & 0x2cu) || early_reaction)
            return stop(frontier, 0x20b394);

        /* 20D138 returns zero. For 2E, 20CAC8 can still draw once
         * on +17BC == 1 before rejecting. The other probes reject
         * 2E without drawing from the shared random owner. */
        if (p[0x17bc] == 1) {
            uint32_t random;
            if (gh_game_mt_next_0030f348(&body->state->random, &random))
                return stop(frontier, 0x31cfe0);
        }
        p[0x2f4] = 1;
        p[0x2f5] = 0x11;
        p[0x2f6] = p[0x2f7] = 0;
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

    if (half(body->bytes + 0x46) == 0x2e)
        return scenery_hit(call, enemy, source, rank, frontier);
    const uint16_t effect = half(body->bytes + 0x46);
    const int triangle = opening_triangle(enemy, p, effect);
    if (source != player || half(player->bytes + 0x2fe) != 0x100 || (effect != 7 && !triangle))
        return stop(frontier, 0x20a178);

    /* Physical R100 stamp19 uses the common effect-7 hit path, with the
     * original 0203 behavior bytes controlling its reaction probes. */
    const uint32_t dependency = word(p + 0x564);
    const int opening_203 = r100_stamp19(enemy, p);
    const int opening_200 = r100_stamp20(enemy, p);
    const int opening_pair = opening_203 || opening_200;

    if (call->phase == 0) {
        const float drain = number(p + 0x770), cooldown = number(p + 0x54c);
        const float block = number(p + 0x1740), armor = number(p + 0x1734);
        const uint16_t hp = half(p + 0x54a);
        uint16_t flags = half(p + 0x3ac);
        if ((!opening_pair && dependency != 0x20a) || !hp || hp >= 0x8000 ||
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
        gh_actor_motion_status status = first_punch_heat(enemy, triangle, frontier);
        if (status) return status;
        call->phase = 2;
    }
    if (call->phase == 2) {
        if (half(p + 0x3ac) & 0x100u) return stop(frontier, 0x20a4e8);
        put(p + 0x17ac, half(p + 0x54a));
        int32_t power;
        memcpy(&power, body->bytes + 0x4c, 4);
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
        if(triangle&&p[0x17bd]==1&&rank>=3){
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
        if (!triangle && p[0x17bd] == 1 && rank >= 3) return stop(frontier, 0x26ecd0);
        /* 20BB58 is the dead-HP effect switch. Ordinary stamp19 effect7
         * reaches it with no special reaction, block, or alternate motion. */
        if (opening_pair && !half(p + 0x54a) && rank && (triangle||rank < 3) && p[0x17bd] == 1 &&
            !(flags & 0x12cu) && !(behavior & 0x806020u) &&
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
        if (p[0x17bc] != (unsigned)opening_pair || p[0x17ba] != (unsigned)opening_pair ||
            p[0x17b9] != (unsigned)opening_200 || p[0x17bb] != 1)
            return stop(frontier, 0x20cac8);
        /* 20D138 returns 0. With selector 1, 20CAC8 draws once before
         * rejecting effect 7. 20D398 requires selector 3 for effect 7,
         * and 20CD00 rejects selector 0. Stamp20 selector1 returns at rank 1.
         * The rank2+ selector1 branch has an additional counter draw. */
        if (!triangle && opening_200 && rank >= 2)
            return stop(frontier, 0x20cd00);
        if (opening_pair) {
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
        if (!word(player->bytes + 0x5fc) && block <= 0 &&
            !body->state->heat[0x10] && !(number(p + 0x17cc) > 0)) {
            uint32_t random;
            if (gh_game_mt_next_0030f348(&body->state->random, &random))
                return stop(frontier, 0x31cfe0);
            unsigned threshold = (random & 3) + (rank == 5 ? 2u : rank >= 3 ? 3u : rank == 2 ? 4u : 5u);
            if (p[0x17c1]) threshold -= 2;
            p[0x17bf]++;
            if (p[0x17bf] > 100) p[0x17bf] = 100;
            if (p[0x17bf] >= threshold) {
                if(!triangle){
                    call->phase = 6;
                    return stop(frontier, 0x20d2a0);
                }
                uint32_t retry;
                if (gh_game_mt_next_0030f348(&body->state->random,&retry))
                    return stop(frontier,0x31cfe0);
                if(!(retry&3u)&&number(p+0x760)<=1.5707964f){
                    restore_counter_hp(p);
                    call->phase=15;
                    return stop(frontier,0x274238);
                }
            }
        }
        call->phase = 4;
    }
    if (call->phase == 6) return stop(frontier, 0x20d2a0);
    if (call->phase == 4) {
        gh_actor_motion_status status = first_punch_wall(enemy, frontier);
        if (status) return status;
        p[0x2f4] = 1;
        p[0x2f5] = triangle ? (number(p + 0x17cc) > 0 ? 7 : 1) :
            (number(p + 0x17cc) > 0 ? 1 : 0);
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
    const int triangle = body->bytes && body->size >= 0x48 &&
        opening_triangle(enemy, p, half(body->bytes + 0x46));
    if (!isfinite(timer) || (word(p + 0x564) != 0x20a && !r100_stamp19(enemy, p) && !r100_stamp20(enemy, p)) ||
        (half(p + 0x3ac) & 0x100u))
        return stop(frontier, 0x208f34);
    unsigned increment = 10;
    if (triangle) {
        uint32_t random;
        if (gh_game_mt_next_0030f348(&enemy->entry.b.init->random, &random))
            return stop(frontier, 0x31cfe0);
        increment = 15u << (random & 1u);
    }
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
    const uint32_t dependency = word(p + 0x564);
    if (actor->size < 0x1944 || !actor->alias_f0 ||
        (call->effect == 0x2e ? !scenery_reaction_dependency(dependency) :
            dependency != 0x20a && !r100_stamp19(enemy, p) && !r100_stamp20(enemy, p)) ||
        (half(p + 0x3ac) & 0x100u) || init->heat[0x10] || word(player->bytes + 0x5fc) ||
        (int32_t)word(init->enemies + 0x514) > 0 || !save.bytes || save.size < 0xaf)
        return stop(frontier, 0x271810);
    const unsigned costume = gh_core_save_costume(save);
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
        .bank = 0x58, .index = 0x10, .scale = 1.0f, .mode = -1,
    };
    call->phase = 10;
    return GH_ACTOR_MOTION_OK;
}
