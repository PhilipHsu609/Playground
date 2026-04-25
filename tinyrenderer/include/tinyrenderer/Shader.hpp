#pragma once

#include "Drawer.hpp"
#include "Matrix.hpp"
#include "Model.hpp"
#include "TGAImage.hpp"
#include "Vector.hpp"

#include <cstddef>
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
    /// Each dereference invokes vertex() three times to populate varyings,
    /// then yields the resulting screen-space Triangle.
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
 * @brief Blinn-Phong shading with a single base color (no diffuse map).
 */
class BlinnPhongShader : public IShader {
  public:
    BlinnPhongShader(Model model, Mat4f mvp, Mat4f vp, Vec3f lightDir, Vec3f eye);

    [[nodiscard]] Vec3f vertex(size_t faceIdx, size_t cornerIdx) override;
    [[nodiscard]] TGAColor fragment(const Vec3f &bary) const override;

  private:
    [[nodiscard]] size_t faceCount() const override { return model_.nfaces(); }

    // uniforms
    Model model_;
    Mat4f mvp_, vp_;
    Vec3f lightDir_;
    Vec3f eye_;
    TGAColor color_ = TGAColor(255, 255, 255);

    // varyings (one column per triangle vertex)
    Mat3f normals_;
    Mat3f worldPositions_;
};

/**
 * @brief Blinn-Phong shading modulated by a per-pixel diffuse texture sample.
 */
class BlinnPhongTexturedShader : public IShader {
  public:
    BlinnPhongTexturedShader(Model model, TGAImage diffuseMap, Mat4f mvp, Mat4f vp,
                             Vec3f lightDir, Vec3f eye);

    [[nodiscard]] Vec3f vertex(size_t faceIdx, size_t cornerIdx) override;
    [[nodiscard]] TGAColor fragment(const Vec3f &bary) const override;

  private:
    [[nodiscard]] size_t faceCount() const override { return model_.nfaces(); }

    // uniforms
    Model model_;
    TGAImage diffuseMap_;
    Mat4f mvp_, vp_;
    Vec3f lightDir_;
    Vec3f eye_;

    // varyings (one column per triangle vertex)
    Mat3f normals_;
    Mat3f worldPositions_;
    Mat<float, 2, 3> texCoords_;
};
