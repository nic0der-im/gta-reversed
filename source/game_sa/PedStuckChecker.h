#pragma once

#include "Vector.h"

class CPed;
class CEventGroup;

enum ePedStuckState : uint16 {
    PED_STUCK_STATE_NONE      = 0, // The ped isn't stuck
    PED_STUCK_STATE_STUCK     = 1, // The ped is stuck (the check was postponed to one of the next frames)
    PED_STUCK_STATE_WAS_STUCK = 2  // The ped was stuck, but has just been recovered from it
};

/*!
 * Detects (and recovers) peds that are stuck in the air, or inside the ground.
 * @addr 0x602C00 - `TestPedStuck`
 */
class CPedStuckChecker {
public:
    CVector        m_lastNonStuckPoint; // 0x00 - Position of the ped when the counter below was last (re)started
    uint16         m_radius;            // 0x0C - Counter of `TestPedStuck` calls since `m_lastNonStuckPoint` was updated (saturates at 10'000)
                                        //        NOTE: Misnamed - It's not a radius, but it's used as such by `CTaskComplexStuckInAir`
    ePedStuckState m_state;             // 0x0E - See `ePedStuckState`

public:
    CPedStuckChecker() { // inlined 0x607292
        m_radius = 0;
        m_state  = PED_STUCK_STATE_NONE;
    }

    // 0x602C00
    bool TestPedStuck(CPed* ped, CEventGroup* eventGroup);

    auto GetState() const { return m_state; }

private:
    // 0x602E40
    //! Checks whether the ped (which hasn't moved for a while) is standing on something, and if it isn't, puts it on the ground below it.
    //! @returns Whether the counters (`m_radius`, `m_state`) should be reset
    bool CheckPedOnGround(CPed* ped);

    // 0x603048
    //! Probes the ground below both directions perpendicular to `displacement` and moves the ped onto the highest one found.
    //! Sets `m_state` to `PED_STUCK_STATE_WAS_STUCK`.
    void RecoverFromStuck(CPed* ped, const CVector& displacement);
};

VALIDATE_SIZE(CPedStuckChecker, 0x10);
