#include "tinyrenderer/Outline.hpp"

#include <algorithm>
#include <cassert>

float quantize(float intensity, int bands) {
    assert(bands >= 1);
    const float clamped = std::clamp(intensity, 0.f, 1.f);
    const int level = std::min(bands, static_cast<int>(clamped * static_cast<float>(bands)) + 1);
    return static_cast<float>(level) / static_cast<float>(bands);
}
