#include "expanded/weapons/toan/kitchen_knife.hpp"

#include <cstring>
#include <optional>
#include <string>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "localize.hpp"
#include "sysmes.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// The lines, system messages of their own.
constexpr int kBlessedMessage = 1003;
constexpr int kFadedMessage = 1004;
constexpr int kMendedMessage = 1005;
constexpr int kMessageFrames = 240;

std::optional<std::string> BlessedLine() {
    return LocalizeText("expanded.kitchen_knife.blessed", "The spring's blessing surges through the Kitchen Knife!");
}

std::optional<std::string> FadedLine() {
    return LocalizeText("expanded.kitchen_knife.faded", "The spring's blessing fades from the Kitchen Knife...");
}

std::optional<std::string> MendedLine() {
    return LocalizeText("expanded.kitchen_knife.mended", "The spring mends the Kitchen Knife.");
}

[[maybe_unused]] const bool kLines =
    (SetSystemMessage(kBlessedMessage, BlessedLine), SetSystemMessage(kFadedMessage, FadedLine), SetSystemMessage(kMendedMessage, MendedLine), true);

// The knife's model: its root is the mesh, and the hit frames are its children, so they grow with it.
constexpr char kKnifeModel[] = "c01w08";

int     left = 0;
bool    soaked = false;     // stood in a spring this step
bool    was_soaked = false; // and the step before
CFrame *blade = nullptr;    // the blade grown, and its own local matrix
float   blade_local[3][4] = {};

void Say(int message) {
    int args[4] = {-1, -1, -1, -1};
    SetSystemMes(message, kMessageFrames, MES_POS_BOTTOM, 0, args, nullptr);
}

CFrame *Blade() {
    if (NowWeapon == nullptr || NowWeapon->frame == nullptr) {
        return nullptr;
    }
    CFrame *root = NowWeapon->frame;
    return std::strncmp(root->name, kKnifeModel, sizeof(kKnifeModel) - 1) == 0 ? root : nullptr;
}

void GrowBlade() {
    CFrame *frame = Blade();
    if (frame == nullptr || frame == blade) {
        return;
    }

    blade = frame;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 4; col++) {
            blade_local[row][col] = frame->local[row][col];
            frame->local[row][col] *= kBlessingBlade;
        }
    }
    frame->world_valid = 0;
}

// The blade back as it was, where the same model still carries it.
void ShrinkBlade() {
    if (blade != nullptr && Blade() == blade) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 4; col++) {
                blade->local[row][col] = blade_local[row][col];
            }
        }
        blade->world_valid = 0;
    }
    blade = nullptr;
}

bool KnifeOut() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_TOAN && WeaponWielded(ITEM_WEAPON_KITCHEN_KNIFE);
}

// The knife's own record, whose WHP the menu shows and the swings drain.
WEAPON_HAVE *KnifeRecord() {
    int          slot = UserStatus->equipped_weapon_slot[CHARA_TOAN];
    WEAPON_HAVE &weapon = UserStatus->chara_weapons[CHARA_TOAN][slot];
    return weapon.item_no == ITEM_WEAPON_KITCHEN_KNIFE && WeaponAbilitiesOn() ? &weapon : nullptr;
}

void End(bool quietly) {
    left = 0;
    ShrinkBlade();
    if (!quietly) {
        Say(kFadedMessage);
    }
}

} // namespace

int SpringsBlessingLeft() {
    return left;
}

void SpringsBlessingStep() {
    was_soaked = soaked;
    soaked = false;

    if (left == 0) {
        return;
    }

    if (!KnifeOut()) {
        End(true);
        return;
    }

    if (--left == 0) {
        End(false);
        return;
    }

    GrowBlade();
}

void SpringsBlessingSoak() {
    if (!KnifeOut()) {
        return;
    }

    soaked = true;
    bool blessing = left == 0;
    left = kBlessingSteps;
    GrowBlade();

    if (was_soaked) {
        return;
    }

    // Stepping in: the knife made whole.
    WEAPON_HAVE *knife = KnifeRecord();
    bool         mended = knife != nullptr && knife->durability_f < knife->durability;
    if (mended) {
        knife->durability_f = knife->durability;
    }

    if (blessing) {
        Say(kBlessedMessage);
    } else if (mended) {
        Say(kMendedMessage);
    }
}

int SpringsBlessingAttack(int attack) {
    return left > 0 && KnifeOut() ? attack * kBlessingAttack : attack;
}

} // namespace expanded
