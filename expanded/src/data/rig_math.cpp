#include "expanded/data/rig_math.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace expanded {

namespace {

double SumOf(const double *xs, std::size_t n) {
    if (n == 0) {
        return 0.0;
    }
    double f = 0.0 + xs[0], c = 0.0;
    for (std::size_t i = 1; i < n; i++) {
        double x = xs[i], t = f + x;
        if (std::fabs(f) >= std::fabs(x)) {
            c += (f - t) + x;
        } else {
            c += (x - t) + f;
        }
        f = t;
    }
    if (c != 0 && std::isfinite(c)) {
        f += c;
    }
    return f;
}

// CPython's vector_norm: the squares summed in double length, with one correction step.
double VectorNorm(std::vector<double> vec, double max) {
    std::size_t n = vec.size();
    if (std::isinf(max)) {
        return max;
    }
    for (double v : vec) {
        if (std::isnan(v)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }
    if (max == 0.0 || n <= 1) {
        return max;
    }
    int max_e = std::ilogb(max) + 1;
    if (max_e < -1023) {
        constexpr double kTiny = std::numeric_limits<double>::denorm_min();
        for (double &v : vec) {
            v /= kTiny;
        }
        return kTiny * VectorNorm(vec, max / kTiny);
    }
    double scale = std::scalbn(1.0, -max_e), csum = 1.0, frac1 = 0.0, frac2 = 0.0;
    for (double v : vec) {
        double x = v * scale;
        double hi = x * x, lo = std::fma(x, x, -hi);
        double sum = csum + hi, sum_lo = (csum - sum) + hi;
        csum = sum;
        frac1 += lo;
        frac2 += sum_lo;
    }
    double h = std::sqrt(csum - 1.0 + (frac1 + frac2));
    double hi = -h * h, lo = std::fma(-h, h, -hi);
    double sum = csum + hi, sum_lo = (csum - sum) + hi;
    csum = sum;
    frac1 += lo;
    frac2 += sum_lo;
    double x = csum - 1.0 + (frac1 + frac2);
    h += x / (2.0 * h);
    return h / scale;
}
} // namespace

double Sum(std::initializer_list<double> values) {
    return SumOf(values.begin(), values.size());
}

double Sum(const std::vector<double> &values) {
    return SumOf(values.data(), values.size());
}

double Round(double x, int ndigits) {
    if (std::isnan(x) || std::isinf(x) || x == 0) {
        return x;
    }
    double magnitude = std::fabs(x), result;
    if (ndigits == 0) {
        result = std::nearbyint(magnitude);
    } else {
        char buffer[512];
        auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer) - 1, magnitude, std::chars_format::fixed, ndigits);
        if (error != std::errc{}) {
            throw std::runtime_error("cannot round");
        }
        *end = '\0';
        result = std::strtod(buffer, nullptr);
    }
    return x < 0 ? -result : result;
}

double FMod(double a, double b) {
    double mod = std::fmod(a, b);
    if (mod != 0 && ((b < 0) != (mod < 0))) {
        mod += b;
    } else if (mod == 0) {
        mod = std::copysign(0.0, b);
    }
    return mod;
}

int Mod(int a, int m) {
    return ((a % m) + m) % m;
}

double Dist(std::span<const double> p, std::span<const double> q) {
    std::vector<double> d(p.size());
    double              max = 0;
    for (std::size_t i = 0; i < p.size(); i++) {
        d[i] = std::fabs(p[i] - q[i]);
        if (d[i] > max) {
            max = d[i];
        }
    }
    return VectorNorm(d, max);
}

