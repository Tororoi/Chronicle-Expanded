#pragma once

class CFrame;
class CMonstorUnit;

namespace expanded {

// _SET_MOTION's flags: loop, hold the last frame, cut in without the cross-fade.
inline constexpr int kMotionLoop = 0;
inline constexpr int kMotionHoldLast = 2;
inline constexpr int kMotionCut = 4;

// _SET_MOTION on a monster and its attachments, at the motion's own speed.
void PlayMonsterMotion(CMonstorUnit &unit, int monster, int motion, int flags);

// Fades a monster out by step of its 128 alpha a frame, and takes it off the floor once faded (true then, or when
// it already was).
void FadeMonster(CMonstorUnit &unit, int monster, float step);
bool RemoveFadedMonster(CMonstorUnit &unit, int monster);

// Takes a monster off the floor at once.
void RemoveMonster(CMonstorUnit &unit, int monster);

// A frame of a monster's body by name, or null (also while it has no body).
CFrame *MonsterFrame(CMonstorUnit &unit, int monster, const char *name);

} // namespace expanded
