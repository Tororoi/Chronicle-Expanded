#include "expanded/town/ladder.hpp"

#include <algorithm>
#include <cmath>

#include "camerafollow.hpp"
#include "character.hpp"
#include "ebattle.hpp"
#include "edit.hpp"
#include "editloop.hpp"
#include "expanded/town/ally.hpp"
#include "itemdata.hpp"
#include "mathutil.hpp"
#include "snd.hpp"

// The allies' ladders, as Dark Cloud Expanded's TownLadder plays them: Xiao leaps, Goro hops up and jumps
// down, Ruby floats, Osmond flies up with his propeller and dives down, and Ungaga refuses. Distances are
// per frame, as an event script steps them.

namespace expanded {

namespace {

// Motion flag: play once and hold the last frame.
constexpr int kPlayOnce = 2;

// The built models' slots (expanded/docs/town-models.md).
constexpr int kIdle = 0;
constexpr int kWalk = 2;
constexpr int kFall = 8;
constexpr int kLand = 9;
constexpr int kRefusal = 7;

constexpr float kPi = 3.14159265f;

// The ladder end the player stands at and the one it goes to.
struct Ladder {
    float x, y, z, yaw; // the player
    float mount[3];     // the near end
    float target[3];    // the far end
    float height;       // rise to the far end, plus the landing margin
    float yaw_ladder;   // the ladder's facing
    float face;         // the player's facing on it
    int   align_frames; // the walk to the near end
    float release;      // the camera's angle at the end, from the player's facing
};

constexpr float kLandMargin = 1.0f;
constexpr float kWalkSpeed = 0.30f;
constexpr int   kAlignMin = 8;
constexpr int   kAlignMax = 60;

float Wrap(float angle) {
    while (angle > kPi) {
        angle -= 2.0f * kPi;
    }
    while (angle < -kPi) {
        angle += 2.0f * kPi;
    }
    return angle;
}

float Distance2(const float *end, float x, float z) {
    float dx = end[0] - x;
    float dz = end[2] - z;
    return dx * dx + dz * dz;
}

bool ReadLadder(const float position[3], float yaw, const ED_EVENT_PARAM &param, Ladder &ladder) {
    if (param.kind != ED_EVENT_POINT_LADDER_BOTTOM && param.kind != ED_EVENT_POINT_LADDER_TOP) {
        return false;
    }

    ladder.x = position[0];
    ladder.y = position[1];
    ladder.z = position[2];
    ladder.yaw = yaw;
    const float *a = param.position;
    const float *b = param.camera_pos;
    bool         a_far = Distance2(a, ladder.x, ladder.z) >= Distance2(b, ladder.x, ladder.z);
    std::copy_n(a_far ? a : b, 3, ladder.target);
    std::copy_n(a_far ? b : a, 3, ladder.mount);
    ladder.height = ladder.target[1] + kLandMargin - ladder.y;

    if (std::fabs(ladder.height) < 2.0f || std::fabs(ladder.height) > 500.0f) {
        return false;
    }

    ladder.yaw_ladder = param.rotation[1];
    ladder.face = ladder.height < 0 ? std::atan2(ladder.target[0] - ladder.mount[0], ladder.target[2] - ladder.mount[2])
                                    : ladder.yaw_ladder;
    float walk = std::sqrt(Distance2(ladder.mount, ladder.x, ladder.z));
    ladder.align_frames = std::clamp(static_cast<int>(walk / kWalkSpeed), kAlignMin, kAlignMax);
    ladder.release = Wrap(ladder.yaw_ladder + kPi - ladder.face);
    return true;
}

// Writes a sequence frame by frame.
class Builder {
public:
    TownLadder sequence;
    float      x = 0, y = 0, z = 0, yaw = 0;

    explicit Builder(const Ladder &ladder)
        : x(ladder.x), y(ladder.y), z(ladder.z), yaw(ladder.yaw) {}

    // The motion from the next frame on; the last one set before a frame wins.
    void Motion(int motion, float speed = -1.0f, int flags = kPlayOnce) {
        next_motion = motion;
        next_speed = speed;
        next_flags = flags;
    }

    void Sound(int sound) {
        next_sound = sound;
    }

    void Frame() {
        TownLadderFrame frame{x, y, z, yaw};
        if (next_motion >= 0 && (next_motion != motion || next_speed != speed || next_flags != flags)) {
            frame.motion = motion = next_motion;
            frame.speed = speed = next_speed;
            frame.flags = flags = next_flags;
        }
        frame.sound = next_sound;
        next_sound = -1;
        sequence.frames.push_back(frame);
    }

