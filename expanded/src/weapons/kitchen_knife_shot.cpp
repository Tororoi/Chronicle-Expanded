#include "expanded/weapons/kitchen_knife_shot.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <optional>

#include "bt_shot_effect.hpp"
#include "character.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/pellets.hpp"
#include "expanded/weapons/toan/kitchen_knife.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"

namespace expanded {

namespace {

// The gem alone: the apple shot drawing the Aquamarine, at no speed, put where it is wanted, no hit (as Big Bang's bomb).
// clang-format off
BT_SHOT_EFFECT gem_shot = {"ringo_ex", 0, 0, {}, {}, -1, 0, 0, 3, 0, {-1, 0, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
// clang-format on

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_KITCHEN_KNIFE, WeaponShotForm::Sphere, &gem_shot, kGemShotSlots, kAquamarineThrowFile), true);

struct State {
    int              effect = -1; // the gem's effect, as last asked
    PelletSlots<int> gems;        // each pellet's gem
};

State state;

void OnFloor() {
    state = {};
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

// The gem's effect, loaded with the floor while the sphere is with the party (asked each time, never loading).
CSHOT_EFFECT *Gems() {
    state.effect = ShotEffectIndex(&gem_shot);
    return LoadedShotEffect(state.effect);
}

void Hide(int gem) {
    CSHOT_EFFECT *gems = LoadedShotEffect(state.effect);
    if (gems != nullptr && gem >= 0) {
        gems->active[gem] = 0;
        gems->chara[gem].motion_flags &= ~1;
    }
}

// The gem on its pellet: its point (its +y) tipped from straight up (about x) to the back of its flight, then turned
// (about y) to face it.
void Ride(int gem, const CSHOT &shots, int slot) {
    CSHOT_EFFECT *gems = LoadedShotEffect(state.effect);
    if (gems == nullptr || gem < 0 || gems->active[gem] == 0) {
        return;
    }
    const float *v = shots.vector[slot];
    float        back[3] = {-v[0], -v[1], -v[2]};
    float        length = std::sqrt(back[0] * back[0] + back[1] * back[1] + back[2] * back[2]);
    CCharacter  &gem_model = gems->chara[gem];
    gem_model.SetPosition(const_cast<float *>(shots.pos[slot]));
    if (length > 0.0f) {
        float tip = std::acos(std::clamp(back[1] / length, -1.0f, 1.0f));
        gem_model.SetRotation(tip, std::atan2(back[0], back[2]), 0.0f);
    }
}

} // namespace

void GemShotStep() {
    if (!KitchenKnifePellets()) {
        return;
    }
    Gems();
    state.gems.ForEach([](const CSHOT &shots, int slot, int &gem) { Ride(gem, shots, slot); });
}

void GemShotReset() {
    state = {};
}

void GemShotFired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kPelletSlots || !KitchenKnifePellets()) {
        return;
    }
    if (int *before = state.gems.InSlot(slot)) {
        Hide(*before);
    }
    CSHOT_EFFECT *gems = Gems();
    int           gem = gems != nullptr ? SpawnStill(*gems, shots.pos[slot], 1, 1) : -1;
    if (gem < 0) {
        state.gems.Drop(slot);
        return;
    }
    gems->chara[gem].SetScale(1.0f, 1.0f, 1.0f);
    gems->chara[gem].ambient_offset[3] = -1.0f;
    gems->chara[gem].motion_flags |= 1; // held still: the apple's flight motion would turn it
    state.gems.Keep(shots, slot, gem);
    Ride(gem, shots, slot);
    shots.size[slot] = 0.0f; // the gem is the pellet
}

void GemShotEnded(const CSHOT &shots, int slot) {
    if (std::optional<int> gem = state.gems.Take(shots, slot)) {
        Hide(*gem);
    }
}

void GemShotLight(const CCharacter &character, float *ambient) {
    CSHOT_EFFECT *gems = state.effect >= 0 ? LoadedShotEffect(state.effect) : nullptr;
    if (gems == nullptr || &character < &gems->chara[0] || &character >= std::end(gems->chara)) {
        return;
    }
    for (int c = 0; c < 3; c++) {
        ambient[c] += kGemTint[c];
    }
}

} // namespace expanded
