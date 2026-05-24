#pragma once

#include <algorithm>
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

    constexpr TGAColor() = default;
    constexpr TGAColor(std::uint8_t r, std::uint8_t g, std::uint8_t b)
        : b(b), g(g), r(r), a(255) {}
    constexpr TGAColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
        : b(b), g(g), r(r), a(a) {}

    constexpr void fromArgb(std::uint32_t argb) {
        a = (argb >> 24) & 0xff;
        r = (argb >> 16) & 0xff;
        g = (argb >> 8) & 0xff;
        b = argb & 0xff;
    }

    [[nodiscard]] constexpr std::uint32_t toArgb() const {
        return static_cast<std::uint32_t>((a << 24) | (r << 16) | (g << 8) | b);
    }

    // Channel-wise multiply, normalizing each channel to [0, 1] internally.
    // e.g. white * red = red; gray50 * white = gray50.
    [[nodiscard]] constexpr TGAColor operator*(const TGAColor &other) const {
        constexpr float inv = 1.f / 255.f;
        return {
            static_cast<std::uint8_t>(static_cast<float>(r) *
                                      static_cast<float>(other.r) * inv),
            static_cast<std::uint8_t>(static_cast<float>(g) *
                                      static_cast<float>(other.g) * inv),
            static_cast<std::uint8_t>(static_cast<float>(b) *
                                      static_cast<float>(other.b) * inv),
        };
    }

    // Scale each channel by a float factor; useful for lighting modulation.
    [[nodiscard]] constexpr TGAColor operator*(float factor) const {
        return {
            static_cast<std::uint8_t>(static_cast<float>(r) * factor),
            static_cast<std::uint8_t>(static_cast<float>(g) * factor),
            static_cast<std::uint8_t>(static_cast<float>(b) * factor),
        };
    }

    // Channel-wise saturating add (clamped to 255).
    [[nodiscard]] constexpr TGAColor operator+(const TGAColor &other) const {
        return {
            static_cast<std::uint8_t>(std::min(255, r + other.r)),
            static_cast<std::uint8_t>(std::min(255, g + other.g)),
            static_cast<std::uint8_t>(std::min(255, b + other.b)),
        };
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
    [[nodiscard]] TGAColor get(T x, T y) const {
        if (static_cast<std::uint16_t>(x) >= width_ ||
            static_cast<std::uint16_t>(y) >= height_) {
            throw std::out_of_range("Coordinates out of bounds");
        }
        const size_t index = static_cast<size_t>(x) + static_cast<size_t>(y) * width_;
        std::uint32_t argb = 0;
        std::memcpy(&argb, &data_[index * bytespp_], bytespp_);
        TGAColor color;
        color.fromArgb(argb);
        return color;
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
