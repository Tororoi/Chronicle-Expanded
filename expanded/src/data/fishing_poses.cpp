#include "expanded/data/fishing_poses.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "expanded/data/arm_reach.hpp"
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

// The axes the poses turn about (RotAbout; Toan faces +Z, his left is +X): about kUp ahead turns to his left, about kBack
// his head leans to his left, about kRight a rod held ahead tips up.
constexpr Vec3 kUp = {0.0, 1.0, 0.0}, kBack = {0.0, 0.0, -1.0}, kRight = {-1.0, 0.0, 0.0};

// A motion at one frame on its model's frames: every joint's local, its tracks' values there or else its bind's.
class Pose {
public:
    Pose(const std::vector<MdsFrame> &frames, const MotFile &mot, double frame) : frames_(frames) {
        std::map<std::pair<std::uint32_t, std::uint32_t>, const MotTrack *> tracks;
        for (const MotTrack &track : mot.tracks) {
            tracks[{track.w0, track.w2}] = &track;
        }
        for (const MdsFrame &f : frames) {
            skeleton.parent.push_back(f.parent);
            Mat4 local = f.local;
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
            skeleton.local.push_back(local);
        }
    }

    // A joint's place in the skeleton, by name; throws when the model has none.
    int operator[](std::string_view name) const {
        const MdsFrame *frame = FindFrame(frames_, name);
        if (frame == nullptr) {
            throw std::runtime_error(std::format("the fishing model has no {}", name));
        }
        return frame->index;
    }

    ArmJoints Arm(const FishingArm &arm) const { return {(*this)[arm.collarbone], (*this)[arm.shoulder], (*this)[arm.elbow], (*this)[arm.wrist]}; }

    const std::vector<MdsFrame> &frames_;
    Skeleton                     skeleton;
};

// A motion, its model's frames, and whether the model holds the rod (the shadow's does not).
struct Rig {
    std::vector<MdsFrame> frames;
    MotFile               mot;
    bool                  rod;
};

// The right wrist's frame in the rod's: where the disc's motion keeps it in every frame.
Mat4 RightGrip(const Rig &body) {
    Pose wait(body.frames, body.mot, kFishingWaitFrame);
    return MatMulExact(wait.skeleton.World(wait[kFishingRightArm.wrist]), RigidInv(wait.skeleton.World(wait[kFishingRod])));
}

// The pull at kFishingPullFrame leant to Toan's left (side 1) or right (-1).
Pose PullTo(const Rig &rig, double side) {
    Pose       pose(rig.frames, rig.mot, kFishingPullFrame);
    Skeleton  &s = pose.skeleton;
    const Vec3 waist = s.Position(pose[kFishingSpine]);
    const Mat3 turn = MatMulExact(RotAbout(kUp, side * kFishingTurn), RotAbout(kBack, side * kFishingLean));
    s.Turn(pose[kFishingSpine], turn);
    s.Turn(pose[kFishingNeck], RotAbout(kUp, -side * kFishingHeadBack));
    if (rig.rod) {
        s.TurnAbout(pose[kFishingRod], turn, waist);
        s.TurnAbout(pose[kFishingRod], RotAbout(kBack, side * kFishingRodLean), s.Position(pose[kFishingRightArm.wrist]));
    }
    return pose;
}

// The wait at kFishingWaitFrame, the rod raised about the right hand, the left arm as in the pull.
Pose Relaxed(const Rig &rig) {
    Pose       pose(rig.frames, rig.mot, kFishingWaitFrame);
    const Pose pull(rig.frames, rig.mot, kFishingPullFrame);
    Skeleton  &s = pose.skeleton;
    if (rig.rod) {
        s.TurnAbout(pose[kFishingRod], RotAbout(kRight, kFishingRodRaise), s.Position(pose[kFishingRightArm.wrist]));
    }
    const int arm = pose[kFishingLeftArmRoot];
    for (std::size_t j = 0; j < s.local.size(); j++) {
        for (int up = int(j); up >= 0; up = s.parent[std::size_t(up)]) {
            if (up == arm) {
                s.local[j] = pull.skeleton.local[j];
                break;
            }
        }
    }
    return pose;
}

// The poses, in key order, each from a frame of the disc's: its source for the tracks no pose turns (the rod's bend,
// a position's fourth value) is the pull's.
struct PoseRecipe {
    int         key;
    const char *name;
    double      from;
    double      side; // 1 leant to his left, -1 to his right, 0 the relaxed wait
};

