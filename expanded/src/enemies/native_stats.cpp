#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>

#include "expanded/data/script_damage.hpp"
#include "expanded/enemies/scaling.hpp"
#include "expanded/enemies/species.hpp"
#include "monstorunit.hpp"
#include "platform/paths.hpp"

namespace expanded {

namespace {

// The data directory's script damage file, read once per data directory; empty, with a note, when it was
// never built.
const std::map<std::string, ScriptDamage, std::less<>> &Scripts() {
    static bool                                             loaded = false;
    static std::filesystem::path                            read_from;
    static std::map<std::string, ScriptDamage, std::less<>> scripts;
    if (!loaded || read_from != PathsDataRoot()) {
        loaded = true;
        read_from = PathsDataRoot();
        scripts.clear();
        if (std::ifstream stream(read_from / kScriptDamageFile, std::ios::binary); stream) {
            scripts = ParseScriptDamage(std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()));
        } else {
            std::fprintf(stderr, "expanded: the data has no %s, so monster damage is left as it is; run `dcdata expanded <data>`\n",
                         kScriptDamageFile);
        }
    }
    return scripts;
}

} // namespace

EnemyStats NativeStats(int model_no) {
    if (Species(model_no) == nullptr) {
        return {};
    }
    const MONSTOR_MODEL &model = *Species(model_no);
    EnemyStats           stats = {model.max_hp, model.exp, model.defense, model.hardness, 0, 0};
    auto                 found = Scripts().find(std::string_view(model.script_name, strnlen(model.script_name, sizeof(model.script_name))));
    if (found != Scripts().end()) {
        stats.melee = found->second.melee;
        stats.shot = found->second.shot;
        // A shot fired without a damage argument deals its effect's.
        for (int which = 0; which < 2; which++) {
            int effect = model.shot_effect[which];
            if (found->second.default_shot[which] && effect >= 0 && BtEntryEffectTbl[effect] != nullptr) {
                stats.shot = std::max(stats.shot, static_cast<int>(BtEntryEffectTbl[effect]->damage));
            }
        }
    }
    return stats;
}

} // namespace expanded
