/* Original source semantics from actual patched ELF, upstream MIT pin
 * eaae18d0dfab1f9a0df276ae5ca1581ad326faf3. See private evidence JSON. */
#include "sound_scene_exit.h"
#include <string.h>
static gh_sound_exit_status bad(gh_sound_scene_exit*s,uint32_t f,int detail,gh_sound_exit_status e){s->frontier=f;s->detail=detail;return e;}
static int valid(const gh_sound_scene_exit*s){return s&&s->scene&&s->scene->control&&s->scene->control->initialized&&s->scene->registry&&s->bgm&&s->voices;}
gh_sound_exit_status gh_sound_scene_exit_bind(gh_sound_scene_exit*s,gh_sound_scene*scene,gh_bgm_requests*b,gh_se_voice_engine*v){
 if(!s||!scene||!scene->startup_bound||!scene->control||!scene->control->initialized||!b||!v||!gh_bgm_requests_uses_owners(b,scene->registry,scene->control)||!gh_se_voice_uses_registry(v,scene->registry))return GH_SOUND_EXIT_ARGUMENT;
 *s=(gh_sound_scene_exit){.scene=scene,.bgm=b,.voices=v};return GH_SOUND_EXIT_OK;
}
gh_sound_exit_status gh_sound_scene_exit_begin_002caa00(gh_sound_scene_exit*s){
 if(!valid(s))return GH_SOUND_EXIT_ARGUMENT;if(s->begun)return GH_SOUND_EXIT_BUSY;s->frontier=0;
 gh_sound_control*c=s->scene->control;c->flags_b0=(c->flags_b0&0xffc7ffffu)|0x800000;c->flags_ac=(c->flags_ac&0xffc7ffffu)|0x800000;
 gh_voice_status ve=gh_se_voice_scene_exit_begin(s->voices);if(ve)return bad(s,gh_se_voice_frontier(s->voices),ve,ve==GH_VOICE_UNSUPPORTED?GH_SOUND_EXIT_UNSUPPORTED:GH_SOUND_EXIT_NATIVE);
 gh_bgm_status be=gh_bgm_requests_scene_exit_begin(s->bgm);if(be)return bad(s,gh_bgm_requests_frontier(s->bgm),be,be==GH_BGM_UNSUPPORTED?GH_SOUND_EXIT_UNSUPPORTED:GH_SOUND_EXIT_NATIVE);
 /* Exact2CB070 clears +28,+24,+20, not +2C linked-list head. */
 s->scene->ts=(gh_bundle_entry){0};s->scene->bmh=(gh_bundle_entry){0};s->scene->seh=(gh_bundle_entry){0};
 ve=gh_se_voice_bind_origin_seh(s->voices,NULL,0);if(ve)return bad(s,0x2cb070,ve,GH_SOUND_EXIT_NATIVE);
 s->begun=1;s->frontier=0;return GH_SOUND_EXIT_OK;
}
gh_sound_exit_status gh_sound_scene_exit_wait_002cab58(gh_sound_scene_exit*s,int*out){
 if(!valid(s)||!out||!s->begun)return GH_SOUND_EXIT_ARGUMENT;int ready;gh_bgm_status e=gh_bgm_requests_scene_exit_ready(s->bgm,&ready);
 if(e)return bad(s,0x2cab58,e,GH_SOUND_EXIT_NATIVE);*out=ready&&!gh_se_voice_active_nodes(s->voices);s->frontier=0;return GH_SOUND_EXIT_OK;
}
static gh_sound_exit_status release_bank(gh_sound_scene_exit*s,int16_t bank){
 if(gh_se_voice_bank_nodes(s->voices,bank))return bad(s,0x2cbe68,bank,GH_SOUND_EXIT_UNSUPPORTED);
 gh_sound_span a,b,c,d;gh_audio_status e=gh_audio_bank_views(s->scene->registry,bank,&a,&b,&c,&d);
 if(e==GH_AUDIO_NOT_FOUND)return GH_SOUND_EXIT_OK;if(e)return bad(s,0x2cf310,e,GH_SOUND_EXIT_NATIVE);
 /* Current caller waited until the real EE voice list was empty. Active
  * bank unregister/IOP consequences remain precise2CBE68 unsupported above. */
 e=gh_audio_release_bank(s->scene->registry,bank);if(e)return bad(s,0x2cf310,e,GH_SOUND_EXIT_NATIVE);return GH_SOUND_EXIT_OK;
}
gh_sound_exit_status gh_sound_scene_exit_finalize_002ca910(gh_sound_scene_exit*s){
 static const int16_t banks[12]={1,7,8,9,10,11,12,13,16,17,18,19};
 if(!valid(s)||!s->begun)return GH_SOUND_EXIT_ARGUMENT;s->frontier=0;
 if(s->scene->loading.pointer||s->scene->job)return bad(s,0x2cf310,0,GH_SOUND_EXIT_BUSY);
 if(s->final_stage==0){s->scene->control->flags_b0&=0xff7fffffu;s->scene->control->flags_ac&=0xff7fffffu;
  gh_sound_exit_status e=release_bank(s,2);if(e)return e;s->scene->bank2=(gh_audio_token){0};s->final_stage=1;}
 if(s->final_stage==1){for(unsigned i=0;i<12;i++){gh_sound_exit_status e=release_bank(s,banks[i]);if(e)return e;}s->final_stage=2;}
 while(s->final_stage<4){unsigned index=s->final_stage-2;gh_sound_heap_registry_owner*heap=&s->scene->control->room_heaps[index];
  uint32_t expected=index?0x603310:0x6036a0;
  if(!heap->identity_verified||heap->registry!=s->scene->registry||heap->original_global!=expected)return bad(s,0x2cd780,0,GH_SOUND_EXIT_ARGUMENT);
  for(int16_t bank=0;bank<48;bank++){int listed=bank==1||(bank>=7&&bank<=13)||(bank>=16&&bank<=19);int belongs=index?(bank!=0&&bank!=3&&!listed):listed;
   if(belongs){gh_sound_span a,b,c,d;gh_audio_status e=gh_audio_bank_views(s->scene->registry,bank,&a,&b,&c,&d);
    if(e==GH_AUDIO_OK)return bad(s,0x2cd890,bank,GH_SOUND_EXIT_UNSUPPORTED);
    if(e!=GH_AUDIO_NOT_FOUND)return bad(s,0x2cd780,e,GH_SOUND_EXIT_NATIVE);}
  }heap->closed=1;s->final_stage++;
 }
 if(s->final_stage==4){gh_bgm_status e=gh_bgm_requests_scene_exit_finalize(s->bgm);if(e)return bad(s,gh_bgm_requests_frontier(s->bgm),e,GH_SOUND_EXIT_NATIVE);s->final_stage=5;}
 s->scene->control->unload_a4=0;s->scene->ready=0;s->frontier=0;return GH_SOUND_EXIT_OK;
}
