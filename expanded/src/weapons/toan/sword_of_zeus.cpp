#include "expanded/weapons/toan/sword_of_zeus.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>
#include <vector>

#include "bt_shot_effect.hpp"
#include "btactstatus.hpp"
#include "camerafollow.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/dungeon/own_sounds.hpp"
#include "expanded/dungeon/scene_light.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/judgement_blade.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/wear.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"


namespace expanded {

namespace {

constexpr const char *kLightningFile = "gedit/s99/chara/lightning.chr";

// lightning.chr: its strike (motion 1) playing once where it is put, no flight and no hit.
// clang-format off
BT_SHOT_EFFECT lightning_shot = {"lightning", 0, 0, {}, {}, -1, 0, 0, 3, 0, {1, -1, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
// clang-format on

// The storm's thunder over Dark Heaven Castle (program 16 of its bank): each of its two samples (notes 39 and 40) rolls
// two claps. A bolt is one clap with its rumble (kClap: the first sample's second, kAnswer: the second sample's first),
// or the first sample whole for a volley, at one of the three pitches the town's storm plays them at (SE 67 to 72: a
// semitone either side).
enum ThunderCut {
    kClap,
    kAnswer,
    kWhole
};

constexpr OwnCut kThunderClaps[] = {
    {39, 0.54f, 3.20f, 0.80f},
    {40, 0.00f, 2.05f, 0.50f},
    {39, 0.00f, 4.00f, 0.00f},
};
constexpr OwnSound kThunder = {"sound/set/snd40.snd", "s30a_c.hd", 16, 39, 190, 3, kThunderClaps, static_cast<int>(std::size(kThunderClaps))};

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_SWORD_OF_ZEUS, WeaponShotForm::Weapon, &lightning_shot, kBoltCount, kLightningFile),
     RegisterWeaponShotEffect(ITEM_WEAPON_SWORD_OF_ZEUS, WeaponShotForm::Sphere, &lightning_shot, kBoltCount, kLightningFile),
     RegisterOwnSound(kThunder), true);

// Toan's charge: the last of the lunge's states (its landing), and the meter's threshold for the lunge.
constexpr int   kLungeEnd = 0x11;
constexpr float kMeterFloor = 1.0f;
constexpr float kLungeMeter = 1.5f;
// The lunge, dash to landing, always takes about this long: the sword is let go so it is in the ground as he lands.
constexpr int kLungeFrames = 38;

bool InLunge(int action) {
    return action == kToanLunge || action == 0x10 || action == 3 || action == 0x19 || action == kLungeEnd;
}

struct Charge {
    bool          winding = false;
    int           level1 = -1; // frames held past the lunge's threshold, -1 before it
    bool          full = false;
    bool          high = false; // the lunge this charge lets go jumps higher
    int           target = -1;
    sceVu0FVECTOR spot = {};
    float         blade = 0.0f;
    bool          lunging = false;
    int           dash = -1; // frames since the dash began
    bool          dropped = false;
    int           fall = 0; // frames since the sword was let go
};

Charge charge;

struct Hold {
    bool  pinned = false;
    float eye = 0.0f;    // the eye's height in the world, held
    float height = 0.0f; // the camera's own height over its follow point, given back
};

Hold hold;

bool Toan() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_TOAN;
}

bool Live(int monster) {
    if (NowMonstorUnit == nullptr || monster < 0 || monster >= kFloorMonsters) {
        return false;
    }
    const MONSTOR &live = NowMonstorUnit->monster[monster];
    return live.state != -1 && live.hp > 0 && live.revealed == -1;
}

// Where the charge's bolt will land: the locked target, else ahead of Toan.
void Aim() {
    charge.target = LockedTarget();
    if (charge.target >= 0) {
        NowMonstorUnit->chara[charge.target][0].GetPosition(charge.spot);
    } else {
        charge.spot[0] = CharaMain.pos[0] + kChargeAhead * std::sin(CharaMain.rotation.y);
        charge.spot[1] = CharaMain.pos[1];
        charge.spot[2] = CharaMain.pos[2] + kChargeAhead * std::cos(CharaMain.rotation.y);
    }
    charge.spot[3] = 1.0f;
}

void StandDown() {
    // Only a charge under way lets its own blade go: a primed Solar Flash's hangs on.
    if (charge.winding || charge.lunging) {
        SceneDimEnd();
        if (charge.level1 >= 0 && JudgementHanging()) {
            JudgementRelease();
        }
    }
    charge = {};
}

} // namespace

