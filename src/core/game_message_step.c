/* Actual patched 2AEFA8 and 2AF028 prefixes. MIT LICENSE.upstream. */
#include "game_message_step.h"

#include <math.h>
#include <string.h>

static uint16_t half(const uint8_t *p)
{
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

static uint32_t word(const uint8_t *p)
{
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static uint64_t doubleword(const uint8_t *p)
{
    return word(p) | (uint64_t)word(p + 4) << 32;
}

static int32_t signed_word(const uint8_t *p)
{
    int32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void put(uint8_t *p, uint32_t value)
{
    memcpy(p, &value, sizeof(value));
}

static void put_half(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put_float(uint8_t *p, float value)
{
    memcpy(p, &value, sizeof(value));
}

static gh_scene_ui_status active_slots(const gh_game_scene_ui *ui, unsigned slots[16],
    unsigned *count)
{
    if (!ui->ready || !ui->root) return GH_SCENE_UI_ARGUMENT;
    uint32_t seen = 0;
    *count = 0;
    for (int32_t i = ui->active_head; i != -1; i = ui->blocks[i].next) {
        if (i < 0 || i >= 16 || (seen & (1u << i))) return GH_SCENE_UI_BOUNDS;
        seen |= 1u << i;
        slots[(*count)++] = (unsigned)i;
    }
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status event_message_env(const gh_event_timing *timing, const uint8_t **bytes)
{
    if (!timing || !timing->components || !timing->allocation.pointer)
        return GH_SCENE_UI_UNSUPPORTED;
    const gh_event_components *components = timing->components;
    const gh_event_component_view env = components->components[0];
    if (!components->constructed || components->event_no != timing->event_number ||
        !env.bytes || env.original_offset != 0xd200 || env.size != 0x44 ||
        word(env.bytes + 8) != timing->count) return GH_SCENE_UI_ARGUMENT;
    *bytes = env.bytes;
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status clear_event_cut(gh_scene_message_sources *source,
    gh_event_timing *timing, uint32_t *frontier)
{
    *frontier = 0x297bf8;
    const uint8_t *env;
    gh_scene_ui_status status = event_message_env(timing, &env);
    if (status) return status;
    const uint32_t cut = word(env + 0x10);
    if (cut >= timing->count) return GH_SCENE_UI_BOUNDS;
    gh_event_cut_timing *rows = timing->allocation.pointer;
    rows[cut].kind = 0;
    source->confirm_pending = 0;
    return GH_SCENE_UI_OK;
}

static int native_confirm(const gh_boot_work *work)
{
    const uint8_t *raw = (const uint8_t *)work;
    /* 31A808 publishes the original language-selected confirm mask. */
    const uint64_t mask = work->language ? UINT64_C(0x10) : UINT64_C(0x100000000);
    return (doubleword(raw + 0x178) & mask) || (word(raw + 0x1a0) & 0x10000000u);
}

static int code_at(const gh_scene_message_sources *source, size_t offset, uint16_t *code)
{
    if (!source->primary.bytes || offset > source->primary.size ||
        source->primary.size - offset < 2) return 0;
    *code = half(source->primary.bytes + offset);
    return 1;
}

static void advance_message_budget(uint8_t *p, const gh_game_scene_ui *ui,
    const gh_boot_work *work)
{
    p[1] = 1; /* 2B45D0 and 2B45D8 are empty. */
    uint32_t flags = word(p + 0x14 - 0xc);
    if (word((const uint8_t *)work + 0x1a0) & 0x10000000u) flags |= 1u;
    put(p + 0x14 - 0xc, flags);
    uint32_t increment = (uint32_t)ui->root->timer;
    if (!word(p + 0x34 - 0xc) && ((flags & 1u) || native_confirm(work))) increment *= 10;
    put(p + 0x68 - 0xc, word(p + 0x68 - 0xc) + increment);
}

static gh_scene_ui_status advance_scenery(uint8_t *p,gh_scene_message_sources *source,
    const gh_game_scene_ui *ui,const gh_boot_work *work,uint32_t *frontier)
{
    *frontier=0x2b3338;
    if(!work->language||work->room!=0x105||half(p+0x64-0xc)!=0x3004||
       (source->cursor_7c!=4&&source->cursor_7c!=46)||
       source->cursor_8c<source->cursor_7c||source->cursor_8c>source->primary.size||
       word(p+0x1c-0xc)||word(p+0x6c-0xc)!=0x3f800000u||p[1]>1)
        return GH_SCENE_UI_UNSUPPORTED;
    advance_message_budget(p, ui, work);
    uint8_t *budget=p+0x68-0xc,*time=p+0x90-0xc;
    while(signed_word(budget)>=signed_word(time)){
        uint16_t code;
        if(!code_at(source,source->cursor_8c,&code))return GH_SCENE_UI_BOUNDS;
        if(code==0xa001){
            *frontier=0x2b3a38;
            /* 2B43B0 stops scanning at the timed glyph after A001. With
             * no voice/actor, 2B4428(0,0) takes its equality return. */
            uint16_t marker,selector,first;
            if(source->cursor_7c!=4||source->cursor_8c!=44||
               !code_at(source,46,&marker)||marker!=0xa804||
               !code_at(source,48,&selector)||selector||
               !code_at(source,54,&first)||first<14||first>=0x1000||
               word(p+0xb4-0xc)||word(p+0x40-0xc))
                return GH_SCENE_UI_UNSUPPORTED;
            source->cursor_8c+=2;
            source->cursor_7c=source->cursor_8c;
            put(p+0x84-0xc,word(time));
            put(budget,word(time));
            ++p[0x30-0xc];
            put(p+0x14-0xc,word(p+0x14-0xc)&~6u);
            continue;
        }
        if(code==0xa804){
            *frontier=0x2b3b40;
            uint16_t selector,hi,lo;
            const size_t offset=source->cursor_8c;
            if(source->cursor_7c!=46||
               !code_at(source,offset+2,&selector)||selector>1||
               !code_at(source,offset+4,&hi)||!code_at(source,offset+6,&lo)||
               hi!=UINT16_MAX||lo!=UINT16_MAX)return GH_SCENE_UI_UNSUPPORTED;
            if(half(p+0x96-0xc)<selector+1)
                put_half(p+0x96-0xc,(uint16_t)(selector+1));
            source->cursor_8c+=8;
            continue;
        }
        if(code==0xa301&&source->cursor_7c==46){
            *frontier=0x2b3ad0;
            uint16_t end;
            if(half(p+0x96-0xc)!=2||
               !code_at(source,source->cursor_8c+2,&end)||end!=0x8001)
                return GH_SCENE_UI_UNSUPPORTED;
            put(budget,word(time));
            p[0]=4;
            p[1]=p[2]=p[3]=0;
            return GH_SCENE_UI_OK;
        }
        if(code<14||code>=0x1000)return GH_SCENE_UI_UNSUPPORTED;
        source->cursor_8c+=2;
        put(time,word(time)+30);
    }
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status wait_scenery_choice(uint8_t *p,const gh_boot_work *work,
    uint32_t *frontier)
{
    *frontier=0x2b3ec8;
    if(work->room!=0x105||!work->language||half(p+0x64-0xc)!=0x3004||
       half(p+0x96-0xc)!=2||half(p+0x94-0xc)>1)
        return GH_SCENE_UI_UNSUPPORTED;
    const uint8_t *raw=(const uint8_t *)work;
    if(word(raw+0x5e4)&0x08000000u)return GH_SCENE_UI_OK;
    const uint64_t pressed=doubleword(raw+0x1a0);
    const unsigned current=half(p+0x94-0xc);
    int next=(int)current;
    next-=(pressed&UINT64_C(0x4000008000000))!=0;
    next+=(pressed&UINT64_C(0x8000004000000))!=0;
    if(next<0)next=0;
    else if(next>1)next=1;
    if((unsigned)next!=current){
        *frontier=0x2b46f0; /* Original option-move SE, before confirm. */
        return GH_SCENE_UI_UNSUPPORTED;
    }
    if(pressed&0x10000000u){
        *frontier=0x2b3fc8; /* Original selected result and confirm SE. */
        return GH_SCENE_UI_UNSUPPORTED;
    }
    put(p+0x14-0xc,word(p+0x14-0xc)|0x20u);
    return GH_SCENE_UI_OK;
}

static int ev1b_timed_caption(const gh_scene_message_sources *source,
    const gh_boot_work *work, const gh_event_timing *timing)
{
    uint16_t command;
    return work->room == 0x102 && timing && timing->event_number == 0x1b &&
        code_at(source, 0, &command) && command == 0xe705;
}

static gh_scene_ui_status advance_event_caption(uint8_t *p, gh_scene_message_sources *source,
    const gh_game_scene_ui *ui, const gh_boot_work *work, const gh_event_timing *timing,
    uint32_t *frontier)
{
    *frontier = 0x2b3338;
    if (source->cursor_7c != 10 || source->cursor_8c < source->cursor_7c)
        return GH_SCENE_UI_UNSUPPORTED;
    advance_message_budget(p, ui, work);
    uint8_t *budget = p + 0x68 - 0xc, *time = p + 0x90 - 0xc;
    while (signed_word(budget) >= signed_word(time)) {
        uint16_t code;
        if (!code_at(source, source->cursor_8c, &code)) return GH_SCENE_UI_BOUNDS;
        if (code == 0xb002) {
            uint16_t percent;
            if (!code_at(source, source->cursor_8c + 2, &percent)) return GH_SCENE_UI_BOUNDS;
            if (percent != 100 && percent != 150) return GH_SCENE_UI_UNSUPPORTED;
            /* 2B34D8 has no state action for B0. 2AF1F8/2AF218 give two
             * halfwords and zero time. The renderer consumes the operand. */
            source->cursor_8c += 4;
            continue;
        }
        if (code == 0xe801) {
            *frontier = 0x2b47c8;
            const uint8_t *env;
            gh_scene_ui_status status = event_message_env(timing, &env);
            if (status) return status;
            const uint32_t cut = word(env + 0x10);
            float frame;
            memcpy(&frame, env + 0x14, sizeof(frame));
            if (cut >= timing->count || !isfinite(frame) || frame < 0) return GH_SCENE_UI_BOUNDS;
            if (cut >= p[0x44 - 0xc] &&
                (cut < p[0x4c - 0xc] || frame <= (float)signed_word(p + 0x50 - 0xc))) {
                put(budget, word(time));
                return GH_SCENE_UI_OK;
            }
            source->cursor_8c += 2;
            continue;
        }
        if (code == 0x8001) {
            *frontier = 0x2b3b70;
            if (word(p + 0xb4 - 0xc) || word(p + 0x40 - 0xc)) return GH_SCENE_UI_UNSUPPORTED;
            p[0] = 6;
            p[1] = p[2] = p[3] = 0;
            put(budget, word(time));
            return GH_SCENE_UI_OK;
        }
        if (code < 14 || code >= 0x1000) return GH_SCENE_UI_UNSUPPORTED;
        source->cursor_8c += 2;
        put(time, word(time) + 30);
    }
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status advance_english(uint8_t *p, gh_scene_message_sources *source,
    const gh_game_scene_ui *ui, const gh_boot_work *work, const gh_event_timing *timing,
    uint32_t *frontier)
{
    if(p[0xa4-0xc]==4)return advance_scenery(p,source,ui,work,frontier);
    *frontier = 0x2b3240;
    const int r104_caption=work->room==0x104&&timing&&timing->event_number==5&&p[0xa4-0xc]==3&&half(p+0x64-0xc)==0x1000;
    if (!work->language || (p[0xa4 - 0xc]&&!r104_caption) || word(p + 0x1c - 0xc) ||
        word(p + 0x6c - 0xc) != 0x3f800000u || p[1] > 1)
        return GH_SCENE_UI_UNSUPPORTED;
    if (ev1b_timed_caption(source, work, timing))
        return advance_event_caption(p, source, ui, work, timing, frontier);
    *frontier = 0x2b3338;
    advance_message_budget(p, ui, work);
    uint8_t *budget = p + 0x68 - 0xc, *time = p + 0x90 - 0xc;
    while (signed_word(budget) >= signed_word(time)) {
        uint16_t code;
        if (!code_at(source, source->cursor_8c, &code)) return GH_SCENE_UI_BOUNDS;
        if (code >= 14 && code < 0x1000) {
            source->cursor_8c += 2;
            put(time, word(time) + 30);
            continue;
        }
        const int r200_key_notice=work->room==0x200&&!p[0xa4-0xc]&&
            half(p+0x64-0xc)==0x1000&&source->cursor_8c==38;
        if((r104_caption||r200_key_notice)&&code==0xa001){source->cursor_8c+=2;continue;}
        if (code == 0xa101) {
            /* 2B3A38 resets the page origin and timer, then 2B4768 advances
             * past A101 at zero cost. The next glyph can appear in this tick. */
            if (word(p + 0xb4 - 0xc) || word(p + 0x40 - 0xc)) {
                *frontier = 0x2b3a38;
                return GH_SCENE_UI_UNSUPPORTED;
            }
            source->cursor_8c += 2;
            source->cursor_7c = source->cursor_8c;
            put(budget, word(time));
            put(p + 0x84 - 0xc, word(time));
            ++p[0x30 - 0xc];
            put(p + 0x14 - 0xc, word(p + 0x14 - 0xc) & ~6u);
            continue;
        }
        if (code != 0xa301 && code != 0x8001) return GH_SCENE_UI_UNSUPPORTED;
        if (code == 0x8001 && (word(p + 0xb4 - 0xc) || word(p + 0x40 - 0xc))) {
            *frontier = 0x2b4428;
            return GH_SCENE_UI_UNSUPPORTED;
        }
        /* Both commands return 2 and clamp the budget before consuming the code. */
        p[0] = code == 0x8001 ? 6 : 3;
        p[1] = p[2] = p[3] = 0;
        put(budget, word(time));
        break;
    }
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status close_window(uint8_t *p, const gh_scene_message_sources *source,
    uint32_t *frontier)
{
    *frontier = 0x2b4188;
    if (p[1] > 1) return GH_SCENE_UI_UNSUPPORTED;
    uint8_t *flags = p + 0x14 - 0xc;
    uint32_t tick = p[1] ? word(p + 0x10 - 0xc) : 0;
    if (tick >= 6) return GH_SCENE_UI_BOUNDS;
    if (tick == 5 && !(word(flags) & 0x40u)) {
        uint16_t code;
        *frontier = 0x2b42f0;
        if (!code_at(source, source->cursor_8c, &code)) return GH_SCENE_UI_BOUNDS;
        if ((code & 0xff00u) != 0x8000) return GH_SCENE_UI_UNSUPPORTED;
    }
    if (!p[1]) {
        put(flags, word(flags) & ~0x10u);
        p[1] = 1;
    }
    ++tick;
    put(p + 0x10 - 0xc, tick);
    const unsigned left = 6 - tick;
    const volatile float fraction = (float)left / 6.0f;
    const volatile float square = (float)(left * left) / 36.0f;
    const float progress = (fraction + fraction) - square;
    put_float(p + 0x98 - 0xc, progress);
    put_float(p + 0x9c - 0xc, progress);
    put_float(p + 0xa0 - 0xc, square);
    if (tick == 6) {
        put(flags, word(flags) & ~8u);
        p[0] = 7;
        p[1] = p[2] = p[3] = 0;
    }
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status recycle_window(gh_game_scene_ui *ui, unsigned slot, uint32_t *frontier)
{
    *frontier = 0x2aef10;
    gh_scene_message_block *block = ui->blocks + slot;
    uint8_t *p = block->payload;
    const int32_t previous = block->previous, next = block->next, free_slot = ui->free_head;
    if (previous < -1 || previous >= 16 || next < -1 || next >= 16 ||
        free_slot < -1 || free_slot >= 16 || free_slot == (int32_t)slot ||
        (previous == -1 ? ui->active_head != (int32_t)slot : ui->blocks[previous].next != (int32_t)slot) ||
        (next != -1 && ui->blocks[next].previous != (int32_t)slot)) return GH_SCENE_UI_BOUNDS;
    const uint32_t flags = word(p + 0x14 - 0xc);
    if (flags & 0x1000u) {
        *frontier = 0x2b2348;
        if (word(p + 0x1c - 0xc) || word(p + 0xb4 - 0xc) || word(p + 0x40 - 0xc))
            return GH_SCENE_UI_UNSUPPORTED;
        put(p + 0x20 - 0xc, UINT32_MAX);
        ui->root->state = 0;
        put(p + 0x14 - 0xc, flags & ~0x1000u);
    }
    if (previous != -1) ui->blocks[previous].next = next;
    else ui->active_head = next;
    if (next != -1) ui->blocks[next].previous = previous;
    if (free_slot != -1) ui->blocks[free_slot].previous = (int32_t)slot;
    block->flags = block->mode = 0;
    block->previous = -1;
    block->next = free_slot;
    ui->free_head = (int32_t)slot;
    ui->sources[slot] = (gh_scene_message_sources){0};
    return GH_SCENE_UI_OK;
}

gh_scene_ui_status gh_scene_message_work_live_002aee10(const gh_game_scene_ui *ui,
    uint16_t work_id,int *live)
{
    if(!ui||!live)return GH_SCENE_UI_ARGUMENT;
    *live=0;
    unsigned slots[16],count;
    gh_scene_ui_status status=active_slots(ui,slots,&count);
    if(status)return status;
    for(unsigned i=0;i<count;++i)
        if(ui->blocks[slots[i]].flags==work_id){*live=1;break;}
    return GH_SCENE_UI_OK;
}

gh_scene_ui_status gh_scene_message_find_002aed40(const gh_game_scene_ui *ui,
    uint16_t message_id,int *slot,uint32_t *frontier)
{
    if(!ui||!slot||!frontier)return GH_SCENE_UI_ARGUMENT;
    *frontier=0x2aed40;
    unsigned slots[16], count;
    gh_scene_ui_status status=active_slots(ui,slots,&count);
    if(status)return status;
    *slot=-1;
    for(unsigned i=0;i<count;i++){
        if(half(ui->blocks[slots[i]].payload+0x64-0x0c)!=message_id)continue;
        *slot=(int)slots[i];
        break;
    }
    *frontier=0;
    return GH_SCENE_UI_OK;
}

gh_scene_ui_status gh_scene_message_delete_by_id_002aec40(gh_game_scene_ui *ui,
    uint16_t message_id,uint32_t *frontier)
{
    int slot;
    gh_scene_ui_status status=gh_scene_message_find_002aed40(ui,message_id,&slot,frontier);
    if(status||slot<0)return status;
    status=recycle_window(ui,(unsigned)slot,frontier);
    if(!status)*frontier=0;
    return status;
}

gh_scene_ui_status gh_scene_message_delete_002aebf8(gh_game_scene_ui *ui,
    uint16_t work_id, int *original_found, uint32_t *frontier)
{
    if (!frontier || !original_found) return GH_SCENE_UI_ARGUMENT;
    *frontier = 0x2aebf8;
    *original_found = 0;
    if (!ui) return GH_SCENE_UI_ARGUMENT;
    unsigned slots[16], count;
    *frontier = 0x2aed10;
    gh_scene_ui_status status = active_slots(ui, slots, &count);
    if (status) return status;
    for (unsigned i = 0; i < count; ++i) {
        const unsigned slot = slots[i];
        if (ui->blocks[slot].flags != work_id) continue;
        status = recycle_window(ui, slot, frontier);
        if (status) return status;
        *original_found = 1;
        break;
    }
    *frontier = 0;
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status wait_english(uint8_t *p, gh_scene_message_sources *source,
    gh_game_scene_ui *ui, const gh_boot_work *work, gh_event_timing *timing, uint32_t *frontier)
{
    *frontier = 0x2b3d70;
    uint16_t command;
    if (!code_at(source, source->cursor_8c, &command)) return GH_SCENE_UI_BOUNDS;
    const int r104_caption=work->room==0x104&&timing&&timing->event_number==5&&p[0xa4-0xc]==3&&half(p+0x64-0xc)==0x1000;
    if (command != 0xa301 || (p[0xa4 - 0xc]&&!r104_caption) || p[1] > 1) return GH_SCENE_UI_UNSUPPORTED;
    uint32_t flags = word(p + 0x14 - 0xc);
    if (!p[1]) {
        if (half(p + 0x2e - 0xc)) return GH_SCENE_UI_OK;
        p[0xb2 - 0xc] = 0;
        flags |= 0x80u;
        put(p + 0x14 - 0xc, flags);
        p[1] = 1;
    }
    ++p[0xb2 - 0xc];
    ++ui->wait_frames_3c2550;
    const uint32_t system = word((const uint8_t *)work + 0x5e4);
    if (system & 0x10u) return GH_SCENE_UI_OK;
    if (word(p + 0x34 - 0xc) == 1) {
        const uint32_t waited = ui->wait_frames_3c2550;
        if (waited < 90 || waited > INT32_MAX) return GH_SCENE_UI_OK;
    } else if (!native_confirm(work)) return GH_SCENE_UI_OK;

    ui->wait_frames_3c2550 = 0;
    put(p + 0x14 - 0xc, flags & ~0x81u);
    source->cursor_8c += 2; /* A301 is a one-halfword, zero-cost command. */
    p[0] = 2;
    p[1] = p[2] = p[3] = 0;
    if (system & 0x20000000u) {
        source->confirm_pending = 1;
        return clear_event_cut(source, timing, frontier);
    }
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status event_caption_start_002b2d88(uint8_t *p, gh_scene_message_sources *source,
    const gh_boot_work *work, gh_event_timing *timing, int *waiting, uint32_t *frontier)
{
    *frontier = 0x2b2d88;
    *waiting = 0;
    if (source->primary.size - source->cursor_8c < 10) return GH_SCENE_UI_BOUNDS;
    if (word((const uint8_t *)work + 0x5e4) & 0x20000000u) {
        const uint8_t *env;
        gh_scene_ui_status status = event_message_env(timing, &env);
        if (status) return status;
        const unsigned cut = word(env + 0x10);
        float frame;
        memcpy(&frame, env + 0x14, sizeof(frame));
        if (cut >= timing->count || !isfinite(frame) || frame < 0) return GH_SCENE_UI_BOUNDS;
        const uint8_t *command = source->primary.bytes + source->cursor_8c;
        const unsigned begin = half(command + 2), end = half(command + 6);
        if ((begin != 0xff && begin >= timing->count) || (end != 0xff && end >= timing->count))
            return GH_SCENE_UI_BOUNDS;
        p[0x44 - 0xc] = (uint8_t)(begin == 0xff ? cut : begin);
        p[0x4c - 0xc] = (uint8_t)(end == 0xff ? cut : end);
        put(p + 0x48 - 0xc, half(command + 4));
        put(p + 0x50 - 0xc, half(command + 8));
        *waiting = cut < p[0x44 - 0xc] || frame < (float)half(command + 4);
    }
    if (!*waiting) source->cursor_8c += 10;
    return GH_SCENE_UI_OK;
}

/* R104's English message 1000 begins with SkipOff and an original
 * cut-sync command. At cut 0 it leaves the message window unopened. */
static gh_scene_ui_status r104_wait_caption_prefix(uint8_t *p,
    gh_scene_message_sources *source,const gh_boot_work *work,gh_event_timing *timing,
    uint16_t *command,int *waiting,uint32_t *frontier)
{
    *waiting=0;
    if(work->room!=0x104||!timing||timing->event_number!=5||
       half(p+0x64-0xc)!=0x1000)return GH_SCENE_UI_OK;
    uint16_t word_at;
    if(*command==0x8702&&source->cursor_8c==0){
        if(!code_at(source,2,&word_at)||word_at!=0x5a)return GH_SCENE_UI_BOUNDS;
        put(p+0x34-0xc,1);
        source->cursor_8c=4;
        if(!code_at(source,4,command))return GH_SCENE_UI_BOUNDS;
    }
    if(*command==0xe403&&source->cursor_8c==4){
        uint16_t begin,end;
        if(!code_at(source,6,&begin)||!code_at(source,8,&end)||begin!=10||end!=10)
            return GH_SCENE_UI_BOUNDS;
        *frontier=0x2b2bd8;
        if(word((const uint8_t *)work+0x5e4)&0x20000000u){
            const uint8_t *env;
            gh_scene_ui_status status=event_message_env(timing,&env);
            if(status)return status;
            if(end>=timing->count)return GH_SCENE_UI_BOUNDS;
            p[0x44-0xc]=(uint8_t)begin;
            p[0x4c-0xc]=(uint8_t)end;
            /* EV05 cut 10 has a nonzero primary camera, so REL6980 stores 2. */
            ((gh_event_cut_timing *)timing->allocation.pointer)[end].kind=2;
            *waiting=word(env+0x10)<begin;
            if(*waiting)return GH_SCENE_UI_OK;
        }
        source->cursor_8c=10;
        if(!code_at(source,10,command))return GH_SCENE_UI_BOUNDS;
    }
    if(*command!=0xe502||source->cursor_8c!=10)return GH_SCENE_UI_OK;
    uint16_t start_frame;
    if(!code_at(source,12,&start_frame)||start_frame!=42)return GH_SCENE_UI_BOUNDS;
    *frontier=0x2b2c98;
    if(word((const uint8_t *)work+0x5e4)&0x20000000u){
        const uint8_t *env;
        gh_scene_ui_status status=event_message_env(timing,&env);
        if(status)return status;
        float frame;
        memcpy(&frame,env+0x14,sizeof(frame));
        if(!isfinite(frame)||frame<0)return GH_SCENE_UI_BOUNDS;
        *waiting=frame<(float)start_frame;
        if(*waiting)return GH_SCENE_UI_OK;
    }
    source->cursor_8c=14;
    return code_at(source,14,command)?GH_SCENE_UI_OK:GH_SCENE_UI_BOUNDS;
}

static gh_scene_ui_status opening(gh_scene_message_block *block, gh_scene_message_sources *source,
    gh_game_scene_ui *ui, const gh_boot_work *work, gh_event_timing *timing,
    const unsigned *slots, unsigned count, uint32_t *frontier)
{
    uint8_t *p = block->payload;
    const unsigned state = p[0], phase = p[1];
    *frontier = 0x2b2438;
    if (!source->bound) return GH_SCENE_UI_UNSUPPORTED;
    if (source->confirm_pending || state == 2 || state == 3 || state == 4 || state == 6) {
        gh_scene_ui_status status = source->confirm_pending ? clear_event_cut(source, timing, frontier) :
            state == 2 ? advance_english(p, source, ui, work, timing, frontier) :
            state == 3 ? wait_english(p, source, ui, work, timing, frontier) :
            state == 4 ? wait_scenery_choice(p,work,frontier) :
            close_window(p, source, frontier);
        if (status) return status;
        goto finish;
    }
    if (state == 7) goto finish;
    if (state > 1) return GH_SCENE_UI_UNSUPPORTED;
    const int r104_text=work->room==0x104&&timing&&timing->event_number==5&&half(p+0x64-0xc)==0x1000;
    if ((p[0xa4 - 0xc] && p[0xa4 - 0xc] != 4 && !(r104_text&&p[0xa4-0xc]==3)) ||
        word(p + 0x18 - 0xc) || word(p + 0x1c - 0xc)) {
        *frontier = state == 0 ? 0x2b2e80 : 0x2b3078;
        return GH_SCENE_UI_UNSUPPORTED;
    }
    if (state == 0) {
        *frontier = 0x2b2588;
        if (phase > 1 || !source->primary.bytes || source->cursor_8c > source->primary.size ||
            source->primary.size - source->cursor_8c < 2) return GH_SCENE_UI_BOUNDS;
        unsigned same_group = 0;
        for (unsigned i = 0; i < count; ++i)
            same_group += ui->blocks[slots[i]].payload[0x2c - 0xc] == p[0x2c - 0xc];
        p[1] = 1; /* 2B2580 is the original empty phase-0 initializer. */
        if (same_group >= 2) goto finish;
        uint16_t command=half(source->primary.bytes+source->cursor_8c);
        int r104_waiting;
        gh_scene_ui_status r104_caption=r104_wait_caption_prefix(p,source,work,timing,
            &command,&r104_waiting,frontier);
        if(r104_caption)return r104_caption;
        if(r104_waiting)goto finish;
        const int timed_caption = command == 0xe705 && ev1b_timed_caption(source, work, timing);
        if (timed_caption) {
            int waiting;
            gh_scene_ui_status status = event_caption_start_002b2d88(p, source, work, timing, &waiting, frontier);
            if (status) return status;
            if (waiting) goto finish;
            if (!code_at(source, source->cursor_8c, &command)) return GH_SCENE_UI_BOUNDS;
        }
        if(r104_text&&command==0x8504&&source->cursor_8c==14){
            uint16_t parameter,x,y;
            *frontier=0x2b2b08;
            if(!code_at(source,16,&parameter)||!code_at(source,18,&x)||
               !code_at(source,20,&y)||parameter!=1||x!=180||y!=300)
                return GH_SCENE_UI_BOUNDS;
            p[0x70-0xc]=2;
            p[0xa4-0xc]=3;
            p[0x3c-0xc]=0;
            put(p+0x18-0xc,0);
            put_half(p+0x62-0xc,parameter);
            put_half(p+0xa6-0xc,x);
            put_half(p+0xa8-0xc,y);
            source->cursor_8c=22;
            if(!code_at(source,22,&command))return GH_SCENE_UI_BOUNDS;
        }
        if (command==0xa602) {
            *frontier=0x2b2b78;
            if (source->primary.size-source->cursor_8c<6) return GH_SCENE_UI_BOUNDS;
            p[0xa4-0xc]=4;
            p[0x70-0xc]=2;
            p[0x3c-0xc]=0;
            put(p+0x18-0xc,0);
            put_half(p+0x94-0xc,half(source->primary.bytes+source->cursor_8c+2));
            put_half(p+0x96-0xc,0);
            /* A602 encodes two halfwords and 2AF218 returns offset zero. */
            source->cursor_8c+=4;
            command=half(source->primary.bytes+source->cursor_8c);
        }
        /* B0 falls through 2B2638 with result 2. 2B2588 opens at the
         * current byte so the renderer still sees the scale command. */
        if ((command & 0x8000u) && !(timed_caption && command == 0xb002)) {
            *frontier = 0x2b2638;
            return GH_SCENE_UI_UNSUPPORTED;
        }
        put_half(p + 0x88 - 0xc, UINT16_MAX);
        ++p[0x30 - 0xc];
        put(p + 0x84 - 0xc, word(p + 0x90 - 0xc));
        source->cursor_7c = source->cursor_8c;
        p[0] = 1;
        p[1] = p[2] = p[3] = 0;
    }
    if(r104_text&&p[0xa4-0xc]==3){
        /* 2B3158/3190 run a six-tick caption-window open. */
        if(!p[1]){
            put(p+0x10-0xc,0);
            put(p+0x14-0xc,word(p+0x14-0xc)|8u);
            p[1]=1;
        }
        const unsigned tick=word(p+0x10-0xc)+1;
        put(p+0x10-0xc,tick);
        const volatile float fraction=(float)tick/6.0f;
        const volatile float square=(float)(tick*tick)/36.0f;
        const float progress=(fraction+fraction)-square;
        put_float(p+0x98-0xc,progress);
        put_float(p+0x9c-0xc,progress);
        put_float(p+0xa0-0xc,square);
        if(tick==6){
            put(p+0x14-0xc,word(p+0x14-0xc)|0x10u);
            p[0]=2;
            p[1]=p[2]=p[3]=0;
        }
    }else if (!p[1]) {
        /* 2B3078 mode 0 sets phase 2, then runs one original 2B3190
         * animation tick before the next call can take phase 2 directly. */
        put(p + 0x10 - 0xc, 1);
        put(p + 0x14 - 0xc, word(p + 0x14 - 0xc) | 8u);
        const volatile float fraction = 1.0f / 6.0f, square = 1.0f / 36.0f;
        const float progress = (fraction + fraction) - square;
        put_float(p + 0x98 - 0xc, progress);
        put_float(p + 0x9c - 0xc, progress);
        put_float(p + 0xa0 - 0xc, square);
        p[1] = 2;
    } else if (p[1] == 2) {
        put(p + 0x14 - 0xc, word(p + 0x14 - 0xc) | 0x10u);
        p[0] = 2;
        p[1] = p[2] = p[3] = 0;
    } else {
        *frontier = 0x2b3190;
        return GH_SCENE_UI_UNSUPPORTED;
    }

finish:
    if (half(p + 0x2e - 0xc))
        put_half(p + 0x2e - 0xc, (uint16_t)(half(p + 0x2e - 0xc) - 1u));
    if (word(p + 0xb4 - 0xc) == 1) put(p + 0xb8 - 0xc, 1);
    return GH_SCENE_UI_OK;
}

gh_scene_ui_status gh_scene_message_update_with_timing_002aefa8(gh_game_scene_ui *ui,
    const gh_boot_work *work, gh_event_timing *timing, uint32_t *frontier)
{
    if (frontier) *frontier = 0;
    if (!ui || !work) return GH_SCENE_UI_ARGUMENT;
    if (word((const uint8_t *)work + 0x5d8) & 0x80000u) return GH_SCENE_UI_OK;
    unsigned slots[16], count;
    gh_scene_ui_status status = active_slots(ui, slots, &count);
    if (status) return status;
    uint32_t child = 0;
    for (unsigned i = 0; i < count;) {
        unsigned slot = slots[i];
        status = opening(ui->blocks + slot, ui->sources + slot, ui, work, timing, slots, count, &child);
        if (!status && ui->blocks[slot].payload[0] == 7) {
            status = recycle_window(ui, slot, &child);
            if (!status) {
                for (unsigned j = i; j + 1 < count; ++j) slots[j] = slots[j + 1];
                --count;
                continue;
            }
        }
        if (status) {
            if (frontier) *frontier = child;
            return status;
        }
        ++i;
    }
    return GH_SCENE_UI_OK;
}

gh_scene_ui_status gh_scene_message_update_002aefa8(gh_game_scene_ui *ui,
    const gh_boot_work *work, uint32_t *frontier)
{
    return gh_scene_message_update_with_timing_002aefa8(ui, work, NULL, frontier);
}

gh_scene_ui_status gh_scene_message_prepare_draw_002af028(gh_game_scene_ui *ui,
    const gh_boot_work *work, uint32_t *frontier)
{
    if (frontier) *frontier = 0;
    if (!ui || !work) return GH_SCENE_UI_ARGUMENT;
    unsigned slots[16], count;
    gh_scene_ui_status status = active_slots(ui, slots, &count);
    if (status) return status;
    const uint8_t *raw = (const uint8_t *)work;
    if ((word(raw + 0x590) & 2) || (word(raw + 0x5e4) & 0x08000000u)) return GH_SCENE_UI_OK;
    for (unsigned i = 0; i < count; ++i) {
        const unsigned slot = slots[i];
        uint8_t *p = ui->blocks[slot].payload;
        const unsigned mode=p[0xa4-0xc];
        const int r104_caption=work->room==0x104&&mode==3&&half(p+0x64-0xc)==0x1000;
        if (!ui->sources[slot].bound || (mode && mode!=4 && !r104_caption)) {
            if (frontier) *frontier = 0x2b16f0;
            return GH_SCENE_UI_UNSUPPORTED;
        }
        if(r104_caption){
            put_half(p+0xae - 0xc,half(p+0xa6-0xc));
            put_half(p+0xb0-0xc,half(p+0xa8-0xc));
            put(p+0x58-0xc,0x3f800000u);
            put_half(p+0x5c-0xc,1);
            put_half(p+0x5e - 0xc,0xffec);
            put(p+0x54-0xc,0x3f800000u);
            if(word(p+0x14-0xc)&0x10u){*frontier=0x2b1c80;return GH_SCENE_UI_UNSUPPORTED;}
            continue;
        }
        put_half(p + 0xa6 - 0xc, 0x100);
        put_half(p + 0xa8 - 0xc, mode==4?0x170:0x182);
        put_half(p + 0xaa - 0xc, 0x1e0);
        put_half(p + 0xac - 0xc, mode==4?0x78:0x88);
        put_half(p + 0xae - 0xc, 0x100);
        put_half(p + 0xb0 - 0xc, mode==4?0x170:0x182);
        if (word(p + 0x14 - 0xc) & 0x10u) {
            if (frontier) *frontier = 0x2b1c80;
            return GH_SCENE_UI_UNSUPPORTED;
        }
    }
    return GH_SCENE_UI_OK;
}
