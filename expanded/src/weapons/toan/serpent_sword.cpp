#include "expanded/weapons/toan/serpent_sword.hpp"

#include <libvu0.h>

#include <algorithm>
#include <cmath>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "frame.hpp"
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
    int         left = 0;  // steps left of the motion playing once (the appearance, a hiss)
    int         quiet = 0; // steps of the idle since the last hiss
    bool        backing[kFloorMonsters] = {};
    float       upright[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    }; // turns the neck's pose at the appearance's end to its rest
    float neck[3] = {};                         // where the neck ends the appearance
    float angles[3] = {};                       // the model's turn (the last solved, the next guess)
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

// Marks the monsters backing off; true when one starts to.
bool MarkBacking(CMonstorUnit &unit) {
    bool started = false;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        float dx = 0.0f, dz = 0.0f, distance = 0.0f;
        if (!Offset(unit, monster, dx, dz, distance)) {
            snake.backing[monster] = false;
        } else if (distance < kSnakeRetreatRadius) {
            started = started || !snake.backing[monster];
            snake.backing[monster] = true;
        } else if (distance > kSnakeCalmRadius) {
            snake.backing[monster] = false;
        }
    }
    return started;
}

// A motion's length in steps, from its key (its frames at its speed).
int Steps(int motion) {
    const MOTION_INFO *info = snake.model->GetMotionInfo(motion);
    if (info == nullptr || info->speed <= 0.0f) {
        return 60;
    }
    return static_cast<int>(std::ceil(static_cast<float>(info->end - info->start) / info->speed));
}

void Play(int motion, bool once) {
    snake.motion = motion;
    snake.model->motion_no = motion;
    snake.model->motion_flags |= kFromFirstFrame;
    snake.model->motion_speed = -1.0f;
    snake.left = once ? Steps(motion) : 0;
}

// Chooses the motion: the appearance played through, then the idle, a hiss now and then and when a monster comes near.
void Choose(bool startled) {
    if (snake.motion < 0) {
        Play(kSnakeAppear, true);
        return;
    }
    if (snake.left > 0 && --snake.left > 0) {
        return;
    }
    if (startled || ++snake.quiet >= kSnakeHissPeriod) {
        snake.quiet = 0;
        Play(kSnakeHiss, true);
    } else if (snake.motion != kSnakeIdle) {
        Play(kSnakeIdle, false);
    }
}

// After the appearance, the neck (and j_01_1, where the event's body joins, which follows the neck's track) held where
// the appearance left it, whatever the idle or the hiss moves it to: only the head turns and the mouth opens.
void Pin() {
    if (snake.motion == kSnakeAppear) {
        return;
    }
    for (const char *name : {"neck", "j_01_1"}) {
        if (CFrame *neck = FrameNamed(snake.model->frame, name)) {
            std::copy(snake.neck, snake.neck + 3, &neck->local[3][0]);
        }
    }
}

// The engine's turn of a frame by angles (CFrame's srt: about x, then y, then z).
void Turned(sceVu0FMATRIX out, const float *angles) {
    sceVu0UnitMatrix(out);
    sceVu0RotMatrixX(out, out, angles[0]);
    sceVu0RotMatrixY(out, out, angles[1]);
    sceVu0RotMatrixZ(out, out, angles[2]);
}

// The angles that turn as target does (its 3x3), from guess on (Gauss-Newton on the nine entries).
void AnglesFor(sceVu0FMATRIX target, float *angles) {
    constexpr float kStep = 1e-3f;
    for (int round = 0; round < 12; round++) {
        sceVu0FMATRIX at;
        Turned(at, angles);
        float residual[9], jacobian[9][3];
        for (int k = 0; k < 9; k++) {
            residual[k] = at[k / 3][k % 3] - target[k / 3][k % 3];
        }
        for (int a = 0; a < 3; a++) {
            float moved[3] = {angles[0], angles[1], angles[2]};
            moved[a] += kStep;
            sceVu0FMATRIX near;
            Turned(near, moved);
            for (int k = 0; k < 9; k++) {
                jacobian[k][a] = (near[k / 3][k % 3] - at[k / 3][k % 3]) / kStep;
            }
        }
        float normal[3][3] = {}, rhs[3] = {};
        for (int a = 0; a < 3; a++) {
            for (int b = 0; b < 3; b++) {
                for (int k = 0; k < 9; k++) {
                    normal[a][b] += jacobian[k][a] * jacobian[k][b];
                }
            }
            for (int k = 0; k < 9; k++) {
                rhs[a] -= jacobian[k][a] * residual[k];
            }
        }
        float det = normal[0][0] * (normal[1][1] * normal[2][2] - normal[1][2] * normal[2][1]) -
                    normal[0][1] * (normal[1][0] * normal[2][2] - normal[1][2] * normal[2][0]) +
                    normal[0][2] * (normal[1][0] * normal[2][1] - normal[1][1] * normal[2][0]);
        if (std::fabs(det) < 1e-9f) {
            return;
        }
        for (int a = 0; a < 3; a++) {
            float swapped[3][3];
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3; c++) {
                    swapped[r][c] = c == a ? rhs[r] : normal[r][c];
                }
            }
            float d = swapped[0][0] * (swapped[1][1] * swapped[2][2] - swapped[1][2] * swapped[2][1]) -
                      swapped[0][1] * (swapped[1][0] * swapped[2][2] - swapped[1][2] * swapped[2][0]) +
                      swapped[0][2] * (swapped[1][0] * swapped[2][1] - swapped[1][1] * swapped[2][0]);
            angles[a] += d / det;
        }
    }
}