    void Wait(int frames) {
        for (int i = 0; i < frames; i++) {
            Frame();
        }
    }

    void Drift(int frames, float dx, float dy, float dz) {
        for (int i = 0; i < frames; i++) {
            x += dx;
            y += dy;
            z += dz;
            Frame();
        }
    }

    // Walks to the near end, turning to the ladder, and takes the ladder's camera.
    void Align(const Ladder &ladder) {
        sequence.camera = true;
        sequence.camera_yaw = ladder.yaw_ladder - kPi;
        sequence.release = ladder.release;
        int   frames = ladder.align_frames;
        float dx = (ladder.mount[0] - ladder.x) / frames;
        float dz = (ladder.mount[2] - ladder.z) / frames;
        float turn = Wrap(ladder.face - ladder.yaw) / frames;
        Motion(kWalk, -1.0f, 0);
        for (int i = 0; i < frames; i++) {
            x += dx;
            z += dz;
            yaw += turn;
            Frame();
        }
        yaw = ladder.face;
    }

    void Snap(const float *at) {
        x = at[0];
        y = at[1];
        z = at[2];
    }

private:
    int   motion = -1;
    float speed = -1.0f;
    int   flags = 0;
    int   next_motion = -1;
    float next_speed = -1.0f;
    int   next_flags = 0;
    int   next_sound = -1;
};

// A leap off or up the ladder: the walk there, an optional step back and wind-up, and a ballistic arc.
struct Jump {
    int   back_frames = 0;
    float back_dx = 0, back_dz = 0;
    int   back_motion = 13;
    int   ready2_motion = 0, ready2_frames = 0;
    float ready2_speed = -1;
    int   ready_motion = 11, ready_frames = 30, ready_flags = 0;
    float ready_speed = -1;
    int   launch_motion = kFall, launch_flags = kPlayOnce;
    float launch_speed = -1;
    int   launch_frames = 0; // >0: the air motion after this many frames, not at the top of the arc
    int   air_motion = kFall, air_flags = kPlayOnce;
    int   frames = 0;
    float vx = 0, vy = 0, vz = 0, gravity = 0.10f;
    // A landing played on the ground; without one the arc ends above it and the town lands the player.
    int   land_motion = 0, land_frames = 0, land_lead = 0, land_flags = kPlayOnce;
    float land_speed = -1;
};

void Leap(Builder &b, const Ladder &ladder, const Jump &jump) {
    b.Align(ladder);
    if (jump.back_frames > 0) {
        b.Motion(jump.back_motion, -1.0f, 0);
        b.Drift(jump.back_frames, jump.back_dx, 0.0f, jump.back_dz);
    }
    if (jump.ready2_frames > 0) {
        b.Motion(jump.ready2_motion, jump.ready2_speed);
        b.Wait(jump.ready2_frames);
    }
    if (jump.ready_frames > 0) {
        b.Motion(jump.ready_motion, jump.ready_speed, jump.ready_flags);
        b.Wait(jump.ready_frames);
    }
    b.Motion(jump.launch_motion, jump.launch_speed, jump.launch_flags);
    float vy = jump.vy;
    for (int left = jump.frames; left > 0; left--) {
        b.x += jump.vx;
        b.y += vy;
        b.z += jump.vz;
        vy -= jump.gravity;
        if (jump.launch_frames > 0 ? left <= jump.frames - jump.launch_frames : vy < 0.0f) {
            b.Motion(jump.air_motion, -1.0f, jump.air_flags);
        }
        if (jump.land_motion > 0 && left <= jump.land_lead) {
            b.Motion(jump.land_motion, jump.land_speed, jump.land_flags);
        }
        b.Frame();
    }
    if (jump.land_motion > 0) {
        b.Snap(ladder.target);
        b.Motion(jump.land_motion, jump.land_speed, jump.land_flags);
        b.Wait(jump.land_frames);
    }
}

// The arc's frames down to height drop below the start, from an upward speed vy.
int FallFrames(float vy, float gravity, float drop) {
    return std::clamp(static_cast<int>((vy + std::sqrt(vy * vy + 2.0f * gravity * drop)) / gravity), 20, 90);
}

// Xiao: the cat's leap, up after a step back and a crouch, or down off the edge.
void XiaoLadder(Builder &b, const Ladder &ladder) {
    constexpr float kGravity = 0.10f;
    constexpr float kBack = 6.0f;
    constexpr float kBackSpeed = 0.20f;
    constexpr float kApex = 8.0f;
    bool            up = ladder.height > 0;
    float           s = std::sin(ladder.face);
    float           c = std::cos(ladder.face);
    Jump            jump;
    float           start_x = ladder.mount[0];
    float           start_z = ladder.mount[2];
    if (up) {
        jump.back_frames = static_cast<int>(std::ceil(kBack / kBackSpeed));
        jump.back_dx = -s * kBackSpeed;
        jump.back_dz = -c * kBackSpeed;
        start_x -= s * kBack;
        start_z -= c * kBack;
        jump.vy = std::sqrt(2.0f * kGravity * (ladder.height + kApex));
        jump.frames = static_cast<int>(std::ceil(jump.vy / kGravity + std::sqrt(2.0f * kApex / kGravity)));
        jump.frames = std::clamp(jump.frames, 20, 90);
        jump.launch_motion = 12;
    } else {
        jump.vy = 1.5f;
        jump.frames = FallFrames(jump.vy, kGravity, -ladder.height);
    }
    jump.vx = (ladder.target[0] - start_x) / jump.frames;
    jump.vz = (ladder.target[2] - start_z) / jump.frames;
    Leap(b, ladder, jump);
}

// Osmond: down, a dive; up, the propeller unfolds, flies him up and over, and folds away.
void OsmondLadder(Builder &b, const Ladder &ladder) {
    constexpr float kGravity = 0.10f;
    constexpr float kFlight = 0.5f;
    if (ladder.height < 0) {
        Jump jump;
        jump.ready_frames = 0;
        jump.launch_motion = 11;
        jump.air_flags = 0;
        jump.vy = 1.5f;
        jump.frames = FallFrames(jump.vy, kGravity, -(ladder.target[1] - ladder.y));
        jump.vx = (ladder.target[0] - ladder.mount[0]) / jump.frames;
        jump.vz = (ladder.target[2] - ladder.mount[2]) / jump.frames;
        jump.land_motion = kLand;
        jump.land_speed = 0.30f;
        jump.land_lead = std::min(12, jump.frames / 3);
        jump.land_frames = std::max(50 - jump.land_lead, 8);
        Leap(b, ladder, jump);
        return;
    }

    constexpr float kHover = 4.0f;
    int             rise = std::clamp(static_cast<int>((ladder.height - kLandMargin + kHover) / 0.5f), 20, 400);
    float           dx = ladder.target[0] - ladder.mount[0];
    float           dz = ladder.target[2] - ladder.mount[2];
    int             forward = std::max(static_cast<int>(std::sqrt(dx * dx + dz * dz) / 0.30f), 8);
    // Whole turns of the flying loop, so the descent picks the spin up where the loop leaves it.
    forward += (10 - (rise + forward) % 10) % 10;
    int descend = std::max(static_cast<int>(kHover / 0.125f), 8);

    b.Align(ladder);
    b.Motion(12, kFlight);
    b.Wait(112);
    b.Motion(13, kFlight);
    b.Wait(30);
    b.Motion(14, kFlight, 0);
    b.Drift(rise, 0.0f, 0.5f, 0.0f);
    b.Drift(forward, dx / forward, 0.0f, dz / forward);
    b.Motion(15, kFlight);
    b.Drift(descend, 0.0f, -0.125f, 0.0f);
    b.Snap(ladder.target);
    b.Motion(16, kFlight);
    b.Wait(116);
}

// Goro: up, four hops up the ladder, the last one onto the ledge; down, a crouch and a jump.
void GoroLadder(Builder &b, const Ladder &ladder) {
    constexpr float kGravity = 0.15f;
    constexpr int   kHops = 4;
    constexpr int   kHopFrames = 31;
    constexpr int   kHopSound = 361;
    constexpr int   kCrouch = 9;
    constexpr int   kStand = 13;

    if (ladder.height < 0) {
        Jump jump;
        jump.ready2_motion = kStand;
        jump.ready2_speed = 0.40f;
        jump.ready2_frames = static_cast<int>(10 / 0.40f);
        jump.ready_motion = kCrouch;
        jump.ready_speed = 0.80f;
        jump.ready_frames = static_cast<int>(10 / 0.80f) + 2;
        jump.ready_flags = kPlayOnce;
        jump.launch_flags = 0;
        jump.air_flags = 0;
        jump.gravity = kGravity;
        jump.vy = 2.1f;
        jump.frames = FallFrames(jump.vy, kGravity, -(ladder.target[1] - ladder.y));
        jump.vx = (ladder.target[0] - ladder.mount[0]) / jump.frames;
        jump.vz = (ladder.target[2] - ladder.mount[2]) / jump.frames;
        jump.land_motion = kCrouch;
        jump.land_lead = std::min(12, jump.frames / 3);
        jump.land_frames = std::max(40 - jump.land_lead, 8);
        Leap(b, ladder, jump);
        return;
    }

    // Each hop crests late: the fewest frames whose arc ends falling at 0.5 a frame.
    float rung = (ladder.height - kLandMargin) / kHops;
    int   frames = 12;
    while (frames < kHopFrames && kGravity * (frames + 1) / 2.0f - rung / frames < 0.5f) {
        frames++;
    }
    float vy0 = rung / frames + kGravity * (frames - 1) / 2.0f;
    float kick = 20.0f / kHopFrames;

    auto arc = [&](int count, float dx, float dz, float &vy) {
        for (int i = 0; i < count; i++) {
            b.y += vy;
            vy -= kGravity;
            b.x += dx;
            b.z += dz;
            b.Frame();
        }
    };

    b.Align(ladder);
    for (int hop = 0; hop < kHops - 1; hop++) {
        b.Sound(kHopSound);
        b.Motion(hop % 2 == 0 ? 11 : 12, kick);
        float vy = vy0;
        arc(frames, 0.0f, 0.0f, vy);
        b.Wait(kHopFrames - frames);
    }

    // The last hop rises well over the ledge and drops onto it, the crouch leading the touchdown.
    float last = (ladder.target[1] - ladder.y) / kHops;
    float vy = std::sqrt(2.0f * kGravity * (last + 15.0f));
    int   count = 0;
    for (float rise = 0.0f, speed = vy; (speed > 0.0f || rise > last) && count < 90; count++) {
        rise += speed;
        speed -= kGravity;
    }
    int   lead = std::min(8, count);
    float dx = (ladder.target[0] - ladder.mount[0]) / count;
    float dz = (ladder.target[2] - ladder.mount[2]) / count;
    b.Sound(kHopSound);
    b.Motion((kHops - 1) % 2 == 0 ? 11 : 12, std::min(1.0f, 20.0f / std::max(count - lead, 1)));
    arc(count - lead, dx, dz, vy);
    if (lead > 0) {
        b.Motion(kCrouch);
        arc(lead, dx, dz, vy);
    }
    b.Snap(ladder.target);
    b.Motion(kCrouch);
    b.Wait(50);
}

// Ruby: she floats up the ladder and over the ledge, or out over the edge and down.
void RubyLadder(Builder &b, const Ladder &ladder) {
    constexpr float kRise = 0.35f;
    constexpr float kSettle = 0.15f;
    constexpr float kHover = 3.0f;
    constexpr int   kEntry = 24;
    constexpr float kEntryLift = 0.08f;
    constexpr int   kRamp = 8;
    float           dx = ladder.target[0] - ladder.mount[0];
    float           dz = ladder.target[2] - ladder.mount[2];
    int             forward = std::max(static_cast<int>(std::sqrt(dx * dx + dz * dz) / 0.30f), 8);
    int             settle = std::max(static_cast<int>(kHover / kSettle), 8);

    b.Align(ladder);
    b.Motion(kFall, 0.03f, 0);
    if (ladder.height > 0) {
        float before = kEntry * kSettle + kRamp * 0.25f;
        int   rise = std::max(static_cast<int>((ladder.height - kLandMargin + kHover - before) / kRise), 4);
        b.Drift(kEntry, 0.0f, kSettle, 0.0f);
        b.Motion(kFall, -1.0f, 0);
        b.Drift(kRamp, 0.0f, 0.25f, 0.0f);
        b.Drift(rise, 0.0f, kRise, 0.0f);
        b.Drift(forward, dx / forward, 0.0f, dz / forward);
        b.Drift(settle, 0.0f, -kSettle, 0.0f);
    } else {
        float drop = ladder.y + kEntry * kEntryLift - (ladder.target[1] + kHover);
        int   glide = std::max(static_cast<int>(drop / kRise), 4);
        b.Drift(kEntry, 0.0f, kEntryLift, 0.0f);
        b.Motion(kFall, -1.0f, 0);
        b.Drift(forward, dx / forward, 0.0f, dz / forward);
        b.Drift(glide, 0.0f, -kRise, 0.0f);
        b.Drift(settle, 0.0f, -kSettle, 0.0f);
    }
    b.Snap(ladder.target);
    b.Motion(kIdle, -1.0f, 0);
    b.Wait(20);
}

// The ladder refusal in slot 7, played where the player stands.
void Refusal(Builder &b, int ally) {
    int frames = ally == CHARA_XIAO ? 175 : ally == CHARA_GORO ? 65
                                        : ally == CHARA_RUBY   ? 105
                                                               : 190;
    b.Motion(kRefusal);
    b.Wait(frames);
    b.Motion(kIdle, -1.0f, 0);
    b.Frame();
}

// The sequence playing, and its next frame.
TownLadder playing;
size_t     next_frame;

} // namespace

TownLadder TownLadderSequence(int ally, const float position[3], float yaw, const ED_EVENT_PARAM &param) {
    Ladder ladder{};
    bool   read = ReadLadder(position, yaw, param, ladder);
    if (!read) {
        ladder.x = position[0];
        ladder.y = position[1];
        ladder.z = position[2];
        ladder.yaw = yaw;
    }

    Builder b(ladder);
    if (ally == CHARA_XIAO && read) {
        XiaoLadder(b, ladder);
    } else if (ally == CHARA_GORO && read) {
        GoroLadder(b, ladder);
    } else if (ally == CHARA_RUBY && read) {
        RubyLadder(b, ladder);
    } else if (ally == CHARA_OSMOND && read) {
        OsmondLadder(b, ladder);
    } else if (ally >= CHARA_XIAO && ally <= CHARA_UNGAGA) {
        Refusal(b, ally);
    }
    return b.sequence;
}

bool TownAllyTakesLadder(const ED_EVENT_PARAM *param) {
    int ally = TownAllyLoaded();
    if (ally <= 0) {
        return false;
    }

    // A disc body has neither the moves nor the refusal.
    if (TownAllyPack(ally).first != kTownAllyBodies[ally].model) {
        return true;
    }

    sceVu0FVECTOR position;
    sceVu0FVECTOR rotation;
    Chara->GetPosition(position);
    Chara->GetRotation(rotation);
    playing = TownLadderSequence(ally, position, rotation[1], *param);
    next_frame = 0;
    return true;
}

bool TownAllyLadderStep() {
    if (playing.frames.empty()) {
        return false;
    }

    auto          *chara = static_cast<CMainChara *>(EdMoveCharaInfo.chara);
    CCameraFollow *camera = EdMoveCharaInfo.interior == 0 && playing.camera ? EdMoveCharaInfo.camera : nullptr;
    sceVu0FVECTOR  position;
    sceVu0FVECTOR  rotation;

    if (next_frame == playing.frames.size()) {
        // Hands the camera back behind the player, as an event's _RESET_CAMERA_ANGLE does.
        if (camera != nullptr) {
            chara->GetPosition(position);
            chara->GetRotation(rotation);
            camera->FollowOn();
            camera->SetAngleSoon(AngleLimit(rotation[1] + playing.release));
            camera->SetFollow(position[0], position[1] + chara->body_height - 3.0f, position[2]);
            EdInitCameraParam(camera);
            camera->Step(-1);
        }
        TownAllyLadderCancel();
        return false;
    }

    const TownLadderFrame &frame = playing.frames[next_frame];
    if (next_frame == 0 && camera != nullptr) {
        // Toan's climb camera: following at the ladder's angle.
        camera->FollowOn();
        camera->SetFollow(frame.x, frame.y + 0.8f * chara->body_height, frame.z);
        camera->SetHeight(30.0f);
        camera->SetDistance(50.0f);
        camera->SetAngle(playing.camera_yaw);
        camera->SetAngleSoon(playing.camera_yaw);
        camera->SetSpeed(8.0f);
    }
    next_frame++;

    position[0] = frame.x;
    position[1] = frame.y;
    position[2] = frame.z;
    position[3] = 1.0f;
    rotation[0] = 0.0f;
    rotation[1] = frame.yaw;
    rotation[2] = 0.0f;
    rotation[3] = 0.0f;
    chara->SetPosition(position);
    chara->SetRotation(rotation);
    chara->SetVelocity(CVector3_f_(0.0f, 0.0f, 0.0f));
    if (frame.motion >= 0) {
        chara->SetMotion(frame.motion, frame.flags);
        if (frame.speed > 0.0f) {
            chara->SetMotionSpeed(frame.speed);
        }
    }
    if (frame.sound >= 0) {
        SndSePlay(frame.sound, -1, 0);
    }
    if (camera != nullptr) {
        camera->SetFollow(frame.x, frame.y + 0.8f * chara->body_height, frame.z);
    }
    chara->Step();
    chara->ShadowStep();
    chara->ClothStep(0);
    EdMoveCharaInfo.event_ready = false;
    EdMoveCharaInfo.acted = false;
    return true;
}

void TownAllyLadderCancel() {
    playing = {};
    next_frame = 0;
}

} // namespace expanded
