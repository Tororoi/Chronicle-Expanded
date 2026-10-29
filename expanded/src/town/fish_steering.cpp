#include "expanded/town/fish_steering.hpp"

#include <libvu0.h>

#include <array>
#include <climits>
#include <cmath>
#include <format>
#include <string_view>

#include "character.hpp"
#include "editloop.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/town/fishing.hpp"
#include "expanded/weapons/toan/mardan.hpp"
#include "fish.hpp"
#include "fishing.hpp"

namespace expanded {

namespace {

constexpr int kFishSlots = 6; // FishingLoadFish's CFish[6]

int                         g_spot = -1;
int                         g_frame = 0; // the frames since the spot's fish loaded, modulo kSteerFrames
int                         g_sword = 0;
int                         g_bait = -1;
std::array<int, kFishSlots> g_since = {}; // the frames since the Mardan sword last turned each fish

void Turn(CFish &fish, int slot, const float *player, std::string_view by) {
    TurnFishToward(fish, player);
    LogEvent("fish_turn", std::format("\"fish\":{},\"kind\":{},\"by\":\"{}\"", slot, fish.fish_kind, by));
}

} // namespace

void TurnFishToward(CFish &fish, const float *position) {
    sceVu0FVECTOR here;
    fish.GetPosition(here);
    float dx = position[0] - here[0];
    float dz = position[2] - here[2];
    if (dx != 0.0f || dz != 0.0f) {
        fish.target_yaw = atan2f(dx, dz);
    }
}

void FishSteeringLoaded(int spot) {
    g_spot = spot;
    g_frame = 0;
    g_sword = MardanSword();
    g_bait = -1;
    g_since.fill(INT_MAX);
}

void FishSteeringStep() {
    if (Fish == nullptr || Chara == nullptr) {
        return;
    }
    if (esa_type >= 0) {
        g_bait = esa_type;
    }

    sceVu0FVECTOR player;
    Chara->GetPosition(player);
    const bool spot_turn = g_frame == 0 && (g_spot == kMatatakiWaterfall || g_spot == kEastHarbor);
    for (int i = 0; i < kFishSlots; i++) {
        CFish &fish = Fish[i];
        if (g_since[i] < INT_MAX) {
            g_since[i]++;
        }
        if (fish.fish_kind < 0) {
            continue;
        }
        if (spot_turn) {
            Turn(fish, i, player, "spot");
        }
        int frames = g_bait < 0 ? 0 : MardanSteerFrames(g_sword, fish.fish_kind, BaitAffinity(fish.info, g_bait));
        if (frames > 0 && g_since[i] >= frames) {
            Turn(fish, i, player, "mardan");
            g_since[i] = 0;
        }
    }
    g_frame = (g_frame + 1) % kSteerFrames;
}

} // namespace expanded
