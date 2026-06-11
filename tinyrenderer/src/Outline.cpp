#include "tinyrenderer/Outline.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

float quantize(float intensity, int bands) {
    assert(bands >= 1);
    const float clamped = std::clamp(intensity, 0.f, 1.f);
    const int level = std::min(bands, static_cast<int>(clamped * static_cast<float>(bands)) + 1);
    return static_cast<float>(level) / static_cast<float>(bands);
}

std::vector<float> sobelMagnitude(const std::vector<float> &buf, size_t w, size_t h) {
    assert(buf.size() == w * h);
    std::vector<float> mag(w * h, 0.f);
    if (w < 3 || h < 3) {
        return mag;
    }
    for (size_t y = 1; y + 1 < h; ++y) {
        for (size_t x = 1; x + 1 < w; ++x) {
            const float tl = buf[(y - 1) * w + (x - 1)];
            const float tc = buf[(y - 1) * w + x];
            const float tr = buf[(y - 1) * w + (x + 1)];
            const float ml = buf[y * w + (x - 1)];
            const float mr = buf[y * w + (x + 1)];
            const float bl = buf[(y + 1) * w + (x - 1)];
            const float bc = buf[(y + 1) * w + x];
            const float br = buf[(y + 1) * w + (x + 1)];
            const float gx = (-tl + tr) + (-2.f * ml + 2.f * mr) + (-bl + br);
            const float gy = (-tl - 2.f * tc - tr) + (bl + 2.f * bc + br);
            mag[y * w + x] = std::sqrt(gx * gx + gy * gy);
        }
    }
    return mag;
}

void applyOutline(TGAImage &image, const std::vector<float> &depth, size_t w, size_t h,
                  float threshold) {
    assert(depth.size() == w * h);
    assert(image.getWidth() == w && image.getHeight() == h);
    constexpr float bgSentinel = 1e30f; // depth at/above this is background

    // Min/max of covered (finite-range) depths, for normalization.
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const float z : depth) {
        if (z < bgSentinel) {
            minZ = std::min(minZ, z);
            maxZ = std::max(maxZ, z);
        }
    }
    const float range = maxZ - minZ;

    std::vector<float> normalized(w * h, 1.f);
    for (size_t i = 0; i < w * h; ++i) {
        if (depth[i] < bgSentinel) {
            normalized[i] = range > 0.f ? (depth[i] - minZ) / range : 0.f;
        }
    }

    const std::vector<float> mag = sobelMagnitude(normalized, w, h);
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            if (mag[y * w + x] > threshold) {
                image.set(static_cast<int>(x), static_cast<int>(y), BLACK_COLOR);
            }
        }
    }
}
