#include "expanded/data/arm_reach.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace expanded {

namespace {

Mat4 Mul(const Mat4 &a, const Mat4 &b) {
    Mat4 r{};
    for (std::size_t i = 0; i < 4; i++) {
        for (std::size_t j = 0; j < 4; j++) {
            r[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
        }
    }
    return r;
}

Vec3 Sub(const Vec3 &a, const Vec3 &b) {
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

double Length(const Vec3 &a) {
    return std::sqrt(Dot(a, a));
}

// v as a row vector through r.
Vec3 Through(const Vec3 &v, const Mat3 &r) {
    return {v[0] * r[0][0] + v[1] * r[1][0] + v[2] * r[2][0], v[0] * r[0][1] + v[1] * r[1][1] + v[2] * r[2][1],
            v[0] * r[0][2] + v[1] * r[1][2] + v[2] * r[2][2]};
}

// Swings the collarbone to put the shoulder as near as it can to an upper arm's length from elbow (where the elbow is
// wanted) and within reach of wrist: the least swing (whole degrees about two axes square to the collarbone) that does
// best.
void Shrug(Skeleton &s, const ArmJoints &arm, const Vec3 &elbow, const Vec3 &wrist) {
    const Vec3   at = s.Position(arm.collarbone), from = Sub(s.Position(arm.shoulder), at);
    const double upper = Length(Sub(s.Position(arm.shoulder), s.Position(arm.elbow)));
    const double reach = upper + Length(Sub(s.Position(arm.elbow), s.Position(arm.wrist))) - kReachSlack;
    Vec3         across = Cross(from, Sub(elbow, at));
    if (Dot(across, across) < 1e-12) {
        across = Cross(from, {0.0, 1.0, 0.0});
    }
    const Vec3               u = Unit(across), v = Unit(Cross(from, u));
    constexpr int            kSteps = 2 * static_cast<int>(kCollarboneSwing) + 1;
    std::array<Mat3, kSteps> about_v{};
    for (int j = 0; j < kSteps; j++) {
        about_v[std::size_t(j)] = RotAbout(v, j - kCollarboneSwing);
    }
    Mat3   best = RotAbout(u, 0.0);
    double best_score = 1e30;
    for (int i = 0; i < kSteps; i++) {
        const Mat3 about_u = RotAbout(u, i - kCollarboneSwing);
        const Vec3 swung = Through(from, about_u);
        for (int j = 0; j < kSteps; j++) {
            const Vec3   shoulder = Add(at, Through(swung, about_v[std::size_t(j)]));
            const double di = i - kCollarboneSwing, dj = j - kCollarboneSwing;
            const double over = std::max(0.0, Length(Sub(shoulder, wrist)) - reach);
            const double score = std::abs(Length(Sub(shoulder, elbow)) - upper) + 10.0 * over + 1e-5 * (di * di + dj * dj);
            if (score < best_score) {
                best_score = score;
                best = MatMul(about_u, about_v[std::size_t(j)]);
            }
        }
    }
    s.Turn(arm.collarbone, best);
}

} // namespace

Mat4 Skeleton::World(int joint) const {
    const int up = parent[std::size_t(joint)];
    return up < 0 ? local[std::size_t(joint)] : Mul(local[std::size_t(joint)], World(up));
}

Vec3 Skeleton::Position(int joint) const {
    return TransOf(World(joint));
}

void Skeleton::TurnAbout(int joint, const Mat3 &turn, const Vec3 &pivot) {
    const Vec3 moved = Through(pivot, turn);
    const Mat4 world = Mul(World(joint), MatFromRT(turn, Sub(pivot, moved)));
    const int  up = parent[std::size_t(joint)];
    local[std::size_t(joint)] = up < 0 ? world : Mul(world, RigidInv(World(up)));
}

void Skeleton::Turn(int joint, const Mat3 &turn) {
    TurnAbout(joint, turn, Position(joint));
}

void Skeleton::Aim(int joint, int end, const Vec3 &target) {
    const Vec3   at = Position(joint), a = Sub(Position(end), at), b = Sub(target, at);
    const Vec3   axis = Cross(a, b);
    const double sine = Length(axis);
    if (sine > 1e-9) {
        Turn(joint, RotAbout(axis, std::atan2(sine, Dot(a, b)) * 180.0 / std::numbers::pi));
    }
}

Vec3 Skeleton::BoneIn(int joint, int toward) const {
    return Unit(Through(Sub(Position(toward), Position(joint)), Transpose(RotOf(World(joint)))));
}

bool PlaceHand(Skeleton &s, const ArmJoints &arm, const Mat4 &target, const Vec3 &forearm) {
    const Vec3   goal = TransOf(target);
    const double lower = Length(Sub(s.Position(arm.elbow), s.Position(arm.wrist)));
    const Vec3   elbow = Add(goal, Scale(Through(forearm, RotOf(target)), lower));
    Shrug(s, arm, elbow, goal);
    const Vec3   at = s.Position(arm.shoulder);
    const double upper = Length(Sub(at, s.Position(arm.elbow))), d = Length(Sub(goal, at));
    const bool   reaches = d < upper + lower;
    if (!reaches || d < 1e-9) {
        s.Aim(arm.shoulder, arm.wrist, goal);
        s.Aim(arm.elbow, arm.wrist, goal);
    } else {
        const Vec3   ahead = Scale(Sub(goal, at), 1.0 / d), out = Sub(elbow, at);
        const Vec3   side = Unit(Sub(out, Scale(ahead, Dot(out, ahead))));
        const double cosine = (upper * upper + d * d - lower * lower) / (2 * upper * d), sine = std::sqrt(std::max(0.0, 1 - cosine * cosine));
        s.Aim(arm.shoulder, arm.elbow, Add(at, Add(Scale(ahead, upper * cosine), Scale(side, upper * sine))));
        s.Aim(arm.elbow, arm.wrist, goal);
    }
    const int  up = s.parent[std::size_t(arm.wrist)];
    const Mat3 r = MatMul(RotOf(target), Transpose(RotOf(s.World(up))));
    for (std::size_t row = 0; row < 3; row++) {
        for (std::size_t col = 0; col < 3; col++) {
            s.local[std::size_t(arm.wrist)][row][col] = r[row][col];
        }
    }
    return reaches;
}

} // namespace expanded
