#include "expanded/weapons/ungaga/babels_spear/copy.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "mglib.hpp"
#include "texture.hpp"

namespace expanded::babels_spear {

namespace {

constexpr int  kWeaponTextureBlock = 0x1D; // the weapon in hand's textures, which the statue draws with
constexpr char kForkFrame[] = "eff30";

struct Copy {
    CCharacter *model = nullptr; // the form up, loaded on this floor
    CopyForm    form = CopyForm::kSpear;
    bool        up = false;
    CFrame     *fork = nullptr;
};

Copy                      copy;
std::vector<CCharacter *> prepared; // models set up this floor

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(CopyReset), true);

// A fresh model set up once: the spear point-up; the statue fork up, its key the catch pose.
void Prepare(CCharacter &model, CopyForm form) {
    if (std::find(prepared.begin(), prepared.end(), &model) != prepared.end()) {
        return;
    }
    prepared.push_back(&model);
    if (form == CopyForm::kSpear) {
        TurnPointUp(*model.frame);
        return;
    }
    PoseBelowRoot(*model.frame, 1, 1);
    // The root keeps its scale alone, its grip's rotation now below it: the statue stands as posed under its turn.
    CFrame *root = model.frame;
    float   scale = std::sqrt(root->local[0][0] * root->local[0][0] + root->local[0][1] * root->local[0][1] + root->local[0][2] * root->local[0][2]);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            root->local[r][c] = r == c ? scale : 0.0f;
        }
    }
    root->world_valid = false;
    if (MOTION_INFO *keys = model.motion_type.motion_info) {
        keys[kStatueKey] = MOTION_INFO{static_cast<s32>(kStatuePose), static_cast<s32>(kStatuePose), 0.0f, 0};
    }
}

CCharacter *Load(CopyForm form) {
    std::string cfg;
    std::string path = WeaponPackPath(form == CopyForm::kSpear ? ITEM_WEAPON_BABEL_S_SPEAR : ITEM_WEAPON_SUPER_STEVE, cfg);
    CCharacter *model = form == CopyForm::kSpear ? EnterFloorCharacter(path.c_str(), cfg.c_str()) : EnterFloorCopy(path.c_str(), cfg.c_str());
    if (model == nullptr || model->frame == nullptr) {
        std::fprintf(stderr, "babel's spear: no copy, %s could not be loaded\n", path.c_str());
        return nullptr;
    }
    Prepare(*model, form);
    return model;
}

} // namespace

bool CopySpawn(CopyForm form) {
    CCharacter *model = Load(form);
    if (model == nullptr) {
        return false;
    }
    copy.model = model;
    copy.form = form;
    copy.up = true;
    copy.fork = form == CopyForm::kStatue ? FrameNamed(model->frame, kForkFrame) : nullptr;
    if (form == CopyForm::kStatue) {
        model->motion_no = kStatueKey;
        model->motion_speed = -1.0f;
        model->motion_flags |= 4; // from its first frame
    }
    return true;
}

void CopyDown() {
    copy.up = false;
}

bool CopyUp() {
    return copy.up;
}

void CopyPlace(const float *root, float yaw, float scale, float tint, float alpha) {
    if (!copy.up) {
        return;
    }
    CCharacter &model = *copy.model;
    model.SetPosition(root[0], root[1], root[2]);
    model.SetRotation(0.0f, yaw, 0.0f);
    model.SetScale(scale, scale, scale);
    for (int c = 0; c < 3; c++) {
        model.ambient_offset[c] = tint;
    }
    model.ambient_offset[3] = 128.0f * std::clamp(alpha, 0.0f, 1.0f);
    if (copy.form == CopyForm::kStatue) {
        // Its root as the draw will set it, so its bones stand where they are drawn.
        model.frame->SetPosition(root[0], root[1], root[2]);
        model.frame->SetRotation(0.0f, yaw, 0.0f);
        model.frame->SetScale(scale, scale, scale);
        model.motion_no = kStatueKey;
        model.Step();
    }
}

bool CopyForkPoint(const float *local, float *world) {
    if (!copy.up || copy.fork == nullptr) {
        return false;
    }
    float point[4] = {local[0], local[1], local[2], 1.0f};
    float at[4];
    copy.fork->GetWorldPosition(at, point);
    std::copy(at, at + 3, world);
    return true;
}

void CopyDraw() {
    if (!copy.up || copy.model->frame == nullptr) {
        return;
    }
    if (copy.form == CopyForm::kSpear) {
        DrawFloorCharacter(*copy.model);
        return;
    }
    TexManager.ReloadTexture(Vif1Packet, kWeaponTextureBlock);
    copy.model->Draw();
}

CCharacter *CopyCharacter() {
    return copy.up ? copy.model : nullptr;
}

void CopyReset() {
    copy = Copy{};
    prepared.clear();
}

} // namespace expanded::babels_spear
