#include "expanded/core/motion_layer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

#include "framevu1.hpp"
#include "gameutil.hpp"

namespace expanded {

namespace {

using Mat3 = std::array<std::array<float, 3>, 3>;
using Quat = std::array<float, 4>; // w, x, y, z

struct Mat4 {
    sceVu0FMATRIX m;
};

constexpr Mat3 kIdentity = {
    {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}
};

Mat3 Rotation(const sceVu0FMATRIX m) {
    Mat3 r{};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            r[i][j] = m[i][j];
        }
    }
    return r;
}

void SetRotation(sceVu0FMATRIX m, const Mat3 &r) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            m[i][j] = r[i][j];
        }
    }
}

// a then b, as the frames' row vectors compose: a child's local times its parent's world.
Mat3 Mul(const Mat3 &a, const Mat3 &b) {
    Mat3 r{};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            r[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j];
        }
    }
    return r;
}

Mat3 Transposed(const Mat3 &a) {
    Mat3 r{};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            r[i][j] = a[j][i];
        }
    }
    return r;
}

// ToQuat and ToMat are each other's inverse on rotations, so a slerp between two matrices through them follows the
// rotation between them whichever way the matrices are written.
Quat ToQuat(const Mat3 &m) {
    const float trace = m[0][0] + m[1][1] + m[2][2];
    Quat        q{};
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q = {0.25f * s, (m[2][1] - m[1][2]) / s, (m[0][2] - m[2][0]) / s, (m[1][0] - m[0][1]) / s};
    } else if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
        const float s = std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
        q = {(m[2][1] - m[1][2]) / s, 0.25f * s, (m[0][1] + m[1][0]) / s, (m[0][2] + m[2][0]) / s};
    } else if (m[1][1] > m[2][2]) {
        const float s = std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
        q = {(m[0][2] - m[2][0]) / s, (m[0][1] + m[1][0]) / s, 0.25f * s, (m[1][2] + m[2][1]) / s};
    } else {
        const float s = std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
        q = {(m[1][0] - m[0][1]) / s, (m[0][2] + m[2][0]) / s, (m[1][2] + m[2][1]) / s, 0.25f * s};
    }
    return q;
}

Mat3 ToMat(const Quat &q) {
    const float w = q[0], x = q[1], y = q[2], z = q[3];
    return {
        {{1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - z * w), 2.0f * (x * z + y * w)},
         {2.0f * (x * y + z * w), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - x * w)},
         {2.0f * (x * z - y * w), 2.0f * (y * z + x * w), 1.0f - 2.0f * (x * x + y * y)}}
    };
}

Quat Slerp(const Quat &a, Quat b, float t) {
    float cosine = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    if (cosine < 0.0f) {
        cosine = -cosine;
        for (float &v : b) {
            v = -v;
        }
    }
    float from = 1.0f - t, to = t;
    if (cosine < 0.9995f) {
        const float angle = std::acos(cosine), sine = std::sin(angle);
        from = std::sin((1.0f - t) * angle) / sine;
        to = std::sin(t * angle) / sine;
    }
    Quat  q{};
    float length = 0.0f;
    for (int i = 0; i < 4; i++) {
        q[i] = from * a[i] + to * b[i];
        length += q[i] * q[i];
    }
    length = std::sqrt(length);
    for (float &v : q) {
        v /= length;
    }
    return q;
}

Mat3 Slerp(const Mat3 &a, const Mat3 &b, float t) {
    return ToMat(Slerp(ToQuat(a), ToQuat(b), t));
}

// t of the way from one local to another: its rotation turned, its translation moved.
void Blend(sceVu0FMATRIX out, const sceVu0FMATRIX from, const sceVu0FMATRIX to, float t) {
    const Mat3 rotation = Slerp(Rotation(from), Rotation(to), t);
    for (int j = 0; j < 3; j++) {
        out[3][j] = from[3][j] + (to[3][j] - from[3][j]) * t;
    }
    SetRotation(out, rotation);
}

bool PosesJoint(const Mot_List &track) {
    return track.type == MOTION_KEY_ROTATION || track.type == MOTION_KEY_TRANSLATION;
}

// One model's frames as the layer splits them (a motion names its frames by their place in the block).
struct Skeleton {
    CFrame           *root = nullptr;
    Mot_List         *tracks = nullptr;
    std::vector<bool> lower;  // outside the seam's subtree
    std::vector<bool> driven; // posed by a rotation or translation track
    std::vector<int>  above;  // the seam's ancestors, the block's root first
    int               seam = -1;
    int               next = -1;
    bool              next_driven = false;
    bool              next_turned = false; // the undriven joint under the seam holds a turn of the layer's
    Mat4              next_rest = {};
};

