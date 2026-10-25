#pragma once

class CCharacter;
class CCloth;

namespace expanded {

// The cape the Super Steve spirit cat wears (and its domino mask), as the mod has them: the cloth the cat's pack builds
// (CLOTH catcape.clo) on its cat_cape frame, taken off the cat's own cloth list so only a caped cat steps and draws it.
// Its ties, collision capsules and wind are sized to the cat; each frame its rest shape is blown back off the cat's back
// (kSpiritCapeLift at the hem, eased along the hang, a swell running collar to hem every kSpiritCapeRippleFrames) and its
// springs turned with the cat; torn (its hem kSpiritCapeTearSpan times the cat's size from its collar) or carried
// across the room, it is put back on its rest shape. The cape and mask take the selected element's colour, drawn under
// its own ambient.
inline constexpr float kSpiritCapeLift = 2.6f;
inline constexpr int   kSpiritCapeRippleFrames = 30;
inline constexpr float kSpiritCapeTearSpan = 20.0f;

// An element's (0 Fire to 4 Holy, 5 none) cape colour and the ambient the cape and mask draw under.
struct SpiritCapeLook {
    unsigned char rgb[3];
    float         tint[3];
};

inline constexpr SpiritCapeLook kSpiritCapeLooks[6] = {
    {{128, 15, 0},   {80.0f, 20.0f, 10.0f}},
    {{9, 45, 104},   {12.0f, 40.0f, 48.0f}},
    {{180, 148, 0},  {34.0f, 31.0f, 6.0f} },
    {{30, 100, 15},  {8.0f, 46.0f, 37.0f} },
    {{193, 79, 160}, {32.0f, 11.0f, 66.0f}},
    {{0, 0, 0},      {0.0f, 0.0f, 0.0f}   },
};

// The cape taken from the cat just loaded, and its mask frame left out of the cat's own draw; false when the pack has
// none.
bool SpiritCapeTake(CCharacter &cat);
// The cape for tests, or null.
CCloth *SpiritCape();
// Sized to a cat scale times its own.
void SpiritCapeSize(float scale);
// Every particle put on its rest shape where the cat now is.
void SpiritCapeReseed();
// A frame of the caped cat (facing along yaw, its floor at floor_y): torn → reseated, else springs turned, wind blown and
// the cloth stepped.
void SpiritCapeStep(float yaw, float floor_y, float scale);
// The cape and mask drawn in the element's colour after the cat, lit (0..1) as it is.
void SpiritCapeDraw(const CCharacter &cat, int element, float lit);
// The cat taken down (the floor ended).
void SpiritCapeReset();

} // namespace expanded
