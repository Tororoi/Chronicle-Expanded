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
#include "expanded/weapons/goro/magical_hammer.hpp"
#include "expanded/weapons/toan/mardan.hpp"
#include "fish.hpp"
#include "fishing.hpp"

namespace expanded {

namespace {

constexpr int kFishSlots = 6; // FishingLoadFish's CFish[6]

// Bait Ward's turns fall on frames of g_frame.
static_assert(kSteerFrames % kBaitWardFrames == 0);

int                         g_spot = -1;
int                         g_frame = 0; // the frames since the spot's fish loaded, modulo kSteerFrames
int                         g_sword = 0;
bool                        g_ward = false; // a Magical Hammer owned as the spot's fish loaded
int                         g_bait = -1;
std::array<int, kFishSlots> g_since = {}; // the frames since the Mardan sword last turned each fish

void Turn(CFish &fish, int slot, const float *player, std::string_view by) {
    TurnFishToward(fish, player);
    LogEvent("fish_turn", std::format("\"fish\":{},\"kind\":{},\"by\":\"{}\"", slot, fish.fish_kind, by));
}

void TurnAway(CFish &fish, int slot, const float *hook) {
    TurnFishAway(fish, hook);
    LogEvent("fish_turn", std::format("\"fish\":{},\"kind\":{},\"by\":\"bait_ward\"", slot, fish.fish_kind));
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

void TurnFishAway(CFish &fish, const float *position) {
    sceVu0FVECTOR here;
    fish.GetPosition(here);
    float dx = here[0] - position[0];
    float dz = here[2] - position[2];
    if (dx != 0.0f || dz != 0.0f) {
        fish.target_yaw = atan2f(dx, dz);
    }
}

void FishSteeringLoaded(int spot) {
    g_spot = spot;
    g_frame = 0;
    g_sword = MardanSword();
    g_ward = BaitWard();
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
        // Bait Ward: a fish with no liking for the bait turns away from the hook on its slot's frame, and only then.
        if (g_ward && g_bait >= 0 && fish.info.food_affinity[g_bait] <= 0.0f) {
            if (g_frame % kBaitWardFrames == i * (kBaitWardFrames / kFishSlots)) {
                TurnAway(fish, i, hookp[0]);
            }
            continue;
        }
        if (spot_turn) {
            Turn(fish, i, player, "spot");
        }
        int frames = g_bait < 0 ? 0 : MardanSteerFrames(g_sword, fish.fish_kind, fish.info.food_affinity[g_bait]);
        if (frames > 0 && g_since[i] >= frames) {
            Turn(fish, i, player, "mardan");
            g_since[i] = 0;
        }
    }
    g_frame = (g_frame + 1) % kSteerFrames;
}

} // namespace expanded
