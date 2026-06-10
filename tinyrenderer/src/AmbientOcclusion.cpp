#include "tinyrenderer/AmbientOcclusion.hpp"

#include <cmath>
#include <numbers>

Vec3f randomHemisphereDir(std::mt19937 &rng) {
    std::uniform_real_distribution<float> u(0.f, 1.f);
    // Uniform direction on the unit hemisphere: pick cos(theta) in [0,1] for
    // the upper half, phi in [0, 2pi).
    const float cosTheta = u(rng);
    const float sinTheta = std::sqrt(std::max(0.f, 1.f - cosTheta * cosTheta));
    const float phi = 2.f * std::numbers::pi_v<float> * u(rng);
    return Vec3f(sinTheta * std::cos(phi), cosTheta, sinTheta * std::sin(phi));
}
