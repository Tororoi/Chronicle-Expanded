#include "expanded/weapons/model_pellets.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <optional>

#include "bt_shot_effect.hpp"
#include "character.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "shot_effect.hpp"

namespace expanded {

CSHOT_EFFECT *ModelPellets::Models() {
    effect_ = ShotEffectIndex(shot_);
    return LoadedShotEffect(effect_);
}

CSHOT_EFFECT *ModelPellets::Loaded() const {
    return effect_ >= 0 ? LoadedShotEffect(effect_) : nullptr;
}

void ModelPellets::Hide(int model) {
    CSHOT_EFFECT *models = Loaded();
    if (models != nullptr && model >= 0) {
        models->active[model] = 0;
        models->chara[model].motion_flags &= ~1;
    }
}

// The model on its pellet; riding, its +y tipped from straight up (about x) to the back of its flight, then turned
// (about y) to face it; spinning, its +z tipped (about x) to the flight's climb, then turned (about y) along it, as the
// apple's face_flight turns it.
void ModelPellets::Ride(int model, const CSHOT &shots, int slot) {
    CSHOT_EFFECT *models = Loaded();
    if (models == nullptr || model < 0 || models->active[model] == 0) {
        return;
    }
    const float *v = shots.vector[slot];
    float        back[3] = {-v[0], -v[1], -v[2]};
    float        length = std::sqrt(back[0] * back[0] + back[1] * back[1] + back[2] * back[2]);
    CCharacter  &shown = models->chara[model];
    shown.SetPosition(const_cast<float *>(shots.pos[slot]));
    if (length <= 0.0f) {
        return;
    }
    if (turn_ == Turn::Ride) {
        float tip = std::acos(std::clamp(back[1] / length, -1.0f, 1.0f));
        shown.SetRotation(tip, std::atan2(back[0], back[2]), 0.0f);
    } else {
        float climb = std::asin(std::clamp(v[1] / length, -1.0f, 1.0f));
        shown.SetRotation(-climb, std::atan2(v[0], v[2]), 0.0f);
    }
}

void ModelPellets::Step() {
    if (!active_()) {
        return;
    }
    Models();
    models_.ForEach([this](const CSHOT &shots, int slot, int &model) { Ride(model, shots, slot); });
}

void ModelPellets::Reset() {
    effect_ = -1;
    models_.Clear();
}

void ModelPellets::Fired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kPelletSlots || !active_()) {
        return;
    }
    if (int *before = models_.InSlot(slot)) {
        Hide(*before);
    }
    CSHOT_EFFECT *models = Models();
    int           model = models != nullptr ? SpawnStill(*models, shots.pos[slot], 1, 1) : -1;
    if (model < 0) {
        models_.Drop(slot);
        return;
    }
    models->chara[model].SetScale(scale_, scale_, scale_);
    models->chara[model].ambient_offset[3] = -1.0f;
    if (turn_ == Turn::Ride) {
        models->chara[model].motion_flags |= 1; // held still: the apple's flight motion would turn it
    } else {
        models->chara[model].motion_flags &= ~1;
    }
    models_.Keep(shots, slot, model);
    Ride(model, shots, slot);
    shots.size[slot] = 0.0f; // the model is the pellet
}

void ModelPellets::Ended(const CSHOT &shots, int slot) {
    if (std::optional<int> model = models_.Take(shots, slot)) {
        Hide(*model);
    }
}

void ModelPellets::Light(const CCharacter &character, float *ambient) const {
    CSHOT_EFFECT *models = Loaded();
    if (tint_ == nullptr || models == nullptr || &character < &models->chara[0] || &character >= std::end(models->chara)) {
        return;
    }
    for (int c = 0; c < 3; c++) {
        ambient[c] += tint_[c];
    }
}

} // namespace expanded
