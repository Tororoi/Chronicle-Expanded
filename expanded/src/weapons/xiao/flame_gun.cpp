#include "expanded/weapons/xiao/flame_gun.hpp"

#include "btactstatus.hpp"
#include "character.hpp"
#include "dataalloc.hpp"
#include "dataread.hpp"
#include "dun/gameloop.hpp"
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

void PutOut() {
    if (hold.lit) {
        OzumondFire.Rset();
    }
    hold = {};
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

void FlameGunShot(int aimed) {
    if (BtActStatus.action_no != kXiaoHold || GamePad.On(PadInput_OK) == 0 || !Playing(CHARA_XIAO) || !FlameGunSphere()) {
        PutOut();
        return;
    }
    hold.held++;
    sceVu0FVECTOR muzzle;
    sceVu0FVECTOR aim;
    if (hold.held < kFlameIgnition || !Muzzle(aimed, muzzle, aim)) {
        return;
    }
    if (!hold.lit) {
        if (!PictureReady()) {
            return;
        }
        OzumondFire.Init(muzzle, aim, 0, 0);
        hold.lit = true;
        hers = true;
        BtActStatus.frames_since_attack = 0;
        SwordDmgCheck1(kIgnitionWear, 0);
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
}

void FlameGunFrame() {
    if (roar.on && ++roar.unfed >= kRoarLinger) {
        LetRoarGo();
    }
}

int FlameOwner(int owner) {
    return hers && Playing(CHARA_XIAO) ? CHARA_XIAO : owner;
}

} // namespace expanded
