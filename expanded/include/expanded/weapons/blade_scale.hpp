#pragma once

namespace expanded {

// A sword in Toan's hand grown scale times its size carries its swing's hit point (dcol1) out with it, past what the
// stock blade would reach. Each swing hit planted at hit_pos is planted again along the blade at the stock blade's
// hit point and every whole multiple of it short of the tip, so the grown blade still hits all the stock one covers.
void PlantAlongBlade(const float *hit_pos, float scale);

// The sword in Toan's hand (MainWeapon) at scale times its size.
void ScaleSwordInHand(float scale);

} // namespace expanded
