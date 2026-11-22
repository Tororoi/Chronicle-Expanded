#pragma once

#include <span>
#include <string>
#include <vector>

#include "expanded/quests/givers.hpp"

namespace expanded {

// Fishing quests: Pike (Norune), Pao (Matataki), Sam (Queens) and Devia (Muska Lacka) each set a catch in their
// waters, half the time a count of one kind (kFishCount of it, a Garayan alone), half the time any fish whose length
// falls in a window (kSizeWidth wide, its least rolled between the waters' least and most). Pao's waters are Peanut
// Pond and the waterfall, half the time either; a kind both hold (the Baku Baku) may come from anywhere in Matataki,
// as may a size quest's fish. Sam's are East Harbor, Devia's the oasis.
//
// A catch counts at the quest's waters, from when it is set; the giver then pays fishing points and sets the next
// quest when next asked: a count quest a roll of the waters' points per fish (a Garayan's twice that at Matataki), a
// size quest a roll up to its window's most, kSizeReward below it, and the waters' bonus. Sam's third quest done pays
// a special reward as well: East Harbor's fish are seen under the water from then on, as the other waters' are.
//
//   unheard  -- asked -->  active  -- the catch -->  complete  -- the reward -->  idle  -- asked -->  active
inline constexpr int kFishCount = 2;
inline constexpr int kSizeReward = 30;
inline constexpr int kSamSpecial = 3;

// Where a quest's fish is caught: a spot, or both of Matataki's.
enum class FishPlace {
    NORUNE,
    PEANUT_POND,
    WATERFALL,
    MATATAKI,
    EAST_HARBOR,
    OASIS
};

struct FishingGiver {
    const char *quest;
    Giver       giver;
    FishPlace   waters;     // the place a size quest names (Pao's: Matataki)
    int         size_least; // the least a window's least rolls
    int         size_most;  // and the most
    int         size_width; // the window's width
    int         points_least;
    int         points_most; // a count quest's points per fish
    int         size_bonus;  // added to a size quest's points
};

std::span<const FishingGiver> FishingGivers();

void RegisterFishingQuests();

// Every text the quests show.
std::vector<dc::quest::Text> FishingQuestTexts();

// Hook: a fish caught (ED_FISHING_MES_EVALUATE): its kind, length in cm, and the spot it was caught at.
void FishingQuestCatch(int fish_kind, int size, int spot);

// Whether the fish at East Harbor are seen under the water (Sam's special reward).
bool EastHarborFishShown();

// What a giver's quest wants, for the quest tracker: false when none is under way.
struct FishingQuest {
    bool      size = false; // a size quest
    int       fish = -1;    // a count quest's kind
    int       left = 0;
    int       total = 0;
    int       least = 0; // a size quest's window
    int       most = 0;
    FishPlace place = FishPlace::NORUNE;
    bool      complete = false;
};

bool FishingQuestOf(const FishingGiver &giver, FishingQuest &quest);

// A kind's name (as many as a count quest wants: one Garayan, else several), and where a place is ("at East Harbor").
std::string QuestFishName(int fish_kind);
std::string QuestFishPlace(FishPlace place);

} // namespace expanded
