#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/Vector.hpp"

#include <algorithm>

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

Vec3f perspectiveCorrect(const Vec3f &bary, const Vec3f &clipW) {
    const Vec3f weighted(bary.x() / clipW.x(), bary.y() / clipW.y(),
                         bary.z() / clipW.z());
    const float sum = weighted.x() + weighted.y() + weighted.z();
    return weighted / sum;
}

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
