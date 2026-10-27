#pragma once

// The maths the model builders share, operation for operation as the PS2 mod's builders did it, so the built bytes are
// the same: row-vector matrices (v' = v * M, rotations as 3x3 rows), quaternions scalar first. A sum is plain (left to
// right) unless the function's name says Exact: then it is Python's sum(), the numerics the spirit cat was authored in.
// Includes no game header, so dcdata can build with it.

#include <array>
#include <initializer_list>
#include <span>
#include <vector>

namespace expanded {

using Vec2 = std::array<double, 2>;
using Vec3 = std::array<double, 3>;
using Vec4 = std::array<double, 4>;
using Mat3 = std::array<Vec3, 3>; // rows
using Mat4 = std::array<Vec4, 4>;

// Python's numerics: sum() (Neumaier-compensated), round(x, ndigits) (half to even on the exact value), a % b for
// floats, math.dist and math.hypot (the double-length sum with its correction), and an int % that is never negative.
double Sum(std::initializer_list<double> values);
double Sum(const std::vector<double> &values);
double Round(double x, int ndigits);
double FMod(double a, double b);
int    Mod(int a, int m);
double Dist(std::span<const double> p, std::span<const double> q);
double Hypot(std::initializer_list<double> values);

// Vectors.
Vec3   Add(const Vec3 &a, const Vec3 &b);
Vec3   Scale(const Vec3 &a, double s);
Vec3   Cross(const Vec3 &a, const Vec3 &b);
double Dot(const Vec3 &a, const Vec3 &b);
double Dot(const Vec4 &a, const Vec4 &b);
Vec3   Unit(const Vec3 &a);
double DotExact(const Vec3 &a, const Vec3 &b);
Vec3   UnitExact(const Vec3 &v);

// Matrices: products, a matrix times a column vector and a row vector times a matrix, the transpose, a matrix from a rotation and translation and back,
// a rigid inverse, a point through one.
Mat3 MatMul(const Mat3 &a, const Mat3 &b);
Mat3 MatMulExact(const Mat3 &a, const Mat3 &b);
Mat4 MatMulExact(const Mat4 &a, const Mat4 &b);
Vec3 MatVec(const Mat3 &m, const Vec3 &v);
Vec3 VecMatExact(const Vec3 &v, const Mat3 &m);
Mat3 Transpose(const Mat3 &a);
Mat4 MatFromRT(const Mat3 &r, const Vec3 &t);
Mat3 RotOf(const Mat4 &m);
Vec3 TransOf(const Mat4 &m);
Mat4 RigidInv(const Mat4 &m);
Vec3 XformPt(const Mat4 &m, const Vec3 &v);

// Rotations: by angle radians about a unit axis, right-hand rule (Rodrigues); by deg degrees about an axis, as row
// vectors; the one taking +Y onto unit d; a vector pitched about X and turned about Y; degrees as radians; smoothstep
// of t clamped to [0, 1].
Mat3   AxisRotation(const Vec3 &axis, double angle);
Mat3   RotAbout(const Vec3 &axis, double deg);
Mat3   RotationYTo(const Vec3 &d);
Vec3   Pitch(const Vec3 &v, double p);
Vec3   RotY(const Vec3 &v, double yaw);
double Radians(double deg);
double Smooth(double t);

// Quaternions (w first): a rotation's (Shepperd's, the engine's quaternion-to-matrix inverted), normalised or not; its
// rotation; the Hamilton product; two slerped the shorter way round, linear past 0.9995, with Python's sums.
Vec4 MatToQuat(const Mat3 &r);
Vec4 MatToQuatUnnormalised(const Mat3 &r);
Mat3 QuatToMat(const Vec4 &q);
Vec4 QuatMul(const Vec4 &a, const Vec4 &b);
Vec4 SlerpExact(const Vec4 &q0, Vec4 q1, double t);

// .mot key values: as doubles; two rotation keys slerped the shorter way round, linear past 0.9995 (plain sums); two
// keys t of the way, a rotation slerped so, the rest lerped.
Vec4 Wide(const std::array<float, 4> &v);
Vec4 SlerpKeys(const std::array<float, 4> &a, const std::array<float, 4> &b, double t);
Vec4 BlendKeys(const std::array<float, 4> &a, const std::array<float, 4> &b, double t, bool rotation);

} // namespace expanded
