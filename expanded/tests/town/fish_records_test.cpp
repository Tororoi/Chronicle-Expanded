#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <span>
#include <utility>
#include <vector>

#include "expanded/town/fish_kinds.hpp"
#include "expanded/town/fish_records.hpp"
#include "expanded/weapons/ruby/fairys_ring.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/mardan.hpp"
#include "fish.hpp"
#include "itemdata.hpp"
#include "savedata.hpp"
#include "test_support.hpp"

namespace {

using expanded::test::OptionScope;
using expanded::test::TestSave;

constexpr int kFishKinds = 18;
constexpr int kCutFish = 8;
constexpr int kAriseOwnMagic = 120;

// A kind's largest size in the game, in centimetres.
int Largest(int kind) {
    return static_cast<int>(GetFishInfo(kind)->max_size * 10.0f);
}

// The top of a kind's records: Arise Mardan's growth on its largest, in whole centimetres.
int Top(int kind) {
    return static_cast<int>(std::floor(expanded::kAriseGrowth * static_cast<float>(Largest(kind))));
}

class FishRecords : public ::testing::Test {
protected:
    void TearDown() override { Arise().magic_max = saved_magic_; }

    static WEAPON_DATA &Arise() { return *GetWeaponData(ITEM_WEAPON_ARISE_MARDAN); }

    std::span<SV_FISH_DATA, 64> Records() { return SaveData->GetFishingRanks(); }

    // The records that are not empty, as (kind, centimetres).
    std::vector<std::pair<int, int>> Listed() {
        std::vector<std::pair<int, int>> listed;
        for (int i = 0; i < 64; i++) {
            if (const SV_FISH_DATA *record = SaveData->GetFishingRank(i)) {
                listed.emplace_back(record->fish_id, static_cast<int>(record->rank));
            }
        }
        return listed;
    }

    // Every kind but the cut fish caught at size(kind) centimetres, each through the game's insert.
    template <class Size>
    void CatchEveryKind(Size size) {
        for (int kind = 0; kind < kFishKinds; kind++) {
            if (kind != kCutFish) {
                SaveData->SetFishingRank(kind, static_cast<float>(size(kind)));
            }
        }
    }

    TestSave save_;
    s16      saved_magic_ = Arise().magic_max;
};

} // namespace

TEST_F(FishRecords, ARecordGrantsNothingBelowTheLargestOneAtItAndTheMostAtTheTop) {
    for (int kind = 0; kind < kFishKinds; kind++) {
        if (kind == kCutFish) {
            EXPECT_EQ(expanded::RecordMagic(kind, 1000), 0);
            continue;
        }
        const int  largest = Largest(kind), top = Top(kind);
        const bool garayan = kind == expanded::fish::kMardanGarayan || kind == expanded::fish::kBaronGarayan;
        const int  most = garayan ? expanded::kGarayanRecordMagic : expanded::kRecordMagic;
        EXPECT_EQ(expanded::RecordMagic(kind, largest - 1), 0) << kind;
        EXPECT_EQ(expanded::RecordMagic(kind, largest), 1) << kind;
        // Every centimetre of the top tenth of the range counts, and its foot is half the most.
        const int foot = top - static_cast<int>(std::lround(expanded::kRecordTopShare * (top - largest)));
        for (int cm = foot + 1; cm <= top; cm++) {
            EXPECT_GT(expanded::RecordMagic(kind, cm), expanded::RecordMagic(kind, cm - 1)) << kind << " " << cm;
        }
        EXPECT_NEAR(expanded::RecordMagic(kind, foot), most / 2.0, 1.0) << kind;
        EXPECT_EQ(expanded::RecordMagic(kind, top), most) << kind;
        EXPECT_EQ(expanded::RecordMagic(kind, 3 * largest), most) << kind;
        int previous = 0;
        for (int cm = largest; cm <= top; cm++) {
            int magic = expanded::RecordMagic(kind, cm);
            EXPECT_GE(magic, previous) << kind << " " << cm;
            EXPECT_LE(magic, most);
            previous = magic;
        }
        EXPECT_LT(expanded::RecordMagic(kind, top - (top - largest) / 10), most) << kind;
    }
    EXPECT_EQ(expanded::RecordMagic(-1, 100), 0);
    EXPECT_EQ(expanded::RecordMagic(kFishKinds, 100), 0);
}

TEST_F(FishRecords, TheTopTenthClimbsStraightAndTheRestSpreads) {
    // Baron Garayan, 300 cm at most in the game and 450 with Arise Mardan: from 59 at 435 cm up about 4 a centimetre to
    // 117; below, 46 at 420, 19 at 375, 4 at 330.
    EXPECT_EQ(Top(expanded::fish::kBaronGarayan), 450);
    EXPECT_EQ(expanded::RecordMagic(expanded::fish::kBaronGarayan, 449), 113);
    EXPECT_EQ(expanded::RecordMagic(expanded::fish::kBaronGarayan, 435), 59);
    EXPECT_EQ(expanded::RecordMagic(expanded::fish::kBaronGarayan, 420), 46);
    EXPECT_EQ(expanded::RecordMagic(expanded::fish::kBaronGarayan, 375), 19);
    EXPECT_EQ(expanded::RecordMagic(expanded::fish::kBaronGarayan, 330), 4);
    // Bobo (200 cm, 300) and Niler (100, 150) alike over their own ranges: 22 at the top tenth's foot, 7 halfway.
    EXPECT_EQ(expanded::RecordMagic(0, 299), 41);
    EXPECT_EQ(expanded::RecordMagic(0, 290), 22);
    EXPECT_EQ(expanded::RecordMagic(0, 250), 7);
    EXPECT_EQ(expanded::RecordMagic(expanded::fish::kNiler, 149), 39);
    EXPECT_EQ(expanded::RecordMagic(expanded::fish::kNiler, 125), 7);
}