bool ZeusActive() {
    return WeaponActive(ITEM_WEAPON_SWORD_OF_ZEUS);
}

bool BoltAt(const float *point, int still, bool bill, Thunder thunder, int spared) {
    int index = ShotEffectIndex(&lightning_shot);
    if (index < 0 || NowMonstorUnit == nullptr || NowWeaponHave == nullptr) {
        return false;
    }
    // A shot that does not fly: aimed straight up at no speed (Set lets it go of the model it was copied from).
    CSHOT_EFFECT *bolts = ShotEffectAt(index);
    sceVu0FVECTOR from, to;
    sceVu0CopyVector(from, const_cast<float *>(point));
    from[3] = 1.0f;
    sceVu0CopyVector(to, from);
    to[1] += 1.0f;
    bolts->Set(from, to, 0, -1, 0, nullptr, 0);
    int slot = bolts->current_slot;
    if (slot < 0) {
        return false;
    }
    bolts->velocity[slot][0] = bolts->velocity[slot][1] = bolts->velocity[slot][2] = 0.0f;
    bolts->chara[slot].SetScale(kBoltScale, kBoltScale, kBoltScale);
    if (thunder != Thunder::kNone) {
        int cut = kClap;
        if (thunder == Thunder::kVolley) {
            cut = kWhole;
        } else if (thunder == Thunder::kCombo) {
            cut = ModRoll(2); // kClap or kAnswer
        }
        PlayOwnSound(kThunder, true, cut);
    }
    BlastOptions options;
    options.damage = kBoltBlast;
    options.reach = kBoltReachShare;
    options.still = still;
    options.spared = spared;
    options.element = WEAPON_ELEMENT_THUNDER;
    PlantBlast(*NowMonstorUnit, point, NowWeaponHave->attack, options);
    if (bill) {
        WearWeapon(kBoltWhp);
    }
    return true;
}

bool BoltOn(int monster, bool bill, Thunder thunder) {
    if (!Live(monster)) {
        return false;
    }
    sceVu0FVECTOR at;
    NowMonstorUnit->chara[monster][0].GetPosition(at);
    return BoltAt(at, monster, bill, thunder);
}

int BoltVolley(const float *origin) {
    if (NowMonstorUnit == nullptr) {
        return 0;
    }
    std::vector<std::pair<float, int>> near;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (!Live(monster)) {
            continue;
        }
        sceVu0FVECTOR at;
        NowMonstorUnit->chara[monster][0].GetPosition(at);
        float distance = std::hypot(at[0] - origin[0], at[2] - origin[2]);
        if (distance <= kBoltReach) {
            near.emplace_back(distance, monster);
        }
    }
    std::sort(near.begin(), near.end());
    near.resize(std::min<std::size_t>(near.size(), kBoltCount));
    // Each takes its own bolt in full: a bolt leaves the volley's other targets alone (one hit lands on a monster at a
    // time, so a neighbour's outer step would otherwise take its own bolt's place).
    int targets = 0;
    for (auto [distance, monster] : near) {
        targets |= 1 << monster;
    }
    int struck = 0;
    for (auto [distance, monster] : near) {
        sceVu0FVECTOR at;
        NowMonstorUnit->chara[monster][0].GetPosition(at);
        if (!BoltAt(at, monster, false, struck == 0 ? Thunder::kVolley : Thunder::kNone, targets & ~(1 << monster))) {
            break;
        }
        struck++;
    }
    if (struck > 0) {
        WearWeapon(kBoltWhp); // the volley is one bill, however many bolts
    }
    return struck;
}

