#include "tinyrenderer/Shader.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

Vec3f project(const Vec3f &world, const Mat4f &mvp, const Mat4f &viewport) {
    const Vec4f v(world.x(), world.y(), world.z(), 1.f);
    const Vec4f clip = mvp * v;
    return Vec3f(viewport * (clip / clip.w()));
}

// Sample a texture using OBJ UV convention (u right, v up, origin bottom-left).
// Our TGAImage is stored top-down internally, so flip v when indexing.
TGAColor sample(const TGAImage &tex, Vec2f uv) {
    const auto w = static_cast<int>(tex.getWidth());
    const auto h = static_cast<int>(tex.getHeight());
    const int x = std::clamp(static_cast<int>(uv.x() * static_cast<float>(w)), 0, w - 1);
    const int y =
        std::clamp(static_cast<int>((1.f - uv.y()) * static_cast<float>(h)), 0, h - 1);
    return tex.get(x, y);
}

// Decode an RGB texel as a tangent-space normal: [0, 255] -> [-1, 1] per channel.
Vec3f sampleNormal(const TGAImage &tex, Vec2f uv) {
    const TGAColor c = sample(tex, uv);
    return Vec3f(static_cast<float>(c.r) / 127.5f - 1.f,
                 static_cast<float>(c.g) / 127.5f - 1.f,
                 static_cast<float>(c.b) / 127.5f - 1.f);
}

} // namespace

BlinnPhongShader::BlinnPhongShader(Model model, Material material, Mat4f mvp, Mat4f vp,
                                   Vec3f lightDir, Vec3f eye)
    : model_(std::move(model)), material_(std::move(material)), mvp_(mvp), vp_(vp),
      lightDir_(lightDir), eye_(eye) {}

Vec3f BlinnPhongShader::vertex(size_t faceIdx, size_t cornerIdx) {
    const auto &v = model_.vert(faceIdx, cornerIdx);
    const auto &n = model_.normal(faceIdx, cornerIdx);
    const auto &uv = model_.texCoord(faceIdx, cornerIdx);
    for (size_t i = 0; i < 3; i++) {
        normals_[i, cornerIdx] = n[i];
        worldPositions_[i, cornerIdx] = v[i];
    }
    for (size_t i = 0; i < 2; i++) {
        texCoords_[i, cornerIdx] = uv[i];
    }
    return project(v, mvp_, vp_);
}

float BlinnPhongShader::blinnPhongFactor(const Vec3f &n, const Vec3f &halfway) const {
    constexpr float ambient = 0.1f;
    const float diffuse = std::max(0.f, dot(n, lightDir_));
    const float specular = std::pow(std::max(0.f, dot(n, halfway)), material_.shininess);
    return std::min(1.f, ambient + diffuse + specular);
}

TGAColor BlinnPhongShader::fragment(const Vec3f &bary) const {
    Vec3f n = (normals_ * bary).normalize();
    const Vec3f worldPos = worldPositions_ * bary;
    const Vec3f viewDir = (eye_ - worldPos).normalize();
    const Vec2f uv = texCoords_ * bary;

    // Tangent-space normal mapping: sample a per-pixel surface-local normal
    // and rotate it into world space via the TBN basis derived from the
    // triangle's world-space edges and UV deltas:
    //   [T B] = E * U^-1
    // with E = [e1 | e2] (3x2 edge matrix) and U = [du1 du2; dv1 dv2].
    if (material_.normalMap) {
        Vec3f e1;
        Vec3f e2;
        for (size_t i = 0; i < 3; i++) {
            e1[i] = worldPositions_[i, 1] - worldPositions_[i, 0];
            e2[i] = worldPositions_[i, 2] - worldPositions_[i, 0];
        }
        const Mat<float, 2, 2> uvMat{
            {texCoords_[0, 1] - texCoords_[0, 0], texCoords_[0, 2] - texCoords_[0, 0]},
            {texCoords_[1, 1] - texCoords_[1, 0], texCoords_[1, 2] - texCoords_[1, 0]},
        };
        // Degenerate UV (two corners share a uv) gives a singular uvMat with
        // no TBN basis; in that case keep the geometric normal `n` as-is.
        if (const auto uvInv = uvMat.inverse()) {
            const Vec3f T = (e1 * (*uvInv)[0, 0] + e2 * (*uvInv)[1, 0]).normalize();
            const Vec3f B = (e1 * (*uvInv)[0, 1] + e2 * (*uvInv)[1, 1]).normalize();
            const Vec3f t = sampleNormal(*material_.normalMap, uv);
            n = (T * t.x() + B * t.y() + n * t.z()).normalize();
        }
    }

    const Vec3f halfway = (lightDir_ + viewDir).normalize();

    TGAColor albedo = material_.baseColor;
    if (material_.diffuse) {
        albedo = albedo * sample(*material_.diffuse, uv);
    }

    TGAColor lit = albedo * blinnPhongFactor(n, halfway);
    if (material_.glow) {
        lit = lit + sample(*material_.glow, uv);
    }

    return lit;
}
