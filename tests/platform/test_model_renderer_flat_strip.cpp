#include "../../src/platform/model_renderer.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

static void check(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

int main() {
    constexpr unsigned width = 512, height = 448;
    constexpr size_t pixels = width * height;
    gh_model_renderer *raw = nullptr;
    check(gh_model_renderer_create(width, height, &raw) == GH_MODEL_GPU_OK, "create renderer");
    std::unique_ptr<gh_model_renderer, decltype(&gh_model_renderer_destroy)> renderer(raw, gh_model_renderer_destroy);

    const uint8_t texel[] = {80, 100, 120, 64};
    gh_stage_material material{};
    material.source = material.rgba_gs = texel;
    material.source_bytes = material.rgba_bytes = sizeof(texel);
    material.cache_id = 1;
    material.width = material.height = 1;
    material.psm = 19;
    material.draw_tcc = 1;
    uint32_t texture = 0;
    check(gh_model_renderer_upload(raw, &material, &texture) == GH_MODEL_GPU_OK, "upload source texel");

    gh_static_gs_vertex vertices[4]{};
    for (unsigned i = 0; i < 4; ++i) {
        vertices[i].xyz_fixed4[0] = (4 + (i & 1) * 8) * 16;
        vertices[i].xyz_fixed4[1] = (4 + (i >> 1) * 8) * 16;
        vertices[i].xyz_fixed4[2] = 384 * 16;
        vertices[i].stq[2] = 1;
        for (auto &channel : vertices[i].rgba) channel = 128;
    }
    const uint32_t indices[] = {0, 1, 2, 2, 1, 3};
    const uint32_t wrong_indices[] = {0, 1, 2, 0, 1, 3};
    gh_model_draw_state state{};
    state.primitive = 0x54;
    state.test = 0x5000d; // Alpha GREATER 0, Z GEQUAL.
    state.alpha = 0x8000000044; // (Cs - Cd) * As / 128 + Cd.
    state.tex1 = 0x60;
    state.zbuf_psm = 58;
    state.z_write = state.colclamp = state.native_float_reference = 1;

    constexpr uint8_t old_rgb[3] = {40, 60, 80};
    constexpr uint8_t new_rgb[3] = {60, 80, 100};
    std::vector<uint8_t> input(pixels * 4), output(input.size());
    std::vector<uint32_t> depth(pixels);
    for (size_t i = 0; i < pixels; ++i) {
        std::copy_n(old_rgb, 3, input.data() + i * 4);
        input[i * 4 + 3] = static_cast<uint8_t>(i ^ (i >> 9));
    }
    auto begin = [&](uint32_t z) {
        check(gh_model_renderer_begin(raw, 0, z) == GH_MODEL_GPU_OK, "begin scene");
        check(gh_model_renderer_replace_color(raw, input.data(), input.size()) == GH_MODEL_GPU_OK, "seed destination pixels");
    };
    auto read = [&]() {
        check(gh_model_renderer_readback(raw, output.data(), output.size(), depth.data(), depth.size()) == GH_MODEL_GPU_OK,
              "read result");
    };
    auto draw = [&]() { return gh_model_renderer_draw(raw, &state, texture, vertices, 4, indices, 6); };

    begin(192);
    state.frame_mask = 0x00ff0000;
    check(draw() == GH_MODEL_GPU_UNSUPPORTED, "reject unsupported RGB mask");
    state.frame_mask = 0xff000000;
    state.primitive = 0x5c;
    check(draw() == GH_MODEL_GPU_UNSUPPORTED, "reject alpha mask on old Gouraud path");
    state.primitive = 0x54;
    vertices[3].rgba[0] = 127;
    check(draw() == GH_MODEL_GPU_UNSUPPORTED, "reject unequal flat colors");
    vertices[3].rgba[0] = 128;
    check(gh_model_renderer_draw(raw, &state, texture, vertices, 4, wrong_indices, 6) == GH_MODEL_GPU_UNSUPPORTED,
          "reject different triangle topology");
    read();
    check(output == input && std::all_of(depth.begin(), depth.end(), [](uint32_t z) { return z == 192; }),
          "rejected draws preserve color and Z");

    for (uint32_t mask : {0u, 0xff000000u}) {
        begin(192);
        state.frame_mask = mask;
        check(draw() == GH_MODEL_GPU_OK, "draw original flat strip");
        read();
        for (unsigned y = 0; y < height; ++y) {
            for (unsigned x = 0; x < width; ++x) {
                const size_t i = y * width + x;
                const bool covered = x >= 4 && x < 12 && y >= 4 && y < 12;
                const uint8_t *rgb = covered ? new_rgb : old_rgb;
                check(std::equal(rgb, rgb + 3, output.data() + i * 4), "RGB blend and one contribution on the shared edge");
                check(output[i * 4 + 3] == (covered && mask == 0 ? 64 : input[i * 4 + 3]), "whole alpha byte write mask");
                check(depth[i] == (covered ? 384u : 192u), "depth independent of framebuffer alpha mask");
            }
        }
    }

    begin(512);
    check(draw() == GH_MODEL_GPU_OK, "strip with failed depth test");
    read();
    check(output == input && std::all_of(depth.begin(), depth.end(), [](uint32_t z) { return z == 512; }), "GEQUAL rejection");
    begin(192);
    state.test = 0x5040d; // Alpha GREATER 64 rejects equality, AFAIL KEEP.
    check(draw() == GH_MODEL_GPU_OK, "strip with failed alpha test");
    read();
    check(output == input && std::all_of(depth.begin(), depth.end(), [](uint32_t z) { return z == 192; }), "alpha KEEP rejection");
    std::puts("PASS original flat STQ strip, RGB-only mask, blending, alpha and depth tests");
}
