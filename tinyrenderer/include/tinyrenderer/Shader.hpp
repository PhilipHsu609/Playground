#pragma once

#include "Drawer.hpp"
#include "Matrix.hpp"
#include "Model.hpp"
#include "TGAImage.hpp"
#include "Vector.hpp"

#include <cstddef>
#include <optional>
#include <ranges>

/**
 * @brief Programmable shader interface.
 *
 * The rasterizer calls vertex() three times per triangle and fragment() once
 * per covered pixel. Concrete shaders cache per-vertex data ("varyings") in
 * member state so fragment() can interpolate them via barycentric coordinates.
 *
 * Iterate with `for (const auto& tri : shader.triangles())`.
 */
struct IShader {
    IShader() = default;
    virtual ~IShader() = default;
    IShader(const IShader &) = default;
    IShader(IShader &&) = default;
    IShader &operator=(const IShader &) = default;
    IShader &operator=(IShader &&) = default;

    [[nodiscard]] virtual Vec3f vertex(size_t faceIdx, size_t cornerIdx) = 0;
    [[nodiscard]] virtual TGAColor fragment(const Vec3f &bary) const = 0;

    /// Lazy view over the model's triangles after vertex processing.
    [[nodiscard]] auto triangles() {
        return std::views::iota(size_t{0}, faceCount()) |
               std::views::transform([this](size_t i) {
                   return Triangle{vertex(i, 0), vertex(i, 1), vertex(i, 2)};
               });
    }

  private:
    [[nodiscard]] virtual size_t faceCount() const = 0;
};

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
class BlinnPhongShader : public IShader {
  public:
    BlinnPhongShader(Model model, Material material, Mat4f mvp, Mat4f vp, Vec3f lightDir,
                     Vec3f eye);

    [[nodiscard]] Vec3f vertex(size_t faceIdx, size_t cornerIdx) override;
    [[nodiscard]] TGAColor fragment(const Vec3f &bary) const override;

  private:
    [[nodiscard]] size_t faceCount() const override { return model_.nfaces(); }

    // Blinn-Phong lighting intensity at a shaded point. Returns a factor in
    // [0, 1] that modulates the surface albedo.
    [[nodiscard]] float blinnPhongFactor(const Vec3f &n, const Vec3f &halfway) const;

    // uniforms
    Model model_;
    Material material_;
    Mat4f mvp_, vp_;
    Vec3f lightDir_;
    Vec3f eye_;

    // varyings (one column per triangle vertex)
    Mat3f normals_;
    Mat3f worldPositions_;
    Mat<float, 2, 3> texCoords_;
};
