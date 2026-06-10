#pragma once

#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/Matrix.hpp"
#include "tinyrenderer/Model.hpp"
#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <optional>
#include <vector>

/**
 * @brief Shadow-map visibility test.
 *
 * Transforms a world-space point into the light's screen space
 * (lightVP * lightMVP * worldPos), looks up the stored light-space depth
 * at the resulting pixel, and reports whether the point is occluded
 * (farther from the light than the stored surface, beyond `bias`).
 *
 * Orthographic light projection => w == 1, so no perspective divide.
 * Points projecting outside the shadow map are treated as lit.
 */
[[nodiscard]] bool inShadow(const Vec3f &worldPos, const Mat4f &lightMVP,
                            const Mat4f &lightVP, const std::vector<float> &shadowMap,
                            size_t shadowW, size_t shadowH, float bias);

/**
 * @brief Surface description: base color modulated by optional texture maps.
 *
 * The final albedo is `baseColor * diffuseMap(uv)` where missing maps fall
 * back to their identity (white texture, no glow, no normal-map override).
 */
struct Material {
    TGAColor baseColor = TGAColor(255, 255, 255);
    float shininess = 50.f; // Blinn-Phong specular exponent (high = tight highlight)
    std::optional<TGAImage> diffuse;
    std::optional<TGAImage> glow;
    std::optional<TGAImage> specular;
    std::optional<TGAImage> normalMap;
};

/**
 * @brief Blinn-Phong shading parameterized by a Material.
 *
 * Handles solid-color, textured, glowing, etc. surfaces uniformly. Fragment
 * branches on which maps the Material carries.
 */
class BlinnPhongShader {
  public:
    // Per-vertex outputs the rasterizer interpolates. T/B are constant per
    // triangle (primitive() writes the same basis to all three corners).
    struct Varyings {
        Vec3f normal, worldPos, T, B;
        Vec2f uv;

        Varyings operator*(float s) const {
            return {.normal = normal * s,
                    .worldPos = worldPos * s,
                    .T = T * s,
                    .B = B * s,
                    .uv = uv * s};
        }
        Varyings operator+(const Varyings &o) const {
            return {.normal = normal + o.normal,
                    .worldPos = worldPos + o.worldPos,
                    .T = T + o.T,
                    .B = B + o.B,
                    .uv = uv + o.uv};
        }
    };

    BlinnPhongShader(Model model, Material material, Mat4f mvp, Vec3f lightDir,
                     Vec3f eye);

    [[nodiscard]] std::array<VertexOut<Varyings>, 3> primitive(size_t faceIdx) const;
    [[nodiscard]] TGAColor fragment(const Varyings &in, int x, int y) const;
    [[nodiscard]] size_t faceCount() const { return model_.nfaces(); }

    // Per-frame uniforms; callers swap these between renders.
    void setMVP(const Mat4f &mvp) { mvp_ = mvp; }
    void setEye(const Vec3f &eye) { eye_ = eye; }
    void setLightDir(const Vec3f &lightDir) { lightDir_ = lightDir; }

    void setShadow(const std::vector<float> *shadowMap, size_t shadowW, size_t shadowH,
                   const Mat4f &lightMVP, const Mat4f &lightVP, float bias) {
        shadowMap_ = shadowMap;
        shadowW_ = shadowW;
        shadowH_ = shadowH;
        lightMVP_ = lightMVP;
        lightVP_ = lightVP;
        shadowBias_ = bias;
    }

    void setAO(const std::vector<float> *ao, size_t aoW, size_t aoH) {
        assert(ao == nullptr || ao->size() == aoW * aoH);
        aoBuffer_ = ao;
        aoW_ = aoW;
        aoH_ = aoH;
    }

    [[nodiscard]] const Model &model() const { return model_; }

    // Mutable so animations can vary the material without rebuilding the shader.
    [[nodiscard]] Material &material() { return material_; }

  private:
    // Direct lighting only (diffuse + specular, no ambient, no clamp).
    [[nodiscard]] float directFactor(const Vec3f &n, const Vec3f &halfway) const;

    // uniforms
    Model model_;
    Material material_;
    Mat4f mvp_;
    Vec3f lightDir_;
    Vec3f eye_;

    // Shadow-map uniforms. When shadowMap_ is null, shading ignores shadows.
    const std::vector<float> *shadowMap_ = nullptr;
    size_t shadowW_ = 0;
    size_t shadowH_ = 0;
    Mat4f lightMVP_, lightVP_;
    float shadowBias_ = 0.f;

    // Ambient-occlusion buffer (screen-space). Null => ambient unscaled.
    const std::vector<float> *aoBuffer_ = nullptr;
    size_t aoW_ = 0;
    size_t aoH_ = 0;
};

/**
 * @brief Depth-only shader for the shadow-map pass.
 *
 * Projects under the light's MVP; only the depth buffer rasterize() fills
 * matters. Borrows its model (non-owning). The depth it stores must stay
 * comparable to the raw clip.z() that inShadow() reads back, which requires the
 * viewport to leave z untouched.
 */
class DepthShader {
  public:
    struct Varyings {
        Varyings operator*(float /*s*/) const { return {}; }
        Varyings operator+(const Varyings & /*o*/) const { return {}; }
    };

    DepthShader(const Model &model, Mat4f lightMVP)
        : model_(&model), lightMVP_(lightMVP) {}

    [[nodiscard]] std::array<VertexOut<Varyings>, 3> primitive(size_t faceIdx) const;
    [[nodiscard]] static TGAColor fragment(const Varyings & /*in*/, int /*x*/,
                                           int /*y*/) {
        return BLACK_COLOR;
    }
    [[nodiscard]] size_t faceCount() const { return model_->nfaces(); }

  private:
    const Model *model_;
    Mat4f lightMVP_;
};
