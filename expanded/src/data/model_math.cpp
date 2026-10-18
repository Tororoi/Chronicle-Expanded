#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

#include "expanded/data/model_parts.hpp"

namespace expanded {

double ExactSum(std::initializer_list<double> values) {
    double sum = 0.0, compensation = 0.0;
    for (double x : values) {
        double t = sum + x;
        compensation += std::fabs(sum) >= std::fabs(x) ? (sum - t) + x : (x - t) + sum;
        sum = t;
    }
    return sum + compensation;
}

Vec3 Add(const Vec3 &a, const Vec3 &b) {
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

Vec3 Scale(const Vec3 &a, double s) {
    return {a[0] * s, a[1] * s, a[2] * s};
}

Vec3 Cross(const Vec3 &a, const Vec3 &b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

double Dot(const Vec3 &a, const Vec3 &b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

Vec3 Unit(const Vec3 &a) {
    double n = std::sqrt(Dot(a, a));
    return Scale(a, 1.0 / (n == 0.0 ? 1.0 : n));
}

Mat4 MatMul4(const Mat4 &a, const Mat4 &b) {
    Mat4 out{};
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            out[r][c] = ExactSum({a[r][0] * b[0][c], a[r][1] * b[1][c], a[r][2] * b[2][c], a[r][3] * b[3][c]});
        }
    }
    return out;
}

Mat3 MatMul(const Mat3 &a, const Mat3 &b) {
    Mat3 out{};
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            out[r][c] = a[r][0] * b[0][c] + a[r][1] * b[1][c] + a[r][2] * b[2][c];
        }
    }
    return out;
}

Vec3 MatVec(const Mat3 &m, const Vec3 &v) {
    return {Dot(m[0], v), Dot(m[1], v), Dot(m[2], v)};
}

Mat3 AxisRotation(const Vec3 &k, double angle) {
    double c = std::cos(angle), s = std::sin(angle), C = 1 - c, x = k[0], y = k[1], z = k[2];
    return {
        {
         {c + x * x * C, x * y * C - z * s, x * z * C + y * s},
         {y * x * C + z * s, c + y * y * C, y * z * C - x * s},
         {z * x * C - y * s, z * y * C + x * s, c + z * z * C},
         }
    };
}

Mat3 RotationYTo(const Vec3 &direction) {
    Vec3   d = Unit(direction);
    Vec3   y = {0.0, 1.0, 0.0};
    double c = std::clamp(Dot(y, d), -1.0, 1.0);
    Vec3   axis = Cross(y, d);
    double s = std::sqrt(Dot(axis, axis));
    if (s < 1e-6) {
        return c > 0 ? Mat3{
                           {{1, 0, 0}, {0, c, 0}, {0, 0, c}}
        }
                     : Mat3{{{1, 0, 0}, {0, -1, 0}, {0, 0, -1}}};
    }
    return AxisRotation(Scale(axis, 1.0 / s), std::atan2(s, c));
}

std::array<double, 4> Wide(const std::array<float, 4> &v) {
    return {v[0], v[1], v[2], v[3]};
}

std::array<double, 4> SlerpKeys(const std::array<float, 4> &a, const std::array<float, 4> &bf, double t) {
    std::array<double, 4> b = {bf[0], bf[1], bf[2], bf[3]};
    double                d = double(a[0]) * b[0] + double(a[1]) * b[1] + double(a[2]) * b[2] + double(a[3]) * b[3];
    if (d < 0) {
        for (double &v : b) {
            v = -v;
        }
        d = -d;
    }
    double w0, w1;
    if (d > 0.9995) {
        w0 = 1 - t;
        w1 = t;
    } else {
        double th = std::acos(std::min(1.0, d)), sn = std::sin(th);
        w0 = std::sin((1 - t) * th) / sn;
        w1 = std::sin(t * th) / sn;
    }
    std::array<double, 4> q;
    for (int i = 0; i < 4; i++) {
        q[i] = w0 * a[i] + w1 * b[i];
    }
    double n = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    for (double &v : q) {
        v /= (n == 0 ? 1.0 : n);
    }
    return q;
}

} // namespace expanded
