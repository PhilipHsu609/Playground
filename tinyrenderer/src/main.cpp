#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/Matrix.hpp"
#include "tinyrenderer/Model.hpp"
#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <fmt/core.h>

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

    Mat4f result;
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
    Mat4f result;
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
    Mat4f result;
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
Vec3f project(const Vec3f &world, const Mat4f &mvp, const Mat4f &viewport) {
    const Vec4f v(world.x(), world.y(), world.z(), 1.f);
    const Vec4f clip = mvp * v;
    return Vec3f(viewport * (clip / clip.w()));
}

struct PhongShader : public IShader {
    // uniforms
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    const Model &model;
    Mat4f MVP, VP;
    Vec3f lightDir;

    // varyings
    Mat3f normals;

    explicit PhongShader(const Model &model, const Mat4f &MVP, const Mat4f &VP,
                         const Vec3f &lightDir)
        : model(model), MVP(MVP), VP(VP), lightDir(lightDir) {}

    [[nodiscard]] Vec3f vertex(size_t faceIdx, size_t cornerIdx) override {
        const auto &v = model.vert(faceIdx, cornerIdx);
        // Cache per-vertex normal as a varying for fragment() to interpolate.
        const auto &n = model.normal(faceIdx, cornerIdx);
        for (size_t i = 0; i < 3; i++) {
            normals(i, cornerIdx) = n[i];
        }
        return project(v, MVP, VP);
    }

    [[nodiscard]] std::optional<TGAColor> fragment(const Vec3f &bary) const override {
        Vec3f n = (normals * bary).normalize(); // interpolate vertex normals
        const float ambient = 0.1f;
        const float intensity = std::max(0.f, dot(n, lightDir));
        const float specular =
            std::pow(std::max(0.f, (2 * n * dot(n, lightDir) - lightDir).z()), 5.f);
        const auto color =
            static_cast<uint8_t>(std::min(1.f, ambient + intensity + specular) * 255);
        return TGAColor(color, color, color);
    }
};

int main() try {
    constexpr size_t width = 800;
    constexpr size_t height = 800;

    TGAImage image(width, height, TGAImage::RGB);
    std::vector<float> zbuffer(width * height, std::numeric_limits<float>::max());

    const Model model("obj/diablo3_pose/diablo3_pose.obj");

    fmt::print("nverts: {}\n", model.nverts());
    fmt::print("nfaces: {}\n", model.nfaces());

    constexpr Vec3f lightDir(1.f, 0.f, 0.f);
    const Camera cam{
        .eye = Vec3f(0.f, 0.f, 3.f),
        .center = Vec3f(0.f, 0.f, 0.f),
        .up = Vec3f(0.f, 1.f, 0.f),
        .fovy = 45.f,
        .aspect = static_cast<float>(width) / height,
        .nearPlane = 3.f,
        .farPlane = 100.f,
    };

    const auto MVP = projectionMatrix(cam) * viewMatrix(cam);
    const auto VP = viewportMatrix(width, height);

    PhongShader shader(model, MVP, VP, lightDir);

    for (size_t i = 0; i < model.nfaces(); i++) {
        const Triangle tri{
            shader.vertex(i, 0),
            shader.vertex(i, 1),
            shader.vertex(i, 2),
        };
        rasterize(tri, shader, zbuffer, image);
    }

    image.flipVertically();
    image.save("output.tga");

    return 0;
} catch (const std::exception &e) {
    fmt::print(stderr, "error: {}\n", e.what());
    return 1;
}
