// A motion of Toan's played by an ally (TownRetarget): each joint pair turns the ally's joint the way Toan's turns in the
// world, the hips follow his, and borrowed joints play the ally's own clip (TownBorrowedJoints).

#include <algorithm>
#include <cmath>
#include <format>
#include <map>
#include <string>

#include "expanded/data/gemron_parts.hpp"
#include "expanded/data/rig_math.hpp"
#include "expanded/data/town_rig.hpp"

namespace expanded::town {

namespace {

// A joint's local turn (a quaternion, w first) and translation.
struct Local {
    Vec4 rotation;
    Vec3 translation;
};

using Pose = std::vector<Local>;

// The model's pose at frame: each joint's tracks there, or else its bind.
Pose PoseAt(const std::vector<MdsFrame> &frames, const MotFile &mot, double frame) {
    std::map<std::pair<std::uint32_t, std::uint32_t>, const MotTrack *> tracks;
    for (const MotTrack &track : mot.tracks) {
        if (!track.keys.empty()) {
            tracks[{track.w0, track.w2}] = &track;
        }
    }
    Pose pose;
    for (const MdsFrame &f : frames) {
        Local local{MatToQuat(RotOf(f.local)), TransOf(f.local)};
        if (auto found = tracks.find({std::uint32_t(f.index), mot::kRotation}); found != tracks.end()) {
            local.rotation = TrackAt(*found->second, frame);
        }
        if (auto found = tracks.find({std::uint32_t(f.index), mot::kTranslation}); found != tracks.end()) {
            Vec4 t = TrackAt(*found->second, frame);
            local.translation = {t[0], t[1], t[2]};
        }
        pose.push_back(local);
    }
    return pose;
}

// Every joint's world turn (row vectors: a child's is its local times its parent's).
std::vector<Mat3> WorldTurns(const std::vector<MdsFrame> &frames, const Pose &pose) {
    std::vector<Mat3> world(frames.size());
    for (std::size_t i = 0; i < frames.size(); i++) {
        Mat3 local = QuatToMat(pose[i].rotation);
        world[i] = frames[i].parent < 0 ? local : MatMul(local, world[std::size_t(frames[i].parent)]);
    }
    return world;
}

Vec4 Slerp(Vec4 a, Vec4 b, double t) {
    double d = Dot(a, b);
    if (d < 0) {
        for (double &c : b) {
            c = -c;
        }
        d = -d;
    }
    Vec4 r;
    if (d > 0.9995) {
        for (std::size_t i = 0; i < 4; i++) {
            r[i] = a[i] + (b[i] - a[i]) * t;
        }
    } else {
        double th = std::acos(std::min(1.0, d)), s = std::sin(th);
        for (std::size_t i = 0; i < 4; i++) {
            r[i] = a[i] * std::sin((1 - t) * th) / s + b[i] * std::sin(t * th) / s;
        }
    }
    double n = std::sqrt(Dot(r, r));
    for (double &c : r) {
        c /= n;
    }
    return r;
}

double Smooth(double u) {
    u = std::clamp(u, 0.0, 1.0);
    return u * u * (3 - 2 * u);
}

// Game frames from the queue's start to Toan's frame f.
double GameTime(const std::vector<std::tuple<int, int, double>> &queue, double f) {
    double t = 0;
    for (auto [lo, hi, speed] : queue) {
        if (f <= hi) {
            return t + (std::max(f, double(lo)) - lo) / speed;
        }
        t += (hi - lo) / speed;
    }
    return t;
}

int FrameIndex(const std::vector<MdsFrame> &frames, std::string_view name, std::string_view model) {
    const MdsFrame *frame = FindFrame(frames, name);
    if (frame == nullptr) {
        Bad(std::format("{} has no {}", model, name));
    }
    return frame->index;
}

// Replaces the track's keys in [lo, lo + values.size()) with one a frame, each turn on the same side as the key before.
void WriteWindow(MotTrack &track, int lo, const std::vector<Vec4> &values, bool rotation) {
    int hi = lo + int(values.size()) - 1;
    std::erase_if(track.keys, [&](const MotKeyframe &key) { return InWindow(key, lo, hi); });
    Vec4 last{};
    for (std::size_t i = 0; i < values.size(); i++) {
        Vec4 v = values[i];
        if (rotation && i > 0 && Dot(v, last) < 0) {
            for (double &c : v) {
                c = -c;
            }
        }
        track.keys.push_back(MotKeyframe::Make(std::uint32_t(lo + int(i)), v));
        last = v;
    }
    SortByFrame(track.keys);
}

} // namespace

void RetargetMotion(ChrPack &base, const CfgMotions &cfg, const TownRetarget &retarget,
                    const std::optional<TownBorrowedJoints> &borrowed, const ChrPack &rig, const ChrPack &src) {
    std::vector<MdsFrame> toan = ReadMdsSkeleton(rig.Require(retarget.rig_mds).Payload());
    std::vector<MdsFrame> ally = ReadMdsSkeleton(base.Require(cfg.body_mds).Payload());
    MotFile               jump = MotFile::FromRecord(src.Require(retarget.src_mot));
    MotFile               body = MotOf(base, cfg.body_mot);

    // Each ally joint's Toan joint.
    std::vector<int> pair(ally.size(), -1);
    for (auto [from, to] : retarget.joints) {
        pair[std::size_t(FrameIndex(ally, to, cfg.body_mds))] = FrameIndex(toan, from, retarget.rig_mds);
    }
    int toan_hip = FrameIndex(toan, retarget.src_hip, retarget.rig_mds), hip = FrameIndex(ally, retarget.hip, cfg.body_mds);

    Pose              idle = PoseAt(ally, body, retarget.ref);
    std::vector<Mat3> idle_world = WorldTurns(ally, idle);
    Pose              toan_ref = PoseAt(toan, jump, retarget.src_ref);
    std::vector<Mat3> toan_ref_world = WorldTurns(toan, toan_ref);

    std::vector<Pose> poses;
    for (int f = retarget.src_lo; f <= retarget.src_hi; f++) {
        Pose              at = PoseAt(toan, jump, f);
        std::vector<Mat3> toan_world = WorldTurns(toan, at);
        Pose              pose = idle;
        std::vector<Mat3> world(ally.size());
        for (std::size_t j = 0; j < ally.size(); j++) {
            int  parent = ally[j].parent;
            Mat3 above = parent < 0 ? kIdentity : world[std::size_t(parent)];
            if (pair[j] >= 0) {
                std::size_t i = std::size_t(pair[j]);
                // Its idle, turned as Toan's joint turns in the world.
                Mat3 turned = MatMul(idle_world[j], MatMul(Transpose(toan_ref_world[i]), toan_world[i]));
                pose[j].rotation = MatToQuat(MatMul(turned, Transpose(above)));
            }
            if (int(j) == hip) {
                for (std::size_t c = 0; c < 3; c++) {
                    pose[j].translation[c] += retarget.hip_scale * (at[std::size_t(toan_hip)].translation[c] -
                                                                    toan_ref[std::size_t(toan_hip)].translation[c]);
                }
            }
            world[j] = MatMul(QuatToMat(pose[j].rotation), above);
        }
        poses.push_back(pose);
    }

    if (borrowed) {
        const TownBorrowedJoints &b = *borrowed;
        for (std::size_t k = 0; k < poses.size(); k++) {
            int    f = retarget.src_lo + int(k);
            double w = Smooth(double(f - b.ease.first) / (b.ease.second - b.ease.first)) *
                       (1 - Smooth(double(f - b.rest.first) / (b.rest.second - b.rest.first)));
            if (w == 0) {
                continue;
            }
            double loop = b.lo + FMod(b.speed * std::max(0.0, GameTime(b.queue, f) - GameTime(b.queue, b.ease.first)),
                                      double(b.hi - b.lo));
            Pose   own = PoseAt(ally, body, loop);
            for (std::string_view name : b.nodes) {
                std::size_t j = std::size_t(FrameIndex(ally, name, cfg.body_mds));
                poses[k][j].rotation = Slerp(poses[k][j].rotation, own[j].rotation, w);
            }
        }
    }

    // The body's tracks, then the shadow's by joint name (its own idle for joints the body lacks).
    auto write = [&](std::string_view mot_name, const std::vector<MdsFrame> &frames, const std::vector<int> &from) {
        MotFile mot = MotOf(base, mot_name);
        Pose    own_idle = PoseAt(frames, mot, retarget.ref);
        for (MotTrack &track : mot.tracks) {
            bool rotation = track.w2 == mot::kRotation;
            if ((!rotation && track.w2 != mot::kTranslation) || track.keys.empty() || track.w0 >= frames.size()) {
                continue;
            }
            int               j = from[track.w0];
            std::vector<Vec4> values;
            for (const Pose &pose : poses) {
                const Local &local = j >= 0 ? pose[std::size_t(j)] : own_idle[track.w0];
                values.push_back(rotation ? local.rotation
                                          : Vec4{local.translation[0], local.translation[1], local.translation[2], 0.0});
            }
            WriteWindow(track, retarget.dest_lo, values, rotation);
        }
        ReplaceMot(base, mot_name, mot);
    };
    std::vector<int> same(ally.size());
    for (std::size_t j = 0; j < ally.size(); j++) {
        same[j] = int(j);
    }
    write(cfg.body_mot, ally, same);
    if (cfg.shadow_mot && cfg.shadow_mds) {
        std::vector<MdsFrame> shadow = ReadMdsSkeleton(base.Require(*cfg.shadow_mds).Payload());
        std::vector<int>      by_name;
        for (const MdsFrame &f : shadow) {
            const MdsFrame *match = FindFrame(ally, f.name);
            by_name.push_back(match != nullptr ? match->index : -1);
        }
        write(*cfg.shadow_mot, shadow, by_name);
    }
}

} // namespace expanded::town
