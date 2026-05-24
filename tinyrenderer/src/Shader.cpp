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
        normals_(i, cornerIdx) = n[i];
        worldPositions_(i, cornerIdx) = v[i];
    }
    for (size_t i = 0; i < 2; i++) {
        texCoords_(i, cornerIdx) = uv[i];
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
    const Vec3f n = (normals_ * bary).normalize();
    const Vec3f worldPos = worldPositions_ * bary;
    const Vec3f viewDir = (eye_ - worldPos).normalize();
    const Vec3f halfway = (lightDir_ + viewDir).normalize();
    const Vec2f uv = texCoords_ * bary;

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
