#include "expanded/weapons/ungaga/cactus/bloom_prop.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "frame.hpp"
#include "itemdata.hpp"

namespace expanded::cactus {

namespace {

struct Prop {
    CCharacter *model = nullptr; // the model, loaded on this floor
    std::string path;
    bool        up = false;
};

Prop                      prop;
std::vector<CCharacter *> upright; // weapons turned point-up this floor

// The weapon turned point-up: its root's rotation (the grip) turned a quarter about x, so the blade's axis, the model's +z,
// stands straight up. The draw keeps the root's own rotation under the character's turn.
void PointUp(CCharacter &model) {
    if (std::find(upright.begin(), upright.end(), &model) != upright.end()) {
        return;
    }
    upright.push_back(&model);
    CFrame *root = model.frame;
    float   row1[3], row2[3];
    std::copy(root->local[1], root->local[1] + 3, row1);
    std::copy(root->local[2], root->local[2] + 3, row2);
    for (int c = 0; c < 3; c++) {
        root->local[1][c] = -row2[c];
        root->local[2][c] = row1[c];
    }
    root->world_valid = false;
}

} // namespace

bool PropLoad(const BloomModel &bloom) {
    std::string path = bloom.pack != nullptr ? bloom.pack : "";
    std::string cfg = bloom.cfg != nullptr ? bloom.cfg : "";
    if (bloom.weapon) {
        path = WeaponPackPath(ITEM_WEAPON_CACTUS, cfg);
    }
    if (prop.model != nullptr && prop.path == path) {
        return true;
    }
    CCharacter *model = EnterFloorCharacter(path.c_str(), cfg.c_str());
    if (model == nullptr || model->frame == nullptr) {
        std::printf("desert bloom: no %s, %s could not be loaded\n", bloom.name, path.c_str());
        return false;
    }
    PropDown();
    prop.model = model;
    prop.path = path;
    if (bloom.weapon) {
        PointUp(*model);
    }
    return true;
}

bool PropUp() {
    return prop.up;
}

bool PropSpawn(const BloomModel &bloom) {
    if (!PropLoad(bloom)) {
        return false;
    }
    prop.up = true;
    return true;
}

void PropDown() {
    prop.up = false;
}

void PropPlace(const float *root, float yaw, float scale, float alpha) {
    if (!prop.up || prop.model == nullptr) {
        return;
    }
    prop.model->SetPosition(root[0], root[1], root[2]);
    prop.model->SetRotation(0.0f, yaw, 0.0f);
    prop.model->SetScale(scale, scale, scale);
    prop.model->ambient_offset[3] = 128.0f * std::clamp(alpha, 0.0f, 1.0f);
}

void PropDraw() {
    if (!prop.up || prop.model == nullptr || prop.model->frame == nullptr) {
        return;
    }
    DrawFloorCharacter(*prop.model);
}

CCharacter *PropCharacter() {
    return prop.model;
}

void PropReset() {
    prop = Prop{};
    upright.clear();
}

} // namespace expanded::cactus
