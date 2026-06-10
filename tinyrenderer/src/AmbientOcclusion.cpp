#include "tinyrenderer/AmbientOcclusion.hpp"
#include "tinyrenderer/Shader.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

Vec3f randomHemisphereDir(std::mt19937 &rng) {
    std::uniform_real_distribution<float> u(0.f, 1.f);
    // Uniform direction on the unit hemisphere: pick cos(theta) in [0,1] for
    // the upper half, phi in [0, 2pi).
    const float cosTheta = u(rng);
    const float sinTheta = std::sqrt(std::max(0.f, 1.f - cosTheta * cosTheta));
    const float phi = 2.f * std::numbers::pi_v<float> * u(rng);
    return Vec3f(sinTheta * std::cos(phi), cosTheta, sinTheta * std::sin(phi));
}

void captureGBuffer(
    const std::array<VertexOut<BlinnPhongShader::Varyings>, 3> &prim,
    const Mat4f &viewport, std::vector<float> &zbuffer, GBuffer &g) {
    using V = BlinnPhongShader::Varyings;

    std::array<Vec3f, 3> screen;
    Vec3f clipW;
    for (size_t i = 0; i < 3; ++i) {
        const float w = prim[i].clip.w();
        clipW[i] = w;
        screen[i] = Vec3f(viewport * (prim[i].clip / w));
    }

    const Vec2f s0(screen[0]);
    const Vec2f s1(screen[1]);
    const Vec2f s2(screen[2]);

    const float minXf = std::min({s0.x(), s1.x(), s2.x()});
    const float maxXf = std::max({s0.x(), s1.x(), s2.x()});
    const float minYf = std::min({s0.y(), s1.y(), s2.y()});
    const float maxYf = std::max({s0.y(), s1.y(), s2.y()});

    const int minX = std::max(static_cast<int>(std::floor(minXf)), 0);
    const int maxX = std::min(static_cast<int>(std::ceil(maxXf)),
                              static_cast<int>(g.w) - 1);
    const int minY = std::max(static_cast<int>(std::floor(minYf)), 0);
    const int maxY = std::min(static_cast<int>(std::ceil(maxYf)),
                              static_cast<int>(g.h) - 1);

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            const Vec2f p(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
            const Vec3f bary = barycentric(p, s0, s1, s2);
            if (bary.x() < 0 || bary.y() < 0 || bary.z() < 0) {
                continue;
            }
            const float z = bary.x() * screen[0].z() + bary.y() * screen[1].z() +
                            bary.z() * screen[2].z();
            const size_t idx = static_cast<size_t>(y) * g.w + static_cast<size_t>(x);
            if (zbuffer[idx] <= z) {
                continue;
            }
            const Vec3f pc = perspectiveCorrect(bary, clipW);
            const V vary =
                prim[0].vary * pc.x() + prim[1].vary * pc.y() + prim[2].vary * pc.z();
            zbuffer[idx] = z;
            g.worldPos[idx] = vary.worldPos;
            g.normal[idx] = vary.normal.normalize();
            g.covered[idx] = 1;
        }
    }
}
