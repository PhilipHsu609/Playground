#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/Model.hpp"
#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <fmt/core.h>

#include <array>
#include <cassert>
#include <vector>

int main() {
    constexpr size_t width = 800;
    constexpr size_t height = 800;

    TGAImage image(width, height, TGAImage::RGB);
    std::vector<float> zbuffer(width * height, std::numeric_limits<float>::min());

    const Model model("obj/african_head/african_head.obj");

    fmt::print("nverts: {}\n", model.nverts());
    fmt::print("nfaces: {}\n", model.nfaces());

    const Vec3f lightDir(0.f, 0.f, -1.f);

    for (size_t i = 0; i < model.nfaces(); i++) {
        const std::vector<size_t> face = model.face(i);

        std::array<Vec3f, 3> screenCoords;
        std::array<Vec3f, 3> worldCoords;

        for (size_t j = 0; j < 3; j++) {
            Vec3f v = model.vert(face[j]);
            screenCoords[j] =
                Vec3f((v[0] + 1.f) * width / 2.f, (v[1] + 1.f) * height / 2.f, v[2]);
            worldCoords[j] = v;
        }

        Vec3f n = (worldCoords[2] - worldCoords[0]) ^ (worldCoords[1] - worldCoords[0]);
        n = n.normalize();
        const float intensity = n * lightDir;
        if (intensity > 0) {
            const TGAColor color(static_cast<std::uint8_t>(intensity * 255),
                                 static_cast<std::uint8_t>(intensity * 255),
                                 static_cast<std::uint8_t>(intensity * 255));
            triangle(screenCoords, zbuffer, image, color);
        }
    }

    image.flipVertically();
    image.save("output.tga");

    return 0;
}