double Hypot(std::initializer_list<double> values) {
    std::vector<double> d;
    double              max = 0;
    for (double v : values) {
        d.push_back(std::fabs(v));
        if (d.back() > max) {
            max = d.back();
        }
    }
    return VectorNorm(d, max);
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

double Dot(const Vec4 &a, const Vec4 &b) {
    double sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

Vec3 Unit(const Vec3 &a) {
    double n = std::sqrt(Dot(a, a));
    return Scale(a, 1.0 / (n == 0.0 ? 1.0 : n));
}

double DotExact(const Vec3 &a, const Vec3 &b) {
    return Sum({a[0] * b[0], a[1] * b[1], a[2] * b[2]});
}

Vec3 UnitExact(const Vec3 &v) {
    double n = std::sqrt(Sum({v[0] * v[0], v[1] * v[1], v[2] * v[2]}));
    if (n == 0) {
        n = 1.0;
    }
    return {v[0] / n, v[1] / n, v[2] / n};
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

Mat3 MatMulExact(const Mat3 &a, const Mat3 &b) {
    Mat3 r{};
    for (std::size_t i = 0; i < 3; i++) {
        for (std::size_t c = 0; c < 3; c++) {
            r[i][c] = Sum({a[i][0] * b[0][c], a[i][1] * b[1][c], a[i][2] * b[2][c]});
        }
    }
    return r;
}

Mat4 MatMulExact(const Mat4 &a, const Mat4 &b) {
    Mat4 r{};
    for (std::size_t i = 0; i < 4; i++) {
        for (std::size_t j = 0; j < 4; j++) {
            r[i][j] = Sum({a[i][0] * b[0][j], a[i][1] * b[1][j], a[i][2] * b[2][j], a[i][3] * b[3][j]});
        }
    }
    return r;
}

Vec3 MatVec(const Mat3 &m, const Vec3 &v) {
    return {Dot(m[0], v), Dot(m[1], v), Dot(m[2], v)};
}

Mat3 Transpose(const Mat3 &a) {
    Mat3 r{};
    for (std::size_t i = 0; i < 3; i++) {
        for (std::size_t c = 0; c < 3; c++) {
            r[i][c] = a[c][i];
        }
    }
    return r;
}

Mat4 MatFromRT(const Mat3 &r, const Vec3 &t) {
    return {
        {{r[0][0], r[0][1], r[0][2], 0.0}, {r[1][0], r[1][1], r[1][2], 0.0}, {r[2][0], r[2][1], r[2][2], 0.0}, {t[0], t[1], t[2], 1.0}}
    };
}

Mat3 RotOf(const Mat4 &m) {
    return {
        {{m[0][0], m[0][1], m[0][2]}, {m[1][0], m[1][1], m[1][2]}, {m[2][0], m[2][1], m[2][2]}}
    };
}

Vec3 TransOf(const Mat4 &m) {
    return {m[3][0], m[3][1], m[3][2]};
}

Mat4 RigidInv(const Mat4 &m) {
    Mat3 rt{};
    for (std::size_t i = 0; i < 3; i++) {
        for (std::size_t j = 0; j < 3; j++) {
            rt[i][j] = m[j][i];
        }
    }
    Vec3 t = {m[3][0], m[3][1], m[3][2]}, nt{};
    for (std::size_t j = 0; j < 3; j++) {
        nt[j] = -(t[0] * rt[0][j] + t[1] * rt[1][j] + t[2] * rt[2][j]);
    }
    return MatFromRT(rt, nt);
}

Vec3 XformPt(const Mat4 &m, const Vec3 &v) {
    return {v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0] + m[3][0], v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1] + m[3][1],
            v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2] + m[3][2]};
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

Mat3 RotAbout(const Vec3 &axis, double deg) {
    Vec3   u = UnitExact(axis);
    double x = u[0], y = u[1], z = u[2], c = std::cos(Radians(deg)), sn = std::sin(Radians(deg)), t = 1 - c;
    Mat3   r = {
        {{t * x * x + c, t * x * y - sn * z, t * x * z + sn * y},
         {t * x * y + sn * z, t * y * y + c, t * y * z - sn * x},
         {t * x * z - sn * y, t * y * z + sn * x, t * z * z + c}}
    };
    return Transpose(r);
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

Vec3 Pitch(const Vec3 &v, double p) {
    double c = std::cos(p), s = std::sin(p);
    return {v[0], v[1] * c + v[2] * s, -v[1] * s + v[2] * c};
}

Vec3 RotY(const Vec3 &v, double yaw) {
    double c = std::cos(yaw), s = std::sin(yaw);
    return {v[0] * c + v[2] * s, v[1], -v[0] * s + v[2] * c};
}

double Radians(double deg) {
    return deg * (std::numbers::pi / 180.0);
}

double Smooth(double t) {
    t = std::min(std::max(t, 0.0), 1.0);
    return t * t * (3 - 2 * t);
}

Vec4 MatToQuat(const Mat3 &r) {
    double m00 = r[0][0], m01 = r[0][1], m02 = r[0][2], m10 = r[1][0], m11 = r[1][1], m12 = r[1][2], m20 = r[2][0], m21 = r[2][1],
           m22 = r[2][2];
    double tr = m00 + m11 + m22, w, x, y, z, s;
    if (tr > 0) {
        s = std::sqrt(tr + 1.0) * 2;
        w = 0.25 * s;
        x = (m21 - m12) / s;
        y = (m02 - m20) / s;
        z = (m10 - m01) / s;
    } else if (m00 >= m11 && m00 >= m22) {
        s = std::sqrt(1.0 + m00 - m11 - m22) * 2;
        w = (m21 - m12) / s;
        x = 0.25 * s;
        y = (m01 + m10) / s;
        z = (m02 + m20) / s;
    } else if (m11 >= m22) {
        s = std::sqrt(1.0 + m11 - m00 - m22) * 2;
        w = (m02 - m20) / s;
        x = (m01 + m10) / s;
        y = 0.25 * s;
        z = (m12 + m21) / s;
    } else {
        s = std::sqrt(1.0 + m22 - m00 - m11) * 2;
        w = (m10 - m01) / s;
        x = (m02 + m20) / s;
        y = (m12 + m21) / s;
        z = 0.25 * s;
    }
    double n = std::sqrt(w * w + x * x + y * y + z * z);
    if (n == 0) {
        n = 1.0;
    }
    return {w / n, x / n, y / n, z / n};
}

Vec4 MatToQuatUnnormalised(const Mat3 &r) {
    double m00 = r[0][0], m01 = r[0][1], m02 = r[0][2], m10 = r[1][0], m11 = r[1][1], m12 = r[1][2], m20 = r[2][0],
           m21 = r[2][1], m22 = r[2][2];
    double tr = m00 + m11 + m22, s;
    if (tr > 0) {
        s = std::sqrt(tr + 1.0) * 2;
        return {0.25 * s, (m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s};
    }
    if (m00 >= m11 && m00 >= m22) {
        s = std::sqrt(1.0 + m00 - m11 - m22) * 2;
        return {(m21 - m12) / s, 0.25 * s, (m01 + m10) / s, (m02 + m20) / s};
    }
    if (m11 >= m22) {
        s = std::sqrt(1.0 + m11 - m00 - m22) * 2;
        return {(m02 - m20) / s, (m01 + m10) / s, 0.25 * s, (m12 + m21) / s};
    }
    s = std::sqrt(1.0 + m22 - m00 - m11) * 2;
    return {(m10 - m01) / s, (m02 + m20) / s, (m12 + m21) / s, 0.25 * s};
}

Mat3 QuatToMat(const Vec4 &q) {
    double w = q[0], x = q[1], y = q[2], z = q[3];
    return {
        {{1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)},
         {2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)},
         {2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)}}
    };
}

