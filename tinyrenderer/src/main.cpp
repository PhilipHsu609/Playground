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

    Model model("obj/african_head/african_head.obj");

    fmt::print("nverts: {}\n", model.nverts());
    fmt::print("nfaces: {}\n", model.nfaces());

    Vec3f light_dir(0.f, 0.f, -1.f);

    for (size_t i = 0; i < model.nfaces(); i++) {
        std::vector<size_t> face = model.face(i);

        std::array<Vec3f, 3> screen_coords;
        std::array<Vec3f, 3> world_coords;

        for (size_t j = 0; j < 3; j++) {
            Vec3f v = model.vert(face[j]);
            screen_coords[j] =
                Vec3f((v[0] + 1.f) * width / 2.f, (v[1] + 1.f) * height / 2.f, v[2]);
            world_coords[j] = v;
        }

        Vec3f n =
            (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n = n.normalize();
        float intensity = n * light_dir;
        if (intensity > 0) {
            TGAColor color(static_cast<std::uint8_t>(intensity * 255),
                           static_cast<std::uint8_t>(intensity * 255),
                           static_cast<std::uint8_t>(intensity * 255));
            triangle(screen_coords, zbuffer, image, color);
        }
    }

    image.flipVertically();
    image.save("output.tga");

    return 0;
}