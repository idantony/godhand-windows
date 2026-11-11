#include "game_scene_message_create.h"

#include <string.h>

enum { WINDOWS = 16 };

static uint32_t word(const uint8_t *p)
{
    uint32_t value;
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

static gh_scene_ui_status message_sources(const gh_game_scene_ui *ui, const gh_boot_work *work,
    uint16_t id, gh_scene_message_sources *out, uint32_t *frontier)
{
    gh_bundle_entry bank;
    *frontier = 0x2aead0;
    switch (id >> 12) {
        case 0: bank = ui->root->data; break;
        case 1: bank = ui->room_data; break;
        case 3: bank = ui->root->texture_data; break;
        default: return GH_SCENE_UI_UNSUPPORTED;
    }
    if (!bank.bytes || bank.size < 4) return GH_SCENE_UI_BOUNDS;
    const uint32_t count = word(bank.bytes), index = id & 0x0fffu;
    if (count > (bank.size - 4) / 8 || index >= count) return GH_SCENE_UI_BOUNDS;
    const uint8_t *row = bank.bytes + 4 + index * 8;
    const size_t directory_end = 4 + (size_t)count * 8;
    const size_t primary = word(row);
    if (primary < directory_end || primary >= bank.size || bank.size - primary < 2)
        return GH_SCENE_UI_BOUNDS;
    *frontier = 0x2aeb00;
    const size_t ruby = work->language == 0 ? word(row + 4) : 0;
    if (ruby && (ruby < directory_end || ruby >= bank.size || bank.size - ruby < 2))
        return GH_SCENE_UI_BOUNDS;
    *out = (gh_scene_message_sources){
        .primary = {bank.bytes + primary, bank.size - primary},
        .ruby = {ruby ? bank.bytes + ruby : NULL, ruby ? bank.size - ruby : 0},
        .bound = 1
    };
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status verify_lists(const gh_game_scene_ui *ui)
{
    uint32_t seen = 0;
    int32_t previous = -1;
    for (int32_t i = ui->active_head; i != -1; i = ui->blocks[i].next) {
        if (i < 0 || i >= WINDOWS || (seen & (1u << i)) || ui->blocks[i].previous != previous)
            return GH_SCENE_UI_BOUNDS;
        seen |= 1u << i;
        previous = i;
    }
    for (int32_t i = ui->free_head; i != -1; i = ui->blocks[i].next) {
        if (i < 0 || i >= WINDOWS || (seen & (1u << i))) return GH_SCENE_UI_BOUNDS;
        seen |= 1u << i;
    }
    return GH_SCENE_UI_OK;
}

static void close_group_zero(gh_game_scene_ui *ui)
{
    for (int32_t i = ui->active_head; i != -1; i = ui->blocks[i].next) {
        uint8_t *payload = ui->blocks[i].payload;
        if (payload[0x2c - 0x0c]) continue;
        put(payload + 0x14 - 0x0c, word(payload + 0x14 - 0x0c) | 0x40u);
        if ((unsigned)payload[0] - 6u >= 2u) {
            payload[0] = 6;
            payload[1] = payload[2] = payload[3] = 0;
        }
    }
}

static void initialize(gh_scene_message_block *block, uint16_t message_id)
{
    /* 2B2080 copies the original cold BSS template at 5E7910. Host list
     * indices and typed MSD references replace its guest pointer words. */
    memset(block, 0, sizeof(*block));
    uint8_t *p = block->payload;
    put(p + 0x14 - 0x0c, 0x1002);
    put(p + 0x20 - 0x0c, UINT32_MAX);
    for (unsigned i = 0; i < 3; ++i) put_half(p + 0x24 - 0x0c + i * 2, UINT16_MAX);
    p[0x30 - 0x0c] = 0xff;
    put_half(p + 0x64 - 0x0c, message_id);
    put(p + 0x6c - 0x0c, 0x3f800000);
    put_half(p + 0x88 - 0x0c, UINT16_MAX);
}

static int positive_priority(uint16_t mode)
{
    return mode && mode <= INT16_MAX;
}

gh_scene_ui_status gh_scene_message_create_002aeb68(gh_game_scene_ui *ui,
    const gh_boot_work *work, uint16_t message_id, uint16_t *work_id, uint32_t *frontier)
{
    if (!frontier) return GH_SCENE_UI_ARGUMENT;
    *frontier = 0x2aeb68;
    if (!ui || !ui->ready || !ui->root || !work || !work_id) return GH_SCENE_UI_ARGUMENT;
    gh_scene_ui_status status = verify_lists(ui);
    if (status) return status;
    const int32_t slot = ui->free_head;
    gh_scene_message_sources sources;
    if (slot != -1) {
        status = message_sources(ui, work, message_id, &sources, frontier);
        if (status) return status;
    }
    close_group_zero(ui);
    if (slot == -1) {
        *work_id = UINT16_MAX;
        *frontier = 0;
        return GH_SCENE_UI_OK;
    }

    gh_scene_message_block *block = ui->blocks + slot;
    ui->free_head = block->next;
    if (ui->free_head != -1) ui->blocks[ui->free_head].previous = block->previous;
    initialize(block, message_id);

    int32_t previous = -1, next = ui->active_head;
    while (next != -1 && positive_priority(ui->blocks[next].mode)) {
        previous = next;
        next = ui->blocks[next].next;
    }
    uint16_t generation = (uint16_t)(ui->generation_bd8 + 1u);
    if (generation == UINT16_MAX) generation = 0;
    ui->generation_bd8 = generation;
    block->flags = generation;
    block->previous = previous;
    block->next = next;
    if (previous != -1) ui->blocks[previous].next = slot;
    else ui->active_head = slot;
    if (next != -1) ui->blocks[next].previous = slot;
    ui->sources[slot] = sources;
    *work_id = generation;
    *frontier = 0;
    return GH_SCENE_UI_OK;
}

gh_game_event_status gh_scene_event_message_create_002aeb68(void *work, gh_game_scene_ui *ui,
    uint16_t message_id, uint16_t *work_id, uint32_t *frontier)
{
    switch (gh_scene_message_create_002aeb68(ui, work, message_id, work_id, frontier)) {
        case GH_SCENE_UI_OK: return GH_EVENT_OK;
        case GH_SCENE_UI_ARGUMENT: return GH_EVENT_ARGUMENT;
        case GH_SCENE_UI_BOUNDS: return GH_EVENT_BOUNDS;
        default: return GH_EVENT_UNSUPPORTED;
    }
}
