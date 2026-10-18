#pragma once

#include <random>

namespace expanded {

// Expanded's own rolls, on a generator of their own so they leave the game's rand() sequence alone: seeded from the
// device until a test seeds it.
std::mt19937 &ModRandom();
void          SeedModRandom(unsigned seed);

// A whole number in [0, n) and a chance's success, on ModRandom.
int  ModRoll(int n);
bool ModChance(double chance);

// The game's rand() (retail's 31-bit generator, whatever the host's RAND_MAX) over 2^31 as a float, as the game's
// own rolls take it, for rolls made where the game would make its own.
float GameFraction();

} // namespace expanded