struct Entry {
    tagMOTION_TYPE *motion = nullptr;
    MotionLayer    *layer = nullptr;
    Skeleton        skeleton;
};

std::array<Entry, 4> entries;

Entry *Find(const tagMOTION_TYPE *motion) {
    for (Entry &entry : entries) {
        if (entry.motion == motion && motion != nullptr) {
            return &entry;
        }
    }
    return nullptr;
}

int Index(CFrame *frame, CFrame *joint) {
    return static_cast<int>(static_cast<CFrameVu1 *>(joint) - static_cast<CFrameVu1 *>(frame));
}

bool Under(CFrame *joint, const CFrame *top) {
    for (CFrame *f = joint; f != nullptr; f = f->parent) {
        if (f == top) {
            return true;
        }
    }
    return false;
}

void Build(Skeleton &skeleton, CFrame *frame, const tagMOTION_TYPE &motion, const MotionLayer &layer) {
    skeleton = {};
    skeleton.root = frame;
    skeleton.tracks = motion.proc_list;
    CFrame *seam = layer.seam != nullptr ? frame->SearchFrame(const_cast<char *>(layer.seam)) : nullptr;
    if (seam == nullptr) {
        return;
    }
    CFrame *next = layer.seam_next != nullptr ? seam->SearchFrame(const_cast<char *>(layer.seam_next)) : nullptr;
    skeleton.seam = Index(frame, seam);
    skeleton.next = next != nullptr && next != seam ? Index(frame, next) : -1;
    int count = std::max(skeleton.seam, skeleton.next) + 1;
    for (const Mot_List *track = motion.proc_list; track != nullptr; track = track->next) {
        count = std::max(count, static_cast<int>(track->frame) + 1);
    }
    auto *frames = static_cast<CFrameVu1 *>(frame);
    skeleton.lower.resize(count);
    skeleton.driven.resize(count);
    for (int j = 0; j < count; j++) {
        skeleton.lower[j] = !Under(&frames[j], seam);
    }
    for (const Mot_List *track = motion.proc_list; track != nullptr; track = track->next) {
        if (PosesJoint(*track)) {
            skeleton.driven[track->frame] = true;
        }
    }
    for (CFrame *f = seam->parent; f != nullptr; f = f->parent) {
        skeleton.above.push_back(Index(frame, f));
        if (f == frame) {
            break;
        }
    }
    std::ranges::reverse(skeleton.above);
    if (skeleton.next >= 0) {
        skeleton.next_driven = skeleton.driven[skeleton.next];
        sceVu0CopyMatrix(skeleton.next_rest.m, frames[skeleton.next].local);
    }
}

void Restore(Skeleton &skeleton) {
    if (!skeleton.next_turned) {
        return;
    }
    CFrameVu1 &next = static_cast<CFrameVu1 *>(skeleton.root)[skeleton.next];
    sceVu0CopyMatrix(next.local, skeleton.next_rest.m);
    next.world_valid = false;
    skeleton.next_turned = false;
}

// The lower body's tracks read at time (its vertex tracks too, when asked), as SetMotionEX reads them.
void Pose(CFrame *frame, const Skeleton &skeleton, float time, bool vertices) {
    MOTION_STATE state = {};
    state.time = time;
    state.frame = static_cast<unsigned int>(time);
    state.next_frame = state.frame + 1;
    state.blend = time - static_cast<float>(state.frame);
    for (Mot_List *track = skeleton.tracks; track != nullptr;) {
        const bool lower = track->frame < skeleton.lower.size() && skeleton.lower[track->frame];
        if (lower && (PosesJoint(*track) || (vertices && track->type == MOTION_KEY_VERTEX))) {
            track = MotionProc(frame, &state, track);
        } else {
            track = track->next;
        }
    }
}

