#pragma once

#include "tinyrenderer/Vector.hpp"

#include <vector>

class Model {
  public:
    explicit Model(const char *filename);
    [[nodiscard]] size_t nverts() const { return verts_.size(); }
    [[nodiscard]] size_t nfaces() const { return faces_.size(); }
    [[nodiscard]] Vec3f vert(size_t i) const { return verts_[i]; }
    [[nodiscard]] std::vector<size_t> face(size_t idx) const { return faces_[idx]; }

  private:
    std::vector<Vec3f> verts_;
    std::vector<std::vector<size_t>> faces_;
};