Vec4 QuatMul(const Vec4 &a, const Vec4 &b) {
    return {
        a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3],
        a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2],
        a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1],
        a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0],
    };
}

Vec4 SlerpExact(const Vec4 &q0, Vec4 q1, double t) {
    double d = Sum({q0[0] * q1[0], q0[1] * q1[1], q0[2] * q1[2], q0[3] * q1[3]});
    if (d < 0) {
        for (double &x : q1) {
            x = -x;
        }
        d = -d;
    }
    Vec4 r;
    if (d > 0.9995) {
        for (std::size_t i = 0; i < 4; i++) {
            r[i] = q0[i] + (q1[i] - q0[i]) * t;
        }
    } else {
        double th = std::acos(std::max(-1.0, std::min(1.0, d))), s0 = std::sin((1 - t) * th) / std::sin(th), s1 = std::sin(t * th) / std::sin(th);
        for (std::size_t i = 0; i < 4; i++) {
            r[i] = q0[i] * s0 + q1[i] * s1;
        }
    }
    double n = std::sqrt(Sum({r[0] * r[0], r[1] * r[1], r[2] * r[2], r[3] * r[3]}));
    if (n == 0) {
        n = 1.0;
    }
    return {r[0] / n, r[1] / n, r[2] / n, r[3] / n};
}

Vec4 Wide(const std::array<float, 4> &v) {
    return {v[0], v[1], v[2], v[3]};
}

Vec4 SlerpKeys(const std::array<float, 4> &a, const std::array<float, 4> &bf, double t) {
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

Vec4 BlendKeys(const std::array<float, 4> &a, const std::array<float, 4> &b, double t, bool rotation) {
    if (rotation) {
        return SlerpKeys(a, b, t);
    }
    Vec4 v;
    for (int i = 0; i < 4; i++) {
        v[std::size_t(i)] = a[std::size_t(i)] + (b[std::size_t(i)] - double(a[std::size_t(i)])) * t;
    }
    return v;
}

} // namespace expanded
