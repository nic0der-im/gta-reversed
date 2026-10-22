#include "StdInc.h"

#include "PedStuckChecker.h"

#include "ColPoint.h"
#include "EventStuckInAir.h"

/*!
 * @addr 0x602C00
 *
 * Detects peds that are stuck (in the air or embedded into the ground) and recovers them.
 *
 * For peds that haven't been standing for a while two checks are done:
 *  - If the ped stayed (nearly) in the same place, a 2 unit tall vertical line is cast at its
 *    position; if the ped is floating above whatever was hit, it's put on top of it.
 *  - Otherwise (only if the ped collided with a single building) the ground below both
 *    directions perpendicular to the displacement of the ped is probed, and the ped is moved
 *    on top of the highest one found, then pushed (and turned) towards it.
 * In both cases the ped is marked as standing (`bIsStanding = true`) and the counters are
 * reset - unless the check was postponed, or the ped was recovered, in which case `m_state`
 * is set, which ends up raising a `CEventStuckInAir` (See the bottom of this function).
 */
bool CPedStuckChecker::TestPedStuck(CPed* ped, CEventGroup* eventGroup) {
    // Peds that can't get stuck aren't tested at all
    if (!ped->m_bUsesCollision
        || ped->m_pAttachedTo
        || ped->m_nPedState == PEDSTATE_DIE
        || ped->m_nPedState == PEDSTATE_DEAD
    ) {
        m_radius = 0;
        m_state  = PED_STUCK_STATE_NONE;
        return false;
    }

    // Only peds that are in the air and have been hit (or the player, if its entity is stuck)
    const bool bShouldCheck = !ped->bIsStanding
        && !ped->bWasStanding
        && ((ped->m_pDamageEntity && ped->m_fDamageIntensity > 0.0f)
            || (ped->IsPlayer() && ped->m_bIsStuck));

    bool bClearCounters = true; // Whether the counters should be reset at the end (only some paths keep them)

    if (bShouldCheck) {
        auto* const intel = ped->GetIntelligence();
        if (!intel->GetTaskSwim() && !intel->GetTaskJetPack() && !intel->GetTaskClimb()) {
            const uint16 counter = m_radius;
            CVector      delta{};

            if (counter > 10'000 || counter == 0) {
                m_radius            = 1;
                m_state             = PED_STUCK_STATE_NONE;
                m_lastNonStuckPoint = ped->GetPosition();
            } else {
                delta    = ped->GetPosition() - m_lastNonStuckPoint;
                m_radius = counter + 1;
            }

            const float timeStep = std::max(0.01f, CTimer::GetTimeStep());
            const float counterF = static_cast<float>(counter);

            // Did the ped stay in (nearly) the same place?
            if (counterF > 50.0f / (4.0f * timeStep) && counterF * 0.01f > delta.SquaredMagnitude()) {
                bClearCounters = CheckPedOnGround(ped);
            } else {
                // The ped was moved, so it may have been pushed into a building
                bClearCounters = false;

                const bool bStayedInPlace = counterF > 50.0f / (2.0f * timeStep)
                                         && std::abs(delta.z) < counterF * 0.004f;
                if (ped->m_nNumEntitiesCollided > 1) {
                    if (bStayedInPlace) {
                        bClearCounters = CheckPedOnGround(ped);
                    }
                } else if (ped->m_nNumEntitiesCollided == 1
                    && ped->m_apCollidedEntities[0]
                    && ped->m_apCollidedEntities[0]->GetType() == ENTITY_TYPE_BUILDING
                    && bStayedInPlace
                ) {
                    RecoverFromStuck(ped, delta);
                }
            }
        }
    }

    if (bClearCounters) {
        m_radius = 0;
        m_state  = PED_STUCK_STATE_NONE;
    }

    // 0x602F76
    if (m_state == PED_STUCK_STATE_NONE) {
        return false;
    }

    // Note: `GetEventOfType` is called twice in the original code
    const auto* const inWaterEvent = static_cast<const CEventInWater*>(eventGroup->GetEventOfType(EVENT_IN_WATER));
    if (!inWaterEvent || inWaterEvent->m_acceleration <= 1.0f) {
        CEventStuckInAir event{ ped };
        eventGroup->Add(&event, false);
        return true;
    }

    // Not stuck, the ped is just being pushed along by a water current
    ped->bIsStanding = false;

    return false;
}

