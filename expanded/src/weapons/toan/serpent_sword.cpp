#include "expanded/weapons/toan/serpent_sword.hpp"

#include <cmath>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "gameutil.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kFromFirstFrame = 4; // CCharacter::motion_flags

struct Snake {
    CCharacter *model = nullptr;
    bool        up = false;
    int         motion = -1;
    int         rear_frames = 0; // left of the rear
    int         target = -1;     // the monster it rears at
    bool        backing[kFloorMonsters] = {};
};

Snake snake;

// A monster in play: its offset from the player across the ground and its distance, false when it is not in play.
bool Offset(CMonstorUnit &unit, int monster, float &dx, float &dz, float &distance) {
    if (monster < 0 || monster >= kFloorMonsters || !MonsterInPlay(unit.monster[monster])) {
        return false;
    }
    float at[4];
    unit.chara[monster][0].GetPosition(at);
    dx = at[0] - CharaMain.pos[0];
    dz = at[2] - CharaMain.pos[2];
    distance = std::sqrt(dx * dx + dz * dz);
    return true;
}

// A monster backing off: from coming within the retreat radius until it is out past the calm radius.
bool Backing(CMonstorUnit &unit, int monster, float &dx, float &dz, float &distance) {
    return Offset(unit, monster, dx, dz, distance) && snake.backing[monster];
}

void MarkBacking(CMonstorUnit &unit) {
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        float dx = 0.0f, dz = 0.0f, distance = 0.0f;
        if (!Offset(unit, monster, dx, dz, distance)) {
            snake.backing[monster] = false;
        } else if (distance < kSnakeRetreatRadius) {
            snake.backing[monster] = true;
        } else if (distance > kSnakeCalmRadius) {
            snake.backing[monster] = false;
        }
    }
}

void Play(int motion) {
    snake.motion = motion;
    snake.model->motion_no = motion;
    snake.model->motion_flags |= kFromFirstFrame;
    snake.model->motion_speed = -1.0f;
}

// The rear's length in steps, from its key (its frames at its speed).
int RearFrames() {
    const MOTION_INFO *info = snake.model->GetMotionInfo(kSnakeRear);
    if (info == nullptr || info->speed <= 0.0f) {
        return 60;
    }
    return static_cast<int>(std::ceil(static_cast<float>(info->end - info->start) / info->speed));
}

// Chooses the motion: the rear at the nearest monster in the radius, played through, else the sway.
void Choose() {
    if (snake.rear_frames > 0 && --snake.rear_frames > 0) {
        return;
    }
    snake.target = NowMonstorUnit != nullptr ? NearestMonster(*NowMonstorUnit, CharaMain.pos, kSnakeRetreatRadius) : -1;
    if (snake.target >= 0) {
        Play(kSnakeRear);
        snake.rear_frames = RearFrames();
    } else if (snake.motion != kSnakeSway) {
        Play(kSnakeSway);
    }
}

void Place() {
    float yaw = PlayerYaw();
    float sy = std::sin(yaw), cy = std::cos(yaw);
    float x = CharaMain.pos[0] + cy * kSnakeSide, z = CharaMain.pos[2] - sy * kSnakeSide;
    float facing = yaw;
    float at[4];
    if (snake.target >= 0 && NowMonstorUnit != nullptr && MonsterInPlay(NowMonstorUnit->monster[snake.target])) {
        NowMonstorUnit->chara[snake.target][0].GetPosition(at);
        facing = std::atan2(at[0] - x, at[2] - z);
    }
    snake.model->SetPosition(x, CharaMain.pos[1], z);
    snake.model->SetRotation(0.0f, facing, 0.0f);
    snake.model->SetScale(kSnakeScale, kSnakeScale, kSnakeScale);
}

} // namespace

bool SerpentSwordActive() {
    return WeaponActive(ITEM_WEAPON_SERPENT_SWORD);
}

int SnakeSlayerAmount(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount) {
    // The wielder's own weapon hits: they carry its anti-monster table.
    bool wielders = hit.vs_monster != nullptr && UserStatus != nullptr && hit.owner == UserStatus->cur_chara;
    if (!wielders || !SerpentSwordActive()) {
        return amount;
    }
    int  kind = ModelKind(unit.monster[monster].base_model);
    bool medu = kind == No(Monster::AUNTIE_MEDU) || kind == No(Monster::AUNTIE_MEDU_ENHANCED);
    return medu ? unit.monster[monster].hp : amount;
}

void SnakeCharmerStep() {
    if (!SerpentSwordActive() || !Guarding() || BtEventMode != 0) {
        CCharacter *model = snake.model;
        snake = Snake{};
        snake.model = model;
        return;
    }
    if (snake.model == nullptr && (snake.model = EnterFloorCharacter(kKillerSnakeFile, kKillerSnakeCfg)) == nullptr) {
        return;
    }
    snake.up = true;
    if (NowMonstorUnit != nullptr) {
        MarkBacking(*NowMonstorUnit);
    }
    Choose();
    Place();
    snake.model->Step();
}

void SnakeCharmerDraw() {
    if (snake.up && snake.model != nullptr && snake.model->frame != nullptr && BtEventMode == 0) {
        DrawFloorCharacter(*snake.model);
    }
}

bool SnakeCharmerUp() {
    return snake.up;
}

bool SnakeCharmerTarget(int monster, float *position) {
    float dx = 0.0f, dz = 0.0f, distance = 0.0f;
    if (!snake.up || NowMonstorUnit == nullptr || !Backing(*NowMonstorUnit, monster, dx, dz, distance)) {
        return false;
    }
    if (distance < 0.01f) {
        dx = 1.0f, dz = 0.0f, distance = 1.0f;
    }
    // Twice the radius behind it, away from the player: it heads there.
    float away = 2.0f * kSnakeRetreatRadius / distance;
    float at[4];
    NowMonstorUnit->chara[monster][0].GetPosition(at);
    position[0] = at[0] + dx * away;
    position[1] = at[1];
    position[2] = at[2] + dz * away;
    position[3] = 1.0f;
    return true;
}

void SnakeCharmerMonsterMove(CMonstorUnit &unit, int monster) {
    MONSTOR &live = unit.monster[monster];
    float    dx = 0.0f, dz = 0.0f, distance = 0.0f;
    if (!snake.up || live.movement_speed == 0.0f || !Backing(unit, monster, dx, dz, distance) || distance < 0.01f) {
        return;
    }
    if (live.movement[0] * dx + live.movement[2] * dz < 0.0f) {
        live.movement[0] = dx / distance;
        live.movement[2] = dz / distance;
    }
}

void SnakeCharmerReset() {
    snake = Snake{};
}

} // namespace expanded
