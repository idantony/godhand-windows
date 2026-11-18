#include "../../src/platform/mc_renderer.h"
#include "../../src/platform/disc.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL %d %s\n", __LINE__, #x); std::exit(1); } } while (0)

struct DrawRecord {
    std::vector<size_t> ids;
    bool saw_glyph_depth = false;
};

static gh_id_error scene_draw(void *opaque, size_t marker, uint8_t *rgba, size_t bytes,
    uint32_t *depth, size_t count)
{
    auto &record = *static_cast<DrawRecord *>(opaque);
    CHECK(count == 512u * 448u && bytes == count * 4);
    if (marker == 12) {
        CHECK(rgba[3] == 73 && depth[0] == 4112);
        rgba[3] = 201;
        depth[0] = 9000;
    } else {
        CHECK(rgba[3] == 201 && depth[0] == 9000);
        for (size_t i = 1; i < count; ++i)
            if (depth[i] == 65535) record.saw_glyph_depth = true;
        rgba[0] = 93;
    }
    record.ids.push_back(marker);
    return GH_ID_OK;
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    gh_disc *disc = nullptr;
    CHECK(!gh_disc_open(argv[1], &disc));
    std::vector<uint8_t> elf((size_t)gh_disc_boot_elf_size(disc));
    CHECK(!elf.empty() && !gh_disc_read_boot_elf(disc, 0, elf.data(), elf.size()));
    size_t file_index;
    gh_disc_entry entry;
    CHECK(!gh_disc_find(disc, "etc/core_usa.bin", &file_index) && !gh_disc_entry_at(disc, file_index, &entry));
    std::vector<uint8_t> language_bytes((size_t)entry.size);
    CHECK(!gh_disc_read(disc, file_index, 0, language_bytes.data(), language_bytes.size()));
    gh_disc_close(disc);
    gh_bundle language={};
    gh_font_state fonts={};
    CHECK(!gh_bundle_open(language_bytes.data(), language_bytes.size(), &language));
    CHECK(!gh_font_initialize(&fonts, &language, 1));

    gh_scene_message_glyph_pair pair={};
    pair.font=fonts.slots;
    pair.bank=6;
    pair.depth=2;
    pair.font_mode=0x21;
    gh_font_draw_state state={};
    state.scale_x=.81f;
    state.scale_y=1.1f;
    state.alignment_flags=9;
    state.screen_width=512;
    state.screen_height=448;
    state.gs_y_origin=1824;
    state.rgba=0x80808080;
    pair.normal.code=pair.shadow.code=14;
    CHECK(!gh_font_draw_quad(&fonts,&state,14,211.25f,187.125f,65535,&pair.normal.quad));
    state.rgba=0xff000000u;
    CHECK(!gh_font_draw_quad(&fonts,&state,14,213.25f,189.125f,65535,&pair.shadow.quad));
    gh_mc_renderer *renderer=nullptr;
    CHECK(!gh_mc_renderer_create(elf.data(),elf.size(),1,&renderer));
    gh_mc_renderer_begin(renderer);
    gh_ui_ot_tag tags[2];
    CHECK(!gh_mc_renderer_queue_message(renderer,&pair,tags));
    CHECK(tags[0].index==0&&tags[1].index==1);
    const size_t order[]={1,0}; // Original same-bucket LIFO.
    int required=-1;
    CHECK(!gh_mc_renderer_queued_message_depth(renderer,order,2,&required)&&required==1);
    const size_t count=512u*448u;
    std::vector<uint8_t> back(count*4,73);
    std::vector<uint32_t> depth(count,4112);
    CHECK(gh_mc_renderer_flush_queued_on_background(renderer,order,2,back.data(),back.size())==GH_ID_UNSUPPORTED);
    gh_mc_render_status status={};
    gh_mc_renderer_inspect(renderer,&status);
    CHECK(status.frontier==0x2a53a8&&!status.jobs);
    CHECK(!gh_mc_renderer_flush_queued_on_background_with_depth(renderer,order,2,back.data(),back.size(),depth.data(),depth.size()));
    size_t bytes;
    CHECK(std::memcmp(back.data(),gh_mc_renderer_pixels(renderer,&bytes),back.size())!=0&&bytes==back.size());

    gh_mc_renderer_begin(renderer);
    CHECK(!gh_mc_renderer_queue_message(renderer,&pair,tags));
    DrawRecord mixed;
    const gh_mc_scene_draw invalid[]={{2,90},{1,91}};
    CHECK(gh_mc_renderer_flush_queued_with_scene_draws(renderer,order,2,back.data(),back.size(),
        depth.data(),depth.size(),invalid,2,scene_draw,&mixed)==GH_ID_ARGUMENT);
    CHECK(mixed.ids.empty());
    const gh_mc_scene_draw draws[]={{0,12},{1,13},{2,14},{2,15}};
    CHECK(!gh_mc_renderer_flush_queued_with_scene_draws(renderer,order,2,back.data(),back.size(),
        depth.data(),depth.size(),draws,4,scene_draw,&mixed));
    CHECK(mixed.ids == (std::vector<size_t>{12,13,14,15}) && mixed.saw_glyph_depth);
    const auto *mixed_pixels=gh_mc_renderer_pixels(renderer,&bytes);
    CHECK(mixed_pixels[0]==93 && mixed_pixels[3]==201 && depth[0]==4112);
    puts("PASS interleaved scene draws preserve global order, RGBA alpha and glyph depth");

    gh_mc_renderer_begin(renderer);
    auto discarded=pair;
    discarded.normal.quad.rgba&=0x00ffffffu;
    discarded.shadow.quad.rgba&=0x00ffffffu;
    CHECK(!gh_mc_renderer_queue_message(renderer,&discarded,tags));
    CHECK(!gh_mc_renderer_queued_message_depth(renderer,order,2,&required)&&required==0);
    CHECK(!gh_mc_renderer_flush_queued_on_background(renderer,order,2,back.data(),back.size()));
    CHECK(!std::memcmp(back.data(),gh_mc_renderer_pixels(renderer,&bytes),back.size()));

    gh_mc_renderer_begin(renderer);
    gh_screen_fade fade={};
    const gh_screen_fade_context source_display={512,1824,0,0};
    const gh_ui_fade_context draw_display={512,448,1824,448.f/480.f,1,1,0,0,0};
    gh_fade_draw draw={};
    CHECK(!gh_screen_fade_start_00307208(&fade,&source_display,0,10,0,0x40204060,0x40204060,1));
    CHECK(!gh_screen_fade_step_00307388(&fade,&source_display,&draw)&&draw.visible);
    gh_ui_ot_tag unknown;
    CHECK(!gh_mc_renderer_queue_fade(renderer,&draw,&draw_display,&unknown));
    CHECK(!gh_mc_renderer_queue_message(renderer,&pair,tags));
    const size_t unsupported_order[]={unknown.index,tags[1].index,tags[0].index};
    CHECK(gh_mc_renderer_queued_message_depth(renderer,unsupported_order,3,&required)==GH_ID_UNSUPPORTED);
    CHECK(gh_mc_renderer_flush_queued_on_background_with_depth(renderer,unsupported_order,3,back.data(),back.size(),depth.data(),depth.size())==GH_ID_UNSUPPORTED);
    gh_mc_renderer_inspect(renderer,&status);
    CHECK(status.frontier==0x2a53a8&&!status.jobs);
    const gh_mc_scene_draw after_fade[]={{1,12}};
    DrawRecord blocked;
    CHECK(gh_mc_renderer_flush_queued_with_scene_draws(renderer,unsupported_order,3,
        back.data(),back.size(),depth.data(),depth.size(),after_fade,1,scene_draw,&blocked)==GH_ID_UNSUPPORTED);
    CHECK(blocked.ids.empty());
    gh_mc_renderer_destroy(renderer);
    puts("PASS actual ISO font Message depth request without Fade, discard-only no-depth, untracked predecessor rejected before drawing");
    return 0;
}
