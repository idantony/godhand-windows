/* Original2AABA0 readable leaf, patched SLUS_215.03. */
#include "id_tick.h"
gh_id_error gh_id_tick_entries_002aaba0(gh_id_base*b){if(!b)return GH_ID_ARGUMENT;if(!b->playing||!b->entry_count||b->entry_count>=0x80000000u)return GH_ID_OK;if(!b->entries)return GH_ID_BOUNDS;for(uint32_t i=0;i<b->entry_count;i++)for(unsigned j=0;j<5;j++){uint16_t n=(uint16_t)((uint16_t)b->entries[i].jitter[j]+1u);b->entries[i].jitter[j]=(int16_t)(n<32768?(int)n:(int)n-65536);}return GH_ID_OK;}