// Above the sword, facing as the player faces; the head held where its root is, whatever its motion moves.
void Place() {
    float   above[3] = {CharaMain.pos[0], CharaMain.pos[1], CharaMain.pos[2]};
    CFrame *hilt = NowWeapon != nullptr && NowWeapon->frame != nullptr ? FrameNamed(NowWeapon->frame, "dcol0") : nullptr;
    CFrame *tip = NowWeapon != nullptr && NowWeapon->frame != nullptr ? FrameNamed(NowWeapon->frame, "dcol1") : nullptr;
    if (hilt != nullptr && tip != nullptr) {
        sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, a, b;
        hilt->GetWorldPosition(a, origin);
        tip->GetWorldPosition(b, origin);
        for (int c = 0; c < 3; c++) {
            above[c] = (a[c] + b[c]) * 0.5f;
        }
    }
    float yaw = PlayerYaw();
    // The counter-turn first, then the player's yaw, as the model's own angles.
    sceVu0FMATRIX upright, target;
    sceVu0UnitMatrix(upright);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            upright[r][c] = snake.upright[r][c];
        }
    }
    sceVu0RotMatrixY(target, upright, yaw);
    AnglesFor(target, snake.angles);
    snake.model->SetRotation(snake.angles[0], snake.angles[1], snake.angles[2]);
    // Where the neck ends the appearance goes kSnakeLift over the sword, the head's middle kSnakeBack ahead of it.
    float back = kSnakeBack * kSnakeScale;
    float spot[3] = {above[0] - std::sin(yaw) * back, above[1] + kSnakeLift, above[2] - std::cos(yaw) * back};
    for (int c = 0; c < 3; c++) {
        spot[c] -= kSnakeScale * (snake.neck[0] * target[0][c] + snake.neck[1] * target[1][c] + snake.neck[2] * target[2][c]);
    }
    snake.model->SetPosition(spot[0], spot[1], spot[2]);
    snake.model->SetScale(kSnakeScale, kSnakeScale, kSnakeScale);
}

// The appearance played through unseen: the turn from the neck's pose at its end to its rest pose (the model's, where
// its skin was made), which the model takes so the appearance ends with the head as the model has it; and where the neck
// ends.
void Calibrate() {
    CFrame *neck = FrameNamed(snake.model->frame, "neck");
    float   rest[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
    if (neck != nullptr) {
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                rest[r][c] = neck->local[r][c];
            }
        }
    }
    Play(kSnakeAppear, true);
    for (int step = 0; step < snake.left; step++) {
        snake.model->Step();
    }
    if (neck != nullptr) {
        std::copy(&neck->local[3][0], &neck->local[3][0] + 3, snake.neck);
        // end · upright = rest, so upright = end⁻¹ · rest (end a turn and a scale: its inverse the transpose over the
        // squared row lengths).
        float inverse[3][3];
        for (int r = 0; r < 3; r++) {
            float length = neck->local[r][0] * neck->local[r][0] + neck->local[r][1] * neck->local[r][1] + neck->local[r][2] * neck->local[r][2];
            for (int c = 0; c < 3; c++) {
                inverse[c][r] = length > 0.0f ? neck->local[r][c] / length : (r == c ? 1.0f : 0.0f);
            }
        }
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                snake.upright[r][c] = inverse[r][0] * rest[0][c] + inverse[r][1] * rest[1][c] + inverse[r][2] * rest[2][c];
            }
        }
    }
    snake.motion = -1;
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
        Snake kept = snake;
        snake = Snake{};
        snake.model = kept.model;
        std::copy(&kept.upright[0][0], &kept.upright[0][0] + 9, &snake.upright[0][0]);
        std::copy(kept.neck, kept.neck + 3, snake.neck);
        return;
    }
    if (snake.model == nullptr) {
        if ((snake.model = EnterFloorCharacter(kKillerSnakeFile, kKillerSnakeCfg)) == nullptr) {
            return;
        }
        Calibrate();
    }
    snake.up = true;
    bool startled = false;
    if (NowMonstorUnit != nullptr) {
        startled = MarkBacking(*NowMonstorUnit);
    }
    Choose(startled);
    Place();
    snake.model->Step();
}

void SnakeCharmerPosed(CFrame *frame) {
    if (snake.model != nullptr && frame == snake.model->frame) {
        Pin();
    }
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
