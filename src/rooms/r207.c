/* Recovered from user-supplied r207.rel (not copied from upstream source).
 * SHA256 ea59932301345e48add7a3eff029b7d0d0138fff1abc67569467880678c653c1
 * See analysis/r207-source-evidence.json for symbols, instructions, relocation
 * sites, exact original offsets, native adaptation and pending CRT routines. */
#include "r207.h"

/* __builtin_new @0x100; relocation imports default heap and allocator. */
gh_r207_allocation gh_r207_builtin_new(const gh_r207_memory *memory, uint32_t size) {
    return memory->allocate(memory->context, size, 16, memory->default_heap);
}
/* __builtin_vec_new @0x128 has the same observed allocation contract. */
gh_r207_allocation gh_r207_builtin_vec_new(const gh_r207_memory *memory, uint32_t size) {
    return memory->allocate(memory->context, size, 16, memory->default_heap);
}
/* __builtin_delete @0x150: null is skipped, owner comes from pointer-0x20.
 * Native allocation.owner replaces that PS2 allocation-header word. */
void gh_r207_builtin_delete(const gh_r207_memory *memory, gh_r207_allocation allocation) {
    if (allocation.pointer)
        memory->release(memory->context, allocation.owner, allocation.pointer);
}
/* __builtin_vec_delete @0x178 shares the same null/header-owner behavior. */
void gh_r207_builtin_vec_delete(const gh_r207_memory *memory, gh_r207_allocation allocation) {
    if (allocation.pointer)
        memory->release(memory->context, allocation.owner, allocation.pointer);
}
/* R207Init__Fv @0x1a0: allocate 1 byte, store returned pointer at 0x480. */
void gh_r207_init(gh_r207_state *state, const gh_r207_memory *memory) {
    state->room_work = gh_r207_builtin_new(memory, 1);
}
/* R207Move__Fv @0x1d0, bytes 0800e00300000000. */
void gh_r207_move(void) {
}
