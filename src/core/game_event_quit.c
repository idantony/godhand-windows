#include "game_event_quit.h"

static uint32_t word(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void put_word(uint8_t *p, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (i * 8));
}

typedef struct {
    uint32_t site, target;
} quit_call;

static const quit_call calls[] = {
    {0x296f3c, 0x4d50}, {0x296f48, 0x1588}, {0x296f50, 0x68a0},
    {0x296f5c, 0xa370}, {0x296f68, 0x5478}, {0x296f74, 0x9728},
    {0x296f80, 0x88a8}, {0x296f8c, 0x75a0}, {0x296f98, 0x4a10},
    {0x296fa4, 0x9cb8}, {0x296fb0, 0x8af0}, {0x296fbc, 0x81b0},
    {0x296fc8, 0xa708}, {0x296fd4, 0xa778}, {0x296fe0, 0x7038},
    {0x296fec, 0x4528}, {0x296ff4, 0x6b40}
};

static gh_game_event_status visit(const gh_event_quit_bindings *b,
    const quit_call *call, const uint8_t *env, uint32_t *frontier, int *relative) {
    switch (call->target) {
    case 0x68a0: /* EventEnv */
    case 0xa370: /* EventVertex */
    case 0x88a8: /* EventObjLight */
    case 0xa708: /* EventVib */
    case 0x7038: /* EventFade */
    case 0x4528: /* EventAdditionalLight */
        /* Each original body is JR RA / NOP. */
        return GH_EVENT_OK;
    case 0x4a10: { /* EventBgm with an empty, already bound track. */
        gh_event_component_view bgm;
        gh_game_event_status status = gh_event_component_at(b->components, 9, &bgm);
        if (status || !bgm.bytes || bgm.original_offset != 0xd1b0 || bgm.size != 0x18) {
            *frontier = call->target;
            *relative = 1;
            return status ? status : GH_EVENT_BOUNDS;
        }
        if (!word(bgm.bytes + 0x14)) return GH_EVENT_OK;
        break;
    }
    case 0xa778: /* EventWait calls 30D168(2). */
        put_word((uint8_t *)b->work + 0x570, 2);
        return GH_EVENT_OK;
    case 0x6b40: /* ExtraPlayQuit uses Env+38; ExtraPlayMoveObj is +34. */
        if (!word(env + 0x38)) return GH_EVENT_OK;
        break;
    }

    *frontier = call->target;
    *relative = 1;
    if (!b->visit) return GH_EVENT_UNSUPPORTED;
    uint32_t child = 0;
    int ignored_end = 0, child_relative = 0;
    gh_game_event_status status =
        b->visit(b->context, call->site, &ignored_end, &child, &child_relative);
    if (status && child) {
        *frontier = child;
        *relative = child_relative != 0;
    }
    return status;
}

gh_game_event_status gh_event_quit_00296ef0(gh_event_quit *s,
    const gh_event_quit_bindings *b, uint32_t *frontier, int *relative) {
    if (!frontier || !relative) return GH_EVENT_ARGUMENT;
    *frontier = 0x296ef0;
    *relative = 0;
    if (!s || !b || !b->event || !b->event->initialized) return GH_EVENT_ARGUMENT;
    if (s->started && (s->event != b->event || s->components != b->components ||
        s->work != b->work || s->event_number != b->event->fields.data_number))
        return GH_EVENT_ARGUMENT;
    if (s->completed || (!s->started && !(b->event->fields.flags & 4u))) {
        *frontier = 0;
        return GH_EVENT_OK;
    }
    if (!b->work || !b->components || !b->components->constructed ||
        b->components->event_no != b->event->fields.data_number ||
        !(b->event->fields.flags & 4u) || s->next >= sizeof(calls) / sizeof(calls[0]))
        return GH_EVENT_ARGUMENT;

    gh_event_component_view env;
    gh_game_event_status status = gh_event_component_at(b->components, 0, &env);
    if (status) return status;
    if (!env.bytes || env.original_offset != 0xd200 || env.size != 0x44)
        return GH_EVENT_BOUNDS;
    if (!s->started) {
        if (word(env.bytes + 0x28) & 0x20u) b->event->fields.skip_ok = 1;
        s->event = b->event;
        s->components = b->components;
        s->work = b->work;
        s->event_number = b->event->fields.data_number;
        s->started = 1;
    }
    for (; s->next < sizeof(calls) / sizeof(calls[0]); ++s->next) {
        status = visit(b, &calls[s->next], env.bytes, frontier, relative);
        if (status) return status;
    }
    b->event->fields.flags &= ~4u;
    uint8_t *work = (uint8_t *)b->work;
    put_word(work + 0x5e4, word(work + 0x5e4) & ~1u);
    s->completed = 1;
    *frontier = 0;
    *relative = 0;
    return GH_EVENT_OK;
}
