#pragma once

#include "expanded/weapons/pellets.hpp"

namespace dc::game {
class CCharacter;
class CSHOT;
struct BT_SHOT_EFFECT;
struct CSHOT_EFFECT;
} // namespace dc::game

namespace expanded {

// Super Steve's pellets flying as a model (a SynthSphere's look): each pellet Xiao fires while active() carries a shot of
// shot's effect (a model thrown as the Bomb Gemron's bomb is, the apple shot with another mesh: loaded with the floor
// while the sphere is with the party, RegisterWeaponShotEffect) at scale, its ambient raised by tint (red, green, blue;
// none with null). With Turn::Ride it is held still with its +y turned back along the pellet's flight, toward Xiao;
// with Turn::Spin it faces along its flight (its +z) and the apple's flight motion spins it about its x, tumbling it
// forward as Witch Illza's apple is. The pellet's sprite is left out
// (its size 0); its hit is as ever. A pellet fired with every one of the effect's slots in the air flies as a
// plain pellet.
class ModelPellets {
public:
    enum class Turn {
        Ride,
        Spin
    };

    ModelPellets(BT_SHOT_EFFECT *shot, bool (*active)(), float scale, const float *tint = nullptr, Turn turn = Turn::Ride)
        : shot_(shot), active_(active), scale_(scale), tint_(tint), turn_(turn) {}

    // One step (once a frame the monsters step unpaused): the models ride their pellets.
    void Step();
    // The floor's monsters put back, or a floor begun.
    void Reset();
    // Xiao fired a pellet into slot of shots; a pellet ended.
    void Fired(CSHOT &shots, int slot);
    void Ended(const CSHOT &shots, int slot);
    // A character's light as it is drawn: a model's tint.
    void Light(const CCharacter &character, float *ambient) const;

private:
    CSHOT_EFFECT *Models();
    CSHOT_EFFECT *Loaded() const;
    void          Hide(int model);
    void          Ride(int model, const CSHOT &shots, int slot);

    BT_SHOT_EFFECT *shot_;
    bool (*active_)();
    float            scale_;
    const float     *tint_;
    Turn             turn_;
    int              effect_ = -1; // the models' effect, as last asked
    PelletSlots<int> models_;      // each pellet's model
};

} // namespace expanded