bool WhirlwindAllowed() {
    return !(Toan() && WeaponWielded(ITEM_WEAPON_SWORD_OF_ZEUS));
}

float LungeGravity(float gravity) {
    return charge.high ? gravity * kChargeHigh : gravity; // armed through the wind-up and the lunge alone
}

void ZeusChargeStep() {
    if (!Toan() || !WeaponWielded(ITEM_WEAPON_SWORD_OF_ZEUS) || SolarFlashPhase() != SolarPhase::kIdle) {
        StandDown();
        return;
    }
    int action = BtActStatus.action_on != 0 ? BtActStatus.action_no : -1;
    if (action == kToanWindup) {
        float meter = BtActStatus.charge_time;
        charge.winding = true;
        if (meter >= kLungeMeter) {
            charge.level1++;
        }
        // Half the charge is the meter climbing to the lunge's threshold, the other half the hold past it.
        float held = charge.level1 < 0 ? 0.0f : std::min(1.0f, static_cast<float>(charge.level1) / kChargeHighFrames);
        float k = charge.level1 < 0 ? 0.5f * std::clamp((meter - kMeterFloor) / (kLungeMeter - kMeterFloor), 0.0f, 1.0f)
                                    : 0.5f + 0.5f * held;
        charge.blade = k;
        SceneDim(kZeusFlash.prime_dim * k);
        if (held >= 1.0f && !charge.full) {
            charge.full = charge.high = true;
            ChargeCompleteFlash();
        }
        // Past the threshold the sword hangs over where the bolt will land, fading in with the hold.
        if (charge.level1 >= 0) {
            Aim();
            JudgementHoverAt(charge.spot, kChargeHangHeight, held);
        }
        return;
    }
    if (!charge.winding && !charge.lunging) {
        return;
    }
    if (!InLunge(action) || !charge.full) {
        StandDown(); // let go short of the hold, or the charge went another way
        return;
    }
    charge.lunging = true;
    charge.blade = 1.0f;
    if (action != kToanLunge) {
        charge.dash++;
    }
    // Let go so it is in the ground as he lands; that is where the bolt comes down.
    if (!charge.dropped && (charge.dash >= kLungeFrames - kChargeFallFrames || action == kLungeEnd)) {
        charge.dropped = true;
        if (Live(charge.target)) {
            NowMonstorUnit->chara[charge.target][0].GetPosition(charge.spot);
            JudgementHoverAt(charge.spot, kChargeHangHeight, 1.0f);
        }
        JudgementDrop(kChargeFallFrames, true);
    }
    if (charge.dropped) {
        // The room plunges to black along the fall.
        SceneDimRamp(kZeusFlash.prime_dim, std::min(1.0f, static_cast<float>(++charge.fall) / kChargeFallFrames));
    }
    if (action != kLungeEnd) {
        return;
    }
    sceVu0FVECTOR point;
    sceVu0CopyVector(point, charge.spot);
    int still = Live(charge.target) ? charge.target : -1;
    charge = {};
    JudgementReset();
    BoltAt(point, still);
    SolarBlastFlash(kZeusFlash, point, 0.0f);
}

void ZeusChargeReset() {
    charge = {};
    hold = {};
}

void ZeusCameraHold(CCameraFollow &camera) {
    if (charge.lunging && charge.high) {
        if (!hold.pinned) {
            hold = {true, camera.follow[1] + camera.height, camera.height};
        }
        camera.height = hold.eye - camera.follow[1];
    } else if (hold.pinned) {
        camera.height = hold.height;
        hold = {};
    }
}

void ZeusChargeLight(const CCharacter &character, float *ambient) {
    // The sword and its hanging copy, hilt and all.
    float white = &character == NowWeapon ? charge.blade : JudgementIsCopy(character) && charge.blade > 0.0f ? JudgementShown()
                                                                                                             : 0.0f;
    for (int c = 0; c < 3; c++) {
        ambient[c] += kSolarBladeWhite * white;
    }
}

} // namespace expanded
