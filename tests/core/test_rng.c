#include "rng.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fenv.h>

static int failures;
static uint32_t bits(float value)
{
    uint32_t result;
    memcpy(&result, &value, sizeof(result));
    return result;
}
static void check(uint32_t got, uint32_t expected, const char *what)
{
    if (got != expected) {
        fprintf(stderr, "%s: got %08x expected %08x\n", what,
                (unsigned)got, (unsigned)expected);
        ++failures;
    }
}

static void run_vectors(void)
{
    /* Fixed independent vectors: modular integer arithmetic, binary32
     * chop conversion, then exact exponent scaling by 2^-31.
     * They are not MIPS execution captures. */
    static const uint32_t words[] = {
        0x00010dcd, 0x1c587629, 0xa6ffb3d5, 0x6ab9d291,
        0xb90ffb1d, 0xfca98f39, 0x7f4695a5, 0x16c43621
    };
    static const uint32_t outputs[] = {
        0x3806e680, 0x3e62c3b1, 0x3fa6ffb3, 0x3f5573a5,
        0x3fb90ffb, 0x3ffca98f, 0x3f7e8d2b, 0x3e3621b1
    };
    gh_rng_state state = {1}, zero = {0}, high = {0x80000000u};
    gh_rng_state wrap = {0xffffffffu}, independent = {1};
    unsigned i;
    for (i = 0; i < sizeof(words) / sizeof(words[0]); ++i) {
        check(bits(gh_rng_0031cf78(&state)), outputs[i], "sequence float");
        check(state.word, words[i], "sequence state");
    }
    check(bits(gh_rng_0031cf78(&zero)), 0, "zero remains zero");
    check(bits(gh_rng_0031cf30(&zero)), 0x3f800000, "zero wrapper returns one");
    check(zero.word, 0, "wrapper preserves zero state");
    check(bits(gh_rng_0031cf78(&high)), 0x3f800000, "high-bit conversion unsigned");
    check(high.word, 0x80000000u, "high-bit wrap");
    check(bits(gh_rng_0031cf78(&wrap)), 0x3ffffef2u, "overflow output");
    check(wrap.word, 0xfffef233u, "overflow state");
    check(bits(gh_rng_0031cf30(&independent)), outputs[0], "positive wrapper unchanged");
    check(independent.word, words[0], "states independent");
    /* Inverse-multiplier seeds force exact conversion boundary words.
     * These distinguish truncation from nearest-even and exercise the
     * high-bit shift/or unsigned conversion path in the original ELF. */
    {
        static const struct { uint32_t seed, next, output; } edges[] = {
            {0xda1d58fb, 0x7fffffff, 0x3f7fffff},
            {0x25e2a705, 0x80000001, 0x3f800000},
            {0x71538280, 0x80000080, 0x3f800000},
            {0x17362985, 0x80000081, 0x3f800000},
            {0x53fa8780, 0x80000180, 0x3f800001},
            {0x5a1d58fb, 0xffffffff, 0x3fffffff}
        };
        for (i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i) {
            gh_rng_state edge = {edges[i].seed};
            check(bits(gh_rng_0031cf78(&edge)), edges[i].output, "rounding edge");
            check(edge.word, edges[i].next, "edge modular product");
        }
    }
}

int main(void)
{
    const int original_mode = fegetround();
    unsigned i, modes_tested = 0;
    const int modes[] = {
        FE_TONEAREST,
#ifdef FE_TOWARDZERO
        FE_TOWARDZERO,
#endif
#ifdef FE_DOWNWARD
        FE_DOWNWARD,
#endif
#ifdef FE_UPWARD
        FE_UPWARD,
#endif
    };
    run_vectors();
    for (i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        if (fesetround(modes[i]) == 0) {
            ++modes_tested;
            run_vectors();
            check((uint32_t)fegetround(), (uint32_t)modes[i], "host mode preserved");
        }
    }
    if (original_mode != -1) (void)fesetround(original_mode);
    printf("Host rounding modes exercised: %u\n", modes_tested);
    if (failures) return 1;
    puts("RNG source-semantics vectors passed (MIPS differential pending)");
    return 0;
}
