#ifndef GH_GAME_EVENT_LINK_H
#define GH_GAME_EVENT_LINK_H
#include "game_event_components.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_EVENT_BINDING_IDENTITY_ONLY,GH_EVENT_BINDING_COMPONENT,
 GH_EVENT_BINDING_SET_EVENT_NO,GH_EVENT_BINDING_STATIC_CTOR} gh_event_binding_kind;
typedef struct {const char *name;uint32_t original_address,first_reloc_site;
 uint16_t index;uint8_t imported,identity_resolved,callable_native;gh_event_binding_kind native_kind;} gh_event_symbol_binding;
typedef struct {uint32_t site,addend,resolved_original_address;uint16_t symbol;uint8_t kind;} gh_event_reloc_binding;
/* ONE Event module-global namespace. The15 mutable component banks andthree
 * mutable staticVF0 vectors are kept together; callers must borrow
 * &owner.components, not allocate a second Event-global bank. Module/bootELF
 * source bytes outlive this owner. Original addresses are diagnostic identities,
 * never hostfunctionpointers. Identity resolution does NOT imply callable code. */
typedef struct {gh_event_components components;uint32_t vectors_cf30_cf50[3][4];
 gh_event_symbol_binding symbols[393];gh_event_reloc_binding relocations[518];const gh_event_module *module;
 uint32_t symbol_count,import_count,reloc_count,constructor_calls,frontier,module_job;
 uint16_t failed_symbol;uint8_t frontier_relative,bound,constructors_done;
} gh_event_link;
/* Native297450/2BE890 boundary: allrequired REL imports resolve againstreal
 * MainSNR2 identities, everyreloc is bounded/typed,15 originaldata banks bind
 * once, actualsoleCtor46B8->4688 performs allthreeVF0 writes exactlyonce.
 * Unknown ctor/import fails beforebound/flags80. This does NOT execute guest
 * code and never makes unrecovered codeexports/imports callable. Originalcreate
 * owns flags80 after this returns; loader itself doesnotset Event flags. */
gh_game_event_status gh_game_event_link_002be890(gh_event_link *,const gh_event_module *,const void *boot_elf,size_t);
gh_game_event_status gh_game_event_link_symbol(const gh_event_link *,const char *,gh_event_symbol_binding *);
/* Only known typed function export, no generic guestpc dispatch. */
gh_game_event_status gh_game_event_link_set_event_no_000054b8(gh_event_link *,int32_t);
/* For futureconsumers: unknown function cannot be accidentallyacknowledged.
 * Always unsupported for identities without a recovered nativebody; no casts. */
gh_game_event_status gh_game_event_link_require_native(const gh_event_link *,const char *,uint32_t *frontier,int *relative);
#ifdef __cplusplus
}
#endif
#endif
