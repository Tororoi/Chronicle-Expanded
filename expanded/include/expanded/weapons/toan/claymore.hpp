#pragma once

namespace expanded {

// Claymore "Greatsword": in Toan's hand the sword is kGreatswordScale times its size. Its swing's hit point (dcol1)
// grows out with it, from 10.4 units to 18.7, so the swing reaches that much farther. So that it still hits what the
// stock sword would, each swing hit is planted again where the stock blade's hit point would be. Every hit throws the
// monster back, as the Baselard's do (baselard.hpp); Super Steve carrying its SynthSphere has the throw alone.
inline constexpr float kGreatswordScale = 1.8f;

// Whether Toan has the Claymore in hand.
bool GreatswordActive();

// Hook: a dungeon step: the sword in hand at the Claymore's size, or its own.
void GreatswordStep();

// Hook: ToanKey_Play planted this swing's hits at hit_pos: each is planted again at the stock hit point.
void GreatswordHits(const float *hit_pos);

} // namespace expanded
