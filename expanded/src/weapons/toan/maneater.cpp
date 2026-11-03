#include "expanded/weapons/toan/maneater.hpp"

#include <algorithm>

#include "dun/gameloop.hpp"
#include "expanded/core/party.hpp"
#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// chara's equipped Maneater: the sword (Toan's) or Super Steve carrying its sphere (Xiao's), else nullptr.
WEAPON_HAVE *Maneater(CUserStatus &status, int chara) {
    return EquippedCarries(status, chara, ITEM_WEAPON_MANEATER) ? EquippedWeapon(status, chara) : nullptr;
}

bool Cursing(const CUserStatus &status, int chara) {
    return EquippedCarries(status, chara, ITEM_WEAPON_MANEATER);
}

// Lifting it costs nothing.
void Lifted(CUserStatus &, int) {}

constexpr CurseRule kBloodPrice = {.cursing = Cursing, .lifted = Lifted, .immune = false};

[[maybe_unused]] const bool kRule = AddCurseRule(&kBloodPrice);

int frames[kPartyMembers] = {};

void Feed(CUserStatus &status, int chara) {
    WEAPON_HAVE *sword = Maneater(status, chara);
    if (sword == nullptr) {
        frames[chara] = 0;
        return;
    }
    if (++frames[chara] < kBloodPriceFrames) {
        return;
    }
    frames[chara] = 0;
    bool warning = sword->durability_f > 0.0f && sword->durability_f <= kBloodPriceWarning * sword->durability;
    bool down = (status.ailments[chara] & AILMENT_KNOCKED_OUT) != 0;
    if (!warning || down || MemberLife(status, chara) <= 1) {
        return;
    }
    status.AddNowLife(chara, -1, 0.0f);
    sword->durability_f = std::min(sword->durability_f + 1.0f, static_cast<float>(sword->durability));
}

} // namespace

bool BloodPriceCursing(int chara) {
    return CurseOn(chara) == &kBloodPrice;
}

void BloodPriceStep() {
    if (UserStatus == nullptr) {
        return;
    }
    for (int chara = 0; chara < kPartyMembers; chara++) {
        Feed(*UserStatus, chara);
    }
}

} // namespace expanded
