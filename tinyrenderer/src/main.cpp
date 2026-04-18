#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/Matrix.hpp"
#include "tinyrenderer/Model.hpp"
#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <fmt/core.h>

#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

Mat<float, 3, 3> rotationMatrix(float angle) {
    float rad = angle / 180.f * std::numbers::pi_v<float>;
    const float c = std::cos(rad);
    const float s = std::sin(rad);
    return Mat<float, 3, 3>{c, 0.f, s, 0.f, 1.f, 0.f, -s, 0.f, c};
}

Vec3f persp(Vec3f v, float c) {
    float w = 1.f - v.z() / c;
    return v / w;
}

Vec3f viewport(Vec3f v, float w, float h) {
    return Vec3f((v.x() + 1.f) * w / 2.f, (v.y() + 1.f) * h / 2.f, v.z());
}

int main() {
    constexpr size_t width = 1600;
    constexpr size_t height = 1600;

    TGAImage image(width, height, TGAImage::RGB);
    std::vector<float> zbuffer(width * height, std::numeric_limits<float>::lowest());

    const Model model("obj/diablo3_pose/diablo3_pose.obj");

    fmt::print("nverts: {}\n", model.nverts());
    fmt::print("nfaces: {}\n", model.nfaces());

    constexpr Vec3f lightDir(0.f, 0.f, -1.f);
    const auto rotY = rotationMatrix(30.f);

    for (size_t i = 0; i < model.nfaces(); i++) {
        const auto &face = model.face(i);

        std::array<Vec3f, 3> screenCoords;
        std::array<Vec3f, 3> worldCoords;

        for (size_t j = 0; j < 3; j++) {
            Vec3f v = rotY * model.vert(face[j]);
            screenCoords[j] = viewport(persp(v, 3.f), width, height);
            worldCoords[j] = v;
        }

        // Compute the normal of the triangle and the intensity of the light on it
        Vec3f n = cross(worldCoords[2] - worldCoords[0], worldCoords[1] - worldCoords[0]);
        n = n.normalize();
        const float intensity = dot(n, lightDir);

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
