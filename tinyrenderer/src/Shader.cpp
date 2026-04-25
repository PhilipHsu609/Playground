#include "tinyrenderer/Shader.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

// Project a world-space vertex through MVP + perspective divide + viewport.
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

// Compute the Blinn-Phong lighting factor (ambient + diffuse + specular),
// clamped to [0, 1]. Shared between the plain and textured shaders.
float lightingFactor(const Vec3f &n, const Vec3f &lightDir, const Vec3f &halfway) {
    constexpr float ambient = 0.1f;
    const float diffuse = std::max(0.f, dot(n, lightDir));
    const float specular = std::pow(std::max(0.f, dot(n, halfway)), 50.f);
    return std::min(1.f, ambient + diffuse + specular);
}

TGAColor modulate(TGAColor c, float factor) {
    return {static_cast<std::uint8_t>(static_cast<float>(c.r) * factor),
            static_cast<std::uint8_t>(static_cast<float>(c.g) * factor),
            static_cast<std::uint8_t>(static_cast<float>(c.b) * factor)};
}

} // namespace

// --- BlinnPhongShader ---

BlinnPhongShader::BlinnPhongShader(Model model, Mat4f mvp, Mat4f vp, Vec3f lightDir,
                                   Vec3f eye)
    : model_(std::move(model)), mvp_(mvp), vp_(vp), lightDir_(lightDir), eye_(eye) {}

Vec3f BlinnPhongShader::vertex(size_t faceIdx, size_t cornerIdx) {
    const auto &v = model_.vert(faceIdx, cornerIdx);
    const auto &n = model_.normal(faceIdx, cornerIdx);
    for (size_t i = 0; i < 3; i++) {
        normals_(i, cornerIdx) = n[i];
        worldPositions_(i, cornerIdx) = v[i];
    }
    return project(v, mvp_, vp_);
}

TGAColor BlinnPhongShader::fragment(const Vec3f &bary) const {
    const Vec3f n = (normals_ * bary).normalize();
    const Vec3f worldPos = worldPositions_ * bary;
    const Vec3f viewDir = (eye_ - worldPos).normalize();
    const Vec3f halfway = (lightDir_ + viewDir).normalize();

    return modulate(color_, lightingFactor(n, lightDir_, halfway));
}

// --- BlinnPhongTexturedShader ---

BlinnPhongTexturedShader::BlinnPhongTexturedShader(Model model, TGAImage diffuseMap,
                                                   Mat4f mvp, Mat4f vp, Vec3f lightDir,
                                                   Vec3f eye)
    : model_(std::move(model)), diffuseMap_(std::move(diffuseMap)), mvp_(mvp), vp_(vp),
      lightDir_(lightDir), eye_(eye) {}

Vec3f BlinnPhongTexturedShader::vertex(size_t faceIdx, size_t cornerIdx) {
    const auto &v = model_.vert(faceIdx, cornerIdx);
    const auto &n = model_.normal(faceIdx, cornerIdx);
    const auto &uv = model_.texCoord(faceIdx, cornerIdx);
    for (size_t i = 0; i < 3; i++) {
        normals_(i, cornerIdx) = n[i];
        worldPositions_(i, cornerIdx) = v[i];
    }
    for (size_t i = 0; i < 2; i++) {
        texCoords_(i, cornerIdx) = uv[i];
    }
    return project(v, mvp_, vp_);
}

TGAColor BlinnPhongTexturedShader::fragment(const Vec3f &bary) const {
    const Vec3f n = (normals_ * bary).normalize();
    const Vec3f worldPos = worldPositions_ * bary;
    const Vec3f viewDir = (eye_ - worldPos).normalize();
    const Vec3f halfway = (lightDir_ + viewDir).normalize();
    const Vec2f uv = texCoords_ * bary;

    const TGAColor texel = sample(diffuseMap_, uv);
    return modulate(texel, lightingFactor(n, lightDir_, halfway));
}
