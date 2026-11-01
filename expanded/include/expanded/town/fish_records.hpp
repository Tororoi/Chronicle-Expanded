#pragma once

#include <span>

namespace dc::game {
class CSaveData;
struct SV_FISH_DATA;
} // namespace dc::game

namespace expanded {

// Fishing records feed Arise Mardan. The records list keeps each kind of fish's best catch alone, largest first, and
// each kind's best grants Arise Mardan Max Magic, a whole number for each record, along the centimetres from the kind's
// largest size in the game's fish_info to kAriseGrowth times it (the top, in whole centimetres, a size only Arise
// Mardan's fish reach): none below the largest. Over the top kRecordTopShare of those centimetres the Max Magic climbs
// straight from kRecordTopMagicShare of the most to all but kRecordLastShare of it a centimetre short of the top, so every
// centimetre there counts, and the last centimetre brings the rest; below them it
// rises from 1 at the largest to that share as 1 + (share * most - 1) * (1 - e)^kRecordCurveExponent, e how far down
// the rest of the way it is. The most is kRecordMagic, kGarayanRecordMagic for the Garayans. Every record at its top
// takes Arise Mardan from its own 120 to kMagicMost. With the Weapon Abilities off, Arise Mardan keeps its own Max Magic.
inline constexpr int    kRecordMagic = 43;
inline constexpr int    kGarayanRecordMagic = 117;
inline constexpr double kRecordTopShare = 0.1, kRecordTopMagicShare = 0.5, kRecordLastShare = 0.2, kRecordCurveExponent = 2.0;
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
