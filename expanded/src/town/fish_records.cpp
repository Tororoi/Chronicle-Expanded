#include "expanded/town/fish_records.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <functional>
#include <vector>

#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/ruby/fairys_ring.hpp"
#include "expanded/weapons/super_steve.hpp"
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
    const int most = std::ranges::contains(kGarayans, fish_kind) ? kGarayanRecordMagic : kRecordMagic;
    if (record_cm >= 2 * largest) {
        return most;
    }
    const double short_by = static_cast<double>(2 * largest - record_cm) / largest;
    const double magic = most / (1.0 + kRecordCurve * std::pow(short_by, kRecordCurveExponent));
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
