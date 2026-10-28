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
#include "randomitem.hpp"
#include "sysmes.hpp"

namespace expanded {

namespace {

enum class Phase {
    kWaiting, // for a spot
    kFalling, // falling or bouncing
    kLying,
};

struct Star {
    Phase        phase = Phase::kWaiting;
    StarPath     path;
    float        frames = 0.0f; // since it started down
    float        shine = 0.0f;  // since its trail ran into it at rest
    bool         bounced = false;
    CRandomItem *items = nullptr; // its drop, lying
    int          slot = -1;
    float        item_at[3] = {};
    bool         refused = false; // its pickup refused (a full attachment bag), until the player walks off
};

std::vector<Star> stars;

std::optional<std::string> StarLine() {
    return LocalizeText("expanded.star_breaker.shooting_star", "The Star Breaker sent you a shooting star!");
}

[[maybe_unused]] const bool kLine = (SetSystemMessage(kShootingStarMessage, StarLine), true);

// A star not down yet goes with the floor; one lying is the floor's drop, and goes as the game's drops go.
void OnFloor() {
    std::erase_if(stars, [](const Star &star) { return star.phase != Phase::kLying; });
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

// Its speed along the floor and down as it falls, and up as it leaves the floor.
const float kFallAcross = kStarFallSpeed * std::cos(kStarFallAngle);
const float kFallDown = kStarFallSpeed * std::sin(kStarFallAngle);
const float kBounceRise = kStarBounceUp * kFallDown;

float Unit() {
    return std::uniform_real_distribution<float>(0.0f, 1.0f)(ModRandom());
}

void Copy(float *to, const float *from) {
    std::copy_n(from, 3, to);
}

// Floor the player can walk to at rest, a bounce of share from touch toward side: floor under it near his ground, and
// every cell from touch to it walkable.
bool BounceRests(const float *player, float yaw, const float *touch, int side, float share, float *rest) {
    float along = static_cast<float>(side) * StarBounceLength() * share;
    rest[0] = touch[0] + std::cos(yaw) * along;
    rest[2] = touch[2] - std::sin(yaw) * along;
    rest[1] = touch[1];
    if (share == 0.0f) {
        return true;
    }
    float ground = 0.0f;
    if (!FloorUnder(rest[0], rest[2], player[1] + kStarStep, &ground) || ground < player[1] - kStarStep) {
        return false;
    }
    rest[1] = ground;
    return NavigateClear(touch, rest, kStarClearance, player[1]);
}

void Begin(Star &star, const float *player, float yaw, int draws) {
    star.phase = Phase::kFalling;
    star.frames = 0.0f;
    EventSparkleReady();
    int args[4] = {-1, -1, -1, -1};
    SetSystemMes(kShootingStarMessage, kStarMessageFrames, MES_POS_BOTTOM, 0, args, nullptr);
    // Where it touches and rests from the player: ahead, and to his left (+x facing +z).
    const StarPath &path = star.path;
    auto            ahead = [&](const float *at) { return (at[0] - player[0]) * std::sin(yaw) + (at[2] - player[2]) * std::cos(yaw); };
    auto            left = [&](const float *at) { return (at[0] - player[0]) * std::cos(yaw) - (at[2] - player[2]) * std::sin(yaw); };
    LogEvent("star", std::format("\"step\":\"falling\",\"ahead\":{:.1f},\"left\":{:.1f},\"rest_ahead\":{:.1f},\"rest_left\":{:.1f},"
                                 "\"x\":{:.1f},\"z\":{:.1f},\"ground\":{:.1f},\"side\":{},\"bounce\":{:.2f},\"draws\":{}",
                                 ahead(path.touch), left(path.touch), ahead(path.rest), left(path.rest), path.rest[0], path.rest[2],
                                 path.rest[1], path.side, path.bounce, draws));
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
        float spot[3];
        if (!ShootingStarSpot(player, yaw, ahead, sideways, spot)) {
            continue;
        }
        int side = ModRoll(2) == 0 ? -1 : 1;
        if (ShootingStarPath(player, yaw, ahead, sideways, side, &star.path)) {
            Begin(star, player, yaw, draw);
            return;
        }
    }
}

// At rest: the floor's drop, set down as one that has just finished its hop, so that the game settles it (its sound) on
// its next step and then waits for it to be taken. With every drop's place in use it stays at rest until one is free.
void Settle(Star &star) {
    CRandomItem *items = RandomItem;
    int          slot = items != nullptr ? items->CheckID() : -1;
    if (slot < 0) {
        return;
    }
    float place[4] = {star.path.rest[0], star.path.rest[1], star.path.rest[2], 1.0f};
    items->Set(place, 1, -1, ITEM_ATTACH_SYNTHESIS_SPHERE);
    items->phase[slot] = std::numbers::pi_v<float>;
    star.items = items;
    star.slot = slot;
    Copy(star.item_at, items->position[slot]);
    star.phase = Phase::kLying;
    LogEvent("star", std::format("\"step\":\"rested\",\"slot\":{}", slot));
}

void Fall(Star &star) {
    star.frames += 1.0f;
    if (!star.bounced && star.frames >= star.path.FallFrames()) {
        star.bounced = true;
        LogEvent("star", "\"step\":\"bounced\"");
    }
    if (star.frames >= star.path.FallFrames() + star.path.BounceFrames()) {
        Settle(star);
    }
}

// Whether a lying star's drop is still where it was set down.
bool Holds(const Star &star) {
    const CRandomItem &items = *star.items;
    int                slot = star.slot;
    return items.id[slot] != -1 && items.amount[slot] == -1 && items.item_no[slot] == ITEM_ATTACH_SYNTHESIS_SPHERE &&
           items.position[slot][0] == star.item_at[0] && items.position[slot][1] == star.item_at[1] &&
           items.position[slot][2] == star.item_at[2];
}

// Whether a lying star's drop is gone: taken (CheckPosition frees its place, its pickup event pending) or swept
// with the floor.
bool Gone(Star &star) {
    if (Holds(star)) {
        bool refused = star.items->pickup_blocked[star.slot] > 0;
        if (refused && !star.refused) {
            LogEvent("star", "\"step\":\"refused\"");
        }
        star.refused = refused;
        return false;
    }
    if (star.items->id[star.slot] == -1 && star.items->pickup_event[star.slot] == ITEM_ATTACH_SYNTHESIS_SPHERE) {
        LogEvent("star", "\"step\":\"picked\"");
    }
    return true;
}

// The frames between the head and the last of its trail.
constexpr float kTrailFrames = kStarTrailGap / kStarFallSpeed * static_cast<float>(kStarTrail);

// The sparkle and its trail, sitting on its path: at rest the trail runs on into the head, which then shines.
void DrawStar(const Star &star) {
    const float gap = kStarTrailGap / kStarFallSpeed;
    const float dip = (1.0f - std::cos(2.0f * std::numbers::pi_v<float> * star.shine / kStarShineFrames)) / 2.0f;
    const float shine = 1.0f - kStarShineDepth * dip;
    const float bright = 1.0f - kStarShineFade * dip;
    for (int k = kStarTrail; k >= 0; k--) {
        float frames = star.frames - gap * static_cast<float>(k);
        if (frames < 0.0f) {
            continue;
        }
        float fade = 1.0f - static_cast<float>(k) / static_cast<float>(kStarTrail + 1);
        float at[3];
        star.path.At(frames, at);
        float half = kStarHeadSize / 2.0f * fade * shine;
        at[1] += half / kSparkleWidth;
        DrawEventSparkle(at, half, half / kSparkleWidth, static_cast<int>(128.0f * fade * bright));
    }
}

} // namespace

float StarPath::FallFrames() const {
    float length = std::sqrt((touch[0] - start[0]) * (touch[0] - start[0]) + (touch[1] - start[1]) * (touch[1] - start[1]) +
                             (touch[2] - start[2]) * (touch[2] - start[2]));
    return length / kStarFallSpeed;
}

float StarPath::BounceFrames() const {
    return 2.0f * kBounceRise / kStarGravity;
}

void StarPath::At(float frames, float *at) const {
    float fall = FallFrames();
    if (frames <= fall) {
        float t = fall > 0.0f ? frames / fall : 1.0f;
        for (int c = 0; c < 3; c++) {
            at[c] = start[c] + (touch[c] - start[c]) * t;
        }
        return;
    }
    float u = std::min(frames - fall, BounceFrames());
    float t = u / BounceFrames();
    for (int c = 0; c < 3; c++) {
        at[c] = touch[c] + (rest[c] - touch[c]) * t;
    }
    at[1] += kBounceRise * u - 0.5f * kStarGravity * u * u;
    if (u >= BounceFrames()) {
        Copy(at, rest);
    }
}

float StarBounceLength() {
    return kStarBounceAcross * kFallAcross * 2.0f * kBounceRise / kStarGravity;
}

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

bool ShootingStarPath(const float *player, float yaw, float ahead, float sideways, int side, StarPath *path) {
    float touch[3];
    if (!ShootingStarSpot(player, yaw, ahead, sideways, touch)) {
        return false;
    }

    // The whole bounce from either side, then half of it, then none.
    struct Try {
        int   side;
        float share;
    };

    const Try tries[] = {
        {side,  1.0f              },
        {-side, 1.0f              },
        {side,  kStarBounceShorter},
        {-side, kStarBounceShorter},
        {side,  0.0f              },
    };
    for (const Try &attempt : tries) {
        float rest[3];
        if (!BounceRests(player, yaw, touch, attempt.side, attempt.share, rest)) {
            continue;
        }
        Copy(path->touch, touch);
        Copy(path->rest, rest);
        path->side = attempt.side;
        path->bounce = attempt.share;
        float length = kStarFallHeight / std::sin(kStarFallAngle);
        float run = length * std::cos(kStarFallAngle) * static_cast<float>(attempt.side);
        path->start[0] = touch[0] - std::cos(yaw) * run;
        path->start[1] = touch[1] + kStarFallHeight;
        path->start[2] = touch[2] + std::sin(yaw) * run;
        return true;
    }
    return false;
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

bool FallingStar(int star, StarPath *path, float *frames) {
    for (const Star &falling : stars) {
        if (falling.phase == Phase::kFalling && star-- == 0) {
            *path = falling.path;
            *frames = falling.frames;
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

bool StarDropHidden(const CRandomItem &items, int slot) {
    return std::ranges::any_of(stars, [&](const Star &star) {
        return star.phase == Phase::kLying && star.items == &items && star.slot == slot && Holds(star);
    });
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
            case Phase::kLying: {
                float merged = star.path.FallFrames() + star.path.BounceFrames() + kTrailFrames;
                if (star.frames < merged) {
                    star.frames = std::min(star.frames + 1.0f, merged);
                } else {
                    star.shine = std::fmod(star.shine + 1.0f, kStarShineFrames);
                }
                break;
            }
        }
    }
    std::erase_if(stars, [](Star &star) { return star.phase == Phase::kLying && Gone(star); });
}

void ShootingStarsDraw() {
    for (const Star &star : stars) {
        if (star.phase == Phase::kFalling || (star.phase == Phase::kLying && star.items == RandomItem)) {
            DrawStar(star);
        }
    }
}

void ShootingStarsVisitBegins() {
    stars.clear();
}

} // namespace expanded
