#pragma once

#include "collisiondata.hpp"

namespace expanded {

// The collision table's hit records (CCollisionData::hit and ::active).
inline constexpr int kHitRecords = sizeof(CCollisionData::hit) / sizeof(CCollisionData::hit[0]);

// CCollisionData::Set's target_mask: the sides a hit strikes.
inline constexpr int kHitsPlayer = 1, kHitsMonsters = 2;

// Its kind: the reaction a hit gives, a swing's (a guard window blocks it).
inline constexpr int kSwingReaction = 2;

// COLLISION_HIT::knockback_mode for a hit that throws what it strikes away from its knockback_origin.
inline constexpr int kKnockbackFromOrigin = 2;

// Whether a hit record is free for CCollisionData::Set.
inline bool HitRecordFree(const CCollisionData &hits) {
    for (int i = 0; i < kHitRecords; i++) {
        if (hits.active[i] == 0) {
            return true;
        }
    }
    return false;
}

} // namespace expanded
