#pragma once

#include "tinyrenderer/Vector.hpp"

#include <random>

// A seeded unit direction over the upper hemisphere (y >= 0): the "sky"
// directions a surface point's visibility is sampled against.
[[nodiscard]] Vec3f randomHemisphereDir(std::mt19937 &rng);
