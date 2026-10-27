#include "expanded/enemies/steve.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <vector>

#include "clsmes.hpp"
#include "dataread.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/sounds.hpp"
#include "expanded/enemies/species.hpp"
#include "mainselect.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

// The sound sets of the staged floor's kinds, and the Steve files read so far.
std::vector<int>                          floor_sets;
std::map<std::string, std::vector<short>> files;

// Reads a Steve file through read_buffer, once; only a floor load may, while read_buffer is free.
void Read(const std::string &path) {
    if (files.contains(path)) {
        return;
    }
    int                 size = 0;
    std::vector<short> &bank = files[path];
    if (LoadFile2(const_cast<char *>(path.c_str()), read_buffer, &size, 0) != 0 && size >= 4) {
        auto *text = reinterpret_cast<const short *>(read_buffer);
        bank.assign(text, text + size / 2);
    }
}

const short *Cached(const std::string &path) {
    auto found = files.find(path);
    return found != files.end() ? found->second.data() : nullptr;
}

} // namespace

std::string SteveFile(int sound_set, int language) {
    char path[64];
    if (sound_set >= kDemonShaftSoundSets) {
        std::snprintf(path, sizeof(path), "dun/message/ww_mes/steve07_%d_%d.mes", sound_set - kDemonShaftSoundSets + 1,
                      language);
    } else {
        std::snprintf(path, sizeof(path), "dun/message/ww_mes/steve0%d_%d.mes", sound_set - kDungeonSoundSets + 1, language);
    }
    return path;
}

const short *SteveMessage(const short *bank, int id) {
    if (bank == nullptr) {
        return nullptr;
    }
    const int    count = bank[0];
    const short *body = &bank[1];
    const auto   end = static_cast<unsigned short>(bank[1]);
    for (int i = 0; i < count; i++) {
        if (body[i * 2 + 1] != id) {
            continue;
        }
        const short *text = &body[count] + body[i * 2 + 2];
        // Some files hold an empty entry pointing at the end of their text.
        if (reinterpret_cast<const char *>(text) - reinterpret_cast<const char *>(bank) >= end || *text == MES_CODE_END) {
            return nullptr;
        }
        return text;
    }
    return nullptr;
}

void SteveLinesFloorLoaded(const CMonstorUnit &unit) {
    floor_sets.clear();
    if (LoadedRoster() == nullptr) {
        return;
    }
    for (int slot = 0; slot < unit.model_count; slot++) {
        int kind = ModelKind(slot);
        if (!ValidKind(kind) || unit.model[slot].name_no <= 0) {
            continue;
        }
        for (int set : HomeSoundSets(kind)) {
            if (std::find(floor_sets.begin(), floor_sets.end(), set) == floor_sets.end()) {
                floor_sets.push_back(set);
                Read(SteveFile(set, LanguageCode));
            }
        }
    }
}

short *SteveLine(const ClsMes &mes, int id) {
    if (&mes != &DngMesStb || floor_sets.empty() || id < kSteveLines || SteveMessage(mes.buff, id) != nullptr) {
        return nullptr;
    }
    for (int set : floor_sets) {
        if (const short *text = SteveMessage(Cached(SteveFile(set, LanguageCode)), id)) {
            return const_cast<short *>(text);
        }
    }
    return nullptr;
}

} // namespace expanded
