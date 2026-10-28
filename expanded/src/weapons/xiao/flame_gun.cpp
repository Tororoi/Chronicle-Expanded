#include "expanded/weapons/xiao/flame_gun.hpp"

#include <format>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dataalloc.hpp"
#include "dataread.hpp"
#include "dun/gameloop.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/pack_files.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "frame.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "shot_firebar.hpp"
#include "shot_freefuncs.hpp"
#include "texture.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr char kMuzzle[] = "eff30"; // where her pellet leaves the slingshot

// Her hold of the shot.
struct Hold {
    int  held = 0;    // frames held drawn
    bool lit = false; // the flame burns
    int  burnt = 0;   // frames it has burnt since its wear last came off
};

// The roar while the flame is fed.
struct Roar {
    bool on = false;
    int  unfed = 0; // frames since the flame was last fed
};

Hold hold;
Roar roar;
bool hers = false;           // the flame was last lit by her
bool picture_failed = false; // the picture could not be entered on this floor: not tried again until the next

void LetRoarGo() {
    if (roar.on) {
        StopOwnSound(kFlameRoar);
    }
    roar = {};
}

void OnFloor() {
    hold = {};
    hers = false;
    picture_failed = false;
    LetRoarGo();
}

[[maybe_unused]] const bool kRegistered = (RegisterOwnSound(kFlameRoar), AddEnemyFloorListener(OnFloor), true);

bool PictureIn() {
    return TexManager.GetTexture(const_cast<char *>(kFlamePicture), -1) != nullptr;
}

// The flame's picture in, entered from his pack when no copy is.
bool PictureReady() {
    if (PictureIn()) {
        return true;
    }
    if (picture_failed) {
        return false;
    }
    static CDataAlloc2<1> arena{-1};
    u_int                *pack = ReadPackIntoArena(kOsmondPack, arena, 1);
    u_int                *image = pack != nullptr ? GetPackFile(pack, const_cast<char *>(kFlameImage), nullptr) : nullptr;
    if (image == nullptr || !ImageTexturesFit(image, kFloorCharacterBlock, kFlameImage)) {
        picture_failed = true;
        return false;
    }
    LOADTEXTURE_INFO2 entries[] = {
        {reinterpret_cast<char *>(image), kFloorCharacterBlock, 0},
        {nullptr,                         0,                    0},
    };
    TexManager.LoadTextureBlockEX(kFloorCharacterBlock, entries);
    picture_failed = !PictureIn();
    return !picture_failed;
}

// Her slingshot's muzzle, and her aim as her pellet takes it: in first person the view's, locked on at her target, else
// the way she faces. False when the slingshot has no muzzle or the aim no length.
bool Muzzle(int aimed, float *muzzle, float *aim) {
    CFrame *frame = NowWeapon != nullptr && NowWeapon->frame != nullptr ? NowWeapon->frame->SearchFrame(const_cast<char *>(kMuzzle)) : nullptr;
    if (frame == nullptr) {
        return false;
    }
    float offset[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    frame->GetWorldPosition(muzzle, offset);
    if (aimed != 0) {
        setShotVector(aim, 1.0f, viewAngleH__2, viewAngleV__2);
    } else if (lockOnTargetFlag != 0) {
        for (int i = 0; i < 3; i++) {
            aim[i] = BtActStatus.target_position[i] - muzzle[i];
        }
        aim[3] = 1.0f;
    } else {
        setShotVector(aim, 1.0f, PlayerYaw(), 0.0f);
    }
    return aim[0] != 0.0f || aim[1] != 0.0f || aim[2] != 0.0f;
}

// The flame's progress in the event log, the gauge with it.
void LogFlame(const char *step) {
    LogEvent("flame_gun", std::format("\"step\":\"{}\",\"gauge\":{:.2f}", step, BtActStatus.action_gauge));
}

void PutOut() {
    if (hold.lit) {
        OzumondFire.Rset();
        LogFlame("out");
    }
    hold = {};
}

// Whether her shot is in its hold.
bool Holding() {
    return Playing(CHARA_XIAO) && BtActStatus.action_on == 1 && BtActStatus.action_no == kXiaoHold;
}

void Feed() {
    roar.unfed = 0;
    if (!roar.on) {
        roar.on = PlayOwnSound(kFlameRoar, false);
    }
}

} // namespace

bool FlameGunSphere() {
    int sphere = SuperSteveSphere();
    return sphere == ITEM_WEAPON_BLESSING_GUN || sphere == ITEM_WEAPON_SKUNK;
}

bool FlameGunLit() {
    return hold.lit;
}

int FlameGunHeld() {
    return hold.held;
}

bool FlameGunShot(int aimed) {
    if (BtActStatus.action_no != kXiaoHold || GamePad.On(PadInput_OK) == 0 || !Playing(CHARA_XIAO) || !FlameGunSphere()) {
        PutOut();
        return false;
    }
    hold.held++;
    sceVu0FVECTOR muzzle;
    sceVu0FVECTOR aim;
    if (hold.held < kFlameIgnition || !Muzzle(aimed, muzzle, aim)) {
        return false;
    }
    if (!hold.lit) {
        if (!PictureReady()) {
            return false;
        }
        OzumondFire.Init(muzzle, aim, 0, 0);
        hold.lit = true;
        hers = true;
        BtActStatus.frames_since_attack = 0;
        SwordDmgCheck1(kIgnitionWear, 0);
        LogFlame("lit");
    }

    // His gauge burns down under the flame, and the action ends as it runs out.
    BtActStatus.action_gauge -= kFlameGaugeDrain;
    bool spent = BtActStatus.action_gauge <= 0.0f;
    if (spent) {
        BtActStatus.gauge_exhausted = 1;
    }

    // Osmond's flame's damage: the weapon's attack, twice it while the weapon is poisoned.
    int damage = NowWeaponHave->attack;
    if (StatusErrCheck(AILMENT_STAMINA) != 0) {
        damage *= 2;
    }
    OzumondFire.Set(muzzle, aim, damage, NowWeaponHave->best_elem);
    Feed();

    if (++hold.burnt >= kBurnWearInterval) {
        hold.burnt = 0;
        BtActStatus.frames_since_attack = 0;
        SwordDmgCheck1(kBurnWear, 0);
    }

    // Run out, the flame fed this frame is left to die out.
    if (spent) {
        LogFlame("empty");
        hold = {};
    }
    return spent;
}

float FlameGunGaugeGain(float gain) {
    if (!Playing(CHARA_XIAO)) {
        return gain;
    }
    if (hold.lit) {
        return 0.0f;
    }
    if (BtActStatus.gauge_exhausted == 0) {
        return gain;
    }
    gain = gain / kXiaoGaugeRate * kSpentGaugeRate;
    if (BtActStatus.action_gauge + gain >= kFullGauge) {
        LogFlame("refilled");
    }
    return gain;
}

void FlameGunFrame() {
    if (roar.on && ++roar.unfed >= kRoarLinger) {
        LetRoarGo();
    }
    if (hold.lit && !Holding()) {
        hold = {};
    }
}

int FlameOwner(int owner) {
    return hers && Playing(CHARA_XIAO) ? CHARA_XIAO : owner;
}

} // namespace expanded
