#pragma once

class CCharacter;

namespace expanded {

// Partisan "Quick Combo": with the Partisan in Ungaga's hand his three combo swings play kSwiftStrikesFactor times as
// fast; his charge keeps its pace. Swing 2 hands over to swing 3 only while its cursor is within half a frame of its
// end, so its step (0.32, 0.43 raised) stays under 0.5 to land there. Super Steve carrying its SynthSphere has Swift
// Strikes' draw and shot (SwiftStrikesSphere).
bool QuickComboWielded();

// Hook: motionDrive, before the player character steps: the speed its motion plays at.
void QuickComboMotion(CCharacter &chara, int chara_no);

} // namespace expanded
