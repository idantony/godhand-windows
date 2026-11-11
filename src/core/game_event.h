/* MIT reconstruction reference: upstream cEvent.h/00296530.c.
 * Complete upstream notice: LICENSE.upstream. */
#ifndef GH_GAME_EVENT_H
#define GH_GAME_EVENT_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Verified prefix only; the original class has additional fields. These
 * 32-bit tokens are diagnostic guest data, never dereferenced as host pointers. */
typedef struct {
    uint32_t on_loaded_token;
    uint8_t state,phase,unknown06,unknown07;
    uint32_t flags,load_handle,resource_token,text_token;
    int32_t data_number;
    uint32_t unknown1c,position_bits[4],angle_bits;
    uint8_t skip_ok,unknown35[3];
} gh_game_event_prefix;
/* Original Event+38 points to this separate record. Its native owner must
 * survive the intermediate room load until the destination request consumes it. */
typedef struct {
    uint32_t flags,mode;
    uint16_t destination_room;
    uint8_t reserved0a[6];
    float position[3];
    uint32_t reserved1c;
    float yaw;
    uint32_t reserved24[3];
} gh_event_reserve_work;
typedef enum {GH_EVENT_OK,GH_EVENT_ARGUMENT,GH_EVENT_BOUNDS,GH_EVENT_FORMAT,
              GH_EVENT_UNSUPPORTED,GH_EVENT_CALLEE} gh_game_event_status;
typedef struct gh_game_event gh_game_event;
/* Required recovered native stage: done is the ORIGINAL return value, whereas
 * nonzero callback return is a distinct host error. Test fixtures are not real
 * cutscene implementations. Release and move-step ignore done as originalvoid. */
typedef int (*gh_event_native_stage)(void *,gh_game_event *,int *done);
struct gh_game_event {
    gh_game_event_prefix fields;
    gh_event_native_stage stages[6];
    void *contexts[6];
    uint32_t frontier;
    int callee_error;
    uint8_t initialized, release_resume;
};
/* Actual initial mapped ELF/BSS bytes at00586AF0. This is not a claim that
 * every earlier boot/constructor ran or a way to restore a live event session. */
gh_game_event_status gh_game_event_from_elf(gh_game_event *,const void *,size_t);
/*0..4=start/clear/create/play/release,5=move-step. Configureactualnative
 * providers when recovered; absent providers remain exact original frontiers. */
gh_game_event_status gh_game_event_set_stage(gh_game_event *,unsigned,gh_event_native_stage,void *);
/* Actual296530: bit8 takes priority, inactive returns, signedstate0..4
 * dispatches with original same-call fallthrough. No fake empty active stage. */
gh_game_event_status gh_game_event_update_00296530(gh_game_event *);
#ifdef __cplusplus
}
#endif
#endif