// The seam turned so the upper body sits torso_turn of the way from the pose the motion gave it in the world (own: the
// lower body's locals before the layer) to riding the layered hips; the joint under it, when given, takes the rest.
void Hold(Skeleton &skeleton, CFrameVu1 *frames, const std::vector<Mat4> &own, float torso_turn) {
    Mat3 kept = kIdentity, laid = kIdentity;
    for (int a : skeleton.above) {
        const Mat3 now = Rotation(frames[a].local);
        const bool layered = skeleton.driven[a] && skeleton.lower[a];
        kept = Mul(layered ? Rotation(own[a].m) : now, kept);
        laid = Mul(now, laid);
    }
    const Mat3  seam = Rotation(frames[skeleton.seam].local);
    const Mat3  seam_kept = Mul(seam, kept), seam_laid = Mul(seam, laid);
    const int   joints = skeleton.next >= 0 ? 2 : 1;
    const float share = torso_turn + (1.0f - torso_turn) * static_cast<float>(joints - 1) / static_cast<float>(joints);
    const Mat3  seam_world = Slerp(seam_kept, seam_laid, share);
    SetRotation(frames[skeleton.seam].local, Mul(seam_world, Transposed(laid)));
    frames[skeleton.seam].world_valid = false;
    if (skeleton.next < 0) {
        return;
    }
    CFrameVu1 &next = frames[skeleton.next];
    const Mat3 local = skeleton.next_driven ? Rotation(next.local) : Rotation(skeleton.next_rest.m);
    const Mat3 world = Slerp(Mul(local, seam_kept), Mul(local, seam_laid), torso_turn);
    SetRotation(next.local, Mul(world, Transposed(seam_world)));
    next.world_valid = false;
    skeleton.next_turned = !skeleton.next_driven;
}

void Apply(Entry &entry, CFrame *frame, const tagMOTION_TYPE &motion) {
    const MotionLayer &layer = *entry.layer;
    Skeleton          &skeleton = entry.skeleton;
    if (skeleton.root != frame || skeleton.tracks != motion.proc_list) {
        Build(skeleton, frame, motion, layer);
    }
    if (skeleton.seam < 0) {
        return;
    }
    if (layer.weight <= 0.0f) {
        Restore(skeleton);
        return;
    }
    auto                    *frames = static_cast<CFrameVu1 *>(frame);
    const int                count = static_cast<int>(skeleton.lower.size());
    static std::vector<Mat4> own, from;
    own.resize(count);
    from.resize(count);
    auto layered = [&](int j) { return skeleton.lower[j] && skeleton.driven[j]; };
    for (int j = 0; j < count; j++) {
        if (layered(j)) {
            sceVu0CopyMatrix(own[j].m, frames[j].local);
        }
    }
    const bool fading = layer.from_time >= 0.0f && layer.from_weight < 1.0f;
    if (fading) {
        Pose(frame, skeleton, layer.from_time, false);
        for (int j = 0; j < count; j++) {
            if (layered(j)) {
                sceVu0CopyMatrix(from[j].m, frames[j].local);
            }
        }
    }
    Pose(frame, skeleton, layer.time, layer.weight >= 0.5f);
    for (int j = 0; j < count; j++) {
        if (!layered(j)) {
            continue;
        }
        CFrameVu1 &joint = frames[j];
        if (fading) {
            Blend(joint.local, from[j].m, joint.local, layer.from_weight);
        }
        if (layer.weight < 1.0f) {
            Blend(joint.local, own[j].m, joint.local, layer.weight);
        }
        joint.world_valid = false;
    }
    Hold(skeleton, frames, own, layer.torso_turn);
}

} // namespace

void LayMotion(tagMOTION_TYPE *motion, MotionLayer *layer) {
    Entry *entry = Find(motion);
    if (entry == nullptr) {
        auto free = std::ranges::find_if(entries, [](const Entry &e) { return e.motion == nullptr; });
        if (free == entries.end()) {
            return;
        }
        entry = &*free;
        *entry = {};
        entry->motion = motion;
    }
    entry->layer = layer;
}

void LiftMotion(tagMOTION_TYPE *motion, bool restore) {
    Entry *entry = Find(motion);
    if (entry == nullptr) {
        return;
    }
    if (restore) {
        Restore(entry->skeleton);
    }
    *entry = {};
}

void MotionPosing(tagMOTION_TYPE *motion, MOTION_STATE *state) {
    const Entry *entry = Find(motion);
    if (entry == nullptr || entry->layer == nullptr) {
        return;
    }
    const MotionLayer &layer = *entry->layer;
    if (layer.carry_key >= 0 && state->playing_no != state->motion_no && state->motion_no == layer.carry_key) {
        state->next_frame = static_cast<unsigned int>(std::lround(layer.carry_frame));
    }
}

void MotionPosed(CFrame *frame, tagMOTION_TYPE *motion) {
    Entry *entry = Find(motion);
    if (entry == nullptr || entry->layer == nullptr || frame == nullptr) {
        return;
    }
    Apply(*entry, frame, *motion);
}

} // namespace expanded
