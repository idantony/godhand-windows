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
    constexpr uint32_t initial_z = 192, next_z = 384;
    constexpr size_t pixels = width * height;
    std::vector<uint8_t> image(pixels * 4), readback(image.size());
    std::vector<uint32_t> depth(pixels);
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            const size_t i = (y * width + x) * 4;
            image[i] = static_cast<uint8_t>(x + y);
            image[i + 1] = static_cast<uint8_t>(x >> 1);
            image[i + 2] = static_cast<uint8_t>(y);
            image[i + 3] = static_cast<uint8_t>(255 - (x ^ y));
        }
    }

    gh_model_renderer *raw = nullptr;
    check(gh_model_renderer_create(width, height, &raw) == GH_MODEL_GPU_OK, "create renderer");
    std::unique_ptr<gh_model_renderer, decltype(&gh_model_renderer_destroy)> renderer(raw, gh_model_renderer_destroy);
    check(gh_model_renderer_replace_color(raw, image.data(), image.size()) == GH_MODEL_GPU_ARGUMENT, "reject before begin");
    check(gh_model_renderer_begin(raw, 0, initial_z) == GH_MODEL_GPU_OK, "begin scene");

    const uint8_t texel[] = {11, 33, 55, 177};
    gh_stage_material material{};
    material.source = material.rgba_gs = texel;
    material.source_bytes = material.rgba_bytes = sizeof(texel);
    material.width = material.height = 1;
    material.psm = 19;
    material.draw_tcc = 1;
    uint32_t first_texture = 0, second_texture = 0, cached_texture = 0;
    material.cache_id = 1;
    check(gh_model_renderer_upload(raw, &material, &first_texture) == GH_MODEL_GPU_OK, "first cached material");
    material.cache_id = 2;
    check(gh_model_renderer_upload(raw, &material, &second_texture) == GH_MODEL_GPU_OK && second_texture != first_texture,
          "second cached material");

    check(gh_model_renderer_replace_color(raw, image.data(), image.size()) == GH_MODEL_GPU_OK, "replace full color");
    check(gh_model_renderer_replace_color(raw, image.data(), image.size() - 1) == GH_MODEL_GPU_ARGUMENT, "reject short image");
    check(gh_model_renderer_replace_color(raw, nullptr, image.size()) == GH_MODEL_GPU_ARGUMENT, "reject null pixels");
    check(gh_model_renderer_readback(raw, readback.data(), readback.size(), nullptr, 0) == GH_MODEL_GPU_OK && readback == image,
          "color-only exact RGBA after upload and rejected writes");
    check(gh_model_renderer_readback(raw, readback.data(), readback.size(), depth.data(), depth.size()) == GH_MODEL_GPU_OK,
          "read existing depth");
    check(std::all_of(depth.begin(), depth.end(), [](uint32_t z) { return z == initial_z; }), "depth survives color replacement");
    check(gh_model_renderer_upload(raw, &material, &cached_texture) == GH_MODEL_GPU_OK && cached_texture == second_texture,
          "second cached material handle survives replacement");

    gh_static_gs_vertex vertices[3]{};
    vertices[1].xyz_fixed4[0] = 8 * 16;
    vertices[2].xyz_fixed4[1] = 8 * 16;
    for (auto &vertex : vertices) {
        vertex.xyz_fixed4[2] = next_z * 16;
        vertex.stq[2] = 1;
        for (auto &channel : vertex.rgba) channel = 128;
    }
    gh_model_draw_state state{};
    state.primitive = 0x1b;
    state.test = 0x50000; // ZTE and GEQUAL.
    state.zbuf_psm = 58;
    state.z_write = state.colclamp = state.native_float_reference = 1;
    const uint32_t indices[] = {0, 1, 2};
    check(gh_model_renderer_draw(raw, &state, second_texture, vertices, 3, indices, 3) == GH_MODEL_GPU_OK, "draw after replacement");
    check(gh_model_renderer_readback(raw, readback.data(), readback.size(), depth.data(), depth.size()) == GH_MODEL_GPU_OK,
          "read after draw");
    constexpr size_t covered = 2 * width + 2, outside = 30 * width + 30;
    check(std::equal(texel, texel + 4, readback.data() + covered * 4) && depth[covered] == next_z, "draw writes replacement target");
    check(std::equal(image.data() + outside * 4, image.data() + outside * 4 + 4, readback.data() + outside * 4) &&
          depth[outside] == initial_z, "draw preserves uncovered replacement pixels and depth");
    std::puts("PASS scene RGBA replacement, preserved depth and material cache, subsequent draw");
}
