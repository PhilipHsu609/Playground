#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

namespace {
template <size_t N>
Vec4i findBbox(const std::array<Vec2i, N> &pts) {
    Vec4i bbox(std::numeric_limits<int>::max(), std::numeric_limits<int>::min(),
               std::numeric_limits<int>::max(), std::numeric_limits<int>::min());

    for (auto const &pt : pts) {
        bbox[0] = std::min(bbox[0], pt[0]);
        bbox[1] = std::max(bbox[1], pt[0]);
        bbox[2] = std::min(bbox[2], pt[1]);
        bbox[3] = std::max(bbox[3], pt[1]);
    }

    return bbox;
}

Vec3f barycentric(Vec2f p, Vec2f t0, Vec2f t1, Vec2f t2) {
    const auto v01 = t1 - t0;
    const auto v02 = t2 - t0;
    const auto v0p = p - t0;

    const Vec2f nac(t0[1] - t2[1], -t0[0] + t2[0]);
    const Vec2f nab(t0[1] - t1[1], -t0[0] + t1[0]);

    const float beta = dot(v0p, nac) / dot(v01, nac);
    const float gamma = dot(v0p, nab) / dot(v02, nab);
    const float alpha = 1.f - beta - gamma;

    return Vec3f(alpha, beta, gamma);
}
} // namespace

void line(Vec2i u, Vec2i v, TGAImage &image, TGAColor color) {
    int x0 = u[0];
    int y0 = u[1];
    int x1 = v[0];
    int y1 = v[1];

    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }

    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    const int dx = x1 - x0;
    const int dy = std::abs(y1 - y0);

    int d = 2 * dy - dx;
    int y = y0;

    for (int x = x0; x <= x1; x++) {
        if (steep) {
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }
        if (d > 0) {
            y += y1 > y0 ? 1 : -1;
            d -= 2 * dx;
        }
        d += 2 * dy;
    }
}

void triangle(const std::array<Vec3f, 3> &pts, std::vector<float> &zbuffer,
              TGAImage &image, TGAColor color) {
    const Vec2i t0(pts[0]);
    const Vec2i t1(pts[1]);
    const Vec2i t2(pts[2]);

    const auto bbox = findBbox(std::array{t0, t1, t2});

    const int minX = std::max(bbox[0], 0);
    const int maxX = std::min(bbox[1], static_cast<int>(image.getWidth()) - 1);
    const int minY = std::max(bbox[2], 0);
    const int maxY = std::min(bbox[3], static_cast<int>(image.getHeight()) - 1);

    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            const Vec2f p(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
            const Vec3f bary = barycentric(p, Vec2f(t0), Vec2f(t1), Vec2f(t2));

            if (bary[0] < 0 || bary[1] < 0 || bary[2] < 0) {
                continue;
            }

            const float z =
                bary[0] * pts[0][2] + bary[1] * pts[1][2] + bary[2] * pts[2][2];
            const auto index =
                static_cast<size_t>(y) * image.getWidth() + static_cast<size_t>(x);
            if (zbuffer[index] < z) {
                zbuffer[index] = z;
                image.set(x, y, color);
            }
        }
    }
}
