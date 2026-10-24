#pragma once

namespace expanded {

// Heaven's Cloud "Typhoon": holding the charge past the whirlwind's threshold grows the sword in Toan's hand, from its
// own size to kTyphoonScale times it over kTyphoonFrames, slowly at first then faster (the time's fraction raised to
// kTyphoonCurve), with the charge-complete flash once it is full grown. The whirlwind it lets go is as much bigger:
// its tornado drawn and hitting that much wider (kTyphoonReach times the size), the sword's own hits riding out with
// the grown blade (planted along it too, blade_scale.hpp), and as it starts every monster within that reach takes the
// whirlwind's hit (kTyphoonDamage times the attack, in the sword's element, through any guard) and is thrown out to
// kTyphoonClear past it. The sword is its own size again once the whirlwind is over. Its Quick Draw
// (small_sword.hpp) and Moonlit Focus (tsukikage.hpp) live with those weapons; Super Steve's sphere is
// heavens_cloud_shot.hpp. Times are game frames.
inline constexpr float kTyphoonScale = 3.0f;
inline constexpr int   kTyphoonFrames = 240;
inline constexpr float kTyphoonCurve = 2.5f;
inline constexpr float kTyphoonReach = 20.0f; // the tornado's own hit radius, at the sword's own size
inline constexpr float kTyphoonDamage = 1.5f; // the whirlwind's, as ToanKey_Play sets the tornado's
inline constexpr float kTyphoonClear = 10.0f;

// The size the charge has grown the sword to (1 its own).
float TyphoonScale();

// Hooks: a dungeon step (the sword grows through the charge, and back after the whirlwind); ToanKey_Play planted this
// swing's hits at hit_pos; the whirlwind's tornado was set going (it is grown with the sword).
void TyphoonStep();
void TyphoonHits(const float *hit_pos);
void TyphoonWhirlwind();

} // namespace expanded
