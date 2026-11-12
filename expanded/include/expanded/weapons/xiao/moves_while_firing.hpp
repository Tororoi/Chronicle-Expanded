#pragma once

namespace expanded {

// Moving while firing: Super Steve carrying the SynthSphere of one of Osmond's weapons (the gun line, by the weapon table's
// owner) lets Xiao walk while she draws, holds and lets go a shot, at the speed she walks without one and still facing
// the way she aims, as Osmond walks while he fires; the game stands her still for them. A feature that gives one of
// those spheres a shot of its own (holding the shot for another weapon's fire, say) keeps her moving by playing it as
// her slingshot action (BtActStatus.action_on 1): the gate asks only that.

// Whether a weapon (item number) is one of Osmond's.
bool OsmondWeapon(int weapon_no);

// The way one of Osmond's guns fires (BtActStatus.gun_type), as Get_Main_EffectPtr sets it from his equipped weapon: the
// machine guns' rapid fire (every gun it does not name: the Machine Gun and the Swallow, and the Jackal, G Crusher and
// Star Breaker it names), the launchers' charged burst (Hexa Blaster, Supernova, Snail) and the flamethrowers' flame
// (Blessing Gun, Skunk). -1 for a weapon that is not his.
inline constexpr int kMachineGun = 0, kLauncher = 1, kFlamethrower = 2;
int                  OsmondGunType(int weapon_no);

// Whether Super Steve's sphere in the active character's hand is an Osmond weapon's (SuperSteveSphere).
bool MovesWhileFiring();

// Hook: MoveChara, the player's step (beside Osmond's own): whether Xiao's slingshot action in play leaves her stick's move
// to her.
bool FiringLeavesMove();

} // namespace expanded
