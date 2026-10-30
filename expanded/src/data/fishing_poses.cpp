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

// The arms: collarbone, shoulder, elbow, wrist.
struct Arm {
    const char *collarbone, *shoulder, *elbow, *wrist;
};

constexpr Arm kLeftArm = {"jnt8_1", "jnt9_1", "jnt9_2", "jnt10_1"}, kRightArm = {"jnt16_1", "jnt17_1", "jnt17_2", "jnt18_1"};

// The left hand on the rod: the wait's grip moved kLeftGripShift across the rod (in the rod's frame), which closes the
// fist round it, then turned kLeftGripTurn degrees round the rod, where the left wrist bends least. The wait holds it
// beside the rod; here its mesh (l_hand) wraps 319 degrees of the rod, its deepest vertex 0.06 inside the rod's surface
// (the right fist's goes 0.18 in).
constexpr Vec3   kLeftGripShift = {-0.35, 0.0, -0.975};
constexpr double kLeftGripTurn = 20.0;
// How short of its full length an arm reaches, and how far its collarbone may swing (degrees, about each of two axes) to
// bring the shoulder where the elbow and wrist want it.
constexpr double kReachSlack = 0.05, kCollarboneSwing = 30.0;

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

    // A joint's direction to its parent (the forearm from the wrist), in the joint's own frame.
    Vec3 BoneIn(std::string_view joint, std::string_view parent) const {
        Vec3 back = Add(Position(parent), Scale(Position(joint), -1.0));
        return Unit(VecMatExact(back, Transpose(RotOf(World(joint)))));
    }

    // Swings the arm's collarbone, at most kCollarboneSwing degrees each way about two axes square to it, to put the
    // shoulder as near as it can to an upper arm's length from elbow (where the elbow is wanted) and within reach of
    // wrist, the least swing that does best.
    void Shrug(const Arm &arm, const Vec3 &elbow, const Vec3 &wrist) {
        Vec3   at = Position(arm.collarbone), from = Add(Position(arm.shoulder), Scale(at, -1.0));
        double upper = Distance(Position(arm.shoulder), Position(arm.elbow)), lower = Distance(Position(arm.elbow), Position(arm.wrist));
        Vec3   across = Cross(from, Add(elbow, Scale(at, -1.0)));
        if (Dot(across, across) < 1e-12) {
            across = Cross(from, kUp);
        }
        Vec3   u = Unit(across), v = Unit(Cross(from, u));
        Mat3   best = kIdentity;
        double best_score = 1e30;
        for (int i = -int(kCollarboneSwing); i <= int(kCollarboneSwing); i++) {
            for (int j = -int(kCollarboneSwing); j <= int(kCollarboneSwing); j++) {
                Mat3   turn = MatMulExact(RotAbout(u, i), RotAbout(v, j));
                Vec3   shoulder = Add(at, VecMatExact(from, turn));
                double over = std::max(0.0, Distance(shoulder, wrist) - (upper + lower - kReachSlack));
                double score = std::abs(Distance(shoulder, elbow) - upper) + 10 * over + 1e-5 * (i * i + j * j);
                if (score < best_score) {
                    best_score = score;
                    best = turn;
                }
            }
        }
        Turn(arm.collarbone, best);
    }

    // Bends a limb of two bones (upper, its child lower, and end under lower) so end's frame becomes target, the elbow on
    // the side of toward, then end turned to the target's rotation. Throws when the limb is too short.
    void Reach(std::string_view upper, std::string_view lower, std::string_view end, const Mat4 &target, const Vec3 &toward) {
        Vec3   at = Position(upper), goal = TransOf(target);
        double a = Distance(at, Position(lower)), b = Distance(Position(lower), Position(end)), d = Distance(at, goal);
        if (d >= a + b) {
            throw std::runtime_error(std::format("{} cannot reach {} away, its limb is {} long", end, d, a + b));
        }
        Vec3   ahead = Scale(Add(goal, Scale(at, -1.0)), 1.0 / d), out = Add(toward, Scale(at, -1.0));
        Vec3   side = Unit(Add(out, Scale(ahead, -Dot(out, ahead))));
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

// Where the hands hold the rod: each wrist's frame in the rod's, the right as in the wait for a bite (kFishingWaitFrame),
// the left on the butt below it, closed round the rod (kLeftGripShift); and the left forearm's direction in the left
// wrist's frame there.
struct Grip {
    Mat4 left, right;
    Vec3 left_forearm;
};

Grip WaitGrip(const Rig &body) {
    Pose wait(body.frames, body.mot, kFishingWaitFrame);
    Mat4 rod = RigidInv(wait.World(kRod));
    Mat4 left = MatMulExact(wait.World(kLeftArm.wrist), rod);
    left[3] = {left[3][0] + kLeftGripShift[0], left[3][1] + kLeftGripShift[1], left[3][2] + kLeftGripShift[2], 1.0};
    left = MatMulExact(left, MatFromRT(RotAbout(kUp, kLeftGripTurn), {0.0, 0.0, 0.0}));
    return {left, MatMulExact(wait.World(kRightArm.wrist), rod), wait.BoneIn(kLeftArm.wrist, kLeftArm.elbow)};
}

// Puts a hand's wrist on target through its arm, the elbow swung as near as the arm allows to where forearm (its
// direction in the wrist's frame) puts it, so the wrist bends as it does there.
void HandTo(Pose &pose, const Arm &arm, const Mat4 &target, const Vec3 &forearm) {
    double length = std::sqrt(Dot(Add(pose.Position(arm.elbow), Scale(pose.Position(arm.wrist), -1.0)),
                                  Add(pose.Position(arm.elbow), Scale(pose.Position(arm.wrist), -1.0))));
    Vec3   elbow = Add(TransOf(target), Scale(VecMatExact(forearm, RotOf(target)), length));
    pose.Shrug(arm, elbow, TransOf(target));
    pose.Reach(arm.shoulder, arm.elbow, arm.wrist, target, elbow);
}

// The right hand, and with two_hands the left, put on the rod (its world frame), the right wrist bent as in the pose's
// source frame (right_forearm), the left as in the wait.
void OnTheRod(Pose &pose, const Grip &grip, const Mat4 &rod, const Vec3 &right_forearm, bool two_hands) {
    HandTo(pose, kRightArm, MatMulExact(grip.right, rod), right_forearm);
    if (two_hands) {
        HandTo(pose, kLeftArm, MatMulExact(grip.left, rod), grip.left_forearm);
    }
}

// A pose: the frame it is made from, and how.
struct PoseRecipe {
    int         key;
    const char *name;
    double      source;
    double      side;      // 1 leant to his left, -1 to his right, 0 not leant
    bool        two_hands; // the left hand on the rod
    bool        raise;     // the rod raised about the left hand's grip (the wait)
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
    Vec3 right_forearm = Pose(rig.frames, rig.mot, recipe.source).BoneIn(kRightArm.wrist, kRightArm.elbow);
    Pose pose = recipe.side != 0.0 ? PullTo(rig, recipe.side) : Pose(rig.frames, rig.mot, recipe.source);
    if (recipe.raise && rig.rod) {
        pose.TurnAbout(kRod, RotAbout(kRight, kFishingRodRaise), TransOf(MatMulExact(grip.left, pose.World(kRod))));
    }
    OnTheRod(pose, grip, rod != nullptr ? *rod : pose.World(kRod), right_forearm, recipe.two_hands);
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
