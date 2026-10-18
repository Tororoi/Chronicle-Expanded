#include "expanded/core/random.hpp"

#include <cstdlib>

namespace expanded {

std::mt19937 &ModRandom() {
    static std::mt19937 random{std::random_device{}()};
    return random;
}

void SeedModRandom(unsigned seed) {
    ModRandom().seed(seed);
}

int ModRoll(int n) {
    return std::uniform_int_distribution<int>(0, n - 1)(ModRandom());
}

bool ModChance(double chance) {
    return std::uniform_real_distribution<double>(0.0, 1.0)(ModRandom()) < chance;
}

float GameFraction() {
    return static_cast<float>(std::rand()) / 2147483648.0f;
}

} // namespace expanded
