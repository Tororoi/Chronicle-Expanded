#include "expanded/data/fishing_poses.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <map>
#include <numbers>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "expanded/data/cfg_text.hpp"
#include "expanded/data/chr_pack.hpp"
#include "expanded/data/gemron_parts.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/rig_math.hpp"

namespace expanded {

namespace {

constexpr const char *kCfg = "c01d_turi.cfg";
constexpr const char *kBodyMds = "c01d_turi.mds", *kBodyMot = "c01d_turi.mot";
constexpr const char *kShadowMds = "c01d_turis.mds", *kShadowMot = "c01d_turis.mot";
// The disc's keys, the fight's first after them.
constexpr int kDiscKeys = 16;

// The joints the poses turn: the spine, the neck, the left shoulder and the ends of both hands; the rod, a frame of its
// own under the root which the motion keeps in the right hand.
constexpr const char *kSpine = "jnt2_1", *kNeck = "jnt11_1", *kLeftShoulder = "jnt9_1", *kLeftHand = "eff10", *kRightHand = "eff18";
constexpr const char *kRod = "turizao";

// The axes the poses turn about (RotAbout; Toan faces +Z, his left is +X): about kUp ahead turns to his left, about kBack
// his head leans to his left, about kRight a rod held ahead tips up.
constexpr Vec3 kUp = {0.0, 1.0, 0.0}, kBack = {0.0, 0.0, -1.0}, kRight = {-1.0, 0.0, 0.0};

// One frame of a motion: every joint's local rotation and position, its tracks' values there or else its bind's.
class Pose {
public:
    Pose(const std::vector<MdsFrame> &frames, const MotFile &mot, double frame) : frames_(frames) {
        std::map<std::pair<std::uint32_t, std::uint32_t>, const MotTrack *> tracks;
        for (const MotTrack &track : mot.tracks) {
            tracks[{track.w0, track.w2}] = &track;
        }
        for (const MdsFrame &f : frames) {
            local_.push_back(f.local);
            Mat4 &local = local_.back();
            if (auto found = tracks.find({std::uint32_t(f.index), mot::kRotation}); found != tracks.end()) {
                Mat3 r = QuatToMat(TrackAt(*found->second, frame));
                for (std::size_t i = 0; i < 3; i++) {
                    for (std::size_t j = 0; j < 3; j++) {
                        local[i][j] = r[i][j];
                    }
                }
            }
            if (auto found = tracks.find({std::uint32_t(f.index), mot::kTranslation}); found != tracks.end()) {
                Vec4 t = TrackAt(*found->second, frame);
                for (std::size_t j = 0; j < 3; j++) {
                    local[3][j] = t[j];
                }
            }
        }
    }

    const Mat4 &Local(std::uint32_t joint) const { return local_.at(joint); }

    Vec3 Position(std::string_view joint) const { return TransOf(World(Joint(joint))); }

    // Turns a joint by a world rotation about a world point, its children with it.
    void TurnAbout(std::string_view joint, const Mat3 &turn, const Vec3 &pivot) {
        std::size_t i = Joint(joint);
        Vec3        moved = XformPt(MatFromRT(turn, {0.0, 0.0, 0.0}), pivot);
        Mat4        world = MatMulExact(World(i), MatFromRT(turn, {pivot[0] - moved[0], pivot[1] - moved[1], pivot[2] - moved[2]}));
        int         parent = frames_[i].parent;
        local_[i] = parent < 0 ? world : MatMulExact(world, RigidInv(World(std::size_t(parent))));
    }

    // Turns a joint about its own origin.
    void Turn(std::string_view joint, const Mat3 &turn) { TurnAbout(joint, turn, Position(joint)); }

    // Swings a joint the shortest way that brings the point end to target.
    void Aim(std::string_view joint, std::string_view end, const Vec3 &target) {
        Vec3   at = Position(joint), from = Position(end);
        Vec3   a = {from[0] - at[0], from[1] - at[1], from[2] - at[2]}, b = {target[0] - at[0], target[1] - at[1], target[2] - at[2]};
        Vec3   axis = Cross(a, b);
        double sine = std::sqrt(Dot(axis, axis));
        if (sine > 1e-9) {
            Turn(joint, RotAbout(axis, std::atan2(sine, Dot(a, b)) * 180.0 / std::numbers::pi));
        }
    }

private:
    std::size_t Joint(std::string_view name) const {
        const MdsFrame *frame = FindFrame(frames_, name);
        if (frame == nullptr) {
            throw std::runtime_error(std::format("the fishing model has no {}", name));
        }
        return std::size_t(frame->index);
    }

    Mat4 World(std::size_t i) const {
        int parent = frames_[i].parent;
        return parent < 0 ? local_[i] : MatMulExact(local_[i], World(std::size_t(parent)));
    }

