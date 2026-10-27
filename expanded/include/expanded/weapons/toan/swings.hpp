#pragma once

namespace expanded {

// Toan's combo swings (kToanFirstSwing to kToanLastSwing) as his motions play them: each clip's frames and the frame it
// comes forward on.
struct ToanSwing {
    float start, end, hit;
};

inline constexpr ToanSwing kToanCombo[] = {
    {820.0f, 830.0f, 826.0f},
    {830.0f, 838.0f, 835.0f},
    {838.0f, 847.0f, 844.0f},
    {847.0f, 857.0f, 853.0f},
    {856.0f, 884.0f, 867.0f},
};

// The opening swing's trail: the frame its blade starts through the air.
inline constexpr float kToanFirstSwingTrail = 824.0f;

// Toan attacking now: a combo swing, the lunge or the whirlwind.
bool ToanAttacking();

// A combo swing under way as action.
bool ToanComboSwing(int action);

// The swing lets a hanging blade go: the opening swing from its trail, a lunge or whirlwind at once.
bool ToanSwingDrops();

// The swing has come forward: a lunge or whirlwind at once, a combo swing at its own frame.
bool ToanSwingForward();

// A combo swing winding up as action: how far it is from its start to coming forward (0..1).
float ToanSwingWindUp(int action);

} // namespace expanded