TEST_F(FishRecords, EveryRecordAtTheTopTakesAriseMardanTo999) {
    EXPECT_EQ(saved_magic_, kAriseOwnMagic);
    CatchEveryKind(Top);
    EXPECT_EQ(expanded::RecordsMagic(Records()), 15 * expanded::kRecordMagic + 2 * expanded::kGarayanRecordMagic);
    EXPECT_EQ(Arise().magic_max, expanded::kMagicMost);
}

TEST_F(FishRecords, EachKindAtTheLargestGivesOne) {
    CatchEveryKind(Largest);
    EXPECT_EQ(Arise().magic_max, kAriseOwnMagic + 17);
    // Under the largest, nothing.
    expanded::KeepBestRecords(Records());
    for (SV_FISH_DATA &record : Records()) {
        record.rank = record.rank > 0.0f ? record.rank - 1.0f : 0.0f;
    }
    expanded::FishRecordsChanged(*SaveData);
    EXPECT_EQ(Arise().magic_max, kAriseOwnMagic);
}

TEST_F(FishRecords, TheListKeepsEachKindsBestLargestFirst) {
    SaveData->SetFishingRank(0, 150.0f);
    SaveData->SetFishingRank(1, 100.0f);
    SaveData->SetFishingRank(0, 210.0f);
    SaveData->SetFishingRank(0, 180.0f); // smaller than the record: gone
    SaveData->SetFishingRank(1, 170.0f);
    SaveData->SetFishingRank(expanded::fish::kBaronGarayan, 120.0f);
    EXPECT_EQ(Listed(), (std::vector<std::pair<int, int>>{
                            {0,                             210},
                            {1,                             170},
                            {expanded::fish::kBaronGarayan, 120}
    }));
    // Bobo's 210 cm is past its 200 and Gobbler's 170 past its 160; the Baron Garayan's 120 is short of 300.
    EXPECT_EQ(Arise().magic_max, kAriseOwnMagic + expanded::RecordMagic(0, 210) + expanded::RecordMagic(1, 170));
    EXPECT_EQ(expanded::RecordMagic(expanded::fish::kBaronGarayan, 120), 0);
}

TEST_F(FishRecords, ALoadedListIsKeptToTheBestKeepingEachEntrysOwnData) {
    std::span<SV_FISH_DATA, 64> records = Records();
    for (int i = 0; i < 64; i++) {
        records[i] = {.fish_id = i % 3, .rank = static_cast<float>(64 - i), .unk_8 = static_cast<float>(i), .unk_C = 0.5f};
    }
    records[40] = {.fish_id = 2, .rank = 500.0f, .unk_8 = 7.0f, .unk_C = 8.0f};
    records[41] = {.fish_id = 30, .rank = 900.0f}; // no such kind
    records[42] = {.fish_id = -1, .rank = 800.0f};
    expanded::FishRecordsChanged(*SaveData);

    EXPECT_EQ(Listed(), (std::vector<std::pair<int, int>>{
                            {2, 500},
                            {0, 64 },
                            {1, 63 }
    }));
    EXPECT_EQ(records[0].unk_8, 7.0f);
    EXPECT_EQ(records[0].unk_C, 8.0f);
    EXPECT_EQ(records[1].unk_8, 0.0f);
    EXPECT_EQ(records[2].unk_8, 1.0f);
    for (int i = 3; i < 64; i++) {
        EXPECT_EQ(SaveData->GetFishingRank(i), nullptr) << i;
    }

    // The game's insert finds room again.
    SaveData->SetFishingRank(5, 20.0f);
    EXPECT_EQ(Listed().back(), std::make_pair(5, 20));
}

TEST_F(FishRecords, WithoutWeaponAbilitiesAriseKeepsItsOwnMaxMagic) {
    OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    CatchEveryKind(Top);
    EXPECT_EQ(Arise().magic_max, kAriseOwnMagic);
    EXPECT_EQ(Listed().size(), 17u);

    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, true));
    expanded::FishRecordsChanged(*SaveData);
    EXPECT_EQ(Arise().magic_max, expanded::kMagicMost);
}

TEST_F(FishRecords, ANewListTakesAriseBackToItsOwn) {
    CatchEveryKind(Top);
    ASSERT_EQ(Arise().magic_max, expanded::kMagicMost);
    for (SV_FISH_DATA &record : Records()) {
        record = {};
    }
    expanded::FishRecordsChanged(*SaveData);
    EXPECT_EQ(Arise().magic_max, kAriseOwnMagic);
    EXPECT_TRUE(Listed().empty());
}

TEST_F(FishRecords, AListAlreadyKeptIsLeftAsItIs) {
    std::span<SV_FISH_DATA, 64> records = Records();
    records[0] = {.fish_id = 3, .rank = 240.0f, .unk_8 = 1.0f, .unk_C = 2.0f};
    records[1] = {.fish_id = 0, .rank = 190.0f, .unk_8 = 3.0f, .unk_C = 4.0f};
    records[2] = {.fish_id = expanded::fish::kMardanGarayan, .rank = 92.0f};
    for (int i = 3; i < 64; i++) {
        records[i] = {.fish_id = i % 2 == 0 ? 0 : -1};
    }
    std::vector<SV_FISH_DATA> before(records.begin(), records.end());
    expanded::FishRecordsChanged(*SaveData);
    EXPECT_EQ(std::memcmp(before.data(), records.data(), records.size_bytes()), 0);
}
