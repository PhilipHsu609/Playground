#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/Matrix.hpp"
#include "tinyrenderer/Model.hpp"
#include "tinyrenderer/Shader.hpp"
#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <fmt/core.h>
#include <fmt/format.h>

#include <array>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <limits>
#include <numbers>
#include <optional>
#include <string>
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

namespace {

/**
 * @brief Computes the view (world-to-camera) matrix from a Camera.
 */
auto viewMatrix(const Camera &cam) {
    const auto z = (cam.eye - cam.center).normalize();
    const auto x = cross(cam.up, z).normalize();
    const auto y = cross(z, x);

    Mat4f result;
    for (size_t i = 0; i < 3; i++) {
        result[0, i] = x[i]; // row 0 = vector x
        result[1, i] = y[i]; // row 1 = vector y
        result[2, i] = z[i]; // row 2 = vector z
    }
    result[0, 3] = -dot(x, cam.eye);
    result[1, 3] = -dot(y, cam.eye);
    result[2, 3] = -dot(z, cam.eye);
    result[3, 3] = 1.f;

    return result;
}

/**
 * @brief Computes the perspective projection matrix from a Camera.
 */
auto projectionMatrix(const Camera &cam) {
    const float rad = cam.fovy * std::numbers::pi_v<float> / 180.f;
    const float f = 1.f / std::tan(rad / 2.f);
    Mat4f result;
    result[0, 0] = f / cam.aspect;
    result[1, 1] = f;
    result[2, 2] = (cam.farPlane + cam.nearPlane) / (cam.nearPlane - cam.farPlane);
    result[2, 3] = (2.f * cam.farPlane * cam.nearPlane) / (cam.nearPlane - cam.farPlane);
    result[3, 2] = -1.f;
    return result;
}

/**
 * @brief Computes the viewport (NDC-to-screen) matrix.
 */
auto viewportMatrix(float w, float h) {
    Mat4f result;
    result[0, 0] = w / 2.f;
    result[1, 1] = h / 2.f;
    result[2, 2] = 1.f;
    result[0, 3] = w / 2.f;
    result[1, 3] = h / 2.f;
    result[3, 3] = 1.f;
    return result;
}

/**
 * @brief Orthographic projection for a directional light.
 *
 * Maps the box [l,r] x [b,t] x [-n,-f] (viewing down -z) to the NDC cube
 * [-1,1]^3. Unlike perspective, it leaves w == 1, so depth is linear and
 * the shadow lookup needs no perspective divide.
 */
auto orthographicMatrix(float l, float r, float b, float t, float n, float f) {
    Mat4f result;
    result[0, 0] = 2.f / (r - l);
    result[1, 1] = 2.f / (t - b);
    result[2, 2] = -2.f / (f - n);
    result[0, 3] = -(r + l) / (r - l);
    result[1, 3] = -(t + b) / (t - b);
    result[2, 3] = -(f + n) / (f - n);
    result[3, 3] = 1.f;
    return result;
}

} // namespace

