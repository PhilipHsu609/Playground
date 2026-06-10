#include "tinyrenderer/AmbientOcclusion.hpp"
#include "tinyrenderer/Model.hpp"
#include "tinyrenderer/Shader.hpp"
#include "tinyrenderer/TGAImage.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>

namespace {

Mat4f lookAt(const Vec3f &eye, const Vec3f &center, const Vec3f &up) {
    const Vec3f z = (eye - center).normalize();
    const Vec3f x = cross(up, z).normalize();
    const Vec3f y = cross(z, x);
    Mat4f m;
    for (size_t i = 0; i < 3; ++i) {
        m[0, i] = x[i];
        m[1, i] = y[i];
        m[2, i] = z[i];
    }
    m[0, 3] = -dot(x, eye);
    m[1, 3] = -dot(y, eye);
    m[2, 3] = -dot(z, eye);
    m[3, 3] = 1.f;
    return m;
}

// Square ortho box [-halfExtent, halfExtent] -> [-1, 1] on both x and y;
// assumes a square viewport (callers with g.w == g.h).
Mat4f ortho(float halfExtent, float n, float f) {
    Mat4f m;
    m[0, 0] = 1.f / halfExtent;
    m[1, 1] = 1.f / halfExtent;
    m[2, 2] = -2.f / (f - n);
    m[2, 3] = -(f + n) / (f - n);
    m[3, 3] = 1.f;
    return m;
}

} // namespace

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

std::vector<float> bakeAO(const GBuffer &g, const std::vector<AoOccluder> &occluders,
                          const Mat4f &viewport, const AoLightParams &light, int numDirs,
                          unsigned seed) {
    assert(g.w == g.h); // ortho() builds a square light box
    const size_t n = g.w * g.h;
    std::vector<int> visible(n, 0);
    std::mt19937 rng(seed);

    const Mat4f proj = ortho(light.orthoHalf, light.lightNear, light.lightFar);
    TGAImage scratch(static_cast<std::uint16_t>(g.w), static_cast<std::uint16_t>(g.h),
                     TGAImage::RGB);

    for (int k = 0; k < numDirs; ++k) {
        const Vec3f dir = randomHemisphereDir(rng);
        // A non-parallel up vector keeps the look-at well-defined for any dir.
        const Vec3f up =
            std::abs(dir.y()) > 0.99f ? Vec3f(0.f, 0.f, 1.f) : Vec3f(0.f, 1.f, 0.f);
        const Mat4f dirMVP = proj * lookAt(dir * light.dist, Vec3f(0.f, 0.f, 0.f), up);

        std::vector<float> depthMap(n, std::numeric_limits<float>::max());
        for (const AoOccluder &occ : occluders) {
            const DepthShader depth(*occ.model, dirMVP);
            for (size_t face = 0; face < depth.faceCount(); ++face) {
                rasterize(depth, depth.primitive(face), viewport, depthMap, scratch);
            }
        }

        for (size_t p = 0; p < n; ++p) {
            if (g.covered[p] == 0) {
                continue;
            }
            if (!inShadow(g.worldPos[p], dirMVP, viewport, depthMap, g.w, g.h,
                          light.bias)) {
                visible[p] += 1;
            }
        }
    }

    std::vector<float> ao(n, 1.f);
    for (size_t p = 0; p < n; ++p) {
        if (g.covered[p] != 0) {
            ao[p] = static_cast<float>(visible[p]) / static_cast<float>(numDirs);
        }
    }
    return ao;
}
