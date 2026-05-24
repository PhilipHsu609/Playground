#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/Matrix.hpp"
#include "tinyrenderer/Model.hpp"
#include "tinyrenderer/Shader.hpp"
#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <fmt/core.h>

#include <cmath>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <limits>
#include <numbers>
#include <optional>
#include <utility>
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

int main() try {
    constexpr size_t width = 1600;
    constexpr size_t height = 1600;

    TGAImage image(width, height, TGAImage::RGB);
    std::vector<float> zbuffer(width * height, std::numeric_limits<float>::max());

    Model model("obj/diablo3_pose/diablo3_pose.obj");
    Material material{
        .baseColor = TGAColor(255, 255, 255),
        .diffuse = TGAImage("obj/diablo3_pose/diablo3_pose_diffuse.tga"),
        .glow = TGAImage("obj/diablo3_pose/diablo3_pose_glow.tga"),
        .specular = std::nullopt,
        .normalMap = std::nullopt,
    };

    fmt::print("nverts: {}\n", model.nverts());
    fmt::print("nfaces: {}\n", model.nfaces());

    constexpr Vec3f lightDir(0.f, 0.f, 1.f);
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

    BlinnPhongShader shader(std::move(model), std::move(material), MVP, VP, lightDir,
                            cam.eye);

    for (const auto &tri : shader.triangles()) {
        rasterize(tri, shader, zbuffer, image);
    }

    image.flipVertically();
    image.save("output.tga");

    // ImageMagick converts the TGA to a smaller PNG; remove the intermediate.
    // NOLINTNEXTLINE(cert-env33-c, concurrency-mt-unsafe)
    if (std::system("convert output.tga output.png") == 0) {
        std::filesystem::remove("output.tga");
    } else {
        fmt::print(stderr, "warning: convert failed; keeping output.tga\n");
    }

    return 0;
} catch (const std::exception &e) {
    fmt::print(stderr, "error: {}\n", e.what());
    return 1;
}
