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

// The joints the poses turn: the spine and the neck; the rod, a frame of its own under the root which the motion keeps
// in the right hand.
constexpr const char *kSpine = "jnt2_1", *kNeck = "jnt11_1";
constexpr const char *kRod = "turizao";

// The arms (shoulder, elbow, wrist), and the way each elbow bends when its hand reaches for the rod: down, out to its own
// side and ahead.
struct Arm {
    const char *shoulder, *elbow, *wrist;
    Vec3        pole;
};

constexpr Arm kLeftArm = {
    "jnt9_1", "jnt9_2", "jnt10_1", {0.5, -1.0, 0.3}
};
constexpr Arm kRightArm = {
    "jnt17_1", "jnt17_2", "jnt18_1", {-0.5, -1.0, 0.3}
};

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

    Mat4 World(std::string_view joint) const { return World(Joint(joint)); }

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

    // Bends a limb of two bones (upper, its child lower, and end under lower) so end's frame becomes target: the elbow
    // swung toward pole, then end turned to the target's rotation. Throws when the limb is too short.
    void Reach(std::string_view upper, std::string_view lower, std::string_view end, const Mat4 &target, const Vec3 &pole) {
        Vec3   at = Position(upper), goal = TransOf(target);
        double a = Distance(at, Position(lower)), b = Distance(Position(lower), Position(end)), d = Distance(at, goal);
        if (d >= a + b) {
            throw std::runtime_error(std::format("{} cannot reach {} away, its limb is {} long", end, d, a + b));
        }
        Vec3   ahead = Scale(Add(goal, Scale(at, -1.0)), 1.0 / d);
        Vec3   side = Unit(Add(pole, Scale(ahead, -Dot(pole, ahead))));
        double cosine = (a * a + d * d - b * b) / (2 * a * d), sine = std::sqrt(1 - cosine * cosine);
        Aim(upper, lower, Add(at, Add(Scale(ahead, a * cosine), Scale(side, a * sine))));
        Aim(lower, end, goal);
        std::size_t i = Joint(end);
        Mat3        parent = RotOf(World(std::size_t(frames_[i].parent)));
        Mat3        r = MatMulExact(RotOf(target), Transpose(parent));
        for (std::size_t row = 0; row < 3; row++) {
            for (std::size_t col = 0; col < 3; col++) {
                local_[i][row][col] = r[row][col];
            }
        }
    }

private:
    static double Distance(const Vec3 &p, const Vec3 &q) {
        Vec3 d = Add(p, Scale(q, -1.0));
        return std::sqrt(Dot(d, d));
    }

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
        pose.TurnAbout(kRod, RotAbout(kBack, side * kFishingRodLean), pose.Position(kRightArm.wrist));
    }
    return pose;
}

// Where the hands hold the rod while he waits for a bite (kFishingWaitFrame), the disc's own two-handed grip: each wrist's
// frame in the rod's, the left hand on the butt below the right.
struct Grip {
    Mat4 left, right;
};

Grip WaitGrip(const Rig &body) {
    Pose wait(body.frames, body.mot, kFishingWaitFrame);
    Mat4 rod = RigidInv(wait.World(kRod));
    return {MatMulExact(wait.World(kLeftArm.wrist), rod), MatMulExact(wait.World(kRightArm.wrist), rod)};
}

// The right hand, and with two_hands the left, put on the rod (its world frame) as they hold it in the wait, through the
// arms.
void OnTheRod(Pose &pose, const Grip &grip, const Mat4 &rod, bool two_hands) {
    pose.Reach(kRightArm.shoulder, kRightArm.elbow, kRightArm.wrist, MatMulExact(grip.right, rod), kRightArm.pole);
    if (two_hands) {
        pose.Reach(kLeftArm.shoulder, kLeftArm.elbow, kLeftArm.wrist, MatMulExact(grip.left, rod), kLeftArm.pole);
    }
}

// A pose: the frame it is made from, and how.
struct PoseRecipe {
    int         key;
    const char *name;
    double      source;
    double      side;      // 1 leant to his left, -1 to his right, 0 not leant
    bool        two_hands; // the left hand on the rod
    bool        raise;     // the rod raised between the hands (the wait)
};

constexpr PoseRecipe kPoses[] = {
    {kFishingPullLeftKey,       "pull left",             kFishingPullFrame, 1.0,  false, false},
    {kFishingPullRightKey,      "pull right",            kFishingPullFrame, -1.0, false, false},
    {kFishingRelaxedKey,        "relaxed",               kFishingWaitFrame, 0.0,  true,  true },
    {kFishingTwoHandedPullKey,  "pull, two hands",       kFishingPullFrame, 0.0,  true,  false},
    {kFishingTwoHandedLeftKey,  "pull left, two hands",  kFishingPullFrame, 1.0,  true,  false},
    {kFishingTwoHandedRightKey, "pull right, two hands", kFishingPullFrame, -1.0, true,  false},
};

// A pose on a rig. The shadow, which has no rod, takes the body's rod (rod) for its hands.
Pose MakePose(const Rig &rig, const PoseRecipe &recipe, const Grip &grip, const Mat4 *rod) {
    Pose pose = recipe.side != 0.0 ? PullTo(rig, recipe.side) : Pose(rig.frames, rig.mot, recipe.source);
    if (recipe.raise && rig.rod) {
        Vec3 left = pose.Position(kLeftArm.wrist), right = pose.Position(kRightArm.wrist);
        pose.TurnAbout(kRod, RotAbout(kRight, kFishingRodRaise), Add(Scale(left, 0.75), Scale(right, 0.25)));
    }
    OnTheRod(pose, grip, rod != nullptr ? *rod : pose.World(kRod), recipe.two_hands);
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
    Rig           body{ReadMdsSkeleton(pack.Require(kBodyMds).Payload()), MotFile::FromRecord(pack.Require(kBodyMot)), true};
    Rig           shadow{ReadMdsSkeleton(pack.Require(kShadowMds).Payload()), MotFile::FromRecord(pack.Require(kShadowMot)), false};
    std::uint32_t first = (std::max(LastFrame(body.mot), LastFrame(shadow.mot)) / 10 + 1) * 10;
    Grip          grip = WaitGrip(body);
    std::string   lines;
    for (std::size_t i = 0; i < std::size(kPoses); i++) {
        const PoseRecipe &recipe = kPoses[i];
        if (recipe.key != kDiscKeys + int(i)) {
            throw std::runtime_error(std::format("the fishing pose {} is not KEY {}", recipe.name, kDiscKeys + int(i)));
        }
        std::uint32_t frame = first + 10 * std::uint32_t(i);
        try {
            Pose pose = MakePose(body, recipe, grip, nullptr);
            Mat4 rod = pose.World(kRod);
            KeyPose(body.mot, pose, recipe.source, frame);
            KeyPose(shadow.mot, MakePose(shadow, recipe, grip, &rod), recipe.source, frame);
        } catch (const std::runtime_error &failure) {
            throw std::runtime_error(std::format("{}: {}", recipe.name, failure.what()));
        }
        lines += std::format("KEY\t{0},\t{0},\t0.0,\t//{1} {2}\r\n", frame, recipe.name, recipe.key);
    }
    pack.Require(kBodyMot).ReplacePayload(body.mot.BuildPayload());
    pack.Require(kShadowMot).ReplacePayload(shadow.mot.BuildPayload());
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
