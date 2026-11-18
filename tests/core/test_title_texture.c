#include "../../src/core/id_jobs.h"
#include "../../src/platform/disc.h"
#include "../../src/assets/core_bundle.h"
#include "../../src/assets/font_texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define C(x) do { if (!(x)) { fprintf(stderr, "FAIL%d %s\n", __LINE__, #x); exit(1); } } while (0)

static uint32_t word(const unsigned char *p)
{
    return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static unsigned bit(unsigned value, unsigned index)
{
    return (value >> index) & 1u;
}

static gh_bundle_entry read_asset(gh_disc *disc, const char *path)
{
    gh_disc_entry entry;
    size_t slot;
    C(gh_disc_find(disc, path, &slot) == GH_DISC_OK);
    C(gh_disc_entry_at(disc, slot, &entry) == GH_DISC_OK);
    unsigned char *data = malloc((size_t)entry.size);
    C(data && gh_disc_read(disc, slot, 0, data, (size_t)entry.size) == GH_DISC_OK);
    return (gh_bundle_entry){.bytes = data, .size = (size_t)entry.size};
}

static void verify_texture(const gh_id_texture *texture, const char *label, const char *export_path)
{
    const unsigned width = texture->width, height = texture->height;
    const unsigned pitch = width / 128;
    const unsigned pixels = width * height, rgba_bytes = pixels * 4;
    unsigned char *rgba = malloc(rgba_bytes);
    unsigned char *expected = malloc(rgba_bytes);
    unsigned char *seen = calloc(pixels, 1);
    C(rgba && expected && seen);
    C(gh_id_texture_rgba(texture, rgba, rgba_bytes) == GH_ID_OK);

    gh_font_slot packet = {.bound = 1, .mst = texture->source.bytes, .mst_size = texture->source.size};
    for (unsigned i = 0; i < 2; ++i) {
        packet.table_offset[i] = word(packet.mst + 8 + i * 4);
        const unsigned char *count = packet.mst + 16 + i * 2;
        packet.table_count[i] = (uint16_t)(count[0] | (unsigned)count[1] << 8);
        C(packet.table_count[i] == 1);
    }
    gh_font_transfer image, clut;
    C(gh_font_transfer_at(&packet, 0, 0, &image) == GH_FONT_OK);
    C(gh_font_transfer_at(&packet, 1, 0, &clut) == GH_FONT_OK);
    C(image.width == 64 && image.height == pixels / 512 && image.pixel_bytes == pixels / 2);
    C(image.psm == 0 && image.destination_base == 0 && image.buffer_width == 1);
    C(clut.width == 16 && clut.height == 16 && clut.pixel_bytes == 1024);
    C(clut.psm == 0 && clut.destination_base == 0 && clut.buffer_width == 1);

    /* Walk physical nibbles in order and invert the GS bitplanes. This oracle
     * does not use the production forward addresses or share their permutation. */
    for (unsigned address = 0; address < pixels; ++address) {
        unsigned page = address >> 14;
        unsigned x_in_page = bit(address, 3) | (bit(address, 5) << 1) |
            ((bit(address, 6) ^ bit(address, 0) ^ bit(address, 7)) << 2) |
            (bit(address, 1) << 3) | (bit(address, 2) << 4) |
            (bit(address, 10) << 5) | (bit(address, 12) << 6);
        unsigned y_in_page = bit(address, 4) | (bit(address, 0) << 1) |
            (bit(address, 7) << 2) | (bit(address, 8) << 3) | (bit(address, 9) << 4) |
            (bit(address, 11) << 5) | (bit(address, 13) << 6);
        unsigned x = (page % pitch) * 128 + x_in_page;
        unsigned y = (page / pitch) * 128 + y_in_page;

        unsigned upload_word = address >> 3;
        unsigned ux = bit(upload_word, 0) | (bit(upload_word, 2) << 1) |
            (bit(upload_word, 3) << 2) | (bit(upload_word, 6) << 3) |
            (bit(upload_word, 8) << 4) | (bit(upload_word, 10) << 5);
        unsigned uy = (bit(upload_word, 1) | (bit(upload_word, 4) << 1) |
            (bit(upload_word, 5) << 2) | (bit(upload_word, 7) << 3) |
            (bit(upload_word, 9) << 4)) + (upload_word >> 11) * 32;
        C(x < width && y < height && ux < image.width && uy < image.height);
        unsigned pixel = y * width + x;
        C(!seen[pixel]);
        seen[pixel] = 1;

        unsigned packed = image.pixels[((size_t)uy * image.width + ux) * 4 + (address % 8) / 2];
        unsigned index = (packed >> (bit(address, 0) * 4)) & 15u;
        const unsigned char *color = clut.pixels + ((index % 8) + (index / 8) * 16) * 4;
        unsigned char *out = expected + (size_t)pixel * 4;
        C(color[3] <= 128);
        memcpy(out, color, 3);
        out[3] = color[3] == 128 ? 255 : (unsigned char)(color[3] * 2);
    }

    C(!memcmp(rgba, expected, rgba_bytes));
    uint64_t hash = UINT64_C(14695981039346656037);
    unsigned alpha_max = 0, nonzero = 0;
    for (unsigned i = 0; i < rgba_bytes; ++i)
        hash = (hash ^ rgba[i]) * UINT64_C(1099511628211);
    for (unsigned i = 0; i < 256; ++i)
        if (clut.pixels[i * 4 + 3] > alpha_max)
            alpha_max = clut.pixels[i * 4 + 3];
    for (unsigned i = 0; i < pixels; ++i)
        nonzero += rgba[i * 4 + 3] != 0;
    if (export_path) {
        FILE *file = fopen(export_path, "wbx");
        C(file && fwrite(rgba, 1, rgba_bytes, file) == rgba_bytes && !fclose(file));
    }
    printf("PASS %s: %u logical pixels/physical nibbles bijective, independent inverse CT32 upload and CSM1 all match; RGBA fnv%016llx alphaMax%u nontransparent%u\n",
           label, pixels, (unsigned long long)hash, alpha_max, nonzero);
    free(seen);
    free(expected);
    free(rgba);
}

int main(int argc, char **argv)
{
    gh_disc *disc;
    C(argc == 2 || argc == 3);
    C(gh_disc_open(argv[1], &disc) == GH_DISC_OK);
    gh_id_state *manager = calloc(1, sizeof(*manager));
    C(manager);

    gh_bundle_entry title = read_asset(disc, "id/local/us/idtitlecore_us.idd");
    gh_id_texture texture;
    C(gh_id_set_data_002accf8(manager, 2, &title) == GH_MENU_OK);
    C(gh_id_texture_open(manager, 2, 7, &texture) == GH_ID_OK);
    C(texture.width == 512 && texture.height == 512 && texture.tex0 == UINT64_C(0x2000000265420000));
    verify_texture(&texture, "actual Title Logo T32 PSMT4", argc == 3 ? argv[2] : NULL);

    gh_bundle_entry language_asset = read_asset(disc, "etc/core_usa.bin");
    gh_bundle language;
    gh_bundle_entry idd;
    C(gh_bundle_open(language_asset.bytes, language_asset.size, &language) == GH_BUNDLE_OK);
    C(gh_id_find_language_data(&language, &idd) == GH_MENU_OK);
    C(gh_id_system_init_002acc70(manager, &idd) == GH_MENU_OK);
    C(gh_id_texture_open(manager, 1, 44, &texture) == GH_ID_OK);
    C(texture.width == 256 && texture.height == 256 && texture.source.size == 34272);
    C(texture.tex0 == UINT64_C(0x2000000221410000));
    verify_texture(&texture, "actual HUD T32 PSMT4 slot 1 item 44", NULL);

    free((void *)language_asset.bytes);
    free((void *)title.bytes);
    free(manager);
    gh_disc_close(disc);
    return 0;
}
