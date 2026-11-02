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

constexpr int   kStopAtEnd = 2, kFromFirstFrame = 4; // CCharacter::motion_flags
constexpr float kOpaque = 128.0f;                    // CCharacter::ambient_offset[3]
constexpr float kTau = 6.2831853f;

struct Snake {
    CCharacter *model = nullptr;
    bool        up = false;
    int         fade = 0; // steps faded in, to kSnakeFadeSteps
    bool        appearing = false;
    int         left = 0;    // steps left of the appearance; the idle after it
    int         idle = 0;    // steps of the idle
    int         mouth = 0;   // steps into the mouth's cycle
    float       gape = 0.0f; // how far this cycle opens it (radians)
    bool        backing[kFloorMonsters] = {};
    float       upright[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };          // turns the neck's pose at the appearance's end to its rest
    float neck[3] = {};            // where the neck ends the appearance
    float jaw_axis[3] = {0, 0, 1}; // the jaw's hinge, in its parent's space
    float jaw_open = 0.0f;         // how far open the appearance leaves it (radians)
    float jaw_scale[3] = {1, 1, 1};
    float angles[3] = {}; // the model's turn (the last solved, the next guess)
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

// The appearance from its first frame, held on its last.
void Appear() {
    snake.model->motion_no = kSnakeAppear;
    snake.model->motion_flags |= kStopAtEnd | kFromFirstFrame;
    snake.model->motion_speed = -1.0f;
    snake.appearing = true;
    snake.left = Steps(kSnakeAppear);
    snake.idle = 0;
}

float Ease(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// How far the mouth's cycle has it open, 0..1: easing open, held, easing shut, then shut to the cycle's end.
float MouthCurve(int step) {
    if (step < kSnakeMouthEase) {
        return Ease(static_cast<float>(step) / kSnakeMouthEase);
    }
    step -= kSnakeMouthEase;
    if (step < kSnakeMouthHold) {
        return 1.0f;
    }
    step -= kSnakeMouthHold;
    return 1.0f - Ease(static_cast<float>(step) / kSnakeMouthEase);
}

constexpr int kMouthShutFrom = 2 * kSnakeMouthEase + kSnakeMouthHold;

// The appearance played through, then the idle: the mouth's cycle (the first shutting it from where the appearance left
// it), opened afresh when a monster comes near while it is shut.
void Idle(bool startled) {
    if (snake.appearing) {
        if (--snake.left > 0) {
            return;
        }
        snake.appearing = false;
        snake.mouth = kSnakeMouthEase + kSnakeMouthHold;
        snake.gape = snake.jaw_open;
        return;
    }
    snake.idle++;
    if (++snake.mouth >= kSnakeMouthPeriod || (startled && snake.mouth >= kMouthShutFrom)) {
        snake.mouth = 0;
        snake.gape = kSnakeMouthAngle;
    }
}

float MouthAngle() {
    return snake.appearing ? snake.jaw_open : snake.gape * MouthCurve(snake.mouth);
}

// The jaw turned angle about its hinge from shut (its rest turn), at its scale.
void SetJaw(CFrame *jaw, float angle) {
    const float *a = snake.jaw_axis;
    float        c = std::cos(angle), s = std::sin(angle), k = 1.0f - c;
    float        turn[3][3] = {
        {c + k * a[0] * a[0],        k * a[0] * a[1] + s * a[2], k * a[0] * a[2] - s * a[1]},
        {k * a[1] * a[0] - s * a[2], c + k * a[1] * a[1],        k * a[1] * a[2] + s * a[0]},
        {k * a[2] * a[0] + s * a[1], k * a[2] * a[1] - s * a[0], c + k * a[2] * a[2]       },
    };
    for (int r = 0; r < 3; r++) {
        for (int col = 0; col < 3; col++) {
            jaw->local[r][col] = snake.jaw_scale[r] * turn[r][col];
        }
    }
}

// The jaw's hinge and how far the appearance leaves it open, from its pose there (a turn from shut, its rest).
void ReadJaw(const CFrame &jaw) {
    float turn[3][3];
    for (int r = 0; r < 3; r++) {
        float length = std::sqrt(jaw.local[r][0] * jaw.local[r][0] + jaw.local[r][1] * jaw.local[r][1] + jaw.local[r][2] * jaw.local[r][2]);
        snake.jaw_scale[r] = length > 0.0f ? length : 1.0f;
        for (int c = 0; c < 3; c++) {
            turn[r][c] = jaw.local[r][c] / snake.jaw_scale[r];
        }
    }
    float axis[3] = {turn[1][2] - turn[2][1], turn[2][0] - turn[0][2], turn[0][1] - turn[1][0]};
    float twice_sin = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    float cos = std::clamp((turn[0][0] + turn[1][1] + turn[2][2] - 1.0f) * 0.5f, -1.0f, 1.0f);
    if (twice_sin < 1e-4f) {
        snake.jaw_open = 0.0f;
        return;
    }
    for (int c = 0; c < 3; c++) {
        snake.jaw_axis[c] = axis[c] / twice_sin;
    }
    snake.jaw_open = std::atan2(twice_sin * 0.5f, cos);
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
    // The idle's stretch along the facing and bob, from nothing as it starts.
    float idle = static_cast<float>(snake.idle);
    float back = kSnakeBack * kSnakeScale - kSnakeStretch * std::sin(kTau * idle / kSnakeStretchPeriod);
    float bob = kSnakeBob * std::sin(kTau * idle / kSnakeBobPeriod);
    float spot[3] = {above[0] - std::sin(yaw) * back, above[1] + kSnakeLift + bob, above[2] - std::cos(yaw) * back};
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
    Appear();
    for (int step = 0; step < snake.left + 2; step++) {
        snake.model->Step();
    }
    if (CFrame *jaw = FrameNamed(snake.model->frame, kSnakeJaw)) {
        ReadJaw(*jaw);
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
    snake.appearing = false;
    snake.left = 0;
}

// Everything but the model and what Calibrate read from it forgotten.
void Forget() {
    Snake kept = snake;
    snake = Snake{};
    snake.model = kept.model;
    std::copy(&kept.upright[0][0], &kept.upright[0][0] + 9, &snake.upright[0][0]);
    std::copy(kept.neck, kept.neck + 3, snake.neck);
    std::copy(kept.jaw_axis, kept.jaw_axis + 3, snake.jaw_axis);
    std::copy(kept.jaw_scale, kept.jaw_scale + 3, snake.jaw_scale);
    snake.jaw_open = kept.jaw_open;
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
    if (!SerpentSwordActive() || BtEventMode != 0) {
        Forget();
        return;
    }
    snake.up = Guarding();
    if (!snake.up) {
        snake.fade = std::max(0, snake.fade - 1);
        std::fill(std::begin(snake.backing), std::end(snake.backing), false);
        if (snake.fade == 0) {
            Forget();
            return;
        }
    }
    if (snake.model == nullptr) {
        if ((snake.model = EnterFloorCharacter(kKillerSnakeFile, kKillerSnakeCfg)) == nullptr) {
            return;
        }
        Calibrate();
    }
    bool startled = false;
    if (snake.up) {
        if (snake.fade == 0) {
            Appear();
        }
        snake.fade = std::min(kSnakeFadeSteps, snake.fade + 1);
        if (NowMonstorUnit != nullptr) {
            startled = MarkBacking(*NowMonstorUnit);
        }
    }
    Idle(startled);
    Place();
    snake.model->Step();
}

void SnakeCharmerPosed(CFrame *frame) {
    if (snake.model == nullptr || frame != snake.model->frame || snake.appearing || snake.fade == 0) {
        return;
    }
    if (CFrame *jaw = FrameNamed(frame, kSnakeJaw)) {
        SetJaw(jaw, MouthAngle());
    }
}

void SnakeCharmerDraw() {
    if (snake.fade > 0 && snake.model != nullptr && snake.model->frame != nullptr && BtEventMode == 0) {
        snake.model->ambient_offset[3] = kOpaque * SnakeCharmerShown();
        DrawFloorCharacter(*snake.model);
    }
}

void SnakeCharmerLight(const CCharacter &character, float *ambient) {
    if (snake.fade == 0 || (&character != snake.model && &character != NowWeapon)) {
        return;
    }
    for (int c = 0; c < 3; c++) {
        ambient[c] += kSnakeGlow * SnakeCharmerShown();
    }
}

bool SnakeCharmerUp() {
    return snake.up;
}

float SnakeCharmerShown() {
    return static_cast<float>(snake.fade) / kSnakeFadeSteps;
}

float SnakeCharmerMouth() {
    return snake.fade > 0 ? MouthAngle() / kSnakeMouthAngle : 0.0f;
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
