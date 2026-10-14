#include "expanded/ungaga.hpp"

#include "collisiondata.hpp"

namespace expanded {

// His shots record a scale rate; his swings record none.
bool ChargeShotHit(int attacker, const COLLISION_HIT *record) {
    return attacker == 4 && record->scale_rate != 0.0f;
}

} // namespace expanded
