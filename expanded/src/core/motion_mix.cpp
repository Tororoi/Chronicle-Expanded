#include "expanded/core/motion_mix.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "framevu1.hpp"
#include "gameutil.hpp"

namespace expanded {

namespace {

using Quat = std::array<float, 4>; // w, x, y, z

struct Local {
    sceVu0FMATRIX m;
};

// ToQuat and ToMat are each other's inverse on a local's rotation rows, so a slerp through them follows the turn
// between two locals.
Quat ToQuat(const sceVu0FMATRIX m) {
    const float trace = m[0][0] + m[1][1] + m[2][2];
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        return {0.25f * s, (m[2][1] - m[1][2]) / s, (m[0][2] - m[2][0]) / s, (m[1][0] - m[0][1]) / s};
    }
    if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
        const float s = std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
        return {(m[2][1] - m[1][2]) / s, 0.25f * s, (m[0][1] + m[1][0]) / s, (m[0][2] + m[2][0]) / s};
    }
    if (m[1][1] > m[2][2]) {
        const float s = std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
        return {(m[0][2] - m[2][0]) / s, (m[0][1] + m[1][0]) / s, 0.25f * s, (m[1][2] + m[2][1]) / s};
    }
    const float s = std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
    return {(m[1][0] - m[0][1]) / s, (m[0][2] + m[2][0]) / s, (m[1][2] + m[2][1]) / s, 0.25f * s};
}

void SetRotation(sceVu0FMATRIX m, const Quat &q) {
    const float w = q[0], x = q[1], y = q[2], z = q[3];
    m[0][0] = 1.0f - 2.0f * (y * y + z * z);
    m[0][1] = 2.0f * (x * y - z * w);
    m[0][2] = 2.0f * (x * z + y * w);
    m[1][0] = 2.0f * (x * y + z * w);
    m[1][1] = 1.0f - 2.0f * (x * x + z * z);
    m[1][2] = 2.0f * (y * z - x * w);
    m[2][0] = 2.0f * (x * z - y * w);
    m[2][1] = 2.0f * (y * z + x * w);
    m[2][2] = 1.0f - 2.0f * (x * x + y * y);
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

// out t of the way from one local to another: its rotation turned, its translation moved.
void Blend(sceVu0FMATRIX out, const sceVu0FMATRIX from, const sceVu0FMATRIX to, float t) {
    const Quat rotation = Slerp(ToQuat(from), ToQuat(to), t);
    for (int j = 0; j < 3; j++) {
        out[3][j] = from[3][j] + (to[3][j] - from[3][j]) * t;
    }
    SetRotation(out, rotation);
}

bool PosesJoint(const Mot_List &track) {
    return track.type == MOTION_KEY_ROTATION || track.type == MOTION_KEY_TRANSLATION;
}

struct Entry {
    tagMOTION_TYPE   *motion = nullptr;
    const MotionMix  *mix = nullptr;
    MixFix            fix = nullptr;
    Mot_List         *tracks = nullptr; // the motion's tracks when it was mixed
    float             weight = 0.0f;
    std::vector<bool> driven; // by joint: posed by a rotation or translation track
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

// The tracks read at time, as SetMotionEX reads them: the joints', or the vertices'.
void Pose(CFrame *frame, Mot_List *tracks, float time, bool vertices) {
    MOTION_STATE state = {};
    state.time = time;
    state.frame = static_cast<unsigned int>(time);
    state.next_frame = state.frame + 1;
    state.blend = time - static_cast<float>(state.frame);
    for (Mot_List *track = tracks; track != nullptr;) {
        if (vertices ? track->type == MOTION_KEY_VERTEX : PosesJoint(*track)) {
            track = MotionProc(frame, &state, track);
        } else {
            track = track->next;
        }
    }
}

void Mix(Entry &entry, CFrame *frame) {
    const MotionMix          &mix = *entry.mix;
    auto                     *frames = static_cast<CFrameVu1 *>(frame);
    const int                 count = static_cast<int>(entry.driven.size());
    static std::vector<Local> played, mixed;
    played.resize(count);
    mixed.resize(count);
    for (int j = 0; j < count; j++) {
        sceVu0CopyMatrix(played[j].m, frames[j].local);
    }
    float total = 0.0f;
    int   heaviest = -1;
    for (int i = 0; i < MotionMix::kTimes; i++) {
        const float share = mix.share[i];
        if (!(share > 0.0f)) {
            continue;
        }
        Pose(frame, entry.tracks, mix.time[i], false);
        total += share;
        for (int j = 0; j < count; j++) {
            if (!entry.driven[j]) {
                continue;
            }
            if (heaviest < 0) {
                sceVu0CopyMatrix(mixed[j].m, frames[j].local);
            } else {
                Blend(mixed[j].m, mixed[j].m, frames[j].local, share / total);
            }
        }
        if (heaviest < 0 || share > mix.share[heaviest]) {
            heaviest = i;
        }
    }
    for (int j = 0; j < count; j++) {
        if (entry.driven[j]) {
            if (heaviest < 0) {
                sceVu0CopyMatrix(frames[j].local, played[j].m);
            } else {
                Blend(frames[j].local, played[j].m, mixed[j].m, entry.weight);
            }
            frames[j].world_valid = false;
        }
    }
    if (heaviest >= 0 && entry.weight > 0.5f) {
        Pose(frame, entry.tracks, mix.time[heaviest], true);
    }
    if (entry.fix != nullptr) {
        entry.fix(frame, reinterpret_cast<const MixLocal *>(played.data()), count);
    }
}

} // namespace

void MixMotion(tagMOTION_TYPE *motion, const MotionMix *mix, MixFix fix) {
    if (motion == nullptr || motion->proc_list == nullptr) {
        return;
    }
    Entry *entry = Find(motion);
    if (entry == nullptr) {
        auto free = std::ranges::find_if(entries, [](const Entry &e) { return e.motion == nullptr; });
        if (free == entries.end()) {
            return;
        }
        entry = &*free;
        *entry = {};
        entry->motion = motion;
        entry->tracks = motion->proc_list;
        int count = 0;
        for (const Mot_List *track = motion->proc_list; track != nullptr; track = track->next) {
            count = std::max(count, static_cast<int>(track->frame) + 1);
        }
        entry->driven.assign(count, false);
        for (const Mot_List *track = motion->proc_list; track != nullptr; track = track->next) {
            if (PosesJoint(*track)) {
                entry->driven[track->frame] = true;
            }
        }
    }
    entry->mix = mix;
    entry->fix = fix;
}

void UnmixMotion(tagMOTION_TYPE *motion) {
    if (Entry *entry = Find(motion)) {
        *entry = {};
    }
}

bool MotionMixed(const tagMOTION_TYPE *motion) {
    return Find(motion) != nullptr;
}

void MixPosed(CFrame *frame, tagMOTION_TYPE *motion) {
    Entry *entry = Find(motion);
    if (entry == nullptr || frame == nullptr) {
        return;
    }
    if (motion->proc_list != entry->tracks) {
        *entry = {};
        return;
    }
    const MotionMix &mix = *entry->mix;
    entry->weight = entry->weight < mix.target ? std::min(mix.target, entry->weight + mix.step)
                                               : std::max(mix.target, entry->weight - mix.step);
    if (entry->weight <= 0.0f && mix.target <= 0.0f) {
        *entry = {};
        return;
    }
    Mix(*entry, frame);
}

} // namespace expanded
