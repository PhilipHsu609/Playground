#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace {
struct BBox {
    int minX, maxX, minY, maxY;
};

template <size_t N>
BBox findBbox(const std::array<Vec2f, N> &pts) {
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto &pt : pts) {
        minX = std::min(minX, pt.x());
        maxX = std::max(maxX, pt.x());
        minY = std::min(minY, pt.y());
        maxY = std::max(maxY, pt.y());
    }

    return BBox{
        static_cast<int>(std::floor(minX)),
        static_cast<int>(std::ceil(maxX)),
        static_cast<int>(std::floor(minY)),
        static_cast<int>(std::ceil(maxY)),
    };
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

float interpolateZ(const Vec3f &bary, const Triangle &triangle) {
    return bary.x() * triangle[0].z() + bary.y() * triangle[1].z() +
           bary.z() * triangle[2].z();
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

void rasterize(const Triangle &triangle, std::vector<float> &zbuffer,
               TGAImage &frameBuffer, TGAColor color) {
    const Vec2f t0(triangle[0]);
    const Vec2f t1(triangle[1]);
    const Vec2f t2(triangle[2]);

    const auto bbox = findBbox(std::array{t0, t1, t2});

    const int minX = std::max(bbox.minX, 0);
    const int maxX = std::min(bbox.maxX, static_cast<int>(frameBuffer.getWidth()) - 1);
    const int minY = std::max(bbox.minY, 0);
    const int maxY = std::min(bbox.maxY, static_cast<int>(frameBuffer.getHeight()) - 1);

    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            const Vec2f p(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
            const Vec3f bary = barycentric(p, t0, t1, t2);

            // If any of the barycentric coordinates is negative, the point is outside the
            // triangle
            if (bary.x() < 0 || bary.y() < 0 || bary.z() < 0) {
                continue;
            }

            // Interpolate the z value using the barycentric coordinates
            const float z = interpolateZ(bary, triangle);

            const auto index =
                static_cast<size_t>(y) * frameBuffer.getWidth() + static_cast<size_t>(x);
            if (zbuffer[index] > z) {
                zbuffer[index] = z;
                frameBuffer.set(x, y, color);
            }
        }
    }
}
