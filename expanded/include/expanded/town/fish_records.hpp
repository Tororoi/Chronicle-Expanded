#pragma once

#include <span>

namespace dc::game {
class CSaveData;
struct SV_FISH_DATA;
} // namespace dc::game

namespace expanded {

// Fishing records feed Arise Mardan. The records list keeps each kind of fish's best catch alone, largest first, and
// each kind's best grants Arise Mardan Max Magic: none below the kind's largest size in the game's fish_info, and from
// it, by the centimetres s short of kAriseGrowth times it (the top, in whole centimetres, a size only Arise Mardan's
// fish reach), the same for every kind: max(1, round(most / (1 + k * s^kRecordCurveExponent))), k = step / (most - step),
// so most at the top and step less a centimetre short of it; most and step kRecordMagic and kRecordTopStep, or for the
// Garayans kGarayanRecordMagic and kGarayanRecordTopStep. Every record at its top takes Arise Mardan from its own 120
// to kMagicMost. With the Weapon Abilities off, Arise Mardan keeps its own Max Magic.
inline constexpr int    kRecordMagic = 43, kRecordTopStep = 10;
inline constexpr int    kGarayanRecordMagic = 117, kGarayanRecordTopStep = 20;
inline constexpr double kRecordCurveExponent = 0.954;
inline constexpr int    kMagicMost = 999;

// The Max Magic a kind's best catch of record_cm centimetres grants.
int RecordMagic(int fish_kind, int record_cm);

// The Max Magic every kind's best catch in records grants together.
int RecordsMagic(std::span<const SV_FISH_DATA> records);

// Keeps records to each kind's best catch, largest first, emptying the rest; a list already kept is left as it is.
void KeepBestRecords(std::span<SV_FISH_DATA> records);

// Hook: CSaveData::SetFishingRank having put a catch in the records, a save loaded, a new game begun: keeps the
// records to each kind's best and gives Arise Mardan (WeaponList) its Max Magic for them.
void FishRecordsChanged(CSaveData &save);

} // namespace expanded
