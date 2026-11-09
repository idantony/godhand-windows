#ifndef GH_R207_H
#define GH_R207_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Native replacement for PS2 allocation pointer + heap word at pointer-0x20.
 * The allocator returns owner explicitly; no guest pointer is dereferenced. */
typedef struct { void *pointer; void *owner; } gh_r207_allocation;
typedef struct {
    void *context;
    void *default_heap;
    gh_r207_allocation (*allocate)(void *context, uint32_t size,
                                   uint32_t alignment, void *heap);
    void (*release)(void *context, void *owner, void *pointer);
} gh_r207_memory;
typedef struct {
    gh_r207_allocation room_work; /* Original image word at REL offset 0x480. */
} gh_r207_state;
/* Required callbacks are supplied by the host. Size is original uint32_t.
 * These are recovered behavior, not a replacement game-loop implementation. */
gh_r207_allocation gh_r207_builtin_new(const gh_r207_memory *, uint32_t size);
gh_r207_allocation gh_r207_builtin_vec_new(const gh_r207_memory *, uint32_t size);
void gh_r207_builtin_delete(const gh_r207_memory *, gh_r207_allocation allocation);
void gh_r207_builtin_vec_delete(const gh_r207_memory *, gh_r207_allocation allocation);
/* R207Init return register is not consumed/typed by this overlay; expose void.
 * Original always allocates one byte; repeated init overwrites stored pointer. */
void gh_r207_init(gh_r207_state *, const gh_r207_memory *);
/* Original R207Move contains exactly jr ra; nop. No state update. */
void gh_r207_move(void);
#ifdef __cplusplus
}
#endif
#endif
