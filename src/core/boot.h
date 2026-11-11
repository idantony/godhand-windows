/* MIT-derived reconstruction; see boot-port-evidence.json and LICENSE.upstream. */
#ifndef GODHAND_CORE_BOOT_H
#define GODHAND_CORE_BOOT_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Exact PS2 byte layout, no native pointers or host-sized longs. */
typedef struct gh_boot_work {
    uint8_t pad000[0x569], phase569, pad56a[2];
    int32_t language;
    uint8_t pad570[4];
    float canonical_rate574;        /* original 00747A14 */
    int32_t mode, unknown57c, unknown580;
    uint32_t flags;
    uint8_t pad588[4];
    uint32_t system_flags;
    uint8_t pad590[8];
    uint32_t flags598, flags59c;
    uint8_t pad5a0[0x10];
    int16_t room;
    uint8_t pad5b2[0x620 - 0x5b2];
} gh_boot_work;

/* External words touched by 0030B298, owned by the future platform/runtime. */
typedef struct gh_boot_state {
    gh_boot_work work;              /* original 0x007474A0 */
    int32_t video_parameter;        /* original 0x003C3CCC */
    int32_t word752d3c;             /* original 0x00752D3C */
} gh_boot_state;

typedef enum gh_boot_operation {
    GH_BOOT_SETUP_30D258, GH_BOOT_CONFIG_30ECF8,
    GH_BOOT_TRAMPOLINE_15B0A0, GH_BOOT_SETUP_31CEB8,
    GH_BOOT_OBJECT_INIT_2BDFF8, GH_BOOT_VIDEO_SETUP_30CB30,
    GH_BOOT_GS_RESET, GH_BOOT_GS_SYNC_V, GH_BOOT_REGISTER_EXIT,
    GH_BOOT_SETUP_30D1F8, GH_BOOT_DATA_INIT_1FF858,
    GH_BOOT_OBJECT_START_2BE6E8, GH_BOOT_OPERATION_COUNT
} gh_boot_operation;

typedef struct gh_boot_request {
    gh_boot_operation operation;
    uint32_t function_address;
    unsigned argument_count;
    uint32_t arguments[4]; /* original guest values/addresses, never host pointers */
} gh_boot_request;

/* A callback performs the actual subsystem operation, not a success stub.
 * Return 0 only when performed, nonzero if unavailable/failed. State is shared
 * so actual global mutations remain visible between original calls. Guest
 * object addresses must be resolved by the adapter into its own real objects.
 * Original callee return values are ignored here just as in 0030B298. */
typedef int (*gh_boot_platform_call)(void *, gh_boot_state *, const gh_boot_request *);
typedef struct gh_boot_platform {
    void *context;
    gh_boot_platform_call calls[GH_BOOT_OPERATION_COUNT];
} gh_boot_platform;
typedef enum gh_boot_status {
    GH_BOOT_OK, GH_BOOT_INVALID_ARGUMENT, GH_BOOT_MISSING_CALLBACK,
    GH_BOOT_PLATFORM_FAILED
} gh_boot_status;
typedef struct gh_boot_result {
    gh_boot_status status;
    gh_boot_operation operation; /* failing op; COUNT on success/invalid argument */
    int platform_error;
} gh_boot_result;

/* Ports 0030B298 only. Requires every callback before any mutation. On platform
 * failure stops immediately, retains partial state, and reports the failing op.
 * This is a runnable dependency slice, not a complete game startup/runtime. */
gh_boot_result gh_boot_0030b298(gh_boot_state *, const gh_boot_platform *);
#ifdef __cplusplus
}
#endif
#endif
