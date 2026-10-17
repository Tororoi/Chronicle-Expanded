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

// Where the MOTION_END after the cfg's KEY_START begins.
std::size_t MotionEnd(const std::string &text) {
    std::size_t start = text.find("KEY_START");
    std::size_t end = start == std::string::npos ? start : text.find("MOTION_END", start);
    if (end == std::string::npos) {
        throw std::runtime_error("the cfg has no KEY_START ... MOTION_END block");
    }
    return end;
}

Bytes ReadFile(const fs::path &file) {
    std::ifstream stream(file, std::ios::binary);
    if (!stream) {
        throw std::runtime_error(std::format("cannot read {}", file.string()));
    }
    return Bytes(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

} // namespace

Bytes AppendKeys(std::span<const unsigned char> cfg, std::string_view lines) {
    std::string text(cfg.begin(), cfg.end());
    text.insert(MotionEnd(text), lines);
    return Bytes(text.begin(), text.end());
}

Bytes AppendEvents(std::span<const unsigned char> cfg, std::string_view lines) {
    std::string text(cfg.begin(), cfg.end());
    std::size_t at = text.find('\n', MotionEnd(text));
    text.insert(at == std::string::npos ? text.size() : at + 1, lines);
    return Bytes(text.begin(), text.end());
}

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
    cfg.ReplacePayload(AppendEvents(AppendKeys(cfg.Payload(), lines), events));
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
    cfg.ReplacePayload(AppendEvents(AppendKeys(cfg.Payload(), lines), events));
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

int BuildBossModel(const fs::path &data, const BossBuild &boss, const std::function<void(const std::string &)> &say) {
    std::error_code error;
    for (const char *source : {boss.battle, boss.cutscene}) {
        if (!fs::is_regular_file(data / source, error)) {
            say(std::format("boss model {}: skipped, the data has no {}\n", boss.name, source));
            return 0;
        }
    }
    try {
        Bytes    model = boss.build(ReadFile(data / boss.battle), ReadFile(data / boss.cutscene));
        fs::path target = data / boss.target;
        bool     current = fs::is_regular_file(target, error) && fs::file_size(target, error) == model.size() &&
                           ReadFile(target) == model;
        if (!current) {
            fs::create_directories(target.parent_path());
            std::ofstream stream(target, std::ios::binary | std::ios::trunc);
            stream.write(reinterpret_cast<const char *>(model.data()), static_cast<std::streamsize>(model.size()));
            stream.close();
            if (!stream) {
                throw std::runtime_error(std::format("cannot write {}", target.string()));
            }
        }
        say(std::format("boss model {}: {} ({} bytes{})\n", boss.name, boss.target, model.size(), current ? ", unchanged" : ""));
        return 0;
    } catch (const std::exception &failure) {
        say(std::format("boss model {}: not built: {}\n", boss.name, failure.what()));
        return 1;
    }
}

} // namespace

int BuildBossModels(const std::filesystem::path &data, std::FILE *log) {
    auto say = [&](const std::string &line) {
        if (log) {
            std::fputs(line.c_str(), log);
        }
    };
    int failures = 0;
    for (const BossBuild &boss : kBossBuilds) {
        failures += BuildBossModel(data, boss, say);
    }
    return failures;
}

} // namespace expanded
