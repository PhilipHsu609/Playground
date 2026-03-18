#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

namespace {
struct BBox {
    int minX, maxX, minY, maxY;
};

template <size_t N>
BBox findBbox(const std::array<Vec2i, N> &pts) {
    BBox bbox{std::numeric_limits<int>::max(), std::numeric_limits<int>::min(),
              std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};

    for (const auto &pt : pts) {
        bbox.minX = std::min(bbox.minX, pt.x());
        bbox.maxX = std::max(bbox.maxX, pt.x());
        bbox.minY = std::min(bbox.minY, pt.y());
        bbox.maxY = std::max(bbox.maxY, pt.y());
    }

    return bbox;
}

Vec3f barycentric(Vec2f p, Vec2f t0, Vec2f t1, Vec2f t2) {
    const auto v01 = t1 - t0;
    const auto v02 = t2 - t0;
    const auto v0p = p - t0;

    const Vec2f nac(t0.y() - t2.y(), t2.x() - t0.x());
    const Vec2f nab(t0.y() - t1.y(), t1.x() - t0.x());

    const float beta = dot(v0p, nac) / dot(v01, nac);
    const float gamma = dot(v0p, nab) / dot(v02, nab);
    const float alpha = 1.f - beta - gamma;

    return Vec3f(alpha, beta, gamma);
}
} // namespace

void line(Vec2i u, Vec2i v, TGAImage &image, TGAColor color) {
    int x0 = u.x();
    int y0 = u.y();
    int x1 = v.x();
    int y1 = v.y();

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

    const int minX = std::max(bbox.minX, 0);
    const int maxX = std::min(bbox.maxX, static_cast<int>(image.getWidth()) - 1);
    const int minY = std::max(bbox.minY, 0);
    const int maxY = std::min(bbox.maxY, static_cast<int>(image.getHeight()) - 1);

    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            const Vec2f p(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
            const Vec3f bary = barycentric(p, Vec2f(t0), Vec2f(t1), Vec2f(t2));

            if (bary.x() < 0 || bary.y() < 0 || bary.z() < 0) {
                continue;
            }

            const float z =
                bary.x() * pts[0].z() + bary.y() * pts[1].z() + bary.z() * pts[2].z();
            const auto index =
                static_cast<size_t>(y) * image.getWidth() + static_cast<size_t>(x);
            if (zbuffer[index] < z) {
                zbuffer[index] = z;
                image.set(x, y, color);
            }
        }
    }
}
