#pragma once

#include <array>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

static_assert(true); // This is a workaround for a bug in clangd
#pragma pack(push, 1)
struct TGAHeader {
    std::uint8_t idLength;
    std::uint8_t colorMapType;
    std::uint8_t imageType;
    std::array<std::uint8_t, 5> colorMapSpec;
    std::uint16_t xOrigin;
    std::uint16_t yOrigin;
    std::uint16_t width;
    std::uint16_t height;
    std::uint8_t pixelDepth;
    std::uint8_t imageDescriptor;
};
#pragma pack(pop)

struct TGAColor {
    std::uint8_t b{}, g{}, r{}, a{};
    std::uint8_t bytespp = 1;

    constexpr TGAColor() = default;
    constexpr TGAColor(std::uint8_t r, std::uint8_t g, std::uint8_t b)
        : b(b), g(g), r(r), a(255), bytespp(3) {}
    constexpr TGAColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
        : b(b), g(g), r(r), a(a), bytespp(4) {}
    constexpr TGAColor(std::uint32_t argb, std::uint8_t bytespp) : bytespp(bytespp) {
        fromArgb(argb);
    }

    constexpr void fromArgb(std::uint32_t argb) {
        a = (argb >> 24) & 0xff;
        r = (argb >> 16) & 0xff;
        g = (argb >> 8) & 0xff;
        b = argb & 0xff;
    }

    [[nodiscard]] constexpr std::uint32_t toArgb() const {
        return static_cast<uint32_t>((a << 24) | (r << 16) | (g << 8) | b);
    }
};

class TGAImage {
  public:
    explicit TGAImage(const char *filename);
    TGAImage(std::uint16_t width, std::uint16_t height, std::uint8_t bytespp);
    void save(const char *filename) const;

    void flipVertically();
    void flipHorizontally();

    template <std::integral T>
    void set(T x, T y, TGAColor color) {
        if (static_cast<std::uint16_t>(x) >= width_ ||
            static_cast<std::uint16_t>(y) >= height_) {
            throw std::out_of_range("Coordinates out of bounds");
        }
        const size_t index = static_cast<size_t>(x) + static_cast<size_t>(y) * width_;
        std::uint32_t argb = color.toArgb();
        std::memcpy(&data_[index * bytespp_], &argb, bytespp_);
    }

    template <std::integral T>
    std::uint8_t get(T x, T y) const {
        if (static_cast<std::uint16_t>(x) >= width_ ||
            static_cast<std::uint16_t>(y) >= height_) {
            throw std::out_of_range("Coordinates out of bounds");
        }
        size_t index = static_cast<size_t>(x) + static_cast<size_t>(y) * width_;
        return data_[index * bytespp_];
    }

    [[nodiscard]] std::uint16_t getWidth() const { return width_; }
    [[nodiscard]] std::uint16_t getHeight() const { return height_; }
    [[nodiscard]] std::uint8_t getBytespp() const { return bytespp_; }
    std::vector<std::uint8_t> &buffer() { return data_; }

    enum Format { GRAYSCALE = 1, RGB = 3, RGBA = 4 };

  private:
    void loadTgaData(const char *filename);
    void writeTgaData(const char *filename, bool rle = true) const;
    void loadRleData(std::ifstream &file);
    void writeRleData(std::ofstream &file) const;

    std::uint16_t width_ = 0;
    std::uint16_t height_ = 0;
    std::uint8_t bytespp_ = 1;
    std::vector<std::uint8_t> data_;
};

// Common color constants
constexpr TGAColor WHITE_COLOR(255, 255, 255, 255);
constexpr TGAColor BLACK_COLOR(0, 0, 0, 255);
constexpr TGAColor RED_COLOR(255, 0, 0, 255);
constexpr TGAColor GREEN_COLOR(0, 255, 0, 255);
constexpr TGAColor BLUE_COLOR(0, 0, 255, 255);
