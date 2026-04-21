#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/Matrix.hpp"
#include "tinyrenderer/Model.hpp"
#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <fmt/core.h>

#include <array>
#include <cmath>
#include <exception>
#include <limits>
#include <numbers>
#include <vector>

struct Camera {
    Vec3f eye;
    Vec3f center;
    Vec3f up;
    float fovy = 45.f; // in degrees
    float aspect = 1.f;
    float nearPlane = 0.1f;
    float farPlane = 100.f;
};

/**
 * @brief Computes the view (world-to-camera) matrix from a Camera.
 */
auto viewMatrix(const Camera &cam) {
    const auto z = (cam.eye - cam.center).normalize();
    const auto x = cross(cam.up, z).normalize();
    const auto y = cross(z, x);

    Mat<float, 4, 4> result;
    for (size_t i = 0; i < 3; i++) {
        result(0, i) = x[i]; // row 0 = vector x
        result(1, i) = y[i]; // row 1 = vector y
        result(2, i) = z[i]; // row 2 = vector z
    }
    result(0, 3) = -dot(x, cam.eye);
    result(1, 3) = -dot(y, cam.eye);
    result(2, 3) = -dot(z, cam.eye);
    result(3, 3) = 1.f;

    return result;
}

/**
 * @brief Computes the perspective projection matrix from a Camera.
 */
auto projectionMatrix(const Camera &cam) {
    const float rad = cam.fovy * std::numbers::pi_v<float> / 180.f;
    const float f = 1.f / std::tan(rad / 2.f);
    Mat<float, 4, 4> result;
    result(0, 0) = f / cam.aspect;
    result(1, 1) = f;
    result(2, 2) = (cam.farPlane + cam.nearPlane) / (cam.nearPlane - cam.farPlane);
    result(2, 3) = (2.f * cam.farPlane * cam.nearPlane) / (cam.nearPlane - cam.farPlane);
    result(3, 2) = -1.f;
    return result;
}

/**
 * @brief Computes the viewport (NDC-to-screen) matrix.
 */
auto viewportMatrix(float w, float h) {
    Mat<float, 4, 4> result;
    result(0, 0) = w / 2.f;
    result(1, 1) = h / 2.f;
    result(2, 2) = 1.f;
    result(0, 3) = w / 2.f;
    result(1, 3) = h / 2.f;
    result(3, 3) = 1.f;
    return result;
}

/**
 * @brief Projects a world-space vertex to screen space.
 *
 * Pipeline: world -> clip (mvp) -> NDC (perspective divide) -> screen (viewport).
 */
Vec3f project(const Vec3f &world, const Mat<float, 4, 4> &mvp,
              const Mat<float, 4, 4> &viewport) {
    const Vec4f v(world.x(), world.y(), world.z(), 1.f);
    const Vec4f clip = mvp * v;
    return Vec3f(viewport * (clip / clip.w()));
}

int main() try {
    constexpr size_t width = 1600;
    constexpr size_t height = 1600;

    TGAImage image(width, height, TGAImage::RGB);
    std::vector<float> zbuffer(width * height, std::numeric_limits<float>::max());

    const Model model("obj/diablo3_pose/diablo3_pose.obj");

    fmt::print("nverts: {}\n", model.nverts());
    fmt::print("nfaces: {}\n", model.nfaces());

    constexpr Vec3f lightDir(0.f, 0.f, -1.f);
    const Camera cam{
        .eye = Vec3f(1.f, 1.f, 3.f),
        .center = Vec3f(0.f, 0.f, 0.f),
        .up = Vec3f(0.f, 1.f, 0.f),
        .fovy = 45.f,
        .aspect = static_cast<float>(width) / height,
        .nearPlane = 3.f,
        .farPlane = 100.f,
    };

    const auto MVP = projectionMatrix(cam) * viewMatrix(cam);
    const auto VP = viewportMatrix(width, height);

    for (size_t i = 0; i < model.nfaces(); i++) {
        const auto &face = model.face(i);

        std::array<Vec3f, 3> screenCoords;
        std::array<Vec3f, 3> worldCoords;

        for (size_t j = 0; j < 3; j++) {
            const Vec3f &v = model.vert(face[j]);
            screenCoords[j] = project(v, MVP, VP);
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
} catch (const std::exception &e) {
    fmt::print(stderr, "error: {}\n", e.what());
    return 1;
}
