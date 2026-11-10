#ifndef GH_NATIVE_INPUT_H
#define GH_NATIVE_INPUT_H
#include <stddef.h>
#include <stdint.h>
#include "../core/boot.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {GH_INPUT_CROSS,GH_INPUT_CIRCLE,GH_INPUT_UP,GH_INPUT_DOWN,GH_INPUT_LEFT,GH_INPUT_RIGHT,GH_INPUT_START,GH_INPUT_TRIANGLE,GH_INPUT_BUTTONS};
typedef struct {uint16_t virtual_key[GH_INPUT_BUTTONS],xinput_mask[GH_INPUT_BUTTONS];uint8_t repeat_multiplier;} gh_native_input_config;
typedef struct {const unsigned char*bytes;size_t size;} gh_native_input_map;
/* Rawwork +0/+8/+C/+18/+1C; logicalwork +190/+1A0/+1A8/+1B8/+1B0.
 * Hostintegration writes these widths explicitly, never PS2 long/pointer casts. */
typedef struct {uint32_t raw_held,raw_pressed,raw_released,raw_repeat_fast,raw_repeat_menu;uint64_t held,pressed,released,repeat_fast,repeat_menu;int focused,connected;} gh_native_input_snapshot;
typedef struct {gh_native_input_config config;gh_native_input_map map;uint32_t keyboard,previous;uint8_t fast[32],menu[32];gh_native_input_snapshot snapshot;} gh_native_input;
/* Physical device names, not semantic confirm/cancel. Hostdefaults Z=Cross,
 * X=Circle, arrows, Enter=Start, Escape=Triangle; XInput A/B/dpad/Start/Y. Configurablehostpolicy. */
void gh_native_input_defaults(gh_native_input_config*);
/* Borrow actual64u32 table at3C3D50 (0) /3C3E50 (1) from bounded ELF32LOAD.
 * Originaltable1 if language!=0 or debugFlags594&800; else0.
 * Caller selects original table policy and keeps bytes alive. */
int gh_native_input_map_from_elf(const void*,size_t,unsigned table,gh_native_input_map*);
int gh_native_input_init(gh_native_input*,const gh_native_input_config*,gh_native_input_map);
void gh_native_input_focus(gh_native_input*,int focused);
/* The configured native keyboard/XInput adapter supplies port0. Focus loss
 * releases keys but does not unplug it. Explicit source removal is separate. */
void gh_native_input_connected(gh_native_input*,int connected);
/* Store one actual native sample, including original31B560 byte14 readiness
 * from31AF40. This is a platform input mapping, never a modal-gate bypass. */
int gh_native_input_store_work(gh_boot_work *,const gh_native_input_snapshot *);
void gh_native_input_key(gh_native_input*,unsigned virtual_key,int down);
/* One originalinput update perenginetick. xinputbuttons0 whenunavailable.
 * Recovered digital31B560 repeat +31AFE0 map only; noanalog/recording policies.
 * m=1/2 explicit(D747A09==1?2:1), not inferredfromhostkeyboard repeat. */
int gh_native_input_tick(gh_native_input*,uint16_t xinputbuttons,gh_native_input_snapshot*);
#ifdef __cplusplus
}
#endif
#endif