// 0x602E40
bool CPedStuckChecker::CheckPedOnGround(CPed* ped) {
    // The (expensive) ground checks are done staggered, so only an 8th of the peds is checked each frame
    if (((CTimer::GetFrameCounter() + (ped->m_nRandomSeed & 0xFFu) + 3u) & 7u) != 0) {
        m_state = PED_STUCK_STATE_STUCK;
        return false;
    }

    const CVector pos = ped->GetPosition();

    CVector probe = pos;
    probe.z += 1.0f;

    CColPoint colPoint;
    CEntity*  hitEntity;
    if (!CWorld::ProcessVerticalLine(probe, pos.z - 1.0f, colPoint, hitEntity, true, true, false, true)) {
        return false;
    }

    // The ped isn't floating above the ground, or it was just knocked down by another ped
    const bool bOnGround = ped->bHeadStuckInCollision && colPoint.m_vecPoint.z + 1.0f >= pos.z;
    const bool bKnockedDownByPed = ped->m_fDamageIntensity > 0.0f
        && ped->m_pDamageEntity
        && ped->m_pDamageEntity->GetType() == ENTITY_TYPE_PED
        && ped->m_vecLastCollisionImpactVelocity.z > 0.3f;
    if (!bOnGround && !bKnockedDownByPed) {
        // Put the ped right on top of the ground below it
        probe.z = colPoint.m_vecPoint.z + 1.0f;
        ped->SetPosn(probe);

        if (ped->bHeadStuckInCollision) {
            ped->bHeadStuckInCollision = false;
        }
    }

    ped->bIsStanding = true;

    return true;
}

// 0x603048
void CPedStuckChecker::RecoverFromStuck(CPed* ped, const CVector& displacement) {
    // Direction perpendicular to the displacement, pointing slightly upwards
    CVector dir{ -displacement.y, displacement.x, 1.0f };
    dir.Normalise();

    const CVector pos = ped->GetPosition();

    // Probe the ground below the forward/backward directions
    CColPoint     colPointFwd{};
    CEntity*      hitEntityFwd;
    const CVector probeFwd  = pos + dir;
    float         groundFwd = 5001.0f; // Sentinels used when the probe couldn't find the ground
    if (CWorld::ProcessVerticalLine(probeFwd, -20.0f, colPointFwd, hitEntityFwd, true)
        && CWorld::GetIsLineOfSightClear(pos, probeFwd, true, true, false, true)
    ) {
        groundFwd = colPointFwd.m_vecPoint.z;
    }

    CColPoint     colPointBwd{};
    CEntity*      hitEntityBwd;
    const CVector probeBwd  = pos - dir;
    float         groundBwd = 5002.0f;
    if (CWorld::ProcessVerticalLine(probeBwd, -20.0f, colPointBwd, hitEntityBwd, true)
        && CWorld::GetIsLineOfSightClear(pos, probeBwd, true, true, false, true)
    ) {
        groundBwd = colPointBwd.m_vecPoint.z;
    }

    int32 probeResult;
    if (groundFwd > 5000.0f && groundBwd > 5000.0f) {
        probeResult = -1; // Neither probe found the ground
    } else if (pos.z - 1.0f < groundFwd && groundFwd < 5000.0f
            && (groundBwd < 5001.0f || groundFwd < groundBwd)) {
        probeResult = 1;
    } else if (pos.z - 1.0f < groundBwd && groundBwd < 5001.0f) {
        probeResult = 2;
    } else {
        probeResult = 0;
    }

    const bool bKnockedDownByPed = ped->m_fDamageIntensity > 0.0f
        && ped->m_pDamageEntity
        && ped->m_pDamageEntity->GetType() == ENTITY_TYPE_PED
        && ped->m_vecLastCollisionImpactVelocity.z > 0.3f;
    if (!ped->bHeadStuckInCollision && !bKnockedDownByPed) {
        // NOTE: If neither probe found something the original code uses `colPointFwd` anyways
        //       (the col points are uninitialized then), here it's zeroed instead.
        ped->SetPosn(probeResult == 2 ? colPointBwd.m_vecPoint : colPointFwd.m_vecPoint);
        ped->GetPosition().z += 1.0f;
    }

    if (groundFwd < groundBwd) {
        dir *= -1.0f;
    }
    dir.z = 1.0f;
    ped->ApplyMoveForce(dir * 4.0f);

    if (probeResult > -1) {
        // Move the ped out of the wall and make it face away from it
        dir *= 0.25f;
        ped->SetPosn(ped->GetPosition() + dir);

        ped->m_fCurrentRotation = CGeneral::GetRadianAngleBetweenPoints(dir.x, dir.y, 0.0f, 0.0f);
        ped->m_fCurrentRotation = CGeneral::LimitRadianAngle(ped->m_fCurrentRotation);
        ped->m_fAimingRotation  = ped->m_fCurrentRotation;

        ped->SetOrientation(0.0f, 0.0f, ped->m_fCurrentRotation);
    }

    m_state = PED_STUCK_STATE_WAS_STUCK;
}
