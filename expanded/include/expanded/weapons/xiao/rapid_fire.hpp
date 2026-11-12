#pragma once

namespace dc::game {
class CCharacter;
class CSHOT;
} // namespace dc::game

namespace expanded {

// Rapid fire: Super Steve carrying the Machine Gun's SynthSphere fires Xiao's pellets one after another while she holds X.
// Her first draw is her own; drawn with X still held, she holds it kRapidHold frames and lets go. kRapidFollowThrough
// frames after each pellet her shot cross-fades back into the draw (the game's own fade between motions, in
// 1 / kRapidBlendStep steps) landing kRapidPullFrom frames into it, pulls back from there at kRapidDrawSpeed (her draw's
// own pace) to land exactly on full draw (kRapidPullTo frames into it: frame 250, the held pose, so the draw never steps
// past its end), holds it kRapidHold frames and lets go again: a pellet every 18 frames, as long as X stays held. The frames count from the draw's own first frame, so first
// person (the hand model's draw, 10 to 21) pulls the same. Each pellet of the stream does the shot's full damage and wears her slingshot kRapidFireWear times what a shot does, as his gun's bullets wear his gun. Let go during
// a pull-back, the stream ends there; let go during a shot, that shot finishes. A tap fires her ordinary shot. She walks
// while she fires (moves_while_firing.hpp).
inline constexpr int   kRapidFollowThrough = 0;
inline constexpr float kRapidBlendStep = 0.2f;
inline constexpr int   kRapidPullFrom = 4;
inline constexpr int   kRapidPullTo = 10;
inline constexpr float kRapidDrawSpeed = 0.7f;
inline constexpr int   kRapidHold = 1;
inline constexpr float kRapidFireWear = 0.5f;

bool RapidFireSphere();

// Whether a stream is going, and the pellets it has fired (for tests).
bool RapidFiring();
int  RapidFireShots();

// Hook: BattleActionPlay_Jinn, before it reads the motion time (XiaoShotPlaying): the hold goes on into the shot, a
// pellet's follow-through fades back into the draw, and the draw pulls to full draw and lets go. True when X was let go
// during the draw and her shot ends.
bool RapidFireShot(CCharacter &chara);

// Hook: a pellet fired into slot (PelletFired): one of the stream's.
void RapidFirePellet(CSHOT &shots, int slot);

// Hook: a shot's wear (XiaoShotWear).
float RapidFireWear(float wear);

// Hook: the top of a dungeon frame (DungeonFrameBegins): a stream whose shot has ended is over.
void RapidFireFrame();

} // namespace expanded