constexpr PoseRecipe kPoses[] = {
    {kFishingPullLeftKey,  "pull left",  kFishingPullFrame, 1.0 },
    {kFishingPullRightKey, "pull right", kFishingPullFrame, -1.0},
    {kFishingRelaxedKey,   "relaxed",    kFishingWaitFrame, 0.0 },
};

// A pose on a rig, the right hand put back on the rod (the body's, rod, for the shadow, which has none), its wrist bent
// as in the frame the pose is made from.
Pose MakePose(const Rig &rig, const PoseRecipe &recipe, const Mat4 &grip, const Mat4 *rod) {
    const Pose from(rig.frames, rig.mot, recipe.from);
    const Vec3 forearm = from.skeleton.BoneIn(from[kFishingRightArm.wrist], from[kFishingRightArm.elbow]);
    Pose       pose = recipe.side != 0.0 ? PullTo(rig, recipe.side) : Relaxed(rig);
    const Mat4 held = rod != nullptr ? *rod : pose.skeleton.World(pose[kFishingRod]);
    if (!PlaceHand(pose.skeleton, pose.Arm(kFishingRightArm), MatMulExact(grip, held), forearm)) {
        throw std::runtime_error("the right hand cannot reach the rod");
    }
    return pose;
}

// Keys every track on frame and the one after it: rotations and positions the pose's, the rest (the rod's bend, a
// position's fourth value) the motion's at kFishingPullFrame.
void KeyPose(MotFile &mot, const Pose &pose, std::uint32_t frame) {
    for (MotTrack &track : mot.tracks) {
        if (track.keys.empty() || track.keys.back().Frame() >= frame) {
            throw std::runtime_error(std::format("a fishing track is keyed past frame {}", frame));
        }
        Vec4        value = TrackAt(track, kFishingPullFrame);
        const Mat4 &local = pose.skeleton.local.at(track.w0);
        if (track.w2 == mot::kRotation) {
            Vec4   turned = MatToQuat(RotOf(local));
            double along = Dot(turned, value) < 0 ? -1.0 : 1.0; // the quaternion on the pull's key's side
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

void HoldTheRod(Skeleton &mixed, const Skeleton &played, const FishingGrip &grip) {
    const Mat4 held = MatMulExact(played.World(grip.right.wrist), RigidInv(played.World(grip.rod)));
    const Mat4 rod = MatMulExact(RigidInv(held), mixed.World(grip.right.wrist));
    const int  up = mixed.parent[std::size_t(grip.rod)];
    mixed.local[std::size_t(grip.rod)] = up < 0 ? rod : MatMulExact(rod, RigidInv(mixed.World(up)));
}

Bytes BuildFishingPoses(std::span<const unsigned char> model) {
    ChrPack    pack = ChrPack::Parse(model);
    ChrRecord &cfg = pack.Require(kCfg);
    if (KeyCount(cfg.Payload()) != kDiscKeys) {
        throw std::runtime_error(std::format("{} has not the disc's {} keys", kCfg, kDiscKeys));
    }
    Rig           body{ReadMdsSkeleton(pack.Require(kBodyMds).Payload()), MotFile::FromRecord(pack.Require(kBodyMot)), true};
    Rig           shadow{ReadMdsSkeleton(pack.Require(kShadowMds).Payload()), MotFile::FromRecord(pack.Require(kShadowMot)), false};
    std::uint32_t first = (std::max(LastFrame(body.mot), LastFrame(shadow.mot)) / 10 + 1) * 10;
    Mat4          grip = RightGrip(body);
    std::string   lines;
    for (std::size_t i = 0; i < std::size(kPoses); i++) {
        const PoseRecipe &recipe = kPoses[i];
        if (recipe.key != kDiscKeys + int(i)) {
            throw std::runtime_error(std::format("the fishing pose {} is not KEY {}", recipe.name, kDiscKeys + int(i)));
        }
        std::uint32_t frame = first + 10 * std::uint32_t(i);
        try {
            Pose pose = MakePose(body, recipe, grip, nullptr);
            Mat4 rod = pose.skeleton.World(pose[kFishingRod]);
            KeyPose(body.mot, pose, frame);
            KeyPose(shadow.mot, MakePose(shadow, recipe, grip, &rod), frame);
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
