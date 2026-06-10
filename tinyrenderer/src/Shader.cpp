#include "tinyrenderer/Shader.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

constexpr float AMBIENT = 0.1f;

// Clip-space position (pre-divide; the rasterizer owns the divide).
Vec4f clipVertex(const Vec3f &world, const Mat4f &mvp) {
    return mvp * Vec4f(world.x(), world.y(), world.z(), 1.f);
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

bool inShadow(const Vec3f &worldPos, const Mat4f &lightMVP, const Mat4f &lightVP,
              const std::vector<float> &shadowMap, size_t shadowW, size_t shadowH,
              float bias) {
    const Vec4f clip = lightMVP * Vec4f(worldPos.x(), worldPos.y(), worldPos.z(), 1.f);
    const Vec4f screen = lightVP * clip; // orthographic: clip.w() == 1
    // Check bounds before casting: a negative coord would truncate into a valid
    // index rather than being rejected.
    const float fx = screen.x();
    const float fy = screen.y();
    if (fx < 0.f || fy < 0.f || fx >= static_cast<float>(shadowW) ||
        fy >= static_cast<float>(shadowH)) {
        return false; // outside the light frustum -> lit
    }
    const float lz = clip.z(); // smaller z == closer to light
    const size_t idx = static_cast<size_t>(fy) * shadowW + static_cast<size_t>(fx);
    return lz > shadowMap[idx] + bias;
}

BlinnPhongShader::BlinnPhongShader(Model model, Material material, Mat4f mvp,
                                   Vec3f lightDir, Vec3f eye)
    : model_(std::move(model)), material_(std::move(material)), mvp_(mvp),
      lightDir_(lightDir), eye_(eye) {}

std::array<VertexOut<BlinnPhongShader::Varyings>, 3>
BlinnPhongShader::primitive(size_t faceIdx) const {
    std::array<Vec3f, 3> worldPos;
    std::array<Vec2f, 3> uv;
    for (size_t c = 0; c < 3; ++c) {
        worldPos[c] = model_.vert(faceIdx, c);
        uv[c] = model_.texCoord(faceIdx, c);
    }

    // Tangent basis from world edges and UV deltas: [T B] = E * U^-1, with
    // E = [e1 | e2], U = [du1 du2; dv1 dv2]. Degenerate UV leaves T/B zero,
    // which disables normal mapping in fragment().
    Vec3f T;
    Vec3f B;
    const Vec3f e1 = worldPos[1] - worldPos[0];
    const Vec3f e2 = worldPos[2] - worldPos[0];
    const Mat<float, 2, 2> uvMat{
        {uv[1].x() - uv[0].x(), uv[2].x() - uv[0].x()},
        {uv[1].y() - uv[0].y(), uv[2].y() - uv[0].y()},
    };
    if (const auto uvInv = uvMat.inverse()) {
        T = (e1 * (*uvInv)[0, 0] + e2 * (*uvInv)[1, 0]).normalize();
        B = (e1 * (*uvInv)[0, 1] + e2 * (*uvInv)[1, 1]).normalize();
    }

    std::array<VertexOut<Varyings>, 3> out;
    for (size_t c = 0; c < 3; ++c) {
        out[c] = {
            .clip = clipVertex(worldPos[c], mvp_),
            .vary = {.normal = model_.normal(faceIdx, c),
                     .worldPos = worldPos[c],
                     .T = T,
                     .B = B,
                     .uv = uv[c]},
        };
    }
    return out;
}

float BlinnPhongShader::directFactor(const Vec3f &n, const Vec3f &halfway) const {
    const float diffuse = std::max(0.f, dot(n, lightDir_));
    const float specular = std::pow(std::max(0.f, dot(n, halfway)), material_.shininess);
    return diffuse + specular;
}

TGAColor BlinnPhongShader::fragment(const Varyings &in, int x, int y) const {
    Vec3f n = in.normal.normalize();
    const Vec3f viewDir = (eye_ - in.worldPos).normalize();

    // Normal mapping: rotate the sampled tangent-space normal into world space
    // via the TBN basis. Zero T (degenerate UV) skips this, keeping geometric n.
    if (material_.normalMap && (in.T.x() != 0.f || in.T.y() != 0.f || in.T.z() != 0.f)) {
        const Vec3f T = in.T.normalize();
        const Vec3f B = in.B.normalize();
        const Vec3f t = sampleNormal(*material_.normalMap, in.uv);
        n = (T * t.x() + B * t.y() + n * t.z()).normalize();
    }

    const Vec3f halfway = (lightDir_ + viewDir).normalize();

    TGAColor albedo = material_.baseColor;
    if (material_.diffuse) {
        albedo = albedo * sample(*material_.diffuse, in.uv);
    }

    float direct = directFactor(n, halfway);
    if (shadowMap_ != nullptr && inShadow(in.worldPos, lightMVP_, lightVP_, *shadowMap_,
                                          shadowW_, shadowH_, shadowBias_)) {
        direct = 0.f; // shadow drops direct light; ambient (with AO) remains
    }
    const float ao =
        aoBuffer_ != nullptr
            ? (*aoBuffer_)[static_cast<size_t>(y) * aoW_ + static_cast<size_t>(x)]
            : 1.f;
    const float factor = std::min(1.f, AMBIENT * ao + direct);

    TGAColor lit = albedo * factor;
    if (material_.glow) {
        lit = lit + sample(*material_.glow, in.uv);
    }

    return lit;
}

std::array<VertexOut<DepthShader::Varyings>, 3>
DepthShader::primitive(size_t faceIdx) const {
    std::array<VertexOut<Varyings>, 3> out;
    for (size_t c = 0; c < 3; ++c) {
        out[c] = {.clip = clipVertex(model_->vert(faceIdx, c), lightMVP_), .vary = {}};
    }
    return out;
}