    const std::vector<MdsFrame> &frames_;
    std::vector<Mat4>            local_;
};

// A motion, its model's frames, and whether the model holds the rod (the shadow's does not).
struct Rig {
    std::vector<MdsFrame> frames;
    MotFile               mot;
    bool                  rod;
};

// The pull at kFishingPullFrame leant to Toan's left (side 1) or right (-1).
Pose PullTo(const Rig &rig, double side) {
    Pose pose(rig.frames, rig.mot, kFishingPullFrame);
    Vec3 waist = pose.Position(kSpine);
    Mat3 turn = MatMulExact(RotAbout(kUp, side * kFishingTurn), RotAbout(kBack, side * kFishingLean));
    pose.Turn(kSpine, turn);
    pose.Turn(kNeck, RotAbout(kUp, -side * kFishingHeadBack));
    if (rig.rod) {
        pose.TurnAbout(kRod, turn, waist);
        pose.TurnAbout(kRod, RotAbout(kBack, side * kFishingRodLean), pose.Position(kRightHand));
    }
    return pose;
}

// The wait at kFishingWaitFrame, the rod raised.
Pose Relaxed(const Rig &rig) {
    Pose pose(rig.frames, rig.mot, kFishingWaitFrame);
    Vec3 grip = pose.Position(kRightHand), left = pose.Position(kLeftHand);
    Mat3 raise = RotAbout(kRight, kFishingRodRaise);
    if (rig.rod) {
        pose.TurnAbout(kRod, raise, grip);
    }
    Vec3 held = XformPt(MatFromRT(raise, {0.0, 0.0, 0.0}), {left[0] - grip[0], left[1] - grip[1], left[2] - grip[2]});
    pose.Aim(kLeftShoulder, kLeftHand, {held[0] + grip[0], held[1] + grip[1], held[2] + grip[2]});
    return pose;
}

// Keys every track on frame and the one after it: rotations and positions the pose's, the rest (the rod's bend, a
// position's fourth value) the motion's at source.
void KeyPose(MotFile &mot, const Pose &pose, double source, std::uint32_t frame) {
    for (MotTrack &track : mot.tracks) {
        if (track.keys.empty() || track.keys.back().Frame() >= frame) {
            throw std::runtime_error(std::format("a fishing track is keyed past frame {}", frame));
        }
        Vec4        value = TrackAt(track, source);
        const Mat4 &local = pose.Local(track.w0);
        if (track.w2 == mot::kRotation) {
            Vec4   turned = MatToQuat(RotOf(local));
            double along = Dot(turned, value) < 0 ? -1.0 : 1.0; // the quaternion on the source key's side
            value = {along * turned[0], along * turned[1], along * turned[2], along * turned[3]};
        } else if (track.w2 == mot::kTranslation) {
            value = {local[3][0], local[3][1], local[3][2], value[3]};
        }
        track.keys.push_back(MotKeyframe::Make(frame, value));
        track.keys.push_back(MotKeyframe::Make(frame + 1, value));
    }
}

std::uint32_t LastFrame(const MotFile &mot) {
    std::uint32_t last = 0;
    for (const MotTrack &track : mot.tracks) {
        last = std::max(last, track.keys.empty() ? 0 : track.keys.back().Frame());
    }
    return last;
}

int KeyCount(std::span<const unsigned char> cfg) {
    std::string text = ToText(cfg);
    std::size_t at = text.find("KEY_START"), end = text.find("MOTION_END", at);
    int         count = 0;
    for (at = text.find('\n', at); at != std::string::npos && at < end; at = text.find('\n', at + 1)) {
        std::size_t first = text.find_first_not_of(" \t", at + 1);
        count += first != std::string::npos && text.compare(first, 3, "KEY") == 0 && text.compare(first, 4, "KEY_") != 0;
    }
    return count;
}

} // namespace

Bytes BuildFishingPoses(std::span<const unsigned char> model) {
    ChrPack    pack = ChrPack::Parse(model);
    ChrRecord &cfg = pack.Require(kCfg);
    if (KeyCount(cfg.Payload()) != kDiscKeys) {
        throw std::runtime_error(std::format("{} has not the disc's {} keys", kCfg, kDiscKeys));
    }
    Rig                 body{ReadMdsSkeleton(pack.Require(kBodyMds).Payload()), MotFile::FromRecord(pack.Require(kBodyMot)), true};
    Rig                 shadow{ReadMdsSkeleton(pack.Require(kShadowMds).Payload()), MotFile::FromRecord(pack.Require(kShadowMot)), false};
    std::uint32_t       first = (std::max(LastFrame(body.mot), LastFrame(shadow.mot)) / 10 + 1) * 10;
    const std::uint32_t frames[] = {first, first + 10, first + 20};
    for (Rig *rig : {&body, &shadow}) {
        Pose left = PullTo(*rig, 1.0), right = PullTo(*rig, -1.0), relaxed = Relaxed(*rig);
        KeyPose(rig->mot, left, kFishingPullFrame, frames[0]);
        KeyPose(rig->mot, right, kFishingPullFrame, frames[1]);
        KeyPose(rig->mot, relaxed, kFishingWaitFrame, frames[2]);
    }
    pack.Require(kBodyMot).ReplacePayload(body.mot.BuildPayload());
    pack.Require(kShadowMot).ReplacePayload(shadow.mot.BuildPayload());
    std::string lines = std::format("KEY\t{0},\t{0},\t0.0,\t//pull left {1}\r\n"
                                    "KEY\t{2},\t{2},\t0.0,\t//pull right {3}\r\n"
                                    "KEY\t{4},\t{4},\t0.0,\t//relaxed {5}\r\n",
                                    frames[0], kFishingPullLeftKey, frames[1], kFishingPullRightKey, frames[2], kFishingRelaxedKey);
    cfg.ReplacePayload(AppendCfgKeys(cfg.Payload(), lines));
    return pack.Rebuild();
}

std::vector<DataBuild> FishingPoseBuilds() {
    using Files = const std::vector<Bytes> &;
    return {
        DataBuild::FromSources("Toan's fishing", std::string(kRebuiltDiscFolder) + kFishingModelFile, {kFishingModelFile},
                               [](Files s) { return BuildFishingPoses(s[0]); }),
    };
}

} // namespace expanded
