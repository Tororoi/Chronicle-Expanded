#include "expanded/weapons/osmond/star_breaker/shooting_stars.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/event_sparkle.hpp"
#include "expanded/dungeon/floor_height.hpp"
#include "expanded/dungeon/navigation.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/player_state.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"
#include "mathutil.hpp"
#include "randomitem.hpp"
#include "sysmes.hpp"

namespace expanded {

namespace {

enum class Phase {
    kWaiting, // for a spot
    kFalling,
    kLying,
};

struct Star {
    Phase        phase = Phase::kWaiting;
    float        spot[3] = {};
    float        at[3] = {};
    float        direction[3] = {};
    CRandomItem *items = nullptr; // its drop, lying
    int          slot = -1;
    float        item_at[3] = {};
    bool         refused = false; // its pickup refused (a full attachment bag), until the player walks off
};

std::vector<Star> stars;
Twinkle           twinkle;

std::optional<std::string> StarLine() {
    return LocalizeText("expanded.star_breaker.shooting_star", "The Star Breaker sent you a shooting star!");
}

[[maybe_unused]] const bool kLine = (SetSystemMessage(kShootingStarMessage, StarLine), true);

// A star not down yet goes with the floor; one lying is the floor's drop, and goes as the game's drops go.
void OnFloor() {
    std::erase_if(stars, [](const Star &star) { return star.phase != Phase::kLying; });
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

float Unit() {
    return std::uniform_real_distribution<float>(0.0f, 1.0f)(ModRandom());
}

void Copy(float *to, const float *from) {
    std::copy_n(from, 3, to);
}

// The way down: across the player's view from his left or right, at kStarFallAngle to the floor.
void Begin(Star &star, const float *player, float yaw, int draws) {
    int   side = ModRoll(2) == 0 ? -1 : 1;
    float across[2] = {static_cast<float>(side) * std::cos(yaw), static_cast<float>(side) * -std::sin(yaw)};
    float run = kStarFallHeight / std::tan(kStarFallAngle);
    float length = std::hypot(run, kStarFallHeight);
    star.direction[0] = across[0] * run / length;
    star.direction[1] = -kStarFallHeight / length;
    star.direction[2] = across[1] * run / length;
    for (int c = 0; c < 3; c++) {
        star.at[c] = star.spot[c] - star.direction[c] * length;
    }
    star.phase = Phase::kFalling;
    EventSparkleReady();
    int args[4] = {-1, -1, -1, -1};
    SetSystemMes(kShootingStarMessage, kStarMessageFrames, MES_POS_BOTTOM, 0, args, nullptr);
    // Where it lands from the player: ahead, and to his left (+x facing +z).
    float dx = star.spot[0] - player[0], dz = star.spot[2] - player[2];
    LogEvent("star", std::format("\"step\":\"falling\",\"x\":{:.1f},\"ground\":{:.1f},\"z\":{:.1f},\"ahead\":{:.1f},\"left\":{:.1f},"
                                 "\"draws\":{},\"side\":{}",
                                 star.spot[0], star.spot[1], star.spot[2], dx * std::sin(yaw) + dz * std::cos(yaw),
                                 dx * std::cos(yaw) - dz * std::sin(yaw), draws, side));
}

void Aim(Star &star) {
    if (UserStatus == nullptr) {
        return;
    }
    float player[3] = {CharaMain.pos[0], CharaMain.pos[1], CharaMain.pos[2]};
    float yaw = PlayerYaw();
    for (int draw = 1; draw <= kStarDraws; draw++) {
        float ahead = Unit();
        float sideways = Unit();
        if (ShootingStarSpot(player, yaw, ahead, sideways, star.spot)) {
            Begin(star, player, yaw, draw);
            return;
        }
    }
}

// Down on the spot: the floor's drop, which hops as it appears and then waits to be taken. With every drop's place in use
// it rests on the spot until one is free.
void Land(Star &star) {
    CRandomItem *items = RandomItem;
    int          slot = items != nullptr ? items->CheckID() : -1;
    if (slot < 0) {
        return;
    }
    float place[4] = {star.spot[0], star.spot[1], star.spot[2], 1.0f};
    items->Set(place, 1, -1, ITEM_ATTACH_SYNTHESIS_SPHERE);
    // Measured now, so the hop draws: the game measures a drop only once it waits.
    items->distance[slot] = DistVector(items->position[slot], CharaMain.pos);
    star.items = items;
    star.slot = slot;
    Copy(star.item_at, items->position[slot]);
    star.phase = Phase::kLying;
    LogEvent("star", std::format("\"step\":\"landed\",\"slot\":{}", slot));
}

void Fall(Star &star) {
    if (star.at[1] > star.spot[1]) {
        for (int c = 0; c < 3; c++) {
            star.at[c] += star.direction[c] * kStarFallSpeed;
        }
    }
    if (star.at[1] <= star.spot[1]) {
        Copy(star.at, star.spot);
        Land(star);
    }
}

// Whether a lying star's drop is gone: taken (CheckPosition frees its place, its pickup event pending) or swept
// with the floor.
bool Gone(Star &star) {
    const CRandomItem &items = *star.items;
    int                slot = star.slot;
    bool               there = items.id[slot] != -1 && items.amount[slot] == -1 && items.item_no[slot] == ITEM_ATTACH_SYNTHESIS_SPHERE &&
                 items.position[slot][0] == star.item_at[0] && items.position[slot][1] == star.item_at[1] &&
                 items.position[slot][2] == star.item_at[2];
    if (there) {
        bool refused = items.pickup_blocked[slot] > 0;
        if (refused && !star.refused) {
            LogEvent("star", "\"step\":\"refused\"");
        }
        star.refused = refused;
        return false;
    }
    if (items.id[slot] == -1 && items.pickup_event[slot] == ITEM_ATTACH_SYNTHESIS_SPHERE) {
        LogEvent("star", "\"step\":\"picked\"");
    }
    return true;
}

void DrawFalling(const Star &star) {
    for (int k = kStarTrail; k >= 0; k--) {
        float fade = 1.0f - static_cast<float>(k) / static_cast<float>(kStarTrail + 1);
        float at[3];
        for (int c = 0; c < 3; c++) {
            at[c] = star.at[c] - star.direction[c] * kStarTrailGap * static_cast<float>(k);
        }
        float half = kStarHeadSize / 2.0f * fade;
        DrawEventSparkle(at, half, half / kSparkleWidth, static_cast<int>(128.0f * fade));
    }
}

void DrawLying(const Star &star, float scale) {
    const CRandomItem &items = *star.items;
    float              at[3];
    Copy(at, items.position[star.slot]);
    at[1] += 8.0f * std::sin(items.phase[star.slot]) + kStarTwinkleLift; // as CRandomItem::Draw lifts the drop
    float half_height = scale * kStarTwinkleSize;
    DrawEventSparkle(at, kSparkleWidth * half_height, half_height, 128);
}

} // namespace

void SendShootingStar() {
    stars.emplace_back();
}

bool ShootingStarSpot(const float *player, float yaw, float ahead, float sideways, float *spot) {
    float distance = kStarNearest + ahead * (kStarFarthest - kStarNearest);
    float aside = (2.0f * sideways - 1.0f) * kStarSideways;
    float x = player[0] + std::sin(yaw) * distance + std::cos(yaw) * aside;
    float z = player[2] + std::cos(yaw) * distance - std::sin(yaw) * aside;
    float ground = 0.0f;
    if (!FloorUnder(x, z, player[1] + kStarStep, &ground) || ground < player[1] - kStarStep) {
        return false;
    }
    float to[3] = {x, ground, z};
    if (!NavigateClear(player, to, kStarClearance, player[1])) {
        return false;
    }
    Copy(spot, to);
    return true;
}

int StarsWaiting() {
    return static_cast<int>(std::ranges::count_if(stars, [](const Star &star) { return star.phase == Phase::kWaiting; }));
}

int StarsFalling() {
    return static_cast<int>(std::ranges::count_if(stars, [](const Star &star) { return star.phase == Phase::kFalling; }));
}

int StarsLying() {
    return static_cast<int>(std::ranges::count_if(stars, [](const Star &star) { return star.phase == Phase::kLying; }));
}

bool FallingStar(int star, float *at, float *direction) {
    for (const Star &falling : stars) {
        if (falling.phase == Phase::kFalling && star-- == 0) {
            Copy(at, falling.at);
            Copy(direction, falling.direction);
            return true;
        }
    }
    return false;
}

bool LyingStar(int star, const CRandomItem **items, int *slot) {
    for (const Star &lying : stars) {
        if (lying.phase == Phase::kLying && star-- == 0) {
            *items = lying.items;
            *slot = lying.slot;
            return true;
        }
    }
    return false;
}

void ShootingStarsStep() {
    for (Star &star : stars) {
        switch (star.phase) {
            case Phase::kWaiting:
                Aim(star);
                break;
            case Phase::kFalling:
                Fall(star);
                break;
            case Phase::kLying:
                break;
        }
    }
    std::erase_if(stars, [](Star &star) { return star.phase == Phase::kLying && Gone(star); });
}

void ShootingStarsDraw() {
    float scale = twinkle.Step();
    for (const Star &star : stars) {
        if (star.phase == Phase::kFalling) {
            DrawFalling(star);
        } else if (star.phase == Phase::kLying && star.items == RandomItem) {
            DrawLying(star, scale);
        }
    }
}

void ShootingStarsVisitBegins() {
    stars.clear();
    twinkle = {};
}

} // namespace expanded
