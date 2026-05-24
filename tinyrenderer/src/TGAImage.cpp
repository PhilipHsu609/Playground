#include "tinyrenderer/TGAImage.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <fmt/core.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

TGAImage::TGAImage(const char *filename) { loadTgaData(filename); }

TGAImage::TGAImage(std::uint16_t width, std::uint16_t height, std::uint8_t bytespp)
    : width_(width), height_(height), bytespp_(bytespp),
      data_(static_cast<size_t>(width) * height * bytespp) {}

void TGAImage::save(const char *filename) const { writeTgaData(filename); }

void TGAImage::savePng(const char *filename) const {
    // Internal byte order is B, G, R(, A) (see TGAColor::toArgb + memcpy in
    // set), but PNG expects R, G, B(, A); swap channels 0 and 2 per pixel.
    std::vector<std::uint8_t> rgb(static_cast<size_t>(width_) * height_ * bytespp_);
    for (size_t i = 0; i < static_cast<size_t>(width_) * height_; ++i) {
        const size_t base = i * bytespp_;
        rgb[base + 0] = data_[base + 2];
        rgb[base + 1] = data_[base + 1];
        rgb[base + 2] = data_[base + 0];
        if (bytespp_ == 4) {
            rgb[base + 3] = data_[base + 3];
        }
    }
    const int stride = static_cast<int>(width_) * bytespp_;
    if (stbi_write_png(filename, width_, height_, bytespp_, rgb.data(), stride) == 0) {
        throw std::runtime_error(std::string("stbi_write_png failed: ") + filename);
    }
}

void TGAImage::flipVertically() {
    if (data_.empty()) {
        return;
    }

    const size_t half = height_ >> 1;
    const size_t stride = static_cast<size_t>(width_) * bytespp_;
    std::uint8_t *start = data_.data();

    for (size_t i = 0; i < half; ++i) {
        std::uint8_t *line1 = &start[i * stride];
        std::uint8_t *line2 = &start[(height_ - 1 - i) * stride];
        std::swap_ranges(line1, line1 + stride, line2);
    }
}

void TGAImage::flipHorizontally() {
    if (data_.empty()) {
        return;
    }

    const size_t stride = static_cast<size_t>(width_) * bytespp_;
    std::uint8_t *start = data_.data();

    for (size_t i = 0; i < height_; ++i) {
        for (size_t j = 0; j < width_ / 2; ++j) {
            const size_t index1 = i * stride + j * bytespp_;
            const size_t index2 = i * stride + (width_ - 1 - j) * bytespp_;
            for (size_t k = 0; k < bytespp_; ++k) {
                std::swap(start[index1 + k], start[index2 + k]);
            }
        }
    }
}

// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
void TGAImage::loadTgaData(const char *filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file: {}", filename));
    }

    TGAHeader header{};
    file.read(reinterpret_cast<char *>(&header), sizeof(TGAHeader));

    width_ = header.width;
    height_ = header.height;
    bytespp_ = header.pixelDepth >> 3;

    if (width_ == 0 || height_ == 0 ||
        (bytespp_ != Format::GRAYSCALE && bytespp_ != Format::RGB &&
         bytespp_ != Format::RGBA)) {
        throw std::runtime_error(fmt::format("Invalid TGA file: {}", filename));
    }

    data_.resize(static_cast<size_t>(width_) * height_ * bytespp_);

    if (header.imageType == 2 || header.imageType == 3) {
        file.read(reinterpret_cast<char *>(data_.data()),
                  static_cast<std::streamsize>(width_) * height_ * bytespp_);
        if (!file.good()) {
            throw std::runtime_error(
                fmt::format("An error occurred while reading the file: {}", filename));
        }
    } else if (header.imageType == 10 || header.imageType == 11) {
        loadRleData(file);
    } else {
        throw std::runtime_error(fmt::format("Unsupported TGA file type: {}", filename));
    }

    if ((header.imageDescriptor & 0x20) == 0) {
        flipVertically();
    }
    if ((header.imageDescriptor & 0x10) != 0) {
        flipHorizontally();
    }

    fmt::print("Loaded TGA image: {} ({}x{}x{})\n", filename, width_, height_,
               bytespp_ * 8);
}

void TGAImage::writeTgaData(const char *filename, bool rle) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file: {}", filename));
    }

    TGAHeader header{};
    if (bytespp_ == Format::GRAYSCALE) {
        header.imageType = rle ? 11 : 3;
    } else {
        header.imageType = rle ? 10 : 2;
    }
    header.width = width_;
    header.height = height_;
    header.pixelDepth = bytespp_ * 8;
    header.imageDescriptor = (1 << 5);

    file.write(reinterpret_cast<const char *>(&header), sizeof(TGAHeader));

    if (rle) {
        writeRleData(file);
    } else {
        file.write(reinterpret_cast<const char *>(data_.data()),
                   static_cast<std::streamsize>(data_.size()));
    }
}

void TGAImage::loadRleData(std::ifstream &file) {
    const size_t pixelCount = static_cast<size_t>(width_) * height_;
    size_t currentPixel = 0;
    std::uint8_t *ptr = data_.data();

    while (currentPixel < pixelCount) {
        std::uint8_t chunkHeader = 0;
        file.read(reinterpret_cast<char *>(&chunkHeader), 1);
        if (!file.good()) {
            throw std::runtime_error("An error occurred while reading the file");
        }

        if (chunkHeader < 128) {
            ++chunkHeader;
            for (size_t i = 0; i < chunkHeader; ++i) {
                file.read(reinterpret_cast<char *>(ptr), bytespp_);
                if (!file.good()) {
                    throw std::runtime_error("An error occurred while reading the file");
                }
                ++currentPixel;
                ptr += bytespp_;
            }
        } else {
            chunkHeader -= 127;
            std::array<std::uint8_t, 4> pixel{};
            file.read(reinterpret_cast<char *>(pixel.data()), bytespp_);
            if (!file.good()) {
                throw std::runtime_error("An error occurred while reading the file");
            }
            for (size_t i = 0; i < chunkHeader; ++i) {
                std::copy_n(pixel.data(), bytespp_, ptr);
                ++currentPixel;
                ptr += bytespp_;
            }
        }
    }
}

void TGAImage::writeRleData(std::ofstream &file) const {
    const size_t pixelCount = static_cast<size_t>(width_) * height_;
    const std::uint8_t maxChunkLength = 128;
    size_t currentPixel = 0;

    while (currentPixel < pixelCount) {
        size_t chunkLength = 1;
        std::array<std::uint8_t, 4> chunkValue{};
        std::copy_n(&data_[currentPixel * bytespp_], bytespp_, chunkValue.data());
        bool stop = false;

        while (currentPixel + chunkLength < pixelCount && chunkLength < maxChunkLength) {
            for (size_t i = 0; i < bytespp_; ++i) {
                if (data_[(currentPixel + chunkLength) * bytespp_ + i] != chunkValue[i]) {
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
        auto header = static_cast<std::uint8_t>(chunkLength + 127);
        file.write(reinterpret_cast<const char *>(&header), 1);
        file.write(reinterpret_cast<const char *>(chunkValue.data()), bytespp_);
    }
}
// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
