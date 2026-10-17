#include "expanded/data/boss_model.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>
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
    std::size_t start = text.find("KEY_START");
    std::size_t end = start == std::string::npos ? start : text.find("MOTION_END", start);
    if (end == std::string::npos) {
        throw std::runtime_error("the cfg has no KEY_START ... MOTION_END block");
    }
    text.insert(end, lines);
    return Bytes(text.begin(), text.end());
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
    cfg.ReplacePayload(AppendKeys(cfg.Payload(), lines));
    return base.Rebuild();
}

int BuildBossModels(const std::filesystem::path &data, std::FILE *log) {
    auto say = [&](const std::string &line) {
        if (log) {
            std::fputs(line.c_str(), log);
        }
    };
    std::error_code error;
    for (const char *source : {kJoeBattleFile, kJoeCutsceneFile}) {
        if (!fs::is_regular_file(data / source, error)) {
            say(std::format("boss model Minotaur Joe: skipped, the data has no {}\n", source));
            return 0;
        }
    }
    try {
        Bytes    model = BuildJoeModel(ReadFile(data / kJoeBattleFile), ReadFile(data / kJoeCutsceneFile));
        fs::path target = data / kJoeModelFile;
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
        say(std::format("boss model Minotaur Joe: {} ({} bytes{})\n", kJoeModelFile, model.size(),
                        current ? ", unchanged" : ""));
        return 0;
    } catch (const std::exception &failure) {
        say(std::format("boss model Minotaur Joe: not built: {}\n", failure.what()));
        return 1;
    }
}

} // namespace expanded
