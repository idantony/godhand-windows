#include "ui_polygon_renderer.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

namespace {
constexpr unsigned width = 512, height = 448;
constexpr size_t pixel_count = width * height;
constexpr uint32_t write_z = 0xffff;
constexpr uint8_t unchanged_rgba[] = {45, 90, 135, 2};

void check(bool condition, const char *label) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", label);
        std::exit(EXIT_FAILURE);
    }
}

gh_id_texture texture_description(unsigned log_size) {
    gh_id_texture texture{};
    texture.width = texture.height = 1u << log_size;
    texture.tex0 = (uint64_t{20} << 20) | (uint64_t{log_size} << 26) | (uint64_t{log_size} << 30);
    return texture;
}

std::vector<uint8_t> texture_pixels(unsigned size) {
    std::vector<uint8_t> pixels(size * size * 4);
    for (unsigned y = 0; y < size; ++y) {
        for (unsigned x = 0; x < size; ++x) {
            uint8_t *pixel = pixels.data() + (y * size + x) * 4;
            std::copy_n(unchanged_rgba, 3, pixel);
            // Upload converts preview alpha to raw GS alpha, so these texels
            // arrive at the exact rejection threshold (1) and just above it (2).
            pixel[3] = x == 0 ? 2 : 4;
        }
    }
    return pixels;
}

struct Surfaces {
    std::vector<uint8_t> background, output;
    std::vector<uint32_t> before, depth;

    Surfaces() : background(pixel_count * 4), output(pixel_count * 4, 0xa5), before(pixel_count), depth(pixel_count) {
        for (size_t i = 0; i < pixel_count; ++i) {
            std::copy_n(unchanged_rgba, 4, background.data() + i * 4);
            before[i] = static_cast<uint32_t>((i * 137 + 19) % write_z);
        }
        depth = before;
    }

    template<class Coverage>
    void verify(const char *label, Coverage passing) const {
        check(output == background, "RGBA must be byte-identical to background");
        size_t writes = 0;
        for (unsigned y = 0; y < height; ++y) {
            for (unsigned x = 0; x < width; ++x) {
                const size_t i = y * width + x;
                const bool hit = passing(static_cast<int>(x), static_cast<int>(y));
                const uint32_t expected = hit ? write_z : before[i];
                if (depth[i] != expected) {
                    std::fprintf(stderr, "%s at (%u,%u): depth=%08X expected=%08X\n", label, x, y,
                                 (unsigned)depth[i], (unsigned)expected);
                    std::exit(EXIT_FAILURE);
                }
                writes += hit;
            }
        }
        check(writes > 0 && writes < pixel_count, "fixture includes writes and untouched pixels");
        std::printf("PASS %s: %zu depth writes, %zu retained, all RGBA unchanged\n", label, writes, pixel_count - writes);
    }
};

void sprite(const gh_ui_sprite_state &state) {
    gh_ui_sprite_renderer *raw = nullptr;
    check(gh_ui_sprite_create(width, height, &raw) == GH_UI_OK, "create sprite renderer");
    std::unique_ptr<gh_ui_sprite_renderer, decltype(&gh_ui_sprite_destroy)> renderer(raw, gh_ui_sprite_destroy);
    const auto description = texture_description(5);
    const auto pixels = texture_pixels(description.width);
    uint32_t texture = 0;
    check(gh_ui_sprite_upload(raw, &description, pixels.data(), pixels.size(), &texture) == GH_UI_OK, "upload PSMT4 32x32 profile");
    gh_ui_sprite_item items[2]{};
    for (unsigned i = 0; i < 2; ++i) {
        auto &quad = items[i].quad;
        items[i].texture = texture;
        quad.texture = description;
        quad.x0 = 10.5f + i * 12;
        quad.x1 = 34.5f + i * 12;
        quad.y0 = 20.25f;
        quad.y1 = 36.75f;
        // Sample exactly at a texel center in the original bilinear profile.
        quad.u0 = quad.u1 = (0.5f + i) / description.width;
        quad.v0 = quad.v1 = 0.5f / description.height;
        quad.color = 0x80808080;
        quad.color_mode = 6;
        quad.font_mode = 5;
    }
    std::swap(items[0], items[1]); // Reject after pass in the overlapping region.
    Surfaces surfaces;
    gh_ui_depth_write depth{surfaces.depth.data(), surfaces.depth.size(), write_z};
    check(gh_ui_sprite_batch_with_depth(raw, &state, items, 2, surfaces.background.data(), surfaces.output.data(),
                                      surfaces.output.size(), &depth) == GH_UI_OK, "draw sprite depth batch");
    // Half-open rectangle and integer sample centers give these integer bounds.
    surfaces.verify("sprite", [](int x, int y) { return x >= 23 && x <= 46 && y >= 21 && y <= 36; });
}

