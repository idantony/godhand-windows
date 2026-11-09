/* Recoverednormalexit: mcportMain280->DeInit140/7280/83E8, ELF2AE8F0
 * DllFree2AE5E8 thenbusybitclear andtailrestart2AE91C. MITupstreamnotice
 * LICENSE.upstream; actualbinary/relocationevidence analysis/mc-exit-evidence.json. */
#include "mc_exit.h"
#include <string.h>
static int valid(const gh_mc_task_bindings*b){return b&&b->dvd&&b->module_heap&&b->subscreen_heap&&b->threads&&b->card&&b->id_manager&&b->id_runtime&&b->fonts&&b->packed&&b->language&&b->work&&b->game_suspend_word;}
gh_mc_task_status gh_mc_exit_run_final_frame(gh_mc_exit_state*x,gh_mc_control_state*c,
 gh_mc_task_state*s,const gh_mc_task_bindings*b,gh_id_post_message post,void*post_context,gh_mc_draw_consumer draw,void*draw_context){
 int original;uint32_t word;if(!x||!c||!s||!valid(b)||!draw)return GH_MC_TASK_ARGUMENT;
 if(x->stage==GH_MC_EXIT_FINISHED)return GH_MC_TASK_BUSY;if(x->stage>GH_MC_EXIT_FINISHED)return GH_MC_TASK_ARGUMENT;
 if(x->stage==GH_MC_EXIT_FINAL_FRAME){if(x->frame_attempted)return GH_MC_TASK_ID_ERROR;memcpy(&word,(const unsigned char*)b->work+0x5e8,4);if(s->stage!=4||!s->ui_initialized||!s->checks.done||!(word&UINT32_C(0x80000000)))return GH_MC_TASK_BUSY;x->frame_attempted=1;x->frontier_address=0x8aa8;x->id_error=gh_mc_control_sleep_frame(c,s,b,post,post_context,draw,draw_context);if(x->id_error){x->frontier_address=c->first_unported?c->first_unported:0x2d5aa0;return GH_MC_TASK_ID_ERROR;}x->frame_completed=1;x->stage=GH_MC_EXIT_ICONS;}
 if(x->stage==GH_MC_EXIT_ICONS){x->frontier_address=0x140;if(s->checks.icon_work.pointer){x->memory_error=gh_heap_release(b->subscreen_heap,s->checks.icon_work);if(x->memory_error)return GH_MC_TASK_MEMORY_ERROR;memset(&s->checks.icon_work,0,sizeof(s->checks.icon_work));}x->stage=GH_MC_EXIT_SUSPEND;}
 if(x->stage==GH_MC_EXIT_SUSPEND){x->frontier_address=0x7280;if(s->flags&0x08000000)*b->game_suspend_word=0;x->stage=GH_MC_EXIT_ID_BASE;}
 if(x->stage==GH_MC_EXIT_ID_BASE){x->frontier_address=0x83e8;if(s->id_resource.allocation.pointer){x->id_error=gh_id_base_release_002aa078(b->id_runtime,&s->id_base,&original,&x->pool_reclaimed);if(x->id_error)return GH_MC_TASK_ID_ERROR;s->ui_initialized=0;x->stage=GH_MC_EXIT_ID_PAIR;}else x->stage=GH_MC_EXIT_SYS_FLAGS;}
 if(x->stage==GH_MC_EXIT_ID_PAIR){memset(&b->id_manager->pair[0xb],0,sizeof(b->id_manager->pair[0xb]));s->id_resource.pair_bound=0;x->stage=GH_MC_EXIT_ID_RESOURCE;}
 if(x->stage==GH_MC_EXIT_ID_RESOURCE){x->memory_error=gh_heap_release(b->subscreen_heap,s->id_resource.allocation);if(x->memory_error)return GH_MC_TASK_MEMORY_ERROR;memset(&s->id_resource.allocation,0,sizeof(s->id_resource.allocation));memset(&s->id_resource.image,0,sizeof(s->id_resource.image));if(b->packed)memset(b->packed,0,sizeof(*b->packed));x->stage=GH_MC_EXIT_ID_FLAGS;}
 if(x->stage==GH_MC_EXIT_ID_FLAGS){s->flags&=UINT32_C(0xafffffff);x->stage=GH_MC_EXIT_SYS_FLAGS;}
 if(x->stage==GH_MC_EXIT_SYS_FLAGS){b->work->flags&=~UINT32_C(0x4000);s->mc_sys_initialized=0;x->stage=GH_MC_EXIT_CARD_END;}
 if(x->stage==GH_MC_EXIT_CARD_END){x->frontier_address=0x3148;if(s->module_loaded){x->card_error=gh_memory_card_end(b->card);if(x->card_error==GH_MC_BUSY)return GH_MC_TASK_BUSY;if(x->card_error)return GH_MC_TASK_PLATFORM_ERROR;}x->stage=GH_MC_EXIT_UNLINK;}
 if(x->stage==GH_MC_EXIT_UNLINK){x->frontier_address=0x394f20;/* Nativemodulebindinghasnoforeignexecutableimports: invalidateownedbinding. */s->module_loaded=0;x->stage=GH_MC_EXIT_MODULE_FREE;}
 if(x->stage==GH_MC_EXIT_MODULE_FREE){x->frontier_address=0x2ae614;if(s->module.pointer){x->memory_error=gh_heap_release(b->module_heap,s->module);if(x->memory_error)return GH_MC_TASK_MEMORY_ERROR;memset(&s->module,0,sizeof(s->module));}x->stage=GH_MC_EXIT_BUSY_CLEAR;}
 if(x->stage==GH_MC_EXIT_BUSY_CLEAR){memcpy(&word,(const unsigned char*)b->work+0x5e8,4);word&=~UINT32_C(0x80000000);memcpy((unsigned char*)b->work+0x5e8,&word,4);s->stage=5;x->tail_entry=0x30da08;x->frontier_address=0x2ae91c;x->body_finished=1;x->stage=GH_MC_EXIT_FINISHED;}
 return GH_MC_TASK_OK;
}
