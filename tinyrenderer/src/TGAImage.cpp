#include "tinyrenderer/TGAImage.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

TGAImage::TGAImage(const char *filename) { load_tga_data(filename); }

TGAImage::TGAImage(std::uint16_t width, std::uint16_t height, std::uint8_t bytespp)
    : width(width), height(height), bytespp(bytespp),
      data(static_cast<size_t>(width) * height * bytespp) {}

void TGAImage::save(const char *filename) const { write_tga_data(filename); }

void TGAImage::flipVertically() {
    if (data.empty()) {
        return;
    }

    const size_t half = height >> 1;
    const size_t stride = static_cast<size_t>(width) * bytespp;
    std::vector<std::uint8_t> buffer(stride);

    std::uint8_t *start = data.data();

    for (size_t i = 0; i < half; ++i) {
        std::uint8_t *line1 = &start[i * stride];
        std::uint8_t *line2 = &start[(height - 1 - i) * stride];
        std::copy(line1, line1 + stride, buffer.data());
        std::copy(line2, line2 + stride, line1);
        std::copy(buffer.begin(), buffer.end(), line2);
    }
}

void TGAImage::flipHorizontally() {
    if (data.empty()) {
        return;
    }

    const size_t stride = static_cast<size_t>(width) * bytespp;
    std::uint8_t *start = data.data();

    for (size_t i = 0; i < height; ++i) {
        for (size_t j = 0; j < width / 2; ++j) {
            size_t index1 = i * stride + j * bytespp;
            size_t index2 = i * stride + (width - 1 - j) * bytespp;
            for (size_t k = 0; k < bytespp; ++k) {
                std::swap(start[index1 + k], start[index2 + k]);
            }
        }
    }
}

void TGAImage::load_tga_data(const char *filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file: {}", filename));
    }

    TGAHeader header{};
    file.read(reinterpret_cast<char *>(&header), sizeof(TGAHeader));

    width = header.width;
    height = header.height;
    bytespp = header.pixelDepth >> 3;

    if (width <= 0 || height <= 0 ||
        (bytespp != Format::GRAYSCALE && bytespp != Format::RGB &&
         bytespp != Format::RGBA)) {
        throw std::runtime_error(fmt::format("Invalid TGA file: {}", filename));
    }

    data.resize(static_cast<size_t>(width) * height * bytespp);

    if (header.imageType == 2 || header.imageType == 3) {
        file.read(reinterpret_cast<char *>(data.data()),
                  static_cast<std::streamsize>(width) * height * bytespp);
        if (!file.good()) {
            throw std::runtime_error(
                fmt::format("An error occurred while reading the file: {}", filename));
        }
    } else if (header.imageType == 10 || header.imageType == 11) {
        load_rle_data(file);
    } else {
        throw std::runtime_error(fmt::format("Unsupported TGA file type: {}", filename));
    }

    if ((header.imageDescriptor & 0x20) == 0) {
        flipVertically();
    }
    if ((header.imageDescriptor & 0x10) != 0) {
        flipHorizontally();
    }

    fmt::print("Loaded TGA image: {} ({}x{}x{})\n", filename, width, height, bytespp * 8);
}

void TGAImage::write_tga_data(const char *filename, bool rle) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file: {}", filename));
    }

    TGAHeader header{};
    header.imageType = (rle ? (bytespp == Format::GRAYSCALE ? 11 : 10)
                            : (bytespp == Format::GRAYSCALE ? 3 : 2));
    header.width = width;
    header.height = height;
    header.pixelDepth = bytespp * 8;
    header.imageDescriptor = (1 << 5);

    file.write(reinterpret_cast<const char *>(&header), sizeof(TGAHeader));

    if (rle) {
        write_rle_data(file);
    } else {
        file.write(reinterpret_cast<const char *>(data.data()),
                   static_cast<std::streamsize>(data.size()));
    }
}

void TGAImage::load_rle_data(std::ifstream &file) {
    const size_t pixelCount = static_cast<size_t>(width) * height;
    size_t currentPixel = 0;
    std::uint8_t *ptr = data.data();

    while (currentPixel < pixelCount) {
        std::uint8_t chunkHeader = 0;
        file.read(reinterpret_cast<char *>(&chunkHeader), 1);
        if (!file.good()) {
            throw std::runtime_error("An error occurred while reading the file");
        }

        if (chunkHeader < 128) {
            ++chunkHeader;
            for (size_t i = 0; i < chunkHeader; ++i) {
                file.read(reinterpret_cast<char *>(ptr), bytespp);
                if (!file.good()) {
                    throw std::runtime_error("An error occurred while reading the file");
                }
                ++currentPixel;
                ptr += bytespp;
            }
        } else {
            chunkHeader -= 127;
            std::array<std::uint8_t, 4> pixel{};
            file.read(reinterpret_cast<char *>(pixel.data()), bytespp);
            if (!file.good()) {
                throw std::runtime_error("An error occurred while reading the file");
            }
            for (size_t i = 0; i < chunkHeader; ++i) {
                std::copy_n(pixel.data(), bytespp, ptr);
                ++currentPixel;
                ptr += bytespp;
            }
        }
    }
}

void TGAImage::write_rle_data(std::ofstream &file) const {
    const size_t pixelCount = static_cast<size_t>(width) * height;
    const std::uint8_t maxChunkLength = 128;
    size_t currentPixel = 0;

    while (currentPixel < pixelCount) {
        size_t chunkLength = 1;
        std::array<std::uint8_t, 4> chunkValue{};
        std::copy_n(&data[currentPixel * bytespp], bytespp, chunkValue.data());
        bool stop = false;

        while (currentPixel + chunkLength < pixelCount && chunkLength < maxChunkLength) {
            for (size_t i = 0; i < bytespp; ++i) {
                if (data[(currentPixel + chunkLength) * bytespp + i] != chunkValue[i]) {
                    stop = true;
                    break;
                }
            }
            if (stop) {
                break;
            }
            ++chunkLength;
        }

        currentPixel += chunkLength;
        chunkLength += 127;
        file.write(reinterpret_cast<const char *>(&chunkLength), 1);
        file.write(reinterpret_cast<const char *>(chunkValue.data()), bytespp);
    }
}