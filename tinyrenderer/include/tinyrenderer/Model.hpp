#pragma once

#include "tinyrenderer/Vector.hpp"

#include <vector>

class Model {
  public:
    Model(const char *filename);
    size_t nverts() const { return verts_.size(); };
    size_t nfaces() const { return faces_.size(); };
    Vec3f vert(size_t i) const { return verts_[i]; };
    std::vector<size_t> face(size_t idx) const { return faces_[idx]; };

  private:
    std::vector<Vec3f> verts_;
    std::vector<std::vector<size_t>> faces_;
};