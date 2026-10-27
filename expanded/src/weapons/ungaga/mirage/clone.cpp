#include "expanded/weapons/ungaga/mirage/clone.hpp"

#include <array>
#include <cstdio>

#include "btitem.hpp"
#include "character.hpp"
#include "cloth.hpp"
#include "dataalloc.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/weapons/ungaga/mirage.hpp"
#include "frame.hpp"
#include "framevu1.hpp"
#include "mds.hpp"
#include "mglib.hpp"
#include "texture.hpp"
#include "userstatus.hpp"

namespace expanded::mirage {

namespace {

// Each party member's character file, as the dungeon loads it (btitem's selectChrUnit).
constexpr std::array<const char *, 6> kCharaFiles = {
    "dun/mainchara/c01d.chr",
    "dun/mainchara/c04b.chr",
    "dun/mainchara/c06b.chr",
    "dun/mainchara/c05a.chr",
    "dun/mainchara/c10b.chr",
    "dun/mainchara/c18a.chr",
};

constexpr char kBaseCfg[] = "base.cfg";             // the dungeon model's cfg in its pack, as LoadChara2 reads it
constexpr char kHandFrame[] = "weapon";             // the bone the weapon in hand hangs from
constexpr int  kBodyTextureBlock = 0x11;            // the active character's textures
constexpr int  kWeaponTextureBlock = 0x1D;          // the weapon in hand's
constexpr int  kArenaQuads = 0x4000;                // 256 KB for the weapon's frames
constexpr int  kHoldStill = 1, kFromFirstFrame = 4; // CCharacter::motion_flags

struct Clone {
    CCharacter   *body = nullptr; // the copy loaded on this floor
    int           chara = -1;     // the party member it was loaded for
    int           failed = -1;    // the party member whose copy could not be loaded on this floor
    bool          up = false;
    float         at[3] = {};
    float         yaw = 0.0f;
    float         alpha = 0.0f;
    CFrame       *hand = nullptr;
    bool          armed = false;           // the weapon copied
    const CFrame *weapon_source = nullptr; // the weapon in hand it was copied from
    int           weapon_item = 0;
};

Clone      clone;
CCharacter weapon;
alignas(64) u_long128 arena_store[kArenaQuads];
CDataAlloc2<1> arena;

bool WeaponCurrent() {
    return clone.armed && NowWeapon != nullptr && NowWeapon->frame == clone.weapon_source && NowWeaponHave != nullptr &&
           NowWeaponHave->item_no == clone.weapon_item;
}

// The weapon in hand copied into the copy's hand: frames of its own, its meshes and textures shared. Again whenever the
// weapon in hand is not the one copied.
void CopyWeapon() {
    if (WeaponCurrent()) {
        return;
    }
    clone.armed = false;
    if (NowWeapon == nullptr || NowWeapon->frame == nullptr || NowWeaponHave == nullptr || clone.hand == nullptr) {
        return;
    }
    arena.base = reinterpret_cast<u_char *>(arena_store);
    arena.limit = kArenaQuads;
    arena.used = 0;
    weapon = *NowWeapon;
    weapon.motion[0] = &weapon.motion_type;
    weapon.cloth = weapon.cloth_buf; // a weapon wears no cloth
    for (CCloth *&piece : weapon.cloth_buf) {
        piece = nullptr;
    }
    weapon.frame = reinterpret_cast<CFrame *>(CopyFrameVu1(reinterpret_cast<CFrameVu1 *>(NowWeapon->frame), &arena));
    if (weapon.frame == nullptr) {
        return;
    }
    weapon.frame->SetReference(clone.hand);
    clone.armed = true;
    clone.weapon_source = NowWeapon->frame;
    clone.weapon_item = NowWeaponHave->item_no;
}

void Place() {
    clone.body->SetPosition(clone.at[0], clone.at[1], clone.at[2]);
    clone.body->SetRotation(0.0f, clone.yaw, 0.0f);
}

} // namespace

bool CloneLoad() {
    if (UserStatus == nullptr || UserStatus->cur_chara < 0 || UserStatus->cur_chara >= 6) {
        return false;
    }
    int chara = UserStatus->cur_chara;
    if (clone.body != nullptr && clone.chara == chara && clone.body->frame != nullptr) {
        return true;
    }
    if (clone.failed == chara) {
        return false; // not again this floor
    }
    CCharacter *body = EnterFloorCopy(kCharaFiles[chara], kBaseCfg);
    if (body == nullptr || body->frame == nullptr) {
        clone.failed = chara;
        std::fprintf(stderr, "mirage: no copy, %s could not be loaded\n", kCharaFiles[chara]);
        return false;
    }
    CloneDown();
    // Clipped as the player's model is (LoadChara2 turns it off).
    CFrameAttr attr;
    attr.clip_enable = false;
    body->frame->SetAttr(attr, 1, 4);
    clone.body = body;
    clone.chara = chara;
    clone.hand = FrameNamed(body->frame, kHandFrame);
    return true;
}

bool CloneSpawn(const float *at, float yaw) {
    if (!CloneLoad()) {
        return false;
    }
    clone.up = true;
    clone.alpha = 0.0f;
    clone.at[0] = at[0], clone.at[1] = at[1], clone.at[2] = at[2];
    clone.yaw = yaw;
    clone.body->SetScale(CharaMain.scale[0], CharaMain.scale[1], CharaMain.scale[2]);
    Place();
    // Posed on the guard loop's first frame, without stepping past it; its cloth hung at rest from that pose.
    clone.body->motion_no = kMirageGuardLoop;
    clone.body->motion_speed = -1.0f;
    clone.body->motion_flags = kHoldStill | kFromFirstFrame;
    clone.body->Step();
    clone.body->motion_flags = 0;
    for (int i = 0; i < CHARA_CLOTH_MAX; i++) {
        if (clone.body->cloth[i] != nullptr) {
            clone.body->cloth[i]->Clear();
        }
    }
    clone.armed = false;
    CopyWeapon();
    return true;
}

void CloneDown() {
    clone.up = false;
    clone.alpha = 0.0f;
    clone.armed = false;
}

bool CloneUp() {
    return clone.up;
}

void CloneStep(float alpha) {
    if (!clone.up || clone.body == nullptr || clone.body->frame == nullptr) {
        return;
    }
    clone.alpha = alpha;
    Place();
    clone.body->motion_no = kMirageGuardLoop;
    clone.body->Step();
    clone.body->ClothStep(0);
    CopyWeapon();
}

void CloneDraw() {
    if (!clone.up || clone.body == nullptr || clone.body->frame == nullptr || clone.alpha <= 0.0f) {
        return;
    }
    // Its model draws with the textures loaded for the party member it was made from.
    if (BtEventMode != 0 || UserStatus == nullptr || UserStatus->cur_chara != clone.chara) {
        return;
    }
    TexManager.ReloadTexture(Vif1Packet, kBodyTextureBlock);
    clone.body->ambient_offset[3] = 128.0f * clone.alpha;
    clone.body->Draw();
    if (WeaponCurrent()) {
        TexManager.ReloadTexture(Vif1Packet, kWeaponTextureBlock);
        weapon.ambient_offset[3] = 128.0f * clone.alpha;
        weapon.Draw();
    }
}

bool CloneRoot(float *out) {
    if (!clone.up || clone.body == nullptr || clone.body->frame == nullptr) {
        return false;
    }
    float world[4];
    FrameOrigin(*clone.body->frame, world);
    out[0] = world[0], out[1] = world[1], out[2] = world[2];
    return true;
}

CCharacter *CloneCharacter() {
    return clone.up ? clone.body : nullptr;
}

CCharacter *CloneWeapon() {
    return clone.up && WeaponCurrent() ? &weapon : nullptr;
}

void CloneReset() {
    clone = Clone{};
}

} // namespace expanded::mirage
