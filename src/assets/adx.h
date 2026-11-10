#ifndef GH_ADX_H
#define GH_ADX_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { GH_ADX_OK, GH_ADX_ARGUMENT, GH_ADX_FORMAT, GH_ADX_BOUNDS,
 GH_ADX_UNSUPPORTED, GH_ADX_KEY, GH_ADX_MEMORY, GH_ADX_DEVICE, GH_ADX_STATE } gh_adx_error;
typedef struct { uint16_t start,mult,add; } gh_adx_key;
typedef struct {
 uint32_t sample_rate,sample_count,loop_start,loop_end,data_offset;
 uint8_t channels,loop_enabled;
 int16_t coefficient[2],initial_history[2][2];
} gh_adx_info;
typedef struct gh_adx_decoder gh_adx_decoder;
/* Reads the actual 2CA34C key-set call and its string plus prime table through
 * bounded ELF32 PT_LOAD spans. No executable bytes are run or host-cast. */
gh_adx_error gh_adx_key_from_elf(const void*,size_t,gh_adx_key*);
/* Restricted ADX encoding3/frame18/bits4/stereo/version0408, 48k/500Hz.
 * Includes the verified 0x28-byte nonlooping header with stereo history.
 * Borrows immutable source until close; scans all real scale headers against
 * the key. This is format validation, not cryptographic authentication. */
gh_adx_error gh_adx_open(const void*,size_t,const gh_adx_key*,gh_adx_decoder**);
void gh_adx_close(gh_adx_decoder*);
gh_adx_error gh_adx_inspect(const gh_adx_decoder*,gh_adx_info*);
uint32_t gh_adx_tell(const gh_adx_decoder*);
/* capacity is sample frames, output requires capacity*2 int16 elements.
 * Returns actual decoded frames; EOF is OK with zero frames, never silence. */
gh_adx_error gh_adx_decode(gh_adx_decoder*,int16_t*,size_t capacity,size_t *frames);
/* Replays prediction from a verified checkpoint; exact arbitrary sample seek.
 * No inferred loop policy: caller chooses whether/when to seek loop_start. */
gh_adx_error gh_adx_seek(gh_adx_decoder*,uint32_t sample);
const char *gh_adx_error_string(gh_adx_error);
#ifdef __cplusplus
}
#endif
#endif
