#include "expanded/town/fish_records.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <functional>
#include <vector>

#include "expanded/debug/event_log.hpp"
#include "expanded/town/fish_kinds.hpp"
#include "expanded/weapons/ruby/fairys_ring.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/mardan.hpp"
#include "fish.hpp"
#include "itemdata.hpp"
#include "savedata.hpp"

namespace expanded {

namespace {

constexpr int kCutFish = 8; // f09a: no model, and no spot loads it

bool Recorded(const SV_FISH_DATA &record) {
    return record.rank > 0.0f && GetFishInfo(record.fish_id) != nullptr;
}

// The index of each kind's best record (the first of its largest), the kinds in the order they first appear.
std::vector<std::size_t> BestRecords(std::span<const SV_FISH_DATA> records) {
    std::vector<std::size_t> best;
    for (std::size_t i = 0; i < records.size(); i++) {
        if (!Recorded(records[i])) {
            continue;
        }
        auto kept = std::ranges::find(best, records[i].fish_id, [&](std::size_t j) { return records[j].fish_id; });
        if (kept == best.end()) {
            best.push_back(i);
        } else if (records[i].rank > records[*kept].rank) {
            *kept = i;
        }
    }
    return best;
}

} // namespace

int RecordMagic(int fish_kind, int record_cm) {
    const FishInfo *info = GetFishInfo(fish_kind);
    if (info == nullptr || fish_kind == kCutFish) {
        return 0;
    }
    const int largest = static_cast<int>(std::floor(info->max_size * 10.0f));
    if (largest <= 0 || record_cm < largest) {
        return 0;
    }
    const int most = std::ranges::contains(fish::kGarayans, fish_kind) ? kGarayanRecordMagic : kRecordMagic;
    const int top = static_cast<int>(std::floor(kAriseGrowth * static_cast<float>(largest)));
    if (record_cm >= top) {
        return most;
    }
    const double span = top - largest, short_cm = top - record_cm, foot = kRecordTopShare * span;
    const double share = kRecordTopMagicShare * most, last = (1.0 - kRecordLastShare) * most;
    const double magic =
        short_cm <= foot
            ? last - (last - share) * (short_cm - 1.0) / (foot - 1.0)
            : 1.0 + (share - 1.0) * std::pow(1.0 - (short_cm / span - kRecordTopShare) / (1.0 - kRecordTopShare), kRecordCurveExponent);
    return std::max(1, static_cast<int>(std::round(magic)));
}

int RecordsMagic(std::span<const SV_FISH_DATA> records) {
    int magic = 0;
    for (std::size_t i : BestRecords(records)) {
        magic += RecordMagic(records[i].fish_id, static_cast<int>(std::floor(records[i].rank)));
    }
    return magic;
}

void KeepBestRecords(std::span<SV_FISH_DATA> records) {
    std::vector<std::size_t> best = BestRecords(records);
    for (std::size_t i = 0; i < records.size(); i++) {
        if (records[i].rank > 0.0f && !std::ranges::contains(best, i)) {
            records[i] = {};
        }
    }
    std::ranges::stable_sort(records, std::ranges::greater{}, &SV_FISH_DATA::rank);
}

void FishRecordsChanged(CSaveData &save) {
    std::span<SV_FISH_DATA> records = save.GetFishingRanks();
    KeepBestRecords(records);

    WEAPON_DATA     *arise = GetWeaponData(ITEM_WEAPON_ARISE_MARDAN);
    static const s16 own = arise->magic_max; // the table's, before any record
    const int        magic = WeaponAbilitiesOn() ? std::min(kMagicMost, own + RecordsMagic(records)) : own;
    if (arise->magic_max != magic) {
        LogEvent("arise_magic", std::format("\"from\":{},\"to\":{}", arise->magic_max, magic));
        arise->magic_max = static_cast<s16>(magic);
    }
}

} // namespace expanded
