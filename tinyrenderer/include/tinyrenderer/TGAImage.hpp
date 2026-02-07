#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <type_traits>
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
    constexpr TGAColor(std::uint32_t rgba, std::uint8_t bytespp) : bytespp(bytespp) {
        this->operator()(rgba);
    }

    void operator()(std::uint32_t rgba) {
        r = (rgba >> 24) & 0xff;
        g = (rgba >> 16) & 0xff;
        b = (rgba >> 8) & 0xff;
        a = rgba & 0xff;
    }

    std::uint32_t operator()() const {
        return static_cast<uint32_t>((a << 24) | (r << 16) | (g << 8) | b);
    }
};

class TGAImage {
  public:
    TGAImage(const char *filename);
    TGAImage(std::uint16_t width, std::uint16_t height, std::uint8_t bytespp);
    void save(const char *filename) const;

    void flipVertically();
    void flipHorizontally();

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    void set(T x, T y, TGAColor color) {
        if (static_cast<std::uint16_t>(x) >= width ||
            static_cast<std::uint16_t>(y) >= height) {
            throw std::out_of_range("Coordinates out of bounds");
        }
        size_t index = static_cast<size_t>(x) + static_cast<size_t>(y) * width;
        std::uint32_t bgra = color();
        std::memcpy(&data[index * bytespp], &bgra, bytespp);
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    std::uint8_t get(T x, T y) const {
        if (static_cast<std::uint16_t>(x) >= width ||
            static_cast<std::uint16_t>(y) >= height) {
            throw std::out_of_range("Coordinates out of bounds");
        }
        size_t index = static_cast<size_t>(x) + static_cast<size_t>(y) * width;
        return data[index * bytespp];
    }

    std::uint16_t get_width() const { return width; }
    std::uint16_t get_height() const { return height; }
    std::uint8_t get_bytespp() const { return bytespp; }
    std::vector<std::uint8_t> &buffer() { return data; }

    enum Format { GRAYSCALE = 1, RGB = 3, RGBA = 4 };

  private:
    void load_tga_data(const char *filename);
    void write_tga_data(const char *filename, bool rle = true) const;
    void load_rle_data(std::ifstream &file);
    void write_rle_data(std::ofstream &file) const;

    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint8_t bytespp = 1;
    std::vector<std::uint8_t> data;
};

// Common color constants
constexpr TGAColor WHITE_COLOR(255, 255, 255, 255);
constexpr TGAColor BLACK_COLOR(0, 0, 0, 255);
constexpr TGAColor RED_COLOR(255, 0, 0, 255);
constexpr TGAColor GREEN_COLOR(0, 255, 0, 255);
constexpr TGAColor BLUE_COLOR(0, 0, 255, 255);