void polygon(const gh_ui_sprite_state &state) {
    gh_ui_polygon_renderer *raw = nullptr;
    check(gh_ui_polygon_create(width, height, &raw) == GH_UI_OK, "create polygon renderer");
    std::unique_ptr<gh_ui_polygon_renderer, decltype(&gh_ui_polygon_destroy)> renderer(raw, gh_ui_polygon_destroy);
    const auto description = texture_description(6);
    const auto pixels = texture_pixels(description.width);
    uint32_t texture = 0;
    check(gh_ui_polygon_upload(raw, &description, pixels.data(), pixels.size(), &texture) == GH_UI_OK,
          "upload PSMT4 64x64 profile");
    gh_ui_polygon_item items[2]{};
    const float x[] = {220, 244, 196, 220};
    const float y[] = {90, 114, 114, 138};
    for (unsigned i = 0; i < 2; ++i) {
        auto &quad = items[i].polygon;
        items[i].texture = texture;
        quad.texture = description;
        quad.color = 0x80808080;
        quad.color_mode = 6;
        quad.font_mode = 5;
        for (unsigned vertex = 0; vertex < 4; ++vertex) {
            quad.x[vertex] = x[vertex] + 12 * i;
            quad.y[vertex] = y[vertex];
            quad.u[vertex] = (0.5f + i) / description.width;
            quad.v[vertex] = 0.5f / description.height;
        }
    }
    std::swap(items[0], items[1]);
    Surfaces surfaces;
    gh_ui_depth_write depth{surfaces.depth.data(), surfaces.depth.size(), write_z};
    gh_ui_depth_write invalid{depth.pixels, depth.count - 1, depth.z};
    check(gh_ui_polygon_batch_with_depth(raw, &state, items, 2, surfaces.background.data(), surfaces.output.data(),
                                       surfaces.output.size(), &invalid) == GH_UI_ARGUMENT, "reject short depth plane");
    check(surfaces.depth == surfaces.before && std::all_of(surfaces.output.begin(), surfaces.output.end(),
          [](uint8_t value) { return value == 0xa5; }), "invalid depth leaves both outputs intact");
    check(gh_ui_polygon_batch_with_depth(raw, &state, items, 2, surfaces.background.data(), surfaces.output.data(),
                                       surfaces.output.size(), &depth) == GH_UI_OK, "draw polygon depth batch");
    // Invert the diamond's two basis vectors (24,24),(-24,24). The outer
    // edges admit 0<=u<1, 0<v<=1. This does not use triangle edge functions.
    surfaces.verify("polygon", [](int px, int py) {
        const int dx = px - 232, dy = py - 90;
        return dx + dy >= 0 && dx + dy < 48 && dy - dx > 0 && dy - dx <= 48;
    });
}
}

int main() {
    const gh_ui_sprite_state state{UINT64_C(0x8000000044), 0x3001d, 0x54, 0x60, 0, 1, 1, 0, 0, 0};
    sprite(state);
    polygon(state);
}
