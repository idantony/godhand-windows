/* Recovered from user-supplied r207.rel (not copied from upstream source).
 * SHA256 ea59932301345e48add7a3eff029b7d0d0138fff1abc67569467880678c653c1
 * See analysis/r207-source-evidence.json for symbols, instructions, relocation
 * sites, exact original offsets and native adaptation. Lifecycle evidence is
 * in analysis/r207-lifecycle-evidence.json. */
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

/* These are specialized to the verified immutable r207 tables, not a generic
 * SNR2 loader. __do_global_ctors @0x230 reads count=0 at 0x400,
 * skips sentinel scanning and makes no indirect call. */
void gh_r207_do_global_ctors(void) {
}
/* __do_global_dtors @0x1d8 reads pointer cursor at 0x300 (0x40c),
 * then zero at 0x40c: returns without advancing cursor or freeing room_work.
 * No exported API permits replacing original table entries with host pointers. */
void gh_r207_do_global_dtors(gh_r207_lifecycle *state) {
    (void)state;
}
void gh_r207_lifecycle_reset(gh_r207_lifecycle *state) {
    state->main_guard = 0;
    state->dtor_cursor = 0x40c;
}
/* __main @0x2e0: set guard BEFORE tail-calling constructors. Guard is never
 * cleared by destructors; reset is a host module-load operation only. */
void gh_r207_main(gh_r207_lifecycle *state) {
    if (!state->main_guard) {
        state->main_guard = 1;
        gh_r207_do_global_ctors();
    }
}
