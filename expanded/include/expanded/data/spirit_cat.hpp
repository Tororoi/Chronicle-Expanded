#pragma once

// The Divine Beast Title's spirit cat, built from the disc at extraction byte for byte as the PS2 mod bakes it
// (CatPackBakes): the s86 town cat, its nodes named catroot / cat_*, its clips trimmed to its keys with the e01 town cat's
// float-up grafted in, Dran's wings, the Super Steve cape (a cloth) and mask, a flat white wing texture, the cape's and
// the element glow disc. Each weapon's cat shows what it wears. Includes no game header, so dcdata can build with it.

#include <functional>
#include <span>
#include <string>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

// The built model under the data directory, and what it is built from.
inline constexpr const char *kSpiritCatModelFile = "expanded/chara/spirit_cat.chr";
inline constexpr const char *kSpiritCatSource = "gedit/s86/chara/c04cat.chr";
inline constexpr const char *kSpiritCatFloatSource = "gedit/e01/chara/e04c04cat.chr";
inline constexpr const char *kSpiritCatWingSource = "dun/monstor/c12a.chr"; // Dran
inline constexpr const char *kSpiritCatCfg = "info.cfg";

// The float-up: the e01 cat's frames 160-169 (its KEY 5's start), grafted by joint name into 285-294, where the s86
// cat has no clip.
inline constexpr int kSpiritCatFloatFirst = 160, kSpiritCatFloatLast = 169;
inline constexpr int kSpiritCatFloatFrame = 285;

// The keys, from KEY_START 64, each "first, last, speed" on the s86 cat's frames.
enum SpiritCatKey {
    kCatStand = 64, // 10-20 at 0.1
    kCatReady,      // 95-105 at 0.4: the crouch
    kCatRun,        // 120-136 at 0.5
    kCatTakeOff,    // 190-204 at 0.5
    kCatLeap,       // 205-214 at 0.5: the fall pose
    kCatLand,       // 215-227 at 0.36
    kCatWalk,       // 60-80 at 1.0
    kCatFloat,      // 285-294 at 0.75: the float-up, the pounce's vertical leap
    kCatSit,        // 30-40 at 0.1
};

// The pack from the s86 cat, the e01 cat, Dran and the glow disc's source pack; log hears the bake's notes.
Bytes BuildSpiritCatModel(std::span<const unsigned char> cat, std::span<const unsigned char> town, std::span<const unsigned char> dran,
                          std::span<const unsigned char> glow_pack, const std::function<void(const std::string &)> &log = {});

// The cat's model.
std::vector<DataBuild> SpiritCatModelBuilds();

} // namespace expanded
