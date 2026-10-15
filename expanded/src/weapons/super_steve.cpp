#include "expanded/weapons/super_steve.hpp"

#include "dun/gameloop.hpp"
#include "itemdata.hpp"
#include "mglib.hpp"
#include "rect.hpp"
#include "shot_freefuncs.hpp"
#include "snd.hpp"
#include "texture.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// itempack's spare cell, blank in the art and outside every copy the game makes.
constexpr int kSpareCellU = 64;
constexpr int kSpareCellV = 32;

// Just above Steve's raised hands; his icon (the equipped weapon's) is 32 x 32 at (29, 420).
constexpr int kIconX = 29;
constexpr int kIconY = 399;
constexpr int kIconSize = 20;

// The active character's equipped weapon record.
const WEAPON_HAVE *EquippedWeapon() {
    if (UserStatus == nullptr) {
        return nullptr;
    }

    int chara = UserStatus->cur_chara;
    return &UserStatus->chara_weapons[chara][UserStatus->equipped_weapon_slot[chara]];
}

// The sphere of a Super Steve record, or 0.
int SuperSteveSphere(const WEAPON_HAVE *weapon) {
    if (weapon == nullptr || weapon->item_no != ITEM_WEAPON_SUPER_STEVE) {
        return 0;
    }

    return SphereWeapon(*weapon);
}

} // namespace

// A weapon holds one SynthSphere at most, so the first is the one.
int SphereWeapon(const WEAPON_HAVE &weapon) {
    for (const ATTACH_LIST &attach : weapon.attach) {
        if (attach.item_no == ITEM_ATTACH_SYNTHESIS_SPHERE) {
            return attach.sphere_weapon_no;
        }
    }

    return 0;
}

int SuperSteveSphere() {
    return SuperSteveSphere(NowWeaponHave);
}

bool WeaponWielded(int weapon_no) {
    return NowWeaponHave != nullptr && NowWeaponHave->item_no == weapon_no;
}

bool WeaponActive(int weapon_no) {
    return WeaponWielded(weapon_no) || (weapon_no != 0 && SuperSteveSphere() == weapon_no);
}

// basefx01 has a cell for each slingshot, Wooden Slingshot to Angel Gear.
int PelletWeapon() {
    int sphere = SuperSteveSphere();

    if (sphere >= ITEM_WEAPON_WOODEN_SLINGSHOT && sphere <= ITEM_WEAPON_ANGE_GEAR) {
        return sphere;
    }

    return NowWeaponHave->item_no;
}

// Reads the sphere off the equipped record, so the icon follows a sphere attached in the menu.
void SphereIconCopy(char *source) {
    int            sphere = SuperSteveSphere(EquippedWeapon());
    COM_ITEM_INFO *info = sphere != 0 ? GetCommonItemInfo(sphere) : nullptr;

    if (info == nullptr) {
        return;
    }

    int icon = info->icon_index;
    setItemToReserved(source, (icon % 8) * 32, (icon >> 3) * 32, (char *) "itempack", kSpareCellU, kSpareCellV);
}

void SphereIconDraw(int alpha) {
    if (SuperSteveSphere(EquippedWeapon()) == 0) {
        return;
    }

    CTexture *icons = TexManager.GetTexture("itempack", -1);
    set2DSprite(Vif1Packet, icons, CRect_i_(kIconX, kIconY, kIconSize, kIconSize), CRect_i_(kSpareCellU, kSpareCellV, 32, 32), alpha);
}

} // namespace expanded
