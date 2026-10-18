#include "expanded/data/boss_model.hpp"

#include <algorithm>
#include <cstdio>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

#include "expanded/data/cfg_text.hpp"
#include "expanded/data/data_build.hpp"
#include "expanded/data/town_model.hpp"

namespace expanded {

namespace {

namespace fs = std::filesystem;

bool IsMot(std::string_view name) {
    return name.size() > 4 && (name.ends_with(".mot") || name.ends_with(".MOT"));
}

// The last keyed frame of the pack's motions.
std::uint32_t LastFrame(const ChrPack &pack) {
    std::uint32_t last = 0;
    for (const ChrRecord &record : pack.records) {
        if (!IsMot(record.name)) {
            continue;
        }
        for (const MotTrack &track : MotFile::FromRecord(record).tracks) {
            for (const MotKeyframe &key : track.keys) {
                last = std::max(last, key.Frame());
            }
        }
    }
    return last;
}

} // namespace

std::vector<std::pair<double, int>> SoundEvents(std::span<const unsigned char> cfg) {
    std::vector<std::pair<double, int>> events;
    std::istringstream                  text(std::string(cfg.begin(), cfg.end()));
    std::string                         line;
    while (std::getline(text, line)) {
        double frame = 0;
        int    kind = 0;
        int    sound = 0;
        if (std::sscanf(line.c_str(), " EVENT %lf , %d , %d", &frame, &kind, &sound) == 3 && kind == 1) {
            events.emplace_back(frame, sound);
        }
    }
    return events;
}

Bytes BuildJoeModel(std::span<const unsigned char> battle, std::span<const unsigned char> cutscene) {
    ChrPack base = ChrPack::Parse(battle);
    ChrPack source = ChrPack::Parse(cutscene);
    // Past the last keyed frame, on the next multiple of ten.
    int first = static_cast<int>(LastFrame(base) / 10 + 1) * 10;
    int last = first + (kJoeFallLast - kJoeFallFirst);
    GraftMotionWindow(base, source, kJoeCutsceneCfg, kJoeFallFirst, kJoeFallLast, first, last);
    ChrRecord  &cfg = base.Require("info.cfg");
    std::string lines = std::format("KEY\t{},\t{},\t{:.1f},\t//stagger and fall {}\r\n"
                                    "KEY\t{},\t{},\t0.0,\t//death loop {}\r\n",
                                    first, last, kJoeFallSpeed, kJoeFallKey, last, last, kJoeDeathLoopKey);

    // The fall's stomps, and the roar the defeat scene plays over it.
    std::vector<std::pair<double, int>> sounds;
    for (auto [frame, sound] : SoundEvents(source.Require(kJoeCutsceneCfg).Payload())) {
        if (frame >= kJoeFallFirst && frame <= kJoeFallLast) {
            sounds.emplace_back(frame, sound);
        }
    }
    sounds.emplace_back(kJoeRoarFrame, kJoeRoarSound);
    std::stable_sort(sounds.begin(), sounds.end(), [](auto &a, auto &b) { return a.first < b.first; });
    std::string events;
    for (auto [frame, sound] : sounds) {
        events += std::format("EVENT {:g},1,{}\r\n", frame - kJoeFallFirst + first, sound);
    }
    cfg.ReplacePayload(AppendCfgEvents(AppendCfgKeys(cfg.Payload(), lines), events));
    return base.Rebuild();
}

Bytes BuildBlackKnightModel(std::span<const unsigned char> battle, std::span<const unsigned char> cutscene) {
    ChrPack base = ChrPack::Parse(battle);
    ChrPack source = ChrPack::Parse(cutscene);
    int     from = kBlackKnightDeath[0].first;
    int     first = static_cast<int>(LastFrame(base) / 10 + 1) * 10;
    GraftMotionWindow(base, source, kBlackKnightCutsceneCfg, from, kBlackKnightDeath[kBlackKnightDeathKeys - 1].last, first,
                      first + kBlackKnightDeath[kBlackKnightDeathKeys - 1].last - from);
    std::string lines;
    for (int i = 0; i < kBlackKnightDeathKeys; i++) {
        const DeathWindow &window = kBlackKnightDeath[i];
        lines += std::format("KEY\t{},\t{},\t{:.2f},\t//{} {}\r\n", window.first - from + first, window.last - from + first,
                             window.speed, window.name, kBlackKnightBowKey + i);
    }
    std::string events = std::format("EVENT {},1,{}\r\n", kBlackKnightDeath[2].first - from + first, kBlackKnightScreamSound);
    ChrRecord  &cfg = base.Require("info.cfg");
    cfg.ReplacePayload(AppendCfgEvents(AppendCfgKeys(cfg.Payload(), lines), events));
    return base.Rebuild();
}

namespace {

struct BossBuild {
    const char *name;
    const char *battle;
    const char *cutscene;
    const char *target;
    Bytes (*build)(std::span<const unsigned char>, std::span<const unsigned char>);
};

const BossBuild kBossBuilds[] = {
    {"Minotaur Joe", kJoeBattleFile,         kJoeCutsceneFile,         kJoeModelFile,         BuildJoeModel        },
    {"Black Knight", kBlackKnightBattleFile, kBlackKnightCutsceneFile, kBlackKnightModelFile, BuildBlackKnightModel},
};

} // namespace

int BuildBossModels(const std::filesystem::path &data, std::FILE *log) {
    std::vector<DataBuild> builds;
    for (const BossBuild &boss : kBossBuilds) {
        std::vector<std::string> sources = {boss.battle, boss.cutscene};
        auto                     build = [&boss](const DiscReader &read) { return boss.build(read(boss.battle), read(boss.cutscene)); };
        builds.push_back(DataBuild{boss.name, boss.target, sources, build});
    }
    return RunDataBuilds(data, "boss model", builds, log);
}

} // namespace expanded
