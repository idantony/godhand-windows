#include "game_scene_ui.h"
gh_scene_ui_status gh_scene_font_bind_002a4c38(gh_font_state*s,const gh_bundle*b,int32_t language){
 if(!s||!b)return GH_SCENE_UI_ARGUMENT;gh_bundle_entry mst={0},msa={0};
 gh_bundle_error a=gh_bundle_find(b,"MST",(unsigned)language,&mst),c=gh_bundle_find(b,"MSA",(unsigned)language,&msa);
 if((a&&a!=GH_BUNDLE_NOT_FOUND)||(c&&c!=GH_BUNDLE_NOT_FOUND))return GH_SCENE_UI_BOUNDS;
 gh_font_error e=gh_font_bind_pair_002a4cb0(s,2,&mst,&msa);return e?(e==GH_FONT_UNSUPPORTED?GH_SCENE_UI_UNSUPPORTED:GH_SCENE_UI_BOUNDS):GH_SCENE_UI_OK;
}
gh_scene_ui_status gh_scene_message_initialize_002aea18(gh_game_scene_ui*s,gh_menu_state*root,const gh_bundle*b,int32_t language){
 if(!s||!root||!b)return GH_SCENE_UI_ARGUMENT;if(s->root&&s->root!=root)return GH_SCENE_UI_ARGUMENT;
 s->root=root;gh_bundle_entry data={0};gh_bundle_error e=gh_bundle_find(b,"MSD",(unsigned)language,&data);
 if(e&&e!=GH_BUNDLE_NOT_FOUND){s->frontier=0x31ccf0;return GH_SCENE_UI_BOUNDS;}s->room_data=data;
 for(unsigned i=0;i<16;i++){
  if(i<15)s->blocks[i+1].previous=(int32_t)i; /*2B22E8 firstwritesnext.prev*/
  s->blocks[i].flags=s->blocks[i].mode=0;s->blocks[i].previous=-1;s->blocks[i].next=i<15?(int32_t)i+1:-1;
 }
 s->generation_bd8=1;s->free_head=0;s->active_head=-1;s->ready=1;s->frontier=0;return GH_SCENE_UI_OK;
}
