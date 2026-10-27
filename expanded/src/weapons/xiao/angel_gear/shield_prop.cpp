#include "expanded/weapons/xiao/angel_gear/shield_prop.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <string>
#include <vector>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/player_state.hpp"
#include "frame.hpp"
#include "gameutil.hpp"
#include "userstatus.hpp"

namespace expanded::angel_gear {

namespace {

constexpr char kPouchFrame[] = "null24"; // the elastic's pouch bone, the one bone the weapon's motion moves
constexpr char kMuzzleFrame[] = "eff30"; // the fork's centre, where her pellets leave from
// The pouch track's draw window, in the motion's frames: the draw from the guard's rest at 240 to full draw at 250 (the
// travel measured to 251), the snap's forward peak by 254, settled by 261.
constexpr u32 kPullStart = 240, kDrawEnd = 251, kDrawnFrame = 250, kPullEnd = 261;
constexpr u32 kIdleFrame = static_cast<u32>(kPropIdleFrame);

struct Prop {
    CCharacter *model = nullptr; // the copy loaded on this floor
    int         weapon = 0;      // the weapon it was loaded for
    bool        up = false;
    int         key = kPropKeyIdle;
    float       orbit = 0.0f, target = 0.0f;
    CFrame     *pouch = nullptr;
    CFrame     *muzzle = nullptr;
    float       catch_pose[3] = {}; // the pouch's own translation at the catch pose
    bool        have_catch = false;
};

Prop                      prop;
std::vector<CCharacter *> prepared; // copies set up this floor (one per weapon loaded)

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(PropReset), true);

float Wrap(float a) {
    constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    while (a > std::numbers::pi_v<float>) {
        a -= kTwoPi;
    }
    while (a <= -std::numbers::pi_v<float>) {
        a += kTwoPi;
    }
    return a;
}

// The pouch track's draw window rescaled about the rest so the draw travels kPropPull, then the draw re-pathed to run from
// the catch pose to full draw as an impact (a cubic ease-out) and everything past the catch held on it: no slack rest.
void RepathPull(Mot_List &track) {
    Mot_Key *keys = track.keys;
    int      count = static_cast<int>(track.key_count);
    float    rest[3] = {};
    bool     have_rest = false;
    for (int i = 0; i < count && keys[i].frame <= kPullStart; i++) {
        std::copy(keys[i].value, keys[i].value + 3, rest);
        have_rest = true;
    }
    if (!have_rest) {
        return;
    }
    float travel = 0.0f;
    for (int i = 0; i < count; i++) {
        if (keys[i].frame >= kPullStart && keys[i].frame <= kDrawEnd) {
            float d[3] = {keys[i].value[0] - rest[0], keys[i].value[1] - rest[1], keys[i].value[2] - rest[2]};
            travel = std::max(travel, std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
        }
    }
    if (travel < 0.01f) {
        return;
    }
    float k = kPropPull / travel;
    int   idle_key = -1, drawn_key = -1;
    for (int i = 0; i < count; i++) {
        if (keys[i].frame < kPullStart || keys[i].frame > kPullEnd) {
            continue;
        }
        for (int c = 0; c < 3; c++) {
            keys[i].value[c] = rest[c] + (keys[i].value[c] - rest[c]) * k;
        }
        idle_key = keys[i].frame == kIdleFrame ? i : idle_key;
        drawn_key = keys[i].frame == kDrawnFrame ? i : drawn_key;
    }
    if (idle_key < 0 || drawn_key < 0) {
        return;
    }
    float from[3], to[3];
    std::copy(keys[idle_key].value, keys[idle_key].value + 3, from);
    std::copy(keys[drawn_key].value, keys[drawn_key].value + 3, to);
    for (int i = 0; i < count; i++) {
        u32 f = keys[i].frame;
        if (f >= kPullStart && f < kDrawnFrame) {
            float t = static_cast<float>(f - kPullStart) / static_cast<float>(kDrawnFrame - kPullStart);
            float e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
            for (int c = 0; c < 3; c++) {
                keys[i].value[c] = from[c] + (to[c] - from[c]) * e;
            }
        } else if (f > kIdleFrame && f <= kPullEnd) {
            std::copy(from, from + 3, keys[i].value);
        }
    }
}

// A fresh copy set up: posed upright, its idle key the catch pose, its pull re-pathed, its pouch and muzzle found.
void Prepare(CCharacter &model) {
    if (std::find(prepared.begin(), prepared.end(), &model) != prepared.end()) {
        return;
    }
    prepared.push_back(&model);
    PoseBelowRoot(*model.frame, kPropPreset & 3, (kPropPreset >> 2) & 3);
    if (MOTION_INFO *keys = model.motion_type.motion_info) {
        keys[kPropKeyIdle] = MOTION_INFO{static_cast<s32>(kIdleFrame), static_cast<s32>(kIdleFrame), 0.0f, 0};
    }
    for (Mot_List *track = model.motion_type.proc_list; track != nullptr; track = track->next) {
        if (track->type == MOTION_KEY_TRANSLATION && track->key_count > 0 && track->keys != nullptr) {
            RepathPull(*track);
        }
    }
}

// The pouch's translation at the catch pose (its track's key there, re-pathed), into out; false without one.
bool CatchPose(const CCharacter &model, float *out) {
    for (const Mot_List *track = model.motion_type.proc_list; track != nullptr; track = track->next) {
        if (track->type != MOTION_KEY_TRANSLATION || track->keys == nullptr) {
            continue;
        }
        for (u32 i = 0; i < track->key_count; i++) {
            if (track->keys[i].frame == kIdleFrame) {
                std::copy(track->keys[i].value, track->keys[i].value + 3, out);
                return true;
            }
        }
    }
    return false;
}

// A frame's origin in the world as the copy stands, or the grip's without the frame.
void WorldOf(CFrame *frame, float *out) {
    if (frame == nullptr || prop.model == nullptr) {
        PropRoot(out);
        return;
    }
    float at[4];
    FrameOrigin(*frame, at);
    std::copy(at, at + 3, out);
}

} // namespace

bool PropLoad() {
    if (NowWeaponHave == nullptr) {
        return false;
    }
    if (prop.model != nullptr && prop.weapon == NowWeaponHave->item_no) {
        return true;
    }
    std::string cfg;
    std::string path = WeaponPackPath(NowWeaponHave->item_no, cfg);
    CCharacter *model = EnterFloorCharacter(path.c_str(), cfg.c_str());
    if (model == nullptr || model->frame == nullptr) {
        static bool logged = false;
        if (!logged) {
            logged = true;
            std::fprintf(stderr, "angel gear: no shield, %s could not be loaded\n", path.c_str());
        }
        return false;
    }
    PropDown();
    prop.model = model;
    prop.weapon = NowWeaponHave->item_no;
    Prepare(*model);
    prop.pouch = FrameNamed(model->frame, kPouchFrame);
    prop.muzzle = FrameNamed(model->frame, kMuzzleFrame);
    prop.have_catch = CatchPose(*model, prop.catch_pose);
    return true;
}

bool PropUp() {
    return prop.up;
}

bool PropSpawn() {
    if (prop.up) {
        return true;
    }
    if (!PropLoad()) {
        return false;
    }
    prop.up = true;
    prop.orbit = prop.target; // up on the wanted bearing, no swing in
    PropKey(kPropKeyIdle);
    return true;
}

void PropDown() {
    prop.up = false;
    prop.orbit = 0.0f;
}

bool PropStale() {
    return prop.model != nullptr && (NowWeaponHave == nullptr || NowWeaponHave->item_no != prop.weapon);
}

void PropKey(int key) {
    prop.key = key;
    if (prop.model != nullptr) {
        prop.model->motion_no = key;
        prop.model->motion_flags |= 4; // from its first frame
        prop.model->motion_speed = key == kPropKeyShoot ? kPropShootRate : -1.0f;
    }
}

int PropKeyNow() {
    return prop.key;
}

void PropOrbitTarget(float bearing) {
    prop.target = Wrap(bearing);
}

float PropOrbit() {
    return prop.orbit;
}

void PropPlace() {
    if (!prop.up || prop.model == nullptr || prop.model->frame == nullptr) {
        return;
    }
    float d = Wrap(prop.target - prop.orbit);
    if (std::fabs(d) > 0.003f) {
        float step = std::clamp(d * kPropOrbitEase, -kPropOrbitRate, kPropOrbitRate);
        if (std::fabs(step) < 0.01f) {
            step = std::copysign(std::min(0.01f, std::fabs(d)), d);
        }
        prop.orbit = Wrap(prop.orbit + step);
    }
    float root[3];
    PropRoot(root);
    float yaw = PlayerYaw() + prop.orbit;
    prop.model->SetPosition(root[0], root[1], root[2]);
    prop.model->SetRotation(0.0f, yaw, 0.0f);
    prop.model->SetScale(kPropScale, kPropScale, kPropScale);
    // Its root as the draw will set it, so its bones stand where they are drawn.
    prop.model->frame->SetPosition(root[0], root[1], root[2]);
    prop.model->frame->SetRotation(0.0f, yaw, 0.0f);
    prop.model->frame->SetScale(kPropScale, kPropScale, kPropScale);
}

void PropAnimate(float alpha) {
    if (!prop.up || prop.model == nullptr || prop.model->frame == nullptr) {
        return;
    }
    prop.model->ambient_offset[3] = 128.0f * std::clamp(alpha, 0.0f, 1.0f);
    prop.model->Step();
}

void PropDraw() {
    if (!prop.up || prop.model == nullptr || prop.model->frame == nullptr) {
        return;
    }
    DrawFloorCharacter(*prop.model);
}

void PropRoot(float *out) {
    float yaw = PlayerYaw() + prop.orbit;
    out[0] = CharaMain.pos[0] + std::sin(yaw) * kPropAhead;
    out[1] = CharaMain.pos[1] + kPropUp;
    out[2] = CharaMain.pos[2] + std::cos(yaw) * kPropAhead;
}

void PropPouch(float *out) {
    WorldOf(prop.up ? prop.pouch : nullptr, out);
}

void PropCatchPoint(float *out) {
    if (!prop.up || prop.pouch == nullptr || !prop.have_catch) {
        PropPouch(out);
        return;
    }
    // The pouch set on the catch pose for the reading, then put back as its motion left it.
    float own[3];
    std::copy(prop.pouch->local[3], prop.pouch->local[3] + 3, own);
    std::copy(prop.catch_pose, prop.catch_pose + 3, prop.pouch->local[3]);
    prop.pouch->world_valid = false;
    WorldOf(prop.pouch, out);
    std::copy(own, own + 3, prop.pouch->local[3]);
    prop.pouch->world_valid = false;
}

void PropMuzzle(float *out) {
    WorldOf(prop.up ? prop.muzzle : nullptr, out);
}

CCharacter *PropCharacter() {
    return prop.model;
}

void PropReset() {
    prop = Prop{};
    prepared.clear();
}

} // namespace expanded::angel_gear
