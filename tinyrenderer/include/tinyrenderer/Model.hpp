#pragma once

#include "tinyrenderer/Vector.hpp"

#include <array>
#include <cstddef>
#include <vector>

class Model {
  public:
    explicit Model(const char *filename);

    [[nodiscard]] size_t nverts() const { return verts_.size(); }
    [[nodiscard]] size_t nnormals() const { return normals_.size(); }
    [[nodiscard]] size_t nfaces() const { return faces_.size(); }

    // Direct indexed access to the vertex and normal buffers.
    [[nodiscard]] const Vec3f &vert(size_t i) const { return verts_[i]; }
    [[nodiscard]] const Vec3f &normal(size_t i) const { return normals_[i]; }

    // Face-relative access: corner `cornerIdx` (0..2) of face `faceIdx`.
    [[nodiscard]] const Vec3f &vert(size_t faceIdx, size_t cornerIdx) const {
        return verts_[faces_[faceIdx][cornerIdx].vertIdx];
    }
    [[nodiscard]] const Vec3f &normal(size_t faceIdx, size_t cornerIdx) const {
        return normals_[faces_[faceIdx][cornerIdx].normalIdx];
    }

  private:
    // An OBJ face entry references one vertex + one normal per corner
    // (texture index is parsed but not yet stored).
    struct FaceCorner {
        size_t vertIdx;
        size_t normalIdx;
    };

    std::vector<Vec3f> verts_;
    std::vector<Vec3f> normals_;
    std::vector<std::array<FaceCorner, 3>> faces_;
};
