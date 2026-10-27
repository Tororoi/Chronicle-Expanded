#include "expanded/core/party.hpp"
#include "expanded/weapons/curse.hpp"

#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kHarmful = AILMENT_FREEZE | AILMENT_POISON | AILMENT_GOO;

struct Cursed {
    const CurseRule *rule = nullptr; // the rule whose curse the member carries
    bool             lifted = false; // an item lifted it
    bool             down = false;   // knocked out
    bool             renew = false;  // a new floor: cursed again
};

Cursed cursed[kPartyMembers];

std::vector<const CurseRule *> &Rules() {
    static std::vector<const CurseRule *> rules;
    return rules;
}

void OnFloor() {
    for (Cursed &member : cursed) {
        member.renew = true;
    }
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

const CurseRule *RuleFor(const CUserStatus &status, int chara) {
    for (const CurseRule *rule : Rules()) {
        if (rule->cursing(status, chara)) {
            return rule;
        }
    }
    return nullptr;
}

void Step(CUserStatus &status, int chara) {
    Cursed          &member = cursed[chara];
    const CurseRule *rule = RuleFor(status, chara);
    s32             &ailments = status.ailments[chara];

    // Unequipped: freed.
    if (rule == nullptr) {
        if (member.rule != nullptr && !member.lifted) {
            ailments &= ~AILMENT_CURSE;
        }
        member = {};
        return;
    }

    bool down = (ailments & AILMENT_KNOCKED_OUT) != 0 || status.hp[chara] <= 0;

    // Equipped, or a new floor: cursed.
    if (member.rule != rule || member.renew) {
        member = {.rule = rule, .down = down};
        if (!down) {
            ailments |= AILMENT_CURSE;
        }
        return;
    }

    if (down) {
        member.down = true;
        return;
    }

    // Revived: the curse comes back unless an item lifted it.
    if (member.down) {
        member.down = false;
        if (!member.lifted) {
            ailments |= AILMENT_CURSE;
        }
        return;
    }

    if (member.lifted) {
        return;
    }

    if ((ailments & AILMENT_CURSE) == 0) {
        member.lifted = true;
        rule->lifted(status, chara);
    } else if (rule->immune) {
        ailments &= ~kHarmful;
    }
}

} // namespace

bool AddCurseRule(const CurseRule *rule) {
    if (rule == nullptr) {
        return false;
    }
    for (const CurseRule *added : Rules()) {
        if (added == rule) {
            return false;
        }
    }
    Rules().push_back(rule);
    return true;
}

const CurseRule *CurseOn(int chara) {
    if (chara < 0 || chara >= kPartyMembers || cursed[chara].lifted) {
        return nullptr;
    }
    return cursed[chara].rule;
}

void CurseStep() {
    if (UserStatus == nullptr) {
        return;
    }
    for (int chara = 0; chara < kPartyMembers; chara++) {
        Step(*UserStatus, chara);
    }
}

bool CurseAllowsAilment(int ailment) {
    if (UserStatus == nullptr || (ailment & kHarmful) == 0) {
        return true;
    }
    const CurseRule *rule = CurseOn(UserStatus->cur_chara);
    return rule == nullptr || !rule->immune;
}

} // namespace expanded
