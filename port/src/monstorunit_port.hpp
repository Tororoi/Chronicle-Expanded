#pragma once

// Advances the step clock monster poison is timed by (monstorunit.cpp). DunMoveChara calls it each
// step CMonstorUnit::Step runs unpaused.
void MonstorStepClockAdvance();
