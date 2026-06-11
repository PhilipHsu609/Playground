#pragma once

#include "tinyrenderer/TGAImage.hpp"

#include <cstddef>
#include <vector>

// Collapse an intensity in [0, 1] into one of `bands` evenly spaced levels.
// Level k (1..bands) covers [(k-1)/bands, k/bands) and has value k/bands, so
// bands=3 -> {0.333, 0.667, 1.0} (the lesson's thresholds). intensity 0 maps to
// the darkest band (1/bands), never pure black. Precondition: bands >= 1.
[[nodiscard]] float quantize(float intensity, int bands);

// Per-pixel Sobel gradient magnitude sqrt(Gx^2 + Gy^2) over a single-channel
// w*h buffer (row-major). Border pixels (no full 3x3 neighborhood) get 0.
[[nodiscard]] std::vector<float> sobelMagnitude(const std::vector<float> &buf, size_t w,
                                                size_t h);
