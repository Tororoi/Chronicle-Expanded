#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <utility>
#include <vector>

#include "framevu1.hpp"
#include "gameutil.hpp"

namespace expanded::test {

// A body on one block of frames, its tracks keyed by hand and posed by the game's own SetMotionEX: what a model's .mds and
// .mot give a motion. Joints are named and parented in block order; Turn and Move key a joint's rotation (radians about an
// axis) and translation at motion frames, Rest turns a joint that no track drives.
class MotionRig {
public:
    struct Joint {
        const char *name;
        int         parent;
        float       x, y, z;
    };

    explicit MotionRig(std::initializer_list<Joint> joints) : frames_(joints.size()) {
        int i = 0;
        for (const Joint &joint : joints) {
            CFrameVu1 &frame = frames_[i++];
            frame.Initialize();
            std::strncpy(frame.name, joint.name, sizeof(frame.name) - 1);
            frame.local[3][0] = joint.x;
            frame.local[3][1] = joint.y;
            frame.local[3][2] = joint.z;
            if (joint.parent >= 0) {
                frame.SetParent(&frames_[joint.parent]);
            }
        }
        for (MOTION_INFO &key : info_) {
            key = {0, 2000, 0.0f, 0};
        }
        Attach(own_);
    }

    MotionRig(const MotionRig &) = delete;
    MotionRig &operator=(const MotionRig &) = delete;

    void Turn(int joint, int axis, std::initializer_list<std::pair<int, float>> keys) {
        Track &track = tracks_.emplace_back(Track{joint, MOTION_KEY_ROTATION, {}});
        for (auto [frame, angle] : keys) {
            Mot_Key key = {};
            key.frame = static_cast<u32>(frame);
            key.value[0] = -std::cos(angle / 2.0f); // the game's keys carry w negated (MotionProc turns it back)
            key.value[1 + axis] = std::sin(angle / 2.0f);
            track.keys.push_back(key);
        }
        Link();
    }

    void Move(int joint, std::initializer_list<std::pair<int, std::array<float, 3>>> keys) {
        Track &track = tracks_.emplace_back(Track{joint, MOTION_KEY_TRANSLATION, {}});
        for (auto [frame, at] : keys) {
            Mot_Key key = {};
            key.frame = static_cast<u32>(frame);
            key.value[0] = at[0];
            key.value[1] = at[1];
            key.value[2] = at[2];
            track.keys.push_back(key);
        }
        Link();
    }

    void Rest(int joint, int axis, float angle) {
        const float c = std::cos(angle), s = std::sin(angle);
        const int   a = (axis + 1) % 3, b = (axis + 2) % 3;
        CFrameVu1  &frame = frames_[joint];
        frame.local[a][a] = c;
        frame.local[a][b] = s;
        frame.local[b][a] = -s;
        frame.local[b][b] = c;
    }

    // The motion the rig's tracks drive from now on (a character's own, say).
    void Attach(tagMOTION_TYPE &motion) {
        motion_ = &motion;
        motion.motion_info = info_.data();
        motion.proc_list2 = nullptr;
        motion.frame_info = nullptr;
        Link();
    }

    // The block posed at time, as SetMotionEX poses a clip standing still on it.
    void Pose(float time) {
        MOTION_STATE &state = motion_->state;
        state.time = time;
        state.motion_no = state.playing_no = 0;
        state.blending = 0;
        state.look_at = 0;
        SetMotionEX(Root(), motion_, info_.data(), &state, nullptr);
    }

    CFrameVu1 *Root() { return frames_.data(); }

    CFrameVu1 &operator[](int joint) { return frames_[joint]; }

    tagMOTION_TYPE &Motion() { return *motion_; }

    MOTION_INFO *Info() { return info_.data(); }

    // A joint's world matrix.
    std::array<float, 16> World(int joint) {
        sceVu0FMATRIX m;
        frames_[joint].GetLWMatrix(m);
        std::array<float, 16> out{};
        std::memcpy(out.data(), m, sizeof(out));
        return out;
    }

    std::array<float, 16> Local(int joint) {
        std::array<float, 16> out{};
        std::memcpy(out.data(), frames_[joint].local, sizeof(out));
        return out;
    }

private:
    struct Track {
        int                  joint;
        int                  type;
        std::vector<Mot_Key> keys;
    };

    void Link() {
        lists_.assign(tracks_.size(), Mot_List{});
        for (size_t i = 0; i < tracks_.size(); i++) {
            Mot_List &list = lists_[i];
            list.frame = static_cast<u32>(tracks_[i].joint);
            list.type = tracks_[i].type;
            list.key_count = static_cast<u32>(tracks_[i].keys.size());
            list.keys = tracks_[i].keys.data();
            list.next = i + 1 < tracks_.size() ? &lists_[i + 1] : nullptr;
        }
        if (motion_ != nullptr) {
            motion_->proc_list = lists_.empty() ? nullptr : lists_.data();
        }
    }

    std::vector<CFrameVu1>      frames_;
    std::vector<Track>          tracks_;
    std::vector<Mot_List>       lists_;
    std::array<MOTION_INFO, 64> info_ = {};
    tagMOTION_TYPE              own_ = {};
    tagMOTION_TYPE             *motion_ = nullptr;
};

// The largest difference between two 4 x 4 matrices' rotations (and translations, with moved).
inline float MatrixGap(const std::array<float, 16> &a, const std::array<float, 16> &b, bool moved = true) {
    float gap = 0.0f;
    for (int i = 0; i < (moved ? 16 : 12); i++) {
        if (i % 4 != 3) {
            gap = std::max(gap, std::fabs(a[i] - b[i]));
        }
    }
    return gap;
}

} // namespace expanded::test
