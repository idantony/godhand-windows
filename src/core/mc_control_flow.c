/* ActualControl2E8 dispatch recovery, pinnedSNR2evidence in
 * analysis/mc-control-flow-evidence.json. Existingphaseconsumersremainowners
 * oftheiractualsourceeffects; noalternativeMCdecisionpolicyintroduced. */
#include "mc_control_flow.h"
#include <string.h>
static gh_mc_boot_error fail(gh_mc_control_flow*f,gh_mc_boot_error e,uint32_t address){f->failure=e;f->first_unported=address;return e;}
static gh_mc_boot_error display(gh_mc_control_flow*f,gh_mc_control_state*c,gh_mc_task_state*s,const gh_mc_task_bindings*b){
 gh_id_error e;if(s->checks.mode==2)e=gh_mc_control_set_boot_display_000072c8(c,s,b);else e=gh_mc_error_set_display(&f->error,c,s,b);
 if(e)return fail(f,GH_MC_BOOT_UNSUPPORTED,f->error.first_unported?f->error.first_unported:(c->first_unported?c->first_unported:0x72c8));return GH_MC_BOOT_OK;
}
gh_mc_boot_error gh_mc_control_flow_advance(gh_mc_control_flow*f,gh_mc_control_state*c,
 gh_mc_task_state*s,const gh_mc_task_bindings*b,gh_mc_sound_dispatch sound,void*sound_context){
 gh_mc_boot_error e;uint32_t phase;if(!f||!c||!s||!b||!b->card||!b->work||!b->id_runtime||!s->ui_initialized)return GH_MC_BOOT_ARGUMENT;if(f->failure)return f->failure;if(f->delay_remaining)return GH_MC_BOOT_NEEDS_SLEEP;if(s->checks.done)return GH_MC_BOOT_ARGUMENT;
 f->first_unported=0;c->first_unported=0;phase=s->checks.phase;
 if(phase<2){e=gh_mc_control_advance(c,s,b);if(e==GH_MC_BOOT_NEEDS_SLEEP){if(!s->checks.delay_frames)return fail(f,GH_MC_BOOT_ARGUMENT,0x398);f->delay_remaining=s->checks.delay_frames;return e;}if(e)return fail(f,e,c->first_unported?c->first_unported:(phase?0x5d0:0x398));}
 else{if(phase==2){uint32_t first=0;e=gh_mc_file_load_00000730(&s->checks,b->card,b->work,f->original_f8,&first);if(e)return fail(f,e,first?first:0x730);}else if(phase==3){e=gh_mc_error_process_00000bd8(&f->error,c,s,b,sound,sound_context);if(e)return fail(f,e,f->error.first_unported?f->error.first_unported:0xbd8);}
  /* Originalswitchhasnodefaultworkforotherphasevalues, butstillSetDisp. */
  e=display(f,c,s,b);if(e)return e;++s->checks.frame;
 }
 ++f->completed_controls;return GH_MC_BOOT_OK;
}
gh_mc_boot_error gh_mc_control_flow_delay_frame(gh_mc_control_flow*f,gh_mc_control_state*c,
 gh_mc_task_state*s,const gh_mc_task_bindings*b,gh_id_post_message post,void*post_context,gh_mc_draw_consumer draw,void*draw_context){
 gh_id_error id;gh_mc_boot_error e;uint32_t busy;if(!f||!c||!s||!b||!b->work||!b->threads||!draw)return GH_MC_BOOT_ARGUMENT;if(f->failure)return f->failure;if(!f->delay_remaining||!s->checks.delay_frames)return GH_MC_BOOT_ARGUMENT;memcpy(&busy,(const unsigned char*)b->work+0x5e8,4);if(!(busy&UINT32_C(0x80000000)))return fail(f,GH_MC_BOOT_ARGUMENT,0x8aa8);
 id=gh_mc_control_sleep_frame(c,s,b,post,post_context,draw,draw_context);if(id)return fail(f,GH_MC_BOOT_UNSUPPORTED,c->first_unported?c->first_unported:0x8aa8);--f->delay_remaining;++f->delay_frames_completed;
 if(f->delay_remaining)return GH_MC_BOOT_NEEDS_SLEEP;e=gh_mc_control_finish_delay(c,s,b);if(e)return fail(f,e,c->first_unported?c->first_unported:0x72c8);++f->completed_controls;return GH_MC_BOOT_OK;
}
