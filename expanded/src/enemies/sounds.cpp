#include "expanded/enemies/sounds.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <utility>

#include "audio/hdbank.hpp"
#include "audio/mixer.hpp"
#include "dataread.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/scaling.hpp"
#include "monstorunit.hpp"
#include "snd.hpp"
#include "sound.hpp"

namespace expanded {

namespace {

constexpr int kKinds = 167;
constexpr int kDungeons = 7;
constexpr int kDemonShaft = 6;
constexpr int kMonsterBankSlot = 3; // the *_g bank's slot in CSound's state
constexpr int kExtraBanks = 15;     // CC 0 selects 1 to 15

// The monster banks bound past the port's own, and the set they were bound for.
std::array<int, kExtraBanks>                                bound_sets = {};
std::array<std::shared_ptr<const audio::Bank>, kExtraBanks> bound = {};
int                                                         bound_for = -1;

// A set's *_g bank, read and decoded once.
std::shared_ptr<const audio::Bank> MonsterBank(int set) {
    static std::map<int, std::shared_ptr<const audio::Bank>> banks;
    if (auto found = banks.find(set); found != banks.end()) {
        return found->second;
    }
    char path[64];
    std::snprintf(path, sizeof(path), "sound/set/snd%d.snd", set);
    std::shared_ptr<const audio::Bank> bank;
    if (LoadFile2(path, read_buffer, nullptr, 0) != 0) {
        char *name = nullptr;
        int   size = 0;
        for (int index = 0; u_int *file = GetPackFile(read_buffer, index, &name, &size); index++) {
            if (std::strlen(name) != 9 || name[5] != 'g' || std::strcmp(name + 6, ".hd") != 0) {
                continue;
            }
            char body[10];
            std::memcpy(body, name, 7);
            std::strcpy(body + 7, "bd");
            int   body_size = 0;
            auto *bd = reinterpret_cast<const std::uint8_t *>(GetPackFile(read_buffer, body, &body_size));
            if (bd != nullptr) {
                bank = audio::Bank::Create({reinterpret_cast<const std::uint8_t *>(file), static_cast<std::size_t>(size)},
                                           {bd, static_cast<std::size_t>(body_size)});
            }
            break;
        }
    }
    banks.emplace(set, bank);
    return bank;
}

void UnbindAll() {
    for (int i = 0; i < kExtraBanks; i++) {
        if (bound[i] != nullptr) {
            audio::DefaultMixer().BindBank(MIDI_PORT_UNK_A, i + 1, nullptr);
        }
        bound[i].reset();
        bound_sets[i] = 0;
    }
    bound_for = -1;
}

// Kinds whose sounds only their arena's event loads (_LOAD_SOUND_SET).
constexpr std::pair<Monster, int> kArenaSets[] = {
    {Monster::MINOTAUR_JOE,       204},
    {Monster::MASTER_UTAN,        201},
    {Monster::ICE_QUEEN,          202},
    {Monster::KINGS_CURSE_COFFIN, 203},
    {Monster::KINGS_CURSE,        203},
    {Monster::BLACK_KNIGHT,       207},
    {Monster::BLACK_KNIGHT_MOUNT, 207},
};

bool Resolves(const audio::Bank *bank, int program, int note) {
    std::array<audio::Layer, 8> layers;
    return bank != nullptr && bank->Resolve(program, note, 127, layers) > 0;
}

} // namespace

std::vector<int> HomeSoundSets(int model_no) {
    std::vector<int> sets;
    for (int dungeon = 0; dungeon < kDungeons; dungeon++) {
        for (BT_ENEMY_FLOOR *layout : {BtEnemyLayoutList[dungeon], BtUraEnemyLayoutList[dungeon]}) {
            for (int floor = 0; floor < maxFloorTbl__3[dungeon]; floor++) {
                for (const BT_ENEMY_LAYOUT &entry : layout[floor].monster) {
                    if (entry.monster_no != model_no) {
                        continue;
                    }
                    int set = 100 + (dungeon == kDemonShaft ? kDemonShaft + floor / 20 : dungeon);
                    if (std::find(sets.begin(), sets.end(), set) == sets.end()) {
                        sets.push_back(set);
                    }
                }
            }
        }
    }
    for (auto [monster, set] : kArenaSets) {
        if (No(monster) == model_no) {
            sets.push_back(set);
        }
    }
    return sets;
}

void MonsterSoundsFloorLoaded(const CMonstorUnit &unit) {
    int now = SndGetNowSetNo();
    if (bound_for != now) {
        UnbindAll();
    }
    if (LoadedRoster() == nullptr) {
        return;
    }
    bound_for = now;
    for (int slot = 0; slot < unit.model_count; slot++) {
        int kind = ModelKind(slot);
        if (kind < 0 || kind >= kKinds) {
            continue;
        }
        for (int set : HomeSoundSets(kind)) {
            if (set == now || std::find(bound_sets.begin(), bound_sets.end(), set) != bound_sets.end()) {
                continue;
            }
            auto                               free = std::find(bound_sets.begin(), bound_sets.end(), 0);
            std::shared_ptr<const audio::Bank> bank = free != bound_sets.end() ? MonsterBank(set) : nullptr;
            if (bank == nullptr) {
                continue;
            }
            int index = static_cast<int>(free - bound_sets.begin());
            bound_sets[index] = set;
            bound[index] = bank;
            audio::DefaultMixer().BindBank(MIDI_PORT_UNK_A, index + 1, bank);
        }
    }
}

int MonsterSoundBank(int port, int program, int note) {
    if (port != MIDI_PORT_UNK_A || bound_for == -1 || bound_for != SndGetNowSetNo()) {
        return -1;
    }
    auto *own = static_cast<const audio::Bank *>(CSnd.GetMidiState()->port[kMonsterBankSlot].bank);
    if (Resolves(own, program, note)) {
        return 0;
    }
    for (int i = 0; i < kExtraBanks; i++) {
        if (Resolves(bound[i].get(), program, note)) {
            return i + 1;
        }
    }
    return 0;
}

} // namespace expanded
