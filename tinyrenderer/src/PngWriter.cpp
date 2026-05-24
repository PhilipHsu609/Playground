#include "tinyrenderer/PngWriter.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

void writePng(const TGAImage &image, const char *filename) {
    const auto w = image.getWidth();
    const auto h = image.getHeight();
    const auto bpp = image.getBytespp();
    const auto &src = image.buffer();

    // TGAImage stores pixels in B, G, R(, A) byte order (see TGAColor::toArgb
    // and memcpy in TGAImage::set). PNG wants R, G, B(, A), so swap channels
    // 0 and 2 per pixel into a scratch buffer before writing.
    std::vector<std::uint8_t> rgb(static_cast<size_t>(w) * h * bpp);
    for (size_t i = 0; i < static_cast<size_t>(w) * h; ++i) {
        const size_t base = i * bpp;
        rgb[base + 0] = src[base + 2]; // R
        rgb[base + 1] = src[base + 1]; // G
        rgb[base + 2] = src[base + 0]; // B
        if (bpp == 4) {
            rgb[base + 3] = src[base + 3];
        }
    }

    const int stride = static_cast<int>(w) * bpp;
    if (stbi_write_png(filename, w, h, bpp, rgb.data(), stride) == 0) {
        throw std::runtime_error(std::string("stbi_write_png failed: ") + filename);
    }
}
