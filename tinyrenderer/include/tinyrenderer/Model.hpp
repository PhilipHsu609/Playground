#pragma once

#include "tinyrenderer/Vector.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <vector>

class Model {
  public:
    explicit Model(const std::filesystem::path &filename);

    [[nodiscard]] size_t nverts() const { return verts_.size(); }
    [[nodiscard]] size_t nnormals() const { return normals_.size(); }
    [[nodiscard]] size_t ntexCoords() const { return texCoords_.size(); }
    [[nodiscard]] size_t nfaces() const { return faces_.size(); }

    // Direct indexed access to the vertex, normal, and texture-coordinate buffers.
    [[nodiscard]] const Vec3f &vert(size_t i) const { return verts_[i]; }
    [[nodiscard]] const Vec3f &normal(size_t i) const { return normals_[i]; }
    [[nodiscard]] const Vec2f &texCoord(size_t i) const { return texCoords_[i]; }

    // Face-relative access: corner `cornerIdx` (0..2) of face `faceIdx`.
    [[nodiscard]] const Vec3f &vert(size_t faceIdx, size_t cornerIdx) const {
        return verts_[faces_[faceIdx][cornerIdx].vertIdx];
    }
    [[nodiscard]] const Vec3f &normal(size_t faceIdx, size_t cornerIdx) const {
        return normals_[faces_[faceIdx][cornerIdx].normalIdx];
    }
    [[nodiscard]] const Vec2f &texCoord(size_t faceIdx, size_t cornerIdx) const {
        return texCoords_[faces_[faceIdx][cornerIdx].texIdx];
    }

  private:
    // An OBJ face entry references one vertex + one texture coord + one normal
    // per corner.
    struct FaceCorner {
        size_t vertIdx;
        size_t texIdx;
        size_t normalIdx;
    };

    std::vector<Vec3f> verts_;
    std::vector<Vec3f> normals_;
    std::vector<Vec2f> texCoords_;
    std::vector<std::array<FaceCorner, 3>> faces_;
};
