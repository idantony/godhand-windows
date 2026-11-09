/* Copyright (c) 2026 Lucas Picoli and god-hand-decomp contributors.
 * MIT: LICENSE.upstream. Source pin eaae18d0dfab1f9a0df276ae5ca1581ad326faf3.
 * Actual patched instruction ranges and boundaries: menu-state-evidence.json.
 */
#include "menu_state.h"
#include <stddef.h>
#include <string.h>
/* Assertions check the recovered PS2 interpretation only. Native state uses
 * full host pointers in bounded views and is deliberately a different layout. */
typedef struct menu_layout32 {
    uint32_t data;uint8_t unknown04[8];uint32_t texture;
    uint8_t unknown10[0xbda-0x10];int16_t timer;
    uint8_t unknownbdc[0x14];uint8_t state;
} menu_layout32;
typedef struct id_layout32 {
    uint32_t pairs[32][2];uint8_t jobs[0x640];uint32_t job_count;
    uint8_t pool[0x1ae00];uint32_t pool_used;
} id_layout32;
_Static_assert(offsetof(menu_layout32,texture)==0xc,"menu texture slot");
_Static_assert(offsetof(menu_layout32,timer)==0xbda,"menu timer");
_Static_assert(offsetof(menu_layout32,state)==0xbf0,"menu state");
_Static_assert(offsetof(id_layout32,jobs)==0x100,"ID jobs");
_Static_assert(offsetof(id_layout32,job_count)==0x740,"ID count");
_Static_assert(offsetof(id_layout32,pool)==0x744,"ID pool");
_Static_assert(offsetof(id_layout32,pool_used)==0x1b544,"ID pool cursor");
static gh_menu_error translate(gh_bundle_error error)
{
    switch(error) {
    case GH_BUNDLE_OK:return GH_MENU_OK;
    case GH_BUNDLE_ARGUMENT:return GH_MENU_ARGUMENT;
    case GH_BUNDLE_NOT_FOUND:return GH_MENU_MISSING;
    case GH_BUNDLE_BOUNDS:return GH_MENU_BOUNDS;
    default:return GH_MENU_UNSUPPORTED;
    }
}
gh_menu_error gh_menu_load_tables_002ae990(gh_menu_state *state,const gh_bundle *language)
{
    gh_bundle_error error;
    if(!state || !language)return GH_MENU_ARGUMENT;
    state->tables_bound=0;
    error=gh_bundle_find(language,"MSD",0,&state->data);
    if(error)return translate(error);
    error=gh_bundle_find(language,"MSD",1,&state->texture_data);
    if(error)return translate(error);
    state->timer=900;state->state=0;state->tables_bound=1;
    return GH_MENU_OK;
}
gh_menu_error gh_id_find_language_data(const gh_bundle *language,gh_bundle_entry *out)
{if(!language || !out)return GH_MENU_ARGUMENT;return translate(gh_bundle_find(language,"IDD",0,out));}
gh_menu_error gh_id_system_init_002acc70(gh_id_state *state,const gh_bundle_entry *idd)
{
    gh_menu_error error;
    if(!state || !idd || !idd->bytes)return GH_MENU_ARGUMENT;
    memset(state->jobs,0,sizeof(state->jobs));
    state->pool_used=0;state->job_count=0;
    memset(state->pair,0,sizeof(state->pair));state->initialized=0;
    error=gh_id_set_data_002accf8(state,1,idd);
    if(error)return error;
    state->initialized=1;
    return GH_MENU_OK;
}
gh_menu_error gh_id_set_data_002accf8(gh_id_state *state,uint32_t slot,const gh_bundle_entry *idd)
{
    gh_bundle table;gh_bundle_error error;
    if(!state || !idd || !idd->bytes)return GH_MENU_ARGUMENT;
    if(slot>=32)return GH_MENU_BOUNDS;
    state->pair[slot].bound=0;
    error=gh_bundle_open(idd->bytes,idd->size,&table);
    if(error)return translate(error);
    error=gh_bundle_find(&table,"TBL",0,&state->pair[slot].data);
    if(error)return translate(error);
    error=gh_bundle_find(&table,"TBL",1,&state->pair[slot].texture_data);
    if(error)return translate(error);
    state->pair[slot].bound=1;
    return GH_MENU_OK;
}
gh_menu_error gh_id_bind_global(gh_id_state **destination,gh_id_state *manager)
{
    if(!destination || !manager)return GH_MENU_ARGUMENT;
    *destination=manager;return GH_MENU_OK;
}
