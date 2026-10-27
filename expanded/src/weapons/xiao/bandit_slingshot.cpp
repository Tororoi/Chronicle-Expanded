#include "expanded/weapons/xiao/bandit_slingshot.hpp"

#include <cstring>
#include <optional>
#include <string>

#include "bt_shot_effect.hpp"
#include "clsmes.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/weapons/pellets.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "gametext.hpp"
#include "itemdata.hpp"
#include "localize.hpp"
#include "menu_misc.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kNameMessage = 3000;       // a monster's name: CharaNameMes's name_no + 3000
const int     kNotices[] = {10, 20, 30}; // ItemGetMes's "acquired" templates

struct Stolen {
    BT_SHOT_EFFECT shot{};
    bool           held = false;
    int            item = -1; // the stolen item, until its notice is made
    std::string    name;      // the monster's, for the notice
    bool           naming = false;
};

Stolen stolen;

void OnFloor() {
    stolen = {};
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

bool SelfBurst(const BT_SHOT_EFFECT &shot) {
    return std::strncmp(shot.model_name, "zibaku", 6) == 0;
}

// A kind's shot she may take: its table index, or -1.
int Projectile(int index) {
    if (index < 0 || index >= 35 || BtEntryEffectTbl[index] == nullptr || SelfBurst(*BtEntryEffectTbl[index])) {
        return -1;
    }
    return index;
}

std::string MonsterName(const CMonstorUnit &unit, int monster) {
    if (CharaNameMes == nullptr) {
        return {};
    }
    const short *codes = CharaNameMes->GetTextLineDataTop(unit.monster[monster].name_no + kNameMessage);
    return codes != nullptr ? GameTextDecode(codes) : std::string{};
}

std::optional<std::string> Notice10() {
    return StealShotNoticeText(10);
}

std::optional<std::string> Notice20() {
    return StealShotNoticeText(20);
}

std::optional<std::string> Notice30() {
    return StealShotNoticeText(30);
}

[[maybe_unused]] const bool kNotice =
    (SetSystemMessage(kNotices[0], Notice10), SetSystemMessage(kNotices[1], Notice20), SetSystemMessage(kNotices[2], Notice30), true);

} // namespace

bool StealShotActive() {
    if (UserStatus == nullptr || UserStatus->cur_chara != CHARA_XIAO) {
        return false;
    }
    int sphere = SuperSteveSphere();
    return WeaponWielded(ITEM_WEAPON_BANDIT_SLINGSHOT) || sphere == ITEM_WEAPON_BANDIT_SLINGSHOT || sphere == ITEM_WEAPON_BANDIT_S_RING;
}

const BT_SHOT_EFFECT *StealShotHeld() {
    return stolen.held ? &stolen.shot : nullptr;
}

std::optional<std::string> StealShotNoticeText(int message) {
    if (!stolen.naming || stolen.name.empty()) {
        return std::nullopt;
    }
    std::string templ;
    switch (message) {
        case 20:
            templ = LocalizeText("expanded.bandit_slingshot.acquired.20", "{icon-orb}\"{insert1}{value1}\" acquired.");
            break;
        case 30:
            templ = LocalizeText("expanded.bandit_slingshot.acquired.30", "{icon-jar}\"{insert1}\" acquired.");
            break;
        default:
            templ = LocalizeText("expanded.bandit_slingshot.acquired.10", "{icon-sword}\"{insert1}{value1}\" acquired.");
            break;
    }
    std::string line = LocalizeText("expanded.bandit_slingshot.stolen", "[monster]'s projectile is now yours");
    if (std::size_t at = line.find("[monster]"); at != std::string::npos) {
        line.replace(at, 9, stolen.name);
    }
    return templ + "\n" + line;
}

void StealShotTaken(const CMonstorUnit &unit, int monster, int item) {
    if (!StealShotActive()) {
        return;
    }
    const MONSTOR_MODEL *kind = Species(ModelKind(unit.monster[monster].base_model));
    if (kind == nullptr) {
        return;
    }
    int index = Projectile(kind->shot_effect[0]);
    if (index < 0) {
        index = Projectile(kind->shot_effect[1]);
    }
    if (index < 0) {
        return;
    }
    stolen.shot = *BtEntryEffectTbl[index];
    stolen.shot.target = kHitsMonsters;
    stolen.held = EnterShotEffect(&stolen.shot, kStealShots) >= 0;
    stolen.item = stolen.held ? item : -1;
    stolen.name = stolen.held ? MonsterName(unit, monster) : std::string{};
}

void StealShotFired(CSHOT &shots, int slot) {
    if (!stolen.held || !StealShotActive() || slot < 0 || slot >= kPelletSlots) {
        return;
    }
    int index = ShotEffectIndex(&stolen.shot);
    if (index < 0) {
        return;
    }
    // The stolen shot flies in the pellet's place, with her weapon's abilities (Steal among them).
    FireInPelletsPlace(*ShotEffectAt(index), shots, slot, static_cast<int>(static_cast<float>(shots.damage[slot]) * kStealShotDamage));
}

void StealShotNotice(bool naming, int item) {
    if (!naming) {
        if (stolen.naming) {
            stolen.naming = false;
            stolen.item = -1;
        }
        return;
    }
    stolen.naming = stolen.held && item == stolen.item;
}

} // namespace expanded
