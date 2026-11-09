/* Copyright (c) 2026 Lucas Picoli and god-hand-decomp contributors.
 * MIT license: LICENSE.upstream. Derived from src/cod/0030b298.c at upstream
 * eaae18d0dfab1f9a0df276ae5ca1581ad326faf3; native fixed-width/callback adapter.
 * Addresses/call order checked against user's patched ELF, not assumed retail.
 */
#include "boot.h"
#include <string.h>
#define OFFSET(field, value) _Static_assert(offsetof(gh_boot_work, field) == value, #field " layout")
OFFSET(phase569, 0x569); OFFSET(language, 0x56c); OFFSET(mode, 0x578);
OFFSET(unknown57c, 0x57c); OFFSET(unknown580, 0x580); OFFSET(flags, 0x584);
OFFSET(system_flags, 0x58c); OFFSET(flags598, 0x598); OFFSET(flags59c, 0x59c);
OFFSET(room, 0x5b0);
_Static_assert(sizeof(gh_boot_work) == 0x620, "original clear extent");

static int invoke(const gh_boot_platform *p, gh_boot_state *s,
                  gh_boot_result *r, gh_boot_operation op, uint32_t pc,
                  unsigned count, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    const gh_boot_request request = {op, pc, count, {a,b,c,d}};
    const int error = p->calls[op](p->context, s, &request);
    if (error) {
        r->status = GH_BOOT_PLATFORM_FAILED;
        r->operation = op;
        r->platform_error = error;
        return 0;
    }
    return 1;
}

gh_boot_result gh_boot_0030b298(gh_boot_state *s, const gh_boot_platform *p)
{
    gh_boot_result result = {GH_BOOT_OK, GH_BOOT_OPERATION_COUNT, 0};
    unsigned i;
    if (!s || !p) {
        result.status = GH_BOOT_INVALID_ARGUMENT;
        return result;
    }
    for (i = 0; i < GH_BOOT_OPERATION_COUNT; ++i)
        if (!p->calls[i]) {
            result.status = GH_BOOT_MISSING_CALLBACK;
            result.operation = (gh_boot_operation)i;
            return result;
        }
#define CALL(op,pc,n,a,b,c,d) do { if (!invoke(p,s,&result,op,pc,n,a,b,c,d)) return result; } while(0)
    CALL(GH_BOOT_SETUP_30D258,0x30d258,0,0,0,0,0);
    CALL(GH_BOOT_CONFIG_30ECF8,0x30ecf8,1,0,0,0,0);
    CALL(GH_BOOT_TRAMPOLINE_15B0A0,0x15b0a0,0,0,0,0,0);
    CALL(GH_BOOT_SETUP_31CEB8,0x31ceb8,0,0,0,0,0);
    memset(&s->work, 0, sizeof(s->work));
    s->work.language = 1;
    s->work.mode = 0;
    s->work.flags59c &= ~UINT32_C(0x100000);
    if (!(s->work.flags598 & 0x200000) || (s->work.flags59c & 0x1000000))
        s->work.room = 0x20;
    else
        s->work.flags |= 0x200;
    if (s->work.system_flags & 0x800)
        s->work.language = 1;
    s->work.unknown57c = 0;
    s->work.unknown580 = 0;
    s->video_parameter = s->work.mode == 1 ? 0x50 : 0x40;
    CALL(GH_BOOT_OBJECT_INIT_2BDFF8,0x2bdff8,1,0x5e8140,0,0,0);
    CALL(GH_BOOT_VIDEO_SETUP_30CB30,0x30cb30,0,0,0,0,0);
    if (s->work.mode == 1) {
        CALL(GH_BOOT_GS_RESET,0x3951f8,4,0,1,3,0);
        CALL(GH_BOOT_GS_SYNC_V,0x395398,1,0,0,0,0);
    }
    CALL(GH_BOOT_REGISTER_EXIT,0x395f68,1,0x30b148,0,0,0);
    CALL(GH_BOOT_SETUP_30D1F8,0x30d1f8,0,0,0,0,0);
    CALL(GH_BOOT_DATA_INIT_1FF858,0x1ff858,2,0x580d40,0x4504d8,0,0);
    s->work.phase569 = 2;
    s->word752d3c = 0;
    CALL(GH_BOOT_OBJECT_START_2BE6E8,0x2be6e8,1,0x5e8640,0,0,0);
#undef CALL
    return result;
}
