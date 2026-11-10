#include "game_player_child_attach.h"
#include <string.h>
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
gh_title_player_status gh_game_player_child_attach_00100ed8(gh_player_child_attachment*s,gh_game_player_child*c,gh_game_player_factory*p,gh_actor_model*m,int32_t index,gh_core_save_view save){
 if(!s||!c||!c->complete||!c->object.view.bytes)return GH_PLAYER_ARGUMENT;
 if(!p)return GH_PLAYER_OK;
 if(!m||!p->complete||!p->b.player||m->root.bytes!=p->b.player->view.bytes||c->model.root.bytes!=c->object.view.bytes||!save.bytes||save.size<GH_CORE_SAVE_BYTES)return GH_PLAYER_ARGUMENT;
 void*identity=NULL;if(gh_room_object_lookup(c->b.pools,c->handle,GH_ROOM_OBJECT_PLAYER_CHILD,&c->object,&identity)||identity!=&c->object)return GH_PLAYER_ARGUMENT;
 const gh_title_player_view*parent_view=NULL;if(gh_game_player_factory_scalar_view(p,p->handle,&parent_view)||parent_view!=&p->b.player->view)return GH_PLAYER_ARGUMENT;
 if(!c->model.parts_allocated||index<0||(uint32_t)index>=m->parts_allocated)return GH_PLAYER_OK;
 const gh_actor_part*child,*parent;if(gh_actor_model_part_at(&c->model,0,&child)||gh_actor_model_part_at(m,(unsigned)index,&parent))return GH_PLAYER_ARGUMENT;
 if(!child->bytes||!parent->bytes)return GH_PLAYER_OK;
 *s=(gh_player_child_attachment){c,p,m,child,parent,index,1};
 ww(child->bytes+0x148,0);ww(c->object.view.bytes+0x20d4,(uint32_t)index);ww(c->object.view.bytes+0x20a8,0);ww(c->object.view.bytes+0x20d0,0);
 unsigned costume=gh_core_save_costume(save);ww(c->object.view.bytes+0x2088,costume==2||costume==3?11:costume==4||costume==5?7:12);return GH_PLAYER_OK;
}
void gh_game_player_child_attachment_clear(gh_player_child_attachment*s){if(s)*s=(gh_player_child_attachment){0};}
