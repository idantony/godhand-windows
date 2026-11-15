#include "game_enemy_row2.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr,"line %d: %s\n",__LINE__,#expr); return 1; } } while (0)

static uint8_t *load(gh_disc *disc,const char *path,size_t *extent)
{
    size_t index;
    gh_disc_entry entry;
    if (gh_disc_find(disc,path,&index) || gh_disc_entry_at(disc,index,&entry)) return NULL;
    uint8_t *bytes=malloc((size_t)entry.size);
    if (!bytes || gh_disc_read(disc,index,0,bytes,(size_t)entry.size)) {
        free(bytes);
        return NULL;
    }
    *extent=(size_t)entry.size;
    return bytes;
}

int main(int argc,char **argv)
{
    CHECK(argc==2);
    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1],&disc));
    size_t extent;
    uint8_t *bytes=load(disc,"em/em02.dat",&extent);
    CHECK(bytes&&extent==4681920);
    gh_object_view view={.kind=0x202,.bytes=bytes,.file_extent=extent,.size=extent,.file_extent_known=1};
    gh_enemy_request request={.object_kind=0x202,.dependency_kind=0x200,.appearance=2,.entry_number=3,.flags=0x04000000u};
    gh_enemy_initial_motion_selection selected;
    CHECK(!gh_enemy202_appearance2_select_00203c38(&request,&view,&selected));
    CHECK(selected.motion_offset==0x212e00&&selected.sequence_offset==0x2154c0);
    CHECK(!selected.main_state&&selected.substate==0x71&&!selected.phase&&!selected.phase_detail);

    /* Physical file extent controls bounds, including a truncated real SEQ. */
    const gh_enemy_initial_motion_selection before=selected;
    view.file_extent=selected.sequence_offset+3;
    CHECK(gh_enemy202_appearance2_select_00203c38(&request,&view,&selected)==GH_ACTOR_MODEL_BOUNDS);
    CHECK(!memcmp(&selected,&before,sizeof(selected)));
    view.file_extent=extent;
    request.dependency_flags=1;
    CHECK(gh_enemy202_appearance2_select_00203c38(&request,&view,&selected)==GH_ACTOR_MODEL_UNSUPPORTED);
    CHECK(!memcmp(&selected,&before,sizeof(selected)));

    unsigned occurrence=99;
    CHECK(!gh_enemy_model_occurrence_002ddea0(0x202,0x200,&occurrence)&&occurrence==0);
    CHECK(!gh_enemy_model_occurrence_002ddea0(0x202,0x20a,&occurrence)&&occurrence==2);
    free(bytes);
    bytes=load(disc,"em/ema2.dat",&extent);
    CHECK(bytes&&extent==778784);
    gh_bundle bundle;
    CHECK(!gh_bundle_open(bytes,extent,&bundle));
    const size_t models[]={0xee20,0x32ee0,0x577e0};
    const size_t textures[]={0x320,0x243e0,0x48460};
    for (unsigned i=0;i<3;i++) {
        gh_bundle_entry md,tm3;
        CHECK(!gh_enemy_model_occurrence_002ddea0(0x2a2,0x2a2+i,&occurrence)&&occurrence==i);
        CHECK(!gh_bundle_find(&bundle,"MD",occurrence,&md)&&md.offset==models[i]);
        CHECK(!gh_bundle_find(&bundle,"TM3",occurrence,&tm3)&&tm3.offset==textures[i]);
    }
    CHECK(gh_enemy_model_occurrence_002ddea0(0x2a2,0x2a5,&occurrence)==GH_ACTOR_MODEL_UNSUPPORTED&&occurrence==2);
    free(bytes);
    gh_disc_close(disc);
    puts("PASS actual EM202 appearance2 motion offsets, extent guards, and EMA2 model occurrences");
    return 0;
}
