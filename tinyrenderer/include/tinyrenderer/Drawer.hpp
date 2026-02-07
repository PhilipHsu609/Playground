#pragma once

#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <array>
#include <vector>

void line(Vec2i u, Vec2i v, TGAImage &image, TGAColor color);

void triangle(const std::array<Vec3f, 3> &pts, std::vector<float> &zbuffer,
              TGAImage &image, TGAColor color);