int main() try {
    constexpr size_t width = 800;
    constexpr size_t height = 800;
    constexpr int numFrames = 36;
    constexpr int frameDelay = 11; // ImageMagick "centiseconds" per frame; ~9 fps.

    Model model("obj/diablo3_pose/diablo3_pose.obj");
    Material material{
        .baseColor = TGAColor(255, 255, 255),
        .shininess = 5.f,
        .diffuse = TGAImage("obj/diablo3_pose/diablo3_pose_diffuse.tga"),
        .glow = TGAImage("obj/diablo3_pose/diablo3_pose_glow.tga"),
        .specular = std::nullopt,
        .normalMap = TGAImage("obj/diablo3_pose/diablo3_pose_nm_tangent.tga"),
    };

    Model floor("obj/floor.obj");
    Material floorMaterial{
        .baseColor = TGAColor(255, 255, 255),
        .shininess = 5.f,
        .diffuse = TGAImage("obj/floor_diffuse.tga"),
        .glow = std::nullopt,
        .specular = std::nullopt,
        .normalMap = TGAImage("obj/floor_nm_tangent.tga"),
    };

    const Camera cam{
        .eye = Vec3f(-1.5f, .5f, 3.5f),
        .center = Vec3f(0.f, 0.f, 0.f),
        .up = Vec3f(0.f, 1.f, 0.f),
        .fovy = 45.f,
        .aspect = static_cast<float>(width) / height,
        .nearPlane = 3.f,
        .farPlane = 100.f,
    };
    Vec3f lightDir(1.f, 0.f, 0.f);

    const auto VP = viewportMatrix(width, height);
    const auto cameraMVP = projectionMatrix(cam) * viewMatrix(cam);
    BlinnPhongShader modelShader(std::move(model), std::move(material), cameraMVP,
                                 lightDir, cam.eye);
    BlinnPhongShader floorShader(std::move(floor), std::move(floorMaterial), cameraMVP,
                                 lightDir, cam.eye);
    const std::array<BlinnPhongShader *, 2> objects{&modelShader, &floorShader};

    // Shadow-map tuning constants
    constexpr float lightDist = 5.f;
    constexpr float orthoHalf = 2.f;
    constexpr float lightNear = 0.1f;
    constexpr float lightFar = 10.f;
    constexpr float shadowBias = 0.01f;

    std::filesystem::create_directory("frames");
    for (int i = 0; i < numFrames; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(numFrames);

        // ====== animation block ======
        // Light orbits overhead. Elevation stays finite so it never points
        // straight down, which would degenerate the light camera's view matrix.
        constexpr float elevation = 1.5f;
        const float theta = t * 2.f * std::numbers::pi_v<float>;
        lightDir = Vec3f(std::cos(theta), elevation, std::sin(theta)).normalize();
        // ====== end animation block ======

        // Light "camera": look from lightDir*dist toward the origin.
        const Camera lightCam{
            .eye = lightDir * lightDist,
            .center = Vec3f(0.f, 0.f, 0.f),
            .up = Vec3f(0.f, 1.f, 0.f),
        };
        const Mat4f lightMVP = orthographicMatrix(-orthoHalf, orthoHalf, -orthoHalf,
                                                  orthoHalf, lightNear, lightFar) *
                               viewMatrix(lightCam);

        // --- Pass 1: depth from the light's view ---
        std::vector<float> lightZ(width * height, std::numeric_limits<float>::max());
        TGAImage scratch(width, height, TGAImage::RGB);
        for (auto *obj : objects) {
            const DepthShader depth(obj->model(), lightMVP);
            for (size_t f = 0; f < depth.faceCount(); ++f) {
                rasterize(depth, depth.primitive(f), VP, lightZ, scratch);
            }
        }

        // --- Pass 2: shaded camera render with shadow lookup ---
        TGAImage image(width, height, TGAImage::RGB);
        std::vector<float> zbuffer(width * height, std::numeric_limits<float>::max());
        for (auto *obj : objects) {
            obj->setMVP(cameraMVP);
            obj->setEye(cam.eye);
            obj->setLightDir(lightDir);
            obj->setShadow(&lightZ, width, height, lightMVP, VP, shadowBias);
            for (size_t f = 0; f < obj->faceCount(); ++f) {
                rasterize(*obj, obj->primitive(f), VP, zbuffer, image);
            }
        }
        image.flipVertically();

        const std::string filename = fmt::format("frames/frame_{:03d}.png", i);
        image.savePng(filename);
        fmt::print("frame {}/{}\n", i + 1, numFrames);
    }

    fmt::print("assembling GIF...\n");
    const std::string gifCmd = fmt::format(
        "convert -delay {} -loop 0 frames/frame_*.png output.gif", frameDelay);
    // NOLINTNEXTLINE(cert-env33-c, concurrency-mt-unsafe)
    if (std::system(gifCmd.c_str()) != 0) {
        fmt::print(stderr,
                   "warning: ImageMagick convert failed; frames remain in frames/\n");
    }

    return 0;
} catch (const std::exception &e) {
    fmt::print(stderr, "error: {}\n", e.what());
    return 1;
